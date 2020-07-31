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

#define SEV_DEBUG_NB_OBJECTS /* Testing */

namespace sev {
namespace /* anonymous */ {

struct BlockPreamble
{
	SEV_AtomicPtr NextBlock;

	SEV_AtomicPtrDiff ReadIdx;
	SEV_AtomicInt32 ReadShared;

#ifdef SEV_DEBUG_NB_OBJECTS
	SEV_AtomicInt32 NbObjects;
#endif
};

struct FunctorPreamble
{
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
		ptrdiff_t finalSize;
	};
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
}

errno_t SEV_ConcurrentFunctorQueue_pushFunctor(SEV_ConcurrentFunctorQueue *me, const SEV_FunctorVt *vt, void *ptr, void(*forwardConstructor)(void *ptr, void *other))
{
	return SEV_ConcurrentFunctorQueue_pushFunctorEx(me, vt, vt->Size, ptr, forwardConstructor);
}

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
	int debugFailedIncrement = 0;
	int debugFailedLockSwap = 0;
	bool debugEnteredAnyWhile = false;
	bool debugLockedWriteSwap = false;
	bool debugCanceledWriteSwap = false;
	bool debugProcessedWriteSwap = false;
	do
	{
		++debugIterations;
		bool debugEnteredWhile = false;
		while (!locked && (idxMasked + sz > blockSize || SEV_AtomicSharedMutex_isLocked(&me->AtomicWriteSwap)))
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
					SEV_AtomicSharedMutex_downgradeLock(&me->AtomicWriteSwap);
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
						SEV_AtomicSharedMutex_downgradeLock(&me->AtomicWriteSwap);
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
#ifdef SEV_DEBUG
				if (SEV_AtomicPtrDiff_exchange(&me->PreWriteIdx, allocNextIdx) != idx)
					SEV_DEBUG_BREAK();
#else
				SEV_AtomicPtrDiff_store(&me->PreWriteIdx, allocNextIdx);
#endif

				// Unlock read
				SEV_ASSERT(!SEV_AtomicPtr_load(&block.preamble->NextBlock));
				SEV_AtomicPtr_store(&block.preamble->NextBlock, allocBlock.data);

				// Done
				SEV_ASSERT(allocNextIdx == SEV_AtomicPtrDiff_load(&me->PreWriteIdx)); // Can not change during lock
				SEV_AtomicSharedMutex_downgradeLock(&me->AtomicWriteSwap);
				idx = allocIdx;
				idxMasked = allocIdxMasked;
				block.ptr = allocBlock.ptr;
				locked = true; // Next index already written into the write pointer
			}
			else
			{
				// Another thread is allocating
				++debugFailedLockSwap;
				SEV_AtomicSharedMutex_unlockShared(&me->AtomicWriteSwap);
				SEV_Thread_yield();

				// Get the latest
				SEV_AtomicSharedMutex_lockShared(&me->AtomicWriteSwap);
				idx = SEV_AtomicPtrDiff_load(&me->PreWriteIdx);
				idxMasked = idx & (blockSize - 1);
				block.ptr = me->WriteBlock;

				SEV_ASSERT(!locked);
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
				SEV_ASSERT(preLockIdx - idx > 0);
				idx = SEV_AtomicPtrDiff_load(&me->PreWriteIdx);
				SEV_ASSERT(idx - preLockIdx >= 0);
				idxMasked = idx & (blockSize - 1);
				SEV_ASSERT(me->WriteBlock == block.ptr); // Can only change while not under shared lock
				++debugFailedIncrement;
				continue; // Try again, preWriteIdx was channged by another thread
			}

			locked = true;
		}
	} while (!locked);


#ifdef SEV_DEBUG_NB_OBJECTS
	SEV_AtomicInt32_increment(&block.preamble->NbObjects);
	SEV_ASSERT(me->WriteBlock == block.ptr);
#endif

	ptrdiff_t ptrIdx = idxMasked + sizeof(sev::FunctorPreamble);
	sev::FunctorPreamble *functorPreamble = (sev::FunctorPreamble *)&block.data[idxMasked];
	SEV_ASSERT(!SEV_AtomicPtrDiff_load(&functorPreamble->Ready)); // Check against duplicate allocation
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
	return SEV_ConcurrentFunctorQueue_tryCallAndPopFunctorEx(me, caller, args);
}

errno_t SEV_ConcurrentFunctorQueue_tryCallAndPopFunctorEx(SEV_ConcurrentFunctorQueue *me, void(*caller)(void *args, void *ptr, void *f), void *args)
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

	bool debugTriedAgain = false;

	for (; ; )
	{
		const auto functorPreamble = (sev::FunctorPreamble *)(&readBlock[readIdx]);
		const bool functorReady = readIdx < blockSize && SEV_AtomicPtrDiff_load(&functorPreamble->Ready);
		if (!functorReady) // No more read space, or flag not set
		{
			// Nothing new in this block
			if (SEV_AtomicPtr_load(&readBlockPreamble->NextBlock)) // Next block available
			{
				// SEV_ASSERT(!(readIdx < blockSize && SEV_AtomicPtrDiff_load(&functorPreamble->Ready)));
				if (readIdx < blockSize && SEV_AtomicPtrDiff_load(&functorPreamble->Ready))
				{
					debugTriedAgain = true;
					continue; // Try again
				}

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
			return ENODATA;
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
	SEV_ASSERT(SEV_AtomicInt32_load(&readBlockPreamble->NbObjects));
	SEV_ASSERT(SEV_AtomicPtrDiff_load(&functorPreamble->Ready));
	caller(args, (void *)&readBlock[readPtrIdx], functorPreamble->Vt->Invoke);
	return 0;
}

/* end of file */
