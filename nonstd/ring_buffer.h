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

#ifndef NONSTD_RING_BUFFER_H
#define NONSTD_RING_BUFFER_H

#ifndef NONSTD_SUPPRESS

#include <memory>

namespace nonstd {

/**
Ring buffer. It's a queue, but using a contiguous memory space.
Automatically increases capacity as needed.
Does not decrease capacity automatically. No function is provided to shrink (yet).
*/
template<class T, class TAlloc = std::allocator<T>>
class ring_buffer
{
public:
	ring_buffer(size_t capacity = 1024) 
		: capacity_mask_(capacity - 1)
		, write_idx_(0)
		, read_idx_(0)
		, buffer_(TAlloc().allocate(capacity))
	{
		// Ensure the capacity is a power of two
		SEV_ASSERT(!((capacity - 1) & capacity));
		static_SEV_ASSERT(!((1024 - 1) & 1024));
		static_SEV_ASSERT(((1536 - 1) & 1536));
		// Ensure the capacity is at least 4
		SEV_ASSERT(capacity >= 4);
	}

	ring_buffer(const ring_buffer &other)
		: capacity_mask_(other.capacity_mask_)
		, write_idx_(other.write_idx_)
		, read_idx_(other.read_idx_)
		, buffer_(TAlloc().allocate(capacity_mask_ + 1))
	{
		// Ensure the capacity is a power of two
		const size_t capacity = capacity_mask_ + 1;
		SEV_ASSERT(!((capacity - 1) & capacity));
		SEV_ASSERT(other.buffer_);

		// Copy
		const size_t mask = capacity_mask_;
		const size_t ri = read_idx_;
		const size_t range = (write_idx_ - ri) & mask;
		TAlloc a;
		for (size_t i = 0; i < range; ++i)
		{
			std::allocator_traits<TAlloc>::construct(a, &buffer_[(ri + i) & mask], other.buffer_[(ri + i) & mask]);
		}
	}

	ring_buffer(ring_buffer &&other)
		: capacity_mask_(other.capacity_mask_)
		, write_idx_(other.write_idx_)
		, read_idx_(other.read_idx_)
		, buffer_(other.buffer_)
	{
		// Ensure the capacity is a power of two
		const size_t capacity = capacity_mask_ + 1;
		SEV_ASSERT(!((capacity - 1) & capacity));

		// Clear other after move
		other.buffer_ = null;
	}

	~ring_buffer()
	{
		if (!buffer_) // After move
			return;

		TAlloc a;
		const size_t capacity = capacity_mask_ + 1;
		SEV_ASSERT(!((capacity - 1) & capacity));
		const size_t mask = capacity_mask_;
		const size_t ri = read_idx_;
		const size_t range = (write_idx_ - ri) & mask;
		for (size_t i = 0; i < range; ++i)
		{
			std::allocator_traits<TAlloc>::destroy(a, &buffer_[(ri + i) & mask]);
		}
		a.deallocate(buffer_, capacity);
#ifndef NDEBUG
		buffer_ = null;
#endif
	}

	void push_back(T &&value)
	{
		size_t wi = write_idx_;
		size_t mask = capacity_mask_;
		{
			const size_t ri = read_idx_;
			const size_t space = (ri - wi - 1) & mask;
			if (space == 0)
			{
				expand();
				wi = write_idx_;
				mask = capacity_mask_;
			}
		}
		TAlloc a;
		std::allocator_traits<TAlloc>::construct(a, &buffer_[wi], std::move(value));
		write_idx_ = (wi + 1) & mask;
	}

	T pop_front()
	{
		const size_t ri = read_idx_;
		const size_t mask = capacity_mask_;
		SEV_ASSERT(write_idx_ != ri);
		read_idx_ = (ri + 1) & mask;
		scoped_destroy_ later(buffer_[ri]);
		return std::move(buffer_[ri]);
	}

	bool try_pop(T &value)
	{
		if (size())
		{
			value = std::move(pop_front());
			return true;
		}
		return false;
	}

	T back()
	{
		return buffer_[write_idx_ - 1];
	}

	T front()
	{
		return buffer_[read_idx_];
	}

	inline size_t capacity()
	{
		const size_t capacity = capacity_mask_ + 1;
		SEV_ASSERT(!((capacity - 1) & capacity));
		return capacity;
	}

	inline size_t size()
	{
		const size_t range = (write_idx_ - read_idx_) & capacity_mask_;
		return range;
	}

private:
	void expand()
	{
		// Expand the buffer
		const size_t mask = capacity_mask_;
		const size_t ri = read_idx_;
		const size_t wi = write_idx_;
		const size_t capacity = (capacity_mask_ + 1);
		const size_t newCapacity = capacity * 2;
		const size_t newMask = newCapacity - 1;
		TAlloc a;
		SEV_ASSERT(!((capacity - 1) & capacity)); // Pow2
		SEV_ASSERT(!((newCapacity - 1) & capacity));
		T *buffer = buffer_;
		T *newBuffer = a.allocate(newCapacity);
		const size_t range = (wi - read_idx_) & mask;
		for (size_t i = 0; i < range; ++i)
		{
			std::allocator_traits<TAlloc>::construct(a, &newBuffer[(ri + i) & newMask], std::move(buffer[(ri + i) & mask]));
			std::allocator_traits<TAlloc>::destroy(a, &buffer[(ri + i) & mask]);
		}
		buffer_ = newBuffer;
		capacity_mask_ = newMask;
		if (wi < ri)
		{
			const size_t newWi = wi + capacity;
			SEV_ASSERT(newWi < newCapacity);
			write_idx_ = newWi;
		}
		a.deallocate(buffer, capacity);
	}

	struct scoped_destroy_
	{
	public:
		scoped_destroy_(T &value) : value_(value) { }
		~scoped_destroy_() { TAlloc a; std::allocator_traits<TAlloc>::destroy(a, &value_); }
	private:
		T &value_;
	};

private:
	T *buffer_;
	size_t capacity_mask_;
	size_t write_idx_;
	size_t read_idx_;

}; /* class ring_buffer */

} /* namespace nonstd */

#else /* #ifndef NONSTD_SUPPRESS */

template<class T, class TAlloc>
class ring_buffer : public std::queue<T, TAlloc>
{
public:
	inline void push_back(T &&value)
	{
		push_back(value);
	}

	inline T pop_front()
	{
		T value = front();
		std::queue<T, TAlloc>::pop_front();
		return value;
	}

	bool try_pop(T &value)
	{
		if (size())
		{
			value = std::move(pop_front());
			return true;
		}
		return false;
	}

}; /* class ring_buffer */

} /* namespace nonstd */

#endif /* #ifndef NONSTD_SUPPRESS */

#endif /* #ifndef NONSTD_RING_BUFFER_H */

/* end of file */
