/*

Copyright (C) 2019  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this 
list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
* Neither the name of the copyright holder nor the names of its contributors 
may be used to endorse or promote products derived from this software without 
specific prior written permission.

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

#ifndef SEV_RING_BUFFER_H
#define SEV_RING_BUFFER_H

#include "self_config.h"
#ifdef SEV_MODULE_RING_BUFFER

#include <memory>

namespace sev {

/**
Ring buffer. It's a queue, but using a contiguous memory space.
Automatically increases capacity as needed.
Does not decrease capacity automatically. No function is provided to shrink.
*/
template<class T, class TAlloc = std::allocator<T>>
class RingBuffer
{
public:
	RingBuffer(size_t capacity = 1024) 
		: m_CapacityMask(capacity - 1)
		, m_WriteIdx(0)
		, m_ReadIdx(0)
		, m_Buffer(TAlloc().allocate(capacity))
	{
		// Ensure the capacity is a power of two
		SEV_ASSERT(!((capacity - 1) & capacity));
		static_SEV_ASSERT(!((1024 - 1) & 1024));
		static_SEV_ASSERT(((1536 - 1) & 1536));
		// Ensure the capacity is at least 4
		SEV_ASSERT(capacity >= 4);
	}

	RingBuffer(const RingBuffer &other)
		: m_CapacityMask(other.m_CapacityMask)
		, m_WriteIdx(other.m_WriteIdx)
		, m_ReadIdx(other.m_ReadIdx)
		, m_Buffer(TAlloc().allocate(m_CapacityMask + 1))
	{
		// Ensure the capacity is a power of two
		const size_t capacity = m_CapacityMask + 1;
		SEV_ASSERT(!((capacity - 1) & capacity));
		SEV_ASSERT(other.m_Buffer);

		// Copy
		const size_t mask = m_CapacityMask;
		const size_t ri = m_ReadIdx;
		const size_t range = (m_WriteIdx - ri) & mask;
		TAlloc a;
		for (size_t i = 0; i < range; ++i)
		{
			std::allocator_traits<TAlloc>::construct(a, &m_Buffer[(ri + i) & mask], other.m_Buffer[(ri + i) & mask]);
		}
	}

	RingBuffer(RingBuffer &&other)
		: m_CapacityMask(other.m_CapacityMask)
		, m_WriteIdx(other.m_WriteIdx)
		, m_ReadIdx(other.m_ReadIdx)
		, m_Buffer(other.m_Buffer)
	{
		// Ensure the capacity is a power of two
		const size_t capacity = m_CapacityMask + 1;
		SEV_ASSERT(!((capacity - 1) & capacity));

		// Clear other after move
		other.m_Buffer = null;
	}

	~RingBuffer()
	{
		if (!m_Buffer) // After move
			return;

		TAlloc a;
		const size_t capacity = m_CapacityMask + 1;
		SEV_ASSERT(!((capacity - 1) & capacity));
		const size_t mask = m_CapacityMask;
		const size_t ri = m_ReadIdx;
		const size_t range = (m_WriteIdx - ri) & mask;
		for (size_t i = 0; i < range; ++i)
		{
			std::allocator_traits<TAlloc>::destroy(a, &m_Buffer[(ri + i) & mask]);
		}
		a.deallocate(m_Buffer, capacity);
#ifndef NDEBUG
		m_Buffer = null;
#endif
	}

	inline void push_back(T &&value)
	{
		pushBack(std::move(value));
	}

	void pushBack(T &&value)
	{
		size_t wi = m_WriteIdx;
		size_t mask = m_CapacityMask;
		{
			const size_t ri = m_ReadIdx;
			const size_t space = (ri - wi - 1) & mask;
			if (space == 0)
			{
				expand();
				wi = m_WriteIdx;
				mask = m_CapacityMask;
			}
		}
		TAlloc a;
		std::allocator_traits<TAlloc>::construct(a, &m_Buffer[wi], std::move(value));
		m_WriteIdx = (wi + 1) & mask;
	}
	
	T front()
	{
		return m_Buffer[ri];
	}

	inline void pop_front()
	{
		popFront();
	}

	T popFront()
	{
		const size_t ri = m_ReadIdx;
		const size_t mask = m_CapacityMask;
		SEV_ASSERT(m_WriteIdx != ri);
		m_ReadIdx = (ri + 1) & mask;
		ScopedDestroy later(m_Buffer[ri]);
		return std::move(m_Buffer[ri]);
	}

	inline size_t capacity()
	{
		const size_t capacity = m_CapacityMask + 1;
		SEV_ASSERT(!((capacity - 1) & capacity));
		return capacity;
	}

	inline size_t size()
	{
		const size_t range = (m_WriteIdx - m_ReadIdx) & m_CapacityMask;
		return range;
	}

private:
	void expand()
	{
		// Expand the buffer
		const size_t mask = m_CapacityMask;
		const size_t ri = m_ReadIdx;
		const size_t wi = m_WriteIdx;
		const size_t capacity = (m_CapacityMask + 1);
		const size_t newCapacity = capacity * 2;
		const size_t newMask = newCapacity - 1;
		TAlloc a;
		SEV_ASSERT(!((capacity - 1) & capacity)); // Pow2
		SEV_ASSERT(!((newCapacity - 1) & capacity));
		T *buffer = m_Buffer;
		T *newBuffer = a.allocate(newCapacity);
		const size_t range = (wi - m_ReadIdx) & mask;
		for (size_t i = 0; i < range; ++i)
		{
			std::allocator_traits<TAlloc>::construct(a, &newBuffer[(ri + i) & newMask], std::move(buffer[(ri + i) & mask]));
			std::allocator_traits<TAlloc>::destroy(a, &buffer[(ri + i) & mask]);
		}
		m_Buffer = newBuffer;
		m_CapacityMask = newMask;
		if (wi < ri)
		{
			const size_t newWi = wi + capacity;
			SEV_ASSERT(newWi < newCapacity);
			m_WriteIdx = newWi;
		}
		a.deallocate(buffer, capacity);
	}

	struct ScopedDestroy
	{
	public:
		ScopedDestroy(T &value) : m_Value(value) { }
		~ScopedDestroy() { TAlloc a; std::allocator_traits<TAlloc>::destroy(a, &m_Value); }
	private:
		T &m_Value;
	};

private:
	T *m_Buffer;
	size_t m_CapacityMask;
	size_t m_WriteIdx;
	size_t m_ReadIdx;

}; /* class RingBuffer */

} /* namespace sev */

#else /* #ifdef SEV_MODULE_RING_BUFFER */

template<class T, class TAlloc>
class RingBuffer : public std::queue<T, TAlloc>
{
public:
	inline void pushBack(T &&value)
	{
		push_back(value);
	}
	
	inline T popFront()
	{
		T value = front();
		pop_front();
		return value;
	}

}; /* class RingBuffer */

} /* namespace sev */

#endif /* #ifdef SEV_MODULE_RING_BUFFER */

#endif /* #ifndef SEV_RING_BUFFER_H */

/* end of file */
