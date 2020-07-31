/*

Copyright (C) 2020  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
Doesn't work yet for multiple reads...
*/

#include "concurrent_functor_queue.h"

#include <thread>
#include <mutex>
#include <shared_mutex>

#define SEV_FUNCTOR_ALIGN_MODMASK ((ptrdiff_t)(SEV_FUNCTOR_ALIGN - 1))
#define SEV_FUNCTOR_ALIGN_MASK (~(ptrdiff_t)(SEV_FUNCTOR_ALIGN - 1))
#define SEV_FUNCTOR_ALIGNED(value) ((ptrdiff_t)(((value) + SEV_FUNCTOR_ALIGN_MODMASK) & SEV_FUNCTOR_ALIGN_MASK))

#ifdef _WIN64
// #define SEV_PREAMBLE_EXTRA /* Reduces space use for C-style functions */
#endif

#define SEV_DEBUG_NB_OBJECTS /* Testing */

namespace sev {
namespace /* anonymous */ {

struct BlockPreamble
{
	SEV_AtomicPtr NextBlock;

	SEV_AtomicPtrDiff ReadIdx;
	SEV_AtomicInt32 ReadShared;
	// volatile long PreWriteShared;

#ifdef SEV_DEBUG_NB_OBJECTS
	SEV_AtomicInt32 NbObjects;
#endif

	// TODO: Might be interesting to have a Shared count that includes the number of items remaining in the block + count 1 while it's in ReadBlock + count 1 while it's in WriteBlock
	// Could figure out a way to remove the lock in the pop function
};

struct FunctorPreamble
{
#ifdef SEV_PREAMBLE_EXTRA
	void *Extra;
#endif
	SEV_AtomicPtrDiff Ready;
	const SEV_FunctorVt *Vt;
	ptrdiff_t Size;
};

#define SEV_BLOCK_PREAMBLE_SIZE (SEV_FUNCTOR_ALIGNED(sizeof(sev::BlockPreamble) + sizeof(sev::FunctorPreamble)))

void wipeBlockOnly(void *block)
{
	uint8_t *b = (uint8_t *)block;
	BlockPreamble *blockPreamble = (BlockPreamble *)block;
	blockPreamble->NextBlock = null;
	blockPreamble->ReadIdx = SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble);
	blockPreamble->ReadShared = 0;
	// blockPreamble->PreWriteShared = 0;
#ifdef SEV_DEBUG_NB_OBJECTS
	blockPreamble->NbObjects = 0;
#endif
}

void wipeBlock(void *block, ptrdiff_t blockSize)
{
	wipeBlockOnly(block);
	uint8_t *b = (uint8_t *)block;
	for (ptrdiff_t i = (SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble)); i < blockSize; i += SEV_FUNCTOR_ALIGN)
		((sev::FunctorPreamble *)&b[i])->Ready = 0;
}

} /* anonymous namespace */
} /* namespace sev */

SEV_ConcurrentFunctorQueue *SEV_ConcurrentFunctorQueue_create(ptrdiff_t blockSize)
{
	static_assert(sizeof(SEV_ConcurrentFunctorQueue) == sizeof(sev::ConcurrentFunctorQueue<void()>));
	SEV_ConcurrentFunctorQueue *concurrentFunctorQueue = (SEV_ConcurrentFunctorQueue *)new (nothrow) sev::ConcurrentFunctorQueue<void()>(nothrow, blockSize);
	if (!concurrentFunctorQueue)
	{
		return null;
	}
	if (!concurrentFunctorQueue->ReadBlock) // ENOMEM
	{
		delete (sev::ConcurrentFunctorQueue<void()> *)concurrentFunctorQueue;
		return null;
	}
	return concurrentFunctorQueue;
}

void SEV_ConcurrentFunctorQueue_destroy(SEV_ConcurrentFunctorQueue *concurrentFunctorQueue)
{
	delete (sev::ConcurrentFunctorQueue<void()> *)concurrentFunctorQueue;
}

errno_t SEV_ConcurrentFunctorQueue_init(SEV_ConcurrentFunctorQueue *me, ptrdiff_t blockSize)
{
	static_assert(SEV_BLOCK_PREAMBLE_SIZE == SEV_FUNCTOR_ALIGN); // Just for testing, it should be exactly this now. It can be any multiple
	me->AtomicWriteSwap = { 0, 0 };
	me->DeleteLock = { 0, 0 };
	// me->PreLockShared = 0;
#ifdef SEV_DEBUG_CFQ_PUSH
	// me->VerifyAllocLock = 0;
#endif
	me->BlockSize = blockSize;
	me->ReadBlock = (uint8_t *)SEV_alignedMAlloc(blockSize, SEV_FUNCTOR_ALIGN);
	if (!me->ReadBlock)
	{
		me->WriteBlock = null;
		me->SpareBlock = null;
		return ENOMEM;
	}
	me->WriteBlock = me->ReadBlock;
	me->SpareBlock = (uint8_t *)SEV_alignedMAlloc(blockSize, SEV_FUNCTOR_ALIGN); // Also works without, but it will end up allocated anyway when flipping during write (no need to check null)
	me->PreWriteIdx = SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble);
	sev::wipeBlock(me->WriteBlock, blockSize);
	if (me->SpareBlock)
		sev::wipeBlock(me->SpareBlock, blockSize);
	return 0;
}

void SEV_ConcurrentFunctorQueue_release(SEV_ConcurrentFunctorQueue *me)
{
	// Do not release while threads are still accessing this object!
	// SEV_ASSERT(!me->PreLockShared);
#ifdef SEV_DEBUG_CFQ_PUSH
	// SEV_ASSERT(!me->VerifyAllocLock);
#endif

	SEV_alignedFree(me->SpareBlock);
#ifdef SEV_DEBUG
	me->SpareBlock = null;
#endif
	// ptrdiff_t idx = me->ReadIdx;
	uint8_t *block = (uint8_t *)me->ReadBlock;
	const ptrdiff_t blockSize = me->BlockSize;
#ifdef SEV_DEBUG
	me->ReadBlock = null;
#endif
	while (block)
	{
		sev::BlockPreamble *blockPreamble = (sev::BlockPreamble *)block;
		ptrdiff_t idx = blockPreamble->ReadIdx;
		for (ptrdiff_t i = idx; i < blockSize; i += ((sev::FunctorPreamble *)(&block[i]))->Size)
		{
			ptrdiff_t ptrIdx = i + sizeof(sev::FunctorPreamble);
			sev::FunctorPreamble *functorPreamble = (sev::FunctorPreamble *)(&block[i]);
			if (!functorPreamble->Ready)
				break; // No more remaining functors
			functorPreamble->Vt->Destroy((void *)&block[ptrIdx]);
		}
		uint8_t *nextBlock = (uint8_t *)blockPreamble->NextBlock;
		SEV_alignedFree((void *)block);
		block = nextBlock;
	}
}

errno_t SEV_ConcurrentFunctorQueue_push(SEV_ConcurrentFunctorQueue *me, void(*f)(void *ptr, void *args), void *ptr, ptrdiff_t size) // Does a memcpy of the data ptr
{
	// A generic function table that calls the function it's passed with the following data as argument pointer
	typedef void(*TFn)(void *ptr, void *args);
	struct DataView
	{
		TFn f;
		void *ptr;
		ptrdiff_t size;
#if 1
		ptrdiff_t finalSize;
#endif
	};
#if 1
	// Calculate the right alignment to store the ptr after the padded data, just before the preamble of the next entry.
	uint8_t d;
	static const sev::FunctorVt<void(void *)> vtable([d](void *args) -> void {
		// Call function from t
		void *ptr = (void *)&d;
		sev::FunctorPreamble *functorPreamble = &((sev::FunctorPreamble *)ptr)[-1];
		TFn f = (TFn)((uint8_t *)ptr)[functorPreamble->Size - sizeof(TFn)];
		f(ptr, args);
	});
	const ptrdiff_t totalSize = SEV_FUNCTOR_ALIGNED(sizeof(sev::FunctorPreamble) + size + sizeof(TFn)) - sizeof(sev::FunctorPreamble);
	const DataView data{ f, ptr, size, totalSize };
	void(*forwardConstructor)(void *, void *) = [](void *ptr, void *other) -> void {
		// Construct the data in the queue serially, place the function after the padded data
		auto data = (DataView *)other;
		memcpy(ptr, data->ptr, data->size);
		*(TFn *)(&((uint8_t *)ptr)[data->finalSize - sizeof(TFn)]) = data->f;
	};
	return SEV_ConcurrentFunctorQueue_pushFunctorEx(me, vtable.raw(), totalSize, (void *)&data, forwardConstructor);
#else
#ifdef SEV_PREAMBLE_EXTRA
	uint8_t d;
	static const sev::FunctorVt<void()> vtable([d]() -> void {
		// Call function from the preamble
		sev::FunctorPreamble *functorPreamble = &((sev::FunctorPreamble *)&d)[-1];
		TFn f = (TFn)functorPreamble->Extra;
		f((void *)&d);
	});
	const DataView data{ f, ptr, size };
	void(*forwardConstructor)(void *, void *) = [](void *ptr, void *other) -> void {
		// Construct the data in the queue serially, with the function in the preamble
		auto data = (DataView *)other;
		sev::FunctorPreamble *functorPreamble = &((sev::FunctorPreamble *)ptr)[-1];
		functorPreamble->Extra = data->f;
		memcpy(ptr, data->ptr, data->size);
	};
	return SEV_ConcurrentFunctorQueue_pushFunctorEx(me, vtable.raw(), size, (void *)&data, forwardConstructor);
#else
	static const ptrdiff_t preamble = SEV_FUNCTOR_ALIGN; // sizeof(f); // Just jump a whole alignment block to enforce alignment...
	static const sev::FunctorVt<void()> vtable([f]() -> void {
		// Call function from the data
		void *ptr = &((uint8_t *)f)[preamble];
		f(ptr);
	});
	const DataView data{ f, ptr, size };
	void(*forwardConstructor)(void *, void *) = [](void *ptr, void *other) -> void {
		// Construct the data in the queue serially
		auto data = (DataView *)other;
		*(TFn *)ptr = data->f;
		memcpy(&((uint8_t *)ptr)[preamble], data->ptr, data->size);
	};
	const ptrdiff_t totalSize = preamble + size;
	return SEV_ConcurrentFunctorQueue_pushFunctorEx(me, vtable.raw(), totalSize, (void *)&data, forwardConstructor);
#endif
#endif
}

errno_t SEV_ConcurrentFunctorQueue_pushFunctor(SEV_ConcurrentFunctorQueue *me, const SEV_FunctorVt *vt, void *ptr, void(*forwardConstructor)(void *ptr, void *other))
{
	try
	{
		return SEV_ConcurrentFunctorQueue_pushFunctorEx(me, vt, vt->Size, ptr, forwardConstructor);
	}
	catch (...)
	{
		return EOTHER;
	}
}

std::unique_ptr<std::shared_mutex> m(std::make_unique<std::shared_mutex>());

errno_t SEV_ConcurrentFunctorQueue_pushFunctorEx(SEV_ConcurrentFunctorQueue *me, const SEV_FunctorVt *vt, ptrdiff_t size, void *ptr, void(*forwardConstructor)(void *ptr, void *other))
{
	union BlockData
	{
		void *ptr;
		uint8_t *data;
		sev::BlockPreamble *preamble;
	};

	// This function only locks while flipping to the next buffer
	static_assert(sizeof(sev::BlockPreamble) + sizeof(sev::FunctorPreamble) < SEV_BLOCK_PREAMBLE_SIZE);
	const ptrdiff_t sz = SEV_FUNCTOR_ALIGNED(size + sizeof(sev::FunctorPreamble)); // Pad
	const ptrdiff_t blockSize = me->BlockSize;
	if (sz + SEV_BLOCK_PREAMBLE_SIZE > blockSize)
		return ENOMEM;
	SEV_AtomicSharedMutex_lockShared(&me->AtomicWriteSwap);
	auto fsh = gsl::finally([&]() {
		SEV_AtomicSharedMutex_unlockShared(&me->AtomicWriteSwap);
	});
	ptrdiff_t idx = SEV_AtomicPtrDiff_load(&me->PreWriteIdx);
	ptrdiff_t idxMasked = idx & (blockSize - 1);
	BlockData block = { me->WriteBlock };
	bool locked = false;
	SEV_ASSERT(idx);
	SEV_ASSERT(block.ptr);
	int debugIterations = 0;
	bool debugEnteredAnyWhile = false;
	bool debugLockedWriteSwap = false;
	bool debugCanceledWriteSwap = false;
	bool debugProcessedWriteSwap = false;
	do
	{
		bool debugEnteredWhile = false;
		while (idxMasked + sz > blockSize || SEV_AtomicSharedMutex_isLocked(&me->AtomicWriteSwap))
		{
			debugEnteredAnyWhile = true;
			debugEnteredWhile = true;
			if (SEV_AtomicSharedMutex_tryPartialLock(&me->AtomicWriteSwap))
			{
				SEV_AtomicSharedMutex_unlockShared(&me->AtomicWriteSwap);
				debugLockedWriteSwap = true;

#if 1
				SEV_ASSERT(idx == SEV_AtomicPtrDiff_load(&me->PreWriteIdx));
				SEV_ASSERT(block.ptr == me->WriteBlock);
#else
				ptrdiff_t verifyIdx = SEV_AtomicPtrDiff_load(&me->PreWriteIdx);
				BlockData verifyBlock = { me->WriteBlock };
				if (block.ptr != verifyBlock.ptr || idx != verifyIdx)
				{
					SEV_AtomicSharedMutex_downgradePartialLock(&me->AtomicWriteSwap);
					idx = SEV_AtomicPtrDiff_load(&me->PreWriteIdx);
					idxMasked = idx & (blockSize - 1);
					block.ptr = me->WriteBlock;
					debugCanceledWriteSwap = true;
					continue;
				}
#endif

				// First obtain a memory allocation
				debugProcessedWriteSwap = true;
				BlockData allocBlock = { SEV_AtomicPtr_exchange(&me->SpareBlock, null) }; // Get spare and switch to null
				static const ptrdiff_t allocIdxMasked = SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble);
				ptrdiff_t allocIdx = ((idx + blockSize - 1) & ~(blockSize - 1)) + allocIdxMasked; // Round up block size and add new starting index
				ptrdiff_t allocNextIdx = allocIdx + sz;
				SEV_ASSERT((allocNextIdx & (blockSize - 1)) > allocIdxMasked);
				if (!allocBlock.ptr)
				{
					allocBlock.ptr = SEV_alignedMAlloc(me->BlockSize, SEV_FUNCTOR_ALIGN);
					if (!allocBlock.ptr)
					{
						errno_t res = errno;
						SEV_ASSERT(res);
						SEV_AtomicSharedMutex_downgradePartialLock(&me->AtomicWriteSwap);
						if (res) return res;
						return ENOMEM;
					}
					sev::wipeBlock(allocBlock.ptr, blockSize);
				}
				SEV_ASSERT(!allocBlock.preamble->NextBlock);
				SEV_ASSERT(allocBlock.preamble->ReadIdx == allocIdxMasked);
				SEV_ASSERT(!allocBlock.preamble->NbObjects);
				SEV_ASSERT(!allocBlock.preamble->ReadShared);

				// Get a full lock and commit the memory allocation
				SEV_AtomicSharedMutex_completePartialLock(&me->AtomicWriteSwap);
#ifdef SEV_DEBUG
				// Any prending writes could not jump block boundaries
				idx = SEV_AtomicPtrDiff_load(&me->PreWriteIdx);
				SEV_ASSERT(allocIdx == (((idx + blockSize - 1) & ~(blockSize - 1)) + allocIdxMasked));
#endif
				// Unlock write
				SEV_ASSERT(block.ptr == me->WriteBlock);
				me->WriteBlock = allocBlock.ptr;
				SEV_ASSERT(idx == SEV_AtomicPtrDiff_load(&me->PreWriteIdx)); // Can not change during lock
				SEV_AtomicPtrDiff_store(&me->PreWriteIdx, allocNextIdx);

				// Unlock read
				SEV_ASSERT(!SEV_AtomicPtr_load(&block.preamble->NextBlock));
				SEV_AtomicPtr_store(&block.preamble->NextBlock, allocBlock.data);

				// Done
				SEV_ASSERT(allocNextIdx == SEV_AtomicPtrDiff_load(&me->PreWriteIdx)); // Can not change during lock
				SEV_AtomicSharedMutex_downgradePartialLock(&me->AtomicWriteSwap);
				idx = allocIdx;
				idxMasked = allocIdxMasked;
				block.ptr = allocBlock.ptr;
				locked = true; // Next index already written into the write pointer
			}
			else
			{
				// Another thread is allocating
				SEV_AtomicSharedMutex_unlockShared(&me->AtomicWriteSwap);
				SEV_Thread_yield();

				// Get the latest
				SEV_AtomicSharedMutex_lockShared(&me->AtomicWriteSwap);
				idx = SEV_AtomicPtrDiff_load(&me->PreWriteIdx);
				idxMasked = idx & (blockSize - 1);
				block.ptr = me->WriteBlock;
			}
		}
		if (!locked) // Under shared lock here
		{
			// Attempt to swap the next idx into the write pointer
			ptrdiff_t nextIdx = idx + sz;
			SEV_ASSERT(me->WriteBlock == block.ptr); // Can only change while not under shared lock

			ptrdiff_t preLockIdx = SEV_AtomicPtrDiff_compareExchange(&me->PreWriteIdx, nextIdx, idx);
			if (preLockIdx != idx)
			{
				idx = SEV_AtomicPtrDiff_load(&me->PreWriteIdx);
				idxMasked = idx & (blockSize - 1);
				SEV_ASSERT(me->WriteBlock == block.ptr); // Can only change while not under shared lock
				continue; // Try again, preWriteIdx was channged by another thread
			}

			locked = true;
		}
		++debugIterations;
	} while (!locked);


#ifdef SEV_DEBUG_NB_OBJECTS
	SEV_AtomicInt32_increment(&block.preamble->NbObjects);
	SEV_ASSERT(me->WriteBlock == block.ptr);
#endif

	ptrdiff_t ptrIdx = idxMasked + sizeof(sev::FunctorPreamble);
	sev::FunctorPreamble *functorPreamble = (sev::FunctorPreamble *)&block.data[idxMasked];
	functorPreamble->Vt = vt;
	functorPreamble->Size = sz; // Size including preamble and post-padding
	SEV_ASSERT(((ptrdiff_t)&block.data[ptrIdx] & SEV_FUNCTOR_ALIGN_MASK) == (ptrdiff_t)&block.data[ptrIdx]); // Check alignment

	// Prepare commit, just in case write throws
	auto fin = gsl::finally([&]() -> void {
		// Commit
		if (SEV_AtomicPtrDiff_exchange(&functorPreamble->Ready, 1))
			SEV_DEBUG_BREAK(); // Duplicate allocation!
		SEV_ASSERT(me->WriteBlock == block.ptr);
	});

	// Really write
	forwardConstructor((void *)&block.data[ptrIdx], ptr);

	return 0;

#if 0
	// This function only locks while flipping to the next buffer
	// std::unique_lock<std::shared_mutex> sl(*m);
	
	// https://docs.microsoft.com/en-us/cpp/intrinsics/compiler-intrinsics?view=vs-2019
	static_assert(sizeof(sev::BlockPreamble) + sizeof(sev::FunctorPreamble) < SEV_BLOCK_PREAMBLE_SIZE);
	const ptrdiff_t sz = SEV_FUNCTOR_ALIGNED(size + sizeof(sev::FunctorPreamble)); // Pad
	const ptrdiff_t blockSize = me->BlockSize;
	if (sz + SEV_BLOCK_PREAMBLE_SIZE > blockSize)
		return ENOMEM;
	ptrdiff_t idx = SEV_AtomicPtrDiff_load(&me->PreWriteIdx);
	MemoryBarrier();
	uint8_t *block = (uint8_t *)SEV_AtomicPtr_load(&me->WriteBlock);
	ptrdiff_t nextIdx SEV_DEBUG_SET(0xCDCDCDCDCDCDCDCD);
	bool locked = false;
	ptrdiff_t lockIdx SEV_DEBUG_SET(0xCDCDCDCDCDCDCDCD);
	sev::BlockPreamble *prevBlockPreamble SEV_DEBUG_SET((sev::BlockPreamble *)0xCDCDCDCDCDCDCDCD);
	bool preLocked = false;
	for (; ;)
	{
		if (idx > blockSize)
		{
			if (preLocked)
			{
				SEV_AtomicInt32_decrement(&me->PreLockShared);
				preLocked = false;
			}
			do
			{
				// Existing preWriteIdx exceeds block size, it means we're allocating in another thread
				SEV_Thread_yield();
				idx = SEV_AtomicPtrDiff_load(&me->PreWriteIdx);
				MemoryBarrier();
				block = (uint8_t *)SEV_AtomicPtr_load(&me->WriteBlock); // If the block and index change, we'll either have (old idx, old block), (old idx, new block), or (new idx, new block), which is safe here
			} while (idx > blockSize);
		}
		nextIdx = idx + sz;
		if (!preLocked)
		{
			SEV_AtomicInt32_increment(&me->PreLockShared);
			preLocked = true;
		}
		ptrdiff_t preLockIdx = SEV_AtomicPtrDiff_compareExchange(&me->PreWriteIdx, nextIdx, idx);
		if (preLockIdx != idx)
		{
			// _InterlockedDecrement(&me->PreLockShared);
			// SEV_Thread_yield();
			idx = SEV_AtomicPtrDiff_load(&me->PreWriteIdx);
			MemoryBarrier();
			block = (uint8_t *)SEV_AtomicPtr_load(&me->WriteBlock);
			continue; // Try again, preWriteIdx was channged by another thread
		}

		// We have a lock, are we inside a block?
		if (nextIdx <= blockSize)
		{
#ifdef SEV_DEBUG
			SEV_ASSERT(preLocked);
			SEV_ASSERT(SEV_AtomicInt32_load(&me->PreLockShared) > 0);
			sev::BlockPreamble *blockPreamble = (sev::BlockPreamble *)block;
			SEV_ASSERT(!SEV_AtomicPtr_load(&blockPreamble->NextBlock)); // Cannot have been set!
#endif
			break; // We have a good allocation, using it!
		}

#ifdef SEV_DEBUG_CFQ_PUSH
		int32_t verifyLock = SEV_AtomicInt32_increment(&me->VerifyAllocLock);
		SEV_ASSERT(verifyLock == 1);
#endif

		SEV_ASSERT(preLocked);

		while (SEV_AtomicInt32_load(&me->PreLockShared) != 1)
			SEV_Thread_yield(); // TEMP

		// Writing is now locked, we're safe here until preWriteIdx is written
		locked = true;
		lockIdx = nextIdx;
		prevBlockPreamble = (sev::BlockPreamble *)block;
		// printf("lock %x\n", prevBlockPreamble);
		SEV_ASSERT(SEV_AtomicPtrDiff_load(&me->PreWriteIdx) == nextIdx); // Can't have changed since locking
		SEV_ASSERT(!SEV_AtomicPtr_load(&prevBlockPreamble->NextBlock)); // Cannot have been set

		// Start from the beginning, offset alignment
		static_assert((SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble)) >= sizeof(sev::BlockPreamble));
		idx = SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble); // Block preamble is preceeding
		nextIdx = idx + sz;

		// Switch to next write buffer block
		// printf("--[Push Block]--\n"); // DEBUG
		block = (uint8_t *)SEV_AtomicPtr_exchange(&me->SpareBlock, null); // Get spare and switch to null
		if (block)
		{
			// Reuse leftover block
#ifdef SEV_DEBUG
			sev::BlockPreamble *badNextBlockPreamble = (sev::BlockPreamble *)SEV_AtomicPtr_load(&prevBlockPreamble->NextBlock);
			SEV_ASSERT(!badNextBlockPreamble); // Can't have been set
#endif
			sev::BlockPreamble *blockPreamble = (sev::BlockPreamble *)block; // This is the new block (from spare)
			SEV_ASSERT(!SEV_AtomicPtr_load(&blockPreamble->NextBlock)); // Can't have been set
			(void)blockPreamble; // Unused warning
		}
		else
		{
			// Create a new block
			block = (uint8_t *)SEV_alignedMAlloc(me->BlockSize, SEV_FUNCTOR_ALIGN);
			if (!block)
			{
				errno_t res = errno;
				SEV_ASSERT(res);
				SEV_AtomicPtrDiff_store(&me->PreWriteIdx, lockIdx - sz);
				SEV_AtomicInt32_decrement(&me->PreLockShared);
#ifdef SEV_DEBUG_CFQ_PUSH
				SEV_AtomicInt32_decrement(&me->VerifyAllocLock);
#endif
				if (res) return res;
				return ENOMEM;
			}
			sev::wipeBlock(block, blockSize);
		}

		sev::BlockPreamble *blockPreamble = (sev::BlockPreamble *)block;
		// SEV_AtomicPtr_store(&blockPreamble->NextBlock, null);
		SEV_ASSERT(!SEV_AtomicPtr_load(&blockPreamble->NextBlock)); // Can't have been set

#ifdef SEV_DEBUG_CFQ_PUSH
		SEV_ASSERT(SEV_AtomicInt32_load(&me->VerifyAllocLock) == 1);
		SEV_AtomicInt32_decrement(&me->VerifyAllocLock);
#endif

		break;
	}

	// Write
	SEV_ASSERT(preLocked);
#ifdef SEV_DEBUG_NB_OBJECTS
	SEV_AtomicInt32_increment(&((sev::BlockPreamble *)block)->NbObjects);
#endif
	// SEV_AtomicInt32_increment(&((sev::BlockPreamble *)block)->PreWriteShared);
	// SEV_AtomicInt32_decrement(&me->PreLockShared);
	ptrdiff_t ptrIdx = idx + sizeof(sev::FunctorPreamble);
	sev::FunctorPreamble *functorPreamble = (sev::FunctorPreamble *)&block[idx];
	functorPreamble->Vt = vt;
	functorPreamble->Size = sz; // Size including preamble and post-padding
	SEV_ASSERT(((ptrdiff_t)&block[ptrIdx] & SEV_FUNCTOR_ALIGN_MASK) == (ptrdiff_t)&block[ptrIdx]); // Check alignment

	// Prepare commit, in case write throws
	auto fin = gsl::finally([&]() -> void {
		// Commit
		if (locked)
		{
			// Unlock
			SEV_ASSERT(SEV_AtomicPtrDiff_load(&me->PreWriteIdx) == lockIdx); // Can't have changed
#ifdef SEV_DEBUG_CFQ_PUSH
			SEV_ASSERT(!SEV_AtomicInt32_load(&me->VerifyAllocLock));
#endif
			sev::BlockPreamble *blockPreamble = (sev::BlockPreamble *)block;
			SEV_AtomicPtr_store(&me->WriteBlock, block);
			MemoryBarrier();
			SEV_AtomicPtrDiff_store(&functorPreamble->Ready, 1);
			// _InterlockedExchangePointer((void *volatile*)(&functorPreamble->Ready), (void *)1);
			SEV_AtomicInt32_decrement(&me->PreLockShared);
			SEV_ASSERT(SEV_AtomicInt32_load(&me->PreLockShared) >= 0);
			// _InterlockedDecrement(&blockPreamble->PreWriteShared);
			// Other threads may still be writing Ready flags, don't commit the block yet.
			// while (prevBlockPreamble->PreWriteShared)
			//	SEV_Thread_yield();
			// SEV_ASSERT(!me->PreLockShared); // TEMP: Since we waited earlier already
			while (SEV_AtomicInt32_load(&me->PreLockShared))
				SEV_Thread_yield();
			SEV_ASSERT(!SEV_AtomicPtr_load(&prevBlockPreamble->NextBlock)); // Can't have been set
			// printf("set %x -> %x\n", prevBlockPreamble, block);
			SEV_AtomicPtr_store(&prevBlockPreamble->NextBlock, block); // Allow read // FIXME: This can be set by another thread that is not the last one! Need a writing shared counter to wait for 0 before committing the block number! (Increment before getting a lock on the writing, decrement after failing or when ready flag is committed!)
#ifdef SEV_DEBUG_CFQ_PUSH
			SEV_ASSERT(!SEV_AtomicInt32_load(&me->VerifyAllocLock));
#endif
			// SEV_ASSERT(!SEV_AtomicInt32_load(&me->PreLockShared));
			// printf("commit %x -> %x\n", prevBlockPreamble, block);
			ptrdiff_t verifyLockIdx = SEV_AtomicPtrDiff_exchange(&me->PreWriteIdx, nextIdx); // Unlock write
			SEV_ASSERT(verifyLockIdx == lockIdx); // Must be the original one
		}
		else
		{
			// Flag ready for reader
			SEV_AtomicPtrDiff_store(&functorPreamble->Ready, 1);
			// _InterlockedExchangePointer((void *volatile*)(&functorPreamble->Ready), (void *)1);
			sev::BlockPreamble *blockPreamble = (sev::BlockPreamble *)block;
#ifdef SEV_DEBUG
			volatile sev::BlockPreamble *badNextBlockPreamble = (sev::BlockPreamble *)SEV_AtomicPtr_load(&blockPreamble->NextBlock);
			SEV_ASSERT(!badNextBlockPreamble); // Can't have been set until PreLockShared is decremented!
#endif
			//SEV_ASSERT(!SEV_AtomicPtr_load(&blockPreamble->NextBlock));
			//_InterlockedDecrement(&blockPreamble->PreWriteShared);
			SEV_AtomicInt32_decrement(&me->PreLockShared);
			SEV_ASSERT(SEV_AtomicInt32_load(&me->PreLockShared) >= 0);
		}

		SEV_ASSERT(nextIdx <= SEV_AtomicPtrDiff_load(&me->BlockSize));
	});

	// Really write
	forwardConstructor((void *)&block[ptrIdx], ptr);

	return 0;
#endif
}

errno_t SEV_ConcurrentFunctorQueue_tryCallAndPop(SEV_ConcurrentFunctorQueue *me, void *args)
{
	typedef void(*TFn)(void *ptr, void *args);
	return SEV_ConcurrentFunctorQueue_tryCallAndPopFunctor(me, [](void *args, void *ptr, void *f) -> void {
		((TFn)f)(ptr, args);
	}, args);
}

errno_t SEV_ConcurrentFunctorQueue_tryCallAndPopFunctor(SEV_ConcurrentFunctorQueue *me, void(*caller)(void *args, void *ptr, void *f), void *args)
{
	try
	{
		return SEV_ConcurrentFunctorQueue_tryCallAndPopFunctorEx(me, caller, args) ? 0 : ENODATA;
	}
	catch (...)
	{
		return EOTHER;
	}
}

bool SEV_ConcurrentFunctorQueue_tryCallAndPopFunctorEx(SEV_ConcurrentFunctorQueue *me, void(*caller)(void *args, void *ptr, void *f), void *args)
{
	// std::unique_lock<std::shared_mutex> l(*m);

	const ptrdiff_t blockSize = me->BlockSize;

	// Safely get the reading block, and increment the sharing counter
	SEV_AtomicSharedMutex_lockShared(&me->DeleteLock);
	uint8_t *readBlock = (uint8_t *)SEV_AtomicPtr_load(&me->ReadBlock);
	auto readBlockPreamble = (sev::BlockPreamble *)readBlock;
	SEV_AtomicInt32_increment(&readBlockPreamble->ReadShared);
	SEV_AtomicSharedMutex_unlockShared(&me->DeleteLock);

	ptrdiff_t readIdx = SEV_AtomicPtrDiff_load(&readBlockPreamble->ReadIdx);

	// Prepare exit, in case of early exit
	auto fin1 = gsl::finally([&]() -> void {
		// Wipe flags
		// ptrdiff_t nextReadIdx = readIdx + functorPreamble->Size;
		// for (ptrdiff_t idx = readIdx; readIdx < nextReadIdx; readIdx += SEV_FUNCTOR_ALIGN)
		// 	((sev::FunctorPreamble *)(&readBlock[readIdx]))->Ready = 0;

		SEV_AtomicSharedMutex_lockShared(&me->DeleteLock);
		long readShared = SEV_AtomicInt32_decrement(&readBlockPreamble->ReadShared);
		uint8_t *currentReadBlock = (uint8_t *)SEV_AtomicPtr_load(&me->ReadBlock);
		SEV_AtomicSharedMutex_unlockShared(&me->DeleteLock);

		if (currentReadBlock != readBlock) // If this block isn't current anymore
		{
			if (!readShared) // New value is 0, no other threads left on this
			{
				// printf("--[Free Block (2)]--\n"); // DEBUG
				SEV_ASSERT((uint8_t *)readBlockPreamble == readBlock);
#ifdef SEV_DEBUG
				auto functorPreamble = (sev::FunctorPreamble *)(&readBlock[readBlockPreamble->ReadIdx]);
				SEV_ASSERT(!(readBlockPreamble->ReadIdx < blockSize && functorPreamble->Ready));
#endif
#ifdef SEV_DEBUG_NB_OBJECTS
				SEV_ASSERT(!SEV_AtomicInt32_load(&readBlockPreamble->NbObjects));
#endif
				SEV_ASSERT(!SEV_AtomicInt32_load(&readBlockPreamble->ReadShared));
				// Attempt to release or spare the old block
				// sev::wipeBlockOnly(readBlock); // , blockSize);
				sev::wipeBlock(readBlock, blockSize);
				uint8_t *spareBlock = (uint8_t *)SEV_AtomicPtr_compareExchange(&me->SpareBlock, readBlock, null);
				if (spareBlock) // Old value was not 0, not using this as a spare block
					SEV_alignedFree((void *)readBlock);
			}
		}
	});

	for (; ; )
	{
		const auto functorPreamble = (sev::FunctorPreamble *)(&readBlock[readIdx]);
		const bool ready = readIdx < blockSize && SEV_AtomicPtrDiff_load(&functorPreamble->Ready);
		if (!ready) // No more read space, or flag not set
		{
			// Nothing new in this block
			if (SEV_AtomicPtr_load(&readBlockPreamble->NextBlock)) // Next block available
			{
				SEV_ASSERT(!(readIdx < blockSize && SEV_AtomicPtrDiff_load(&functorPreamble->Ready)));
				if (readIdx < blockSize && SEV_AtomicPtrDiff_load(&functorPreamble->Ready))
					continue; // Try again

				// Old block
				sev::BlockPreamble *oldReadBlock = readBlockPreamble;

				// while (SEV_AtomicPtrDiff_load(&me->PreWriteIdx) > me->BlockSize)
				// 	SEV_Thread_yield(); // TEST

				// Swap to the next block (if we're still reading the current block) (and fetch the block that's being read now)
				// readBlock = (uint8_t *)_InterlockedCompareExchangePointer((void *volatile *)(&me->ReadBlock), readBlockPreamble->NextBlock, readBlock);
				SEV_AtomicSharedMutex_lock(&me->DeleteLock);
				if (SEV_AtomicPtr_load(&me->ReadBlock) == readBlock)
				{
					// printf("--[Pop Block]--\n"); // DEBUG
					readBlock = (uint8_t *)SEV_AtomicPtr_load(&readBlockPreamble->NextBlock);
					SEV_AtomicPtr_store(&me->ReadBlock, readBlock);
				}
				else
				{
					readBlock = (uint8_t *)SEV_AtomicPtr_load(&me->ReadBlock);
				}
				readBlockPreamble = (sev::BlockPreamble *)readBlock;
				SEV_AtomicInt32_increment(&readBlockPreamble->ReadShared);
#ifdef SEV_DEBUG
				SEV_ASSERT(!(readIdx < blockSize && SEV_AtomicPtrDiff_load(&functorPreamble->Ready)));
#endif
				long readShared = SEV_AtomicInt32_decrement(&oldReadBlock->ReadShared);
				SEV_ASSERT(readShared >= 0);
				SEV_AtomicSharedMutex_unlock(&me->DeleteLock);
				readIdx = SEV_AtomicPtrDiff_load(&readBlockPreamble->ReadIdx);

				if (!readShared) // New value is 0, no other threads left on this
				{
					// printf("--[Free Block (1)]--\n"); // DEBUG
#ifdef SEV_DEBUG_NB_OBJECTS
					SEV_ASSERT(!SEV_AtomicInt32_load(&oldReadBlock->NbObjects));
#endif
					SEV_ASSERT(!SEV_AtomicInt32_load(&oldReadBlock->ReadShared));
					// Attempt to release or spare the old block
					// sev::wipeBlockOnly(oldReadBlock); // , blockSize);
					sev::wipeBlock(oldReadBlock, blockSize);
					uint8_t *spareBlock = (uint8_t *)SEV_AtomicPtr_compareExchange(&me->SpareBlock, oldReadBlock, null);
					if (spareBlock) // Old value was not 0, not using this as a spare block
						SEV_alignedFree(oldReadBlock);
				}

				continue; // Go back and see if there's anything to read
			}

			// Queue is empty
			return false;
		}
		else
		{
			// Try to advance the current index
			ptrdiff_t currentReadIdx = readIdx;
			ptrdiff_t nextReadIdx = readIdx + functorPreamble->Size;
			if ((readIdx = SEV_AtomicPtrDiff_compareExchange(&readBlockPreamble->ReadIdx, nextReadIdx, currentReadIdx)) != currentReadIdx)
			{
				// Other thread already attempted to pop this entry
				continue; // Check for the next entry
			}

			// We have a reading!
			break;
		}
	}

	// Prepare calls
	auto functorPreamble = (sev::FunctorPreamble *)(&readBlock[readIdx]);
	ptrdiff_t readPtrIdx = readIdx + sizeof(sev::FunctorPreamble);
	SEV_ASSERT(SEV_FUNCTOR_ALIGNED(readPtrIdx) == readPtrIdx);

	// Prepare exit, in case invoke call throws
	auto fin2 = gsl::finally([&]() -> void {
		// Destructor
		functorPreamble->Vt->Destroy((void *)&readBlock[readPtrIdx]);
#ifdef SEV_DEBUG_NB_OBJECTS
		SEV_AtomicInt32_decrement(&readBlockPreamble->NbObjects);
#endif
	});

	// Call
	caller(args, (void *)&readBlock[readPtrIdx], functorPreamble->Vt->Invoke);
	return true;
}

/* end of file */
