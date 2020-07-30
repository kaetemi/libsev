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

#include "concurrent_functor_queue.h"

#include <thread>

#define SEV_FUNCTOR_ALIGN_MODMASK ((ptrdiff_t)(SEV_FUNCTOR_ALIGN - 1))
#define SEV_FUNCTOR_ALIGN_MASK (~(ptrdiff_t)(SEV_FUNCTOR_ALIGN - 1))
#define SEV_FUNCTOR_ALIGNED(value) ((ptrdiff_t)(((value) + SEV_FUNCTOR_ALIGN_MODMASK) & SEV_FUNCTOR_ALIGN_MASK))

#ifdef _WIN64
// #define SEV_PREAMBLE_EXTRA /* Reduces space use for C-style functions */
#endif

namespace sev {
namespace /* anonymous */ {

struct BlockPreamble
{
	uint8_t *NextBlock;

	ptrdiff_t ReadIdx;
	long ReadShared;

	// TODO: Might be interesting to have a Shared count that includes the number of items remaining in the block + count 1 while it's in ReadBlock + count 1 while it's in WriteBlock
	// Could figure out a way to remove the lock in the pop function
};

struct FunctorPreamble
{
#ifdef SEV_PREAMBLE_EXTRA
	void *Extra;
#endif
	volatile ptrdiff_t Ready;
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
	me->DeleteLock = { 0, 0 };
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
	/*
	((sev::BlockPreamble *)me->WriteBlock)->NextBlock = null;
	((sev::BlockPreamble *)me->WriteBlock)->ReadIdx = SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble);
	for (ptrdiff_t i = (SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble)); i < blockSize; i += SEV_FUNCTOR_ALIGN)
		((sev::FunctorPreamble *)&me->WriteBlock[i])->Ready = 0;
	if (me->SpareBlock)
	{
		((sev::BlockPreamble *)me->SpareBlock)->NextBlock = null;
		((sev::BlockPreamble *)me->SpareBlock)->ReadIdx = SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble);
		for (ptrdiff_t i = (SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble)); i < blockSize; i += SEV_FUNCTOR_ALIGN)
			((sev::FunctorPreamble *)&me->SpareBlock[i])->Ready = 0;
	}
	*/
	return 0;
}

void SEV_ConcurrentFunctorQueue_release(SEV_ConcurrentFunctorQueue *me)
{
	// Do not release while threads are still accessing this object!

	SEV_alignedFree(me->SpareBlock);
#ifdef SEV_DEBUG
	me->SpareBlock = null;
#endif
	// ptrdiff_t idx = me->ReadIdx;
	uint8_t *block = me->ReadBlock;
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
			functorPreamble->Vt->Destroy(&block[ptrIdx]);
		}
		uint8_t *nextBlock = blockPreamble->NextBlock;
		SEV_alignedFree(block);
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
	return SEV_ConcurrentFunctorQueue_pushFunctorEx(me, vt, vt->Size, ptr, forwardConstructor);
}

errno_t SEV_ConcurrentFunctorQueue_pushFunctorEx(SEV_ConcurrentFunctorQueue *me, const SEV_FunctorVt *vt, ptrdiff_t size, void *ptr, void(*forwardConstructor)(void *ptr, void *other))
{
	// This function only locks while flipping to the next buffer

	// https://docs.microsoft.com/en-us/cpp/intrinsics/compiler-intrinsics?view=vs-2019
	static_assert(sizeof(sev::BlockPreamble) + sizeof(sev::FunctorPreamble) < SEV_BLOCK_PREAMBLE_SIZE);
	const ptrdiff_t sz = SEV_FUNCTOR_ALIGNED(size + sizeof(sev::FunctorPreamble)); // Pad
	const ptrdiff_t blockSize = me->BlockSize;
	if (sz + SEV_BLOCK_PREAMBLE_SIZE > blockSize)
		return ENOMEM;
	ptrdiff_t idx = me->PreWriteIdx;
	uint8_t *block = me->WriteBlock;
	ptrdiff_t nextIdx;
	bool locked = false;
	ptrdiff_t lockIdx;
	sev::BlockPreamble *prevBlockPreamble;
	for (; ;)
	{
		while (idx > blockSize)
		{
			// Existing preWriteIdx exceeds block size, it means we're allocating in another thread
			SEV_Thread_yield();
			idx = me->PreWriteIdx;
			block = me->WriteBlock; // If the block and index change, we'll either have (old idx, old block), (old idx, new block), or (new idx, new block), which is safe here
		}
		nextIdx = idx + sz;
		if (_InterlockedCompareExchangePointer((void *volatile *)(&me->PreWriteIdx), (void *)nextIdx, (void *)idx) != (void *)idx)
		{
			// std::this_thread::yield();
			idx = me->PreWriteIdx;
			block = me->WriteBlock;
			continue; // Try again, preWriteIdx was channged by another thread
		}

		// We have a lock, are we inside a block?
		if (nextIdx <= blockSize)
		{
			break; // We have a good allocation, using it!
		}

		// Writing is now locked, we're safe here until preWriteIdx is written
		locked = true;
		lockIdx = nextIdx;
		prevBlockPreamble = (sev::BlockPreamble *)block;
		SEV_ASSERT(me->PreWriteIdx == nextIdx); // Can't have changed since locking

		// Start from the beginning, offset alignment
		static_assert((SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble)) >= sizeof(sev::BlockPreamble));
		idx = SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble); // Block preamble is preceeding
		nextIdx = idx + sz;

		// Switch to next write buffer block
		// printf("--[Push Block]--\n"); // DEBUG
		block = (uint8_t *)_InterlockedExchangePointer((void *volatile *)(&me->SpareBlock), null); // Get spare and switch to null
		if (block)
		{
			// Reuse leftover block
			SEV_ASSERT(prevBlockPreamble->NextBlock == null); // Can't have been set
			sev::BlockPreamble *blockPreamble = (sev::BlockPreamble *)block;
			SEV_ASSERT(blockPreamble->NextBlock == null); // Can't have been set
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
				me->PreWriteIdx = lockIdx - sz;
				if (res) return res;
				return ENOMEM;
			}
			sev::wipeBlock(block, blockSize);
		}

		sev::BlockPreamble *blockPreamble = (sev::BlockPreamble *)block;
		blockPreamble->NextBlock = null;

		break;
	}

	// Write
	ptrdiff_t ptrIdx = idx + sizeof(sev::FunctorPreamble);
	sev::FunctorPreamble *functorPreamble = (sev::FunctorPreamble *)&block[idx];
	functorPreamble->Vt = vt;
	functorPreamble->Size = sz; // Size including preamble and post-padding
	SEV_ASSERT(((ptrdiff_t)&block[ptrIdx] & SEV_FUNCTOR_ALIGN_MASK) == (ptrdiff_t)&block[ptrIdx]); // Check alignment
	forwardConstructor(&block[ptrIdx], ptr);

	// Commit
	if (locked)
	{
		// Unlock
		SEV_ASSERT(me->PreWriteIdx == lockIdx); // Can't have changed
		sev::BlockPreamble *blockPreamble = (sev::BlockPreamble *)block;
		me->WriteBlock = block;
		functorPreamble->Ready = 1;
		prevBlockPreamble->NextBlock = block; // Allow read
		me->PreWriteIdx = nextIdx; // Unlock write
	}
	else
	{
		// Flag ready for reader
		functorPreamble->Ready = 1;
	}

	SEV_ASSERT(nextIdx <= me->BlockSize);

	return 0;
}

bool SEV_ConcurrentFunctorQueue_tryCallAndPop(SEV_ConcurrentFunctorQueue *me, void *args)
{
	typedef void(*TFn)(void *ptr, void *args);
	return SEV_ConcurrentFunctorQueue_tryCallAndPopFunctor(me, [](void *args, void *ptr, void *f) -> void {
		((TFn)f)(ptr, args);
	}, args);
}

bool SEV_ConcurrentFunctorQueue_tryCallAndPopFunctor(SEV_ConcurrentFunctorQueue *me, void(*caller)(void *args, void *ptr, void *f), void *args)
{
	const ptrdiff_t blockSize = me->BlockSize;

	// Safely get the reading block, and increment the sharing counter
	SEV_AtomicSharedMutex_lockShared(&me->DeleteLock);
	uint8_t *readBlock = me->ReadBlock;
	auto readBlockPreamble = (sev::BlockPreamble *)readBlock;
	_InterlockedIncrement(&readBlockPreamble->ReadShared);
	SEV_AtomicSharedMutex_unlockShared(&me->DeleteLock);

	ptrdiff_t readIdx = readBlockPreamble->ReadIdx;
	for (; ; )
	{
		const auto functorPreamble = (sev::FunctorPreamble *)(&readBlock[readIdx]);
		const bool ready = readIdx < blockSize && functorPreamble->Ready;
		if (!ready) // No more read space, or flag not set
		{
			// Nothing new in this block
			if (readBlockPreamble->NextBlock) // Next block available
			{
				// Old block
				sev::BlockPreamble *oldReadBlock = readBlockPreamble;

				// Swap to the next block (if we're still reading the current block) (and fetch the block that's being read now)
				// readBlock = (uint8_t *)_InterlockedCompareExchangePointer((void *volatile *)(&me->ReadBlock), readBlockPreamble->NextBlock, readBlock);
				SEV_AtomicSharedMutex_lock(&me->DeleteLock);
				if (me->ReadBlock == readBlock)
				{
					// printf("--[Pop Block]--\n"); // DEBUG
					readBlock = readBlockPreamble->NextBlock;
					me->ReadBlock = readBlock;
				}
				else
				{
					readBlock = me->ReadBlock;
				}
				readBlockPreamble = (sev::BlockPreamble *)readBlock;
				_InterlockedIncrement(&readBlockPreamble->ReadShared);
				long readShared = _InterlockedDecrement(&oldReadBlock->ReadShared);
				SEV_AtomicSharedMutex_unlock(&me->DeleteLock);
				readIdx = readBlockPreamble->ReadIdx;

				// Attempt to release or spare the old block
				if (!readShared) // New value is 0, no other threads left on this
				{
					// printf("--[Free Block (1)]--\n"); // DEBUG
					sev::wipeBlockOnly(oldReadBlock); // , blockSize);
					// sev::wipeBlock(oldReadBlock, blockSize);
					uint8_t *spareBlock = (uint8_t *)_InterlockedCompareExchangePointer((void *volatile *)(&me->SpareBlock), oldReadBlock, null);
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
			if ((readIdx = (ptrdiff_t)_InterlockedCompareExchangePointer((void *volatile *)(&readBlockPreamble->ReadIdx), (void *)nextReadIdx, (void *)readIdx)) != currentReadIdx)
			{
				// Other thread already attempted to pop this entry
				continue; // Check for the next entry
			}

			// We have a reading!
			break;
		}
	}

	// Call
	auto functorPreamble = (sev::FunctorPreamble *)(&readBlock[readIdx]);
	ptrdiff_t readPtrIdx = readIdx + sizeof(sev::FunctorPreamble);
	SEV_ASSERT(SEV_FUNCTOR_ALIGNED(readPtrIdx) == readPtrIdx);
	caller(args, &readBlock[readPtrIdx], functorPreamble->Vt->Invoke);

	// Destructor
	functorPreamble->Vt->Destroy(&readBlock[readPtrIdx]);

	// Wipe flags
	ptrdiff_t nextReadIdx = readIdx + functorPreamble->Size;
	for (ptrdiff_t idx = readIdx; readIdx < nextReadIdx; readIdx += SEV_FUNCTOR_ALIGN)
		((sev::FunctorPreamble *)(&readBlock[readIdx]))->Ready = 0;

	SEV_AtomicSharedMutex_lockShared(&me->DeleteLock);
	long readShared = _InterlockedDecrement(&readBlockPreamble->ReadShared);
	uint8_t *currentReadBlock = me->ReadBlock;
	SEV_AtomicSharedMutex_unlockShared(&me->DeleteLock);

	if (currentReadBlock != readBlock) // If this block isn't current anymore
	{
		if (!readShared) // New value is 0, no other threads left on this
		{
			// printf("--[Free Block (2)]--\n"); // DEBUG
			// Attempt to release or spare the old block
			sev::wipeBlockOnly(readBlock); // , blockSize);
			// sev::wipeBlock(readBlock, blockSize);
			uint8_t *spareBlock = (uint8_t *)_InterlockedCompareExchangePointer((void *volatile *)(&me->SpareBlock), readBlock, null);
			if (spareBlock) // Old value was not 0, not using this as a spare block
				SEV_alignedFree(readBlock);
		}
	}

	return true;
}

/* end of file */
