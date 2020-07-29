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

namespace sev {
namespace /* anonymous */ {

struct BlockPreamble
{
	uint8_t *NextBlock;
};

struct FunctorPreamble
{
	volatile ptrdiff_t Ready;
	const SEV_FunctorVt *Vt;
	ptrdiff_t Size;
};

/*
void recursiveRelease(BlockPreamble *blockPreamble)
{
	if (blockPreamble->NextBlock)
		recursiveRelease((BlockPreamble *)blockPreamble->NextBlock);
	_aligned_free(blockPreamble);
}
*/

} /* anonymous namespace */
} /* namespace sev */

#define SEV_BLOCK_PREAMBLE_SIZE (SEV_FUNCTOR_ALIGNED(sizeof(sev::BlockPreamble) + sizeof(sev::FunctorPreamble)))

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
	me->BlockSize = blockSize;
	me->ReadBlock = (uint8_t *)_aligned_malloc(blockSize, SEV_FUNCTOR_ALIGN);
	if (!me->ReadBlock)
	{
		me->WriteBlock = null;
		me->SpareBlock = null;
		return ENOMEM;
	}
	me->WriteBlock = me->ReadBlock;
	me->SpareBlock = (uint8_t *)_aligned_malloc(blockSize, SEV_FUNCTOR_ALIGN); // Also works without, but it will end up allocated anyway when flipping during write (no need to check null)
	me->ReadIdx = SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble);
	me->PreWriteIdx = SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble);
	((sev::BlockPreamble *)me->WriteBlock)->NextBlock = null;
	for (ptrdiff_t i = (SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble)); i < blockSize; i += SEV_FUNCTOR_ALIGN)
		((sev::FunctorPreamble *)&me->WriteBlock[i])->Ready = 0;
	if (me->SpareBlock)
	{
		((sev::BlockPreamble *)me->SpareBlock)->NextBlock = null;
		for (ptrdiff_t i = (SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble)); i < blockSize; i += SEV_FUNCTOR_ALIGN)
			((sev::FunctorPreamble *)&me->SpareBlock[i])->Ready = 0;
	}
	return 0;
}

void SEV_ConcurrentFunctorQueue_release(SEV_ConcurrentFunctorQueue *me)
{
	// Do not release while threads are still accessing this object!

	_aligned_free(me->SpareBlock);
#ifdef SEV_DEBUG
	me->SpareBlock = null;
#endif
	ptrdiff_t idx = me->ReadIdx;
	uint8_t *block = me->ReadBlock;
	const ptrdiff_t blockSize = me->BlockSize;
#ifdef SEV_DEBUG
	me->ReadBlock = null;
#endif
	while (block)
	{
		sev::BlockPreamble *blockPreamble = (sev::BlockPreamble *)block;
		for (ptrdiff_t i = idx; i < blockSize; i += ((sev::FunctorPreamble *)(&block[i]))->Size)
		{
			ptrdiff_t ptrIdx = i + sizeof(sev::FunctorPreamble);
			sev::FunctorPreamble *functorPreamble = (sev::FunctorPreamble *)(&block[i]);
			if (!functorPreamble->Ready)
				break; // No more remaining functors
			functorPreamble->Vt->Destroy(&block[ptrIdx]);
		}
		uint8_t *nextBlock = blockPreamble->NextBlock;
		_aligned_free(block);
		block = nextBlock;
		idx = SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble);
	}
}

errno_t SEV_ConcurrentFunctorQueue_pushFunctor(SEV_ConcurrentFunctorQueue *me, const SEV_FunctorVt *vt, void *ptr, void(*forwardConstructor)(void *ptr, void *other))
{
	// This function only locks while flipping to the next buffer
	// https://docs.microsoft.com/en-us/cpp/intrinsics/compiler-intrinsics?view=vs-2019
	// FIXME: Blocks need to be cleared... (at least the Ready flag on all alignment boundaries...)

	static_assert(sizeof(sev::BlockPreamble) + sizeof(sev::FunctorPreamble) < SEV_BLOCK_PREAMBLE_SIZE);
	const ptrdiff_t sz = SEV_FUNCTOR_ALIGNED(vt->Size + sizeof(sev::FunctorPreamble)); // Pad
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
			std::this_thread::yield();
			idx = me->PreWriteIdx;
			block = me->WriteBlock; // If the block and index change, we'll either have (old idx, old block), (old idx, new block), or (new idx, new block), which is safe here
		}
		nextIdx = idx + sz;
		if (_InterlockedCompareExchangePointer((void *volatile *)(&me->PreWriteIdx), (void *)nextIdx, (void *)idx) != (void *)idx)
		{
			std::this_thread::yield();
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
			block = (uint8_t *)_aligned_malloc(me->BlockSize, SEV_FUNCTOR_ALIGN);
			if (!block)
			{
				errno_t res = errno;
				SEV_ASSERT(res);
				me->PreWriteIdx = lockIdx - sz;
				if (res) return res;
				return ENOMEM;
			}
			((sev::BlockPreamble *)block)->NextBlock = null;
			for (ptrdiff_t i = (SEV_BLOCK_PREAMBLE_SIZE - sizeof(sev::FunctorPreamble)); i < blockSize; i += SEV_FUNCTOR_ALIGN)
				((sev::FunctorPreamble *)&block[i])->Ready = 0;
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

/* end of file */