/*

Copyright (C) 2019  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
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

#ifndef ADDTL_CONCURRENT_BUCKET_MAP_H
#define ADDTL_CONCURRENT_BUCKET_MAP_H

// #ifndef ADDTL_SUPPRESS

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace addtl {

/**

Concurrent bucket map.

Optimized for cases where keys are near to each other.
Allocates space for values in buckets.
Indexes into buckets in a recursive manner, guaranteeing a O(1) lookup.
Iterating the map is slower and unoptimized.
Inserting values from multiple threads is supported, searching from multiple threads is safe too.
Removing values is not supported, as it is not thread safe.
Inserting two values with the same key is only thread safe when the value type is a basic type.
Inserting two identical values with the same key is only thread safe when the value type is a POD struct.
Clear is only thread safe for POD structs.
If the value is a complex type, it should be wrapped inside an std::atomic<...> to ensure safe access.
If the value has significant internal construction behaviour, consider std::atomic<std::optional<...>>.
Index size is the number of bits to use for indexing at each bucket depth level.

TODO: Match concurrent_map interface, set as alternative implementation for testing.

It's possible to implement an std::vector-like container on top of this, effectively allocating values in sequence.

\author Jan BOON (Kaetemi) <jan.boon@kaetemi.be>

*/
template<class TKey, class TValue, size_t tIndexSizeBits = 8, size_t tKeyBits = (sizeof(TKey) * 8), size_t tDepth = ((key_bits + (index_size_bits - 1)) / index_size_bits)>
class concurrent_bucket_map
{
public:
	using key_t = TKey;
	using value_t = TValue;

	static constexpr size_t key_bits = tKeyBits;
	static constexpr size_t index_size_bits = tIndexSizeBits; // 8 bits index per bucket level
	static constexpr size_t bucket_depth = tDepth; // 2, 4, or 8 bytes
	static constexpr size_t bucket_size = 1ULL << index_size_bits;
	static constexpr size_t bucket_mask = bucket_size - 1;
	
	static constexpr size_t root_index_size_bits = key_bits - (index_size_bits * (bucket_depth - 1));
	static constexpr size_t root_bucket_size = 1ULL << root_index_size_bits;
	static constexpr size_t root_bucket_mask = root_bucket_size - 1;
	
private:
	using data_t_ = TValue;
	
public:
	inline concurrent_bucket_map()
	{
		static_assert(bucket_depth >= 2, "Lower depth is useless!");
		buckets_ = static_cast<void *>(new void *[root_bucket_size]);
		memset(buckets_, 0, sizeof(void *) * root_bucket_size);
	}
	
	inline ~concurrent_bucket_map()
	{
		release_(buckets_, 0);
	}
	
	/// Get the value of a key without checking if it exists. This is thread safe.
	/// Attempting to get a key that doesn't exist causes undefined behaviour.
	inline const TValue &get_unchecked(TKey key)
	{
		if (index_size_bits == 8 && bucket_depth == 2)
		{
			uint16_t k = key;
			data_t_ **d = static_cast<data_t_**>(buckets_);
			return d
				[(key >> 8) & root_bucket_mask]
				[key & bucket_mask];
		}
		else if (index_size_bits == 8 && bucket_depth == 4)
		{
			uint32_t k = key;
			data_t_ ****d = static_cast<data_t_****>(buckets_);
			return d
				[(key >> 24) & root_bucket_mask]
				[(key >> 16) & bucket_mask]
				[(key >> 8) & bucket_mask]
				[key & bucket_mask];
		}
		else if (index_size_bits == 8 && bucket_depth == 8)
		{
			uint64_t k = key;
			data_t_ ********d = static_cast<data_t_********>(buckets_);
			return d
				[(key >> 56) & root_bucket_mask]
				[(key >> 48) & bucket_mask]
				[(key >> 40) & bucket_mask]
				[(key >> 32) & bucket_mask]
				[(key >> 24) & bucket_mask]
				[(key >> 16) & bucket_mask]
				[(key >> 8) & bucket_mask]
				[key & bucket_mask];
		}
		else
		{
			return *find_bucket_(key);
		}
	}
	
	/// Insert a value into the map.
	inline void insert(TKey key, TValue &&value)
	{
		make_bucket_(key) = std::move(value);
	}

	/// Find a value, always returns a usable value.
	inline TValue &find_or_emplace(TKey key)
	{
		return make_bucket_(key);
	}
	
	/// Check if this map contains the specified key.
	inline bool contains(TKey key)
	{
		return find_bucket_(key);
	}
	
	/// Try to get a value, nullptr if not found at all. Returned value may be a default value.
	inline TValue *find(TKey key)
	{
		return find_bucket_(key);
	}

	/// Clears all entries to their default value. Does not release memory.
	inline void clear()
	{
		release_(buckets_, 0);
	}
	
private:
	void release_(void *buckets, size_t depth)
	{
		size_t next_depth = depth + 1;
		if (next_depth < bucket_depth)
		{
			void **b = static_cast<void **>(buckets);
			for (size_t i = 0; i < bucket_size; ++i)
			{
				if (b[i])
					release_(b[i], next_depth);
			}
			delete[] b;
		}
		else
		{
			data_t_ *d = static_cast<data_t_ *>(buckets);
			delete[] d;
		}
	}

	void clear_(void *buckets, size_t depth)
	{
		size_t next_depth = depth + 1;
		if (next_depth < bucket_depth)
		{
			void **b = static_cast<void **>(buckets);
			for (size_t i = 0; i < bucket_size; ++i)
			{
				if (b[i])
					clear_(b[i], next_depth);
			}
		}
		else
		{
			data_t_ *d = static_cast<data_t_ *>(buckets);
			for (size_t i = 0; i < bucket_size; ++i)
			{
				// *d = std::move(data_t_());
				d->~data_t_();
				new (d) data_t();
			}
		}
	}
	
	inline TValue *find_bucket_(TKey key)
	{
		if (index_size_bits == 8 && bucket_depth == 2)
		{
			uint16_t k = key & 0xFFFF;
			data_t_ **d1 = static_cast<data_t_**>(buckets_);
			data_t_ *d0 = d1[(k >> 8) & root_bucket_mask];
			if (!d0) return nullptr;
			return &d0[k & bucket_mask];
		}
		else if (index_size_bits == 8 && bucket_depth == 4)
		{
			uint32_t k = key & 0xFFFFFFFF;
			data_t_ ****d3 = static_cast<data_t_****>(buckets_);
			data_t_ ***d2 = d3[(k >> 24) & root_bucket_mask];
			if (!d2) return nullptr;
			data_t_ **d1 = d2[(k >> 16) & bucket_mask];
			if (!d1) return nullptr;
			data_t_ *d0 = d1[(k >> 8) & bucket_mask];
			if (!d0) return nullptr;
			return &d0[k & bucket_mask];
		}
		else if (index_size_bits == 8 && bucket_depth == 8)
		{
			uint64_t k = key;
			data_t_ ********d7 = static_cast<data_t_********>(buckets_);
			data_t_ *******d6 = d7[(k >> 56) & root_bucket_mask];
			if (!d6) return nullptr;
			data_t_ ******d5 = d6[(k >> 48) & bucket_mask];
			if (!d5) return nullptr;
			data_t_ *****d4 = d5[(k >> 40) & bucket_mask];
			if (!d4) return nullptr;
			data_t_ ****d3 = d4[(k >> 32) & bucket_mask];
			if (!d3) return nullptr;
			data_t_ ***d2 = d3[(k >> 24) & bucket_mask];
			if (!d2) return nullptr;
			data_t_ **d1 = d2[(k >> 16) & bucket_mask];
			if (!d1) return nullptr;
			data_t_ *d0 = d1[(k >> 8) & bucket_mask];
			if (!d0) return nullptr;
			return &d0[k & bucket_mask];
		}
		else
		{
			return find_bucket_(key, buckets_, 0);
		}
	}

	static_assert(sizeof(TKey) <= sizeof(uint64_t), "Key size too large");
	inline data_t_ *find_bucket_(uint64_t key, void *buckets, size_t depth)
	{
		size_t next_depth = depth + 1;
		size_t i = (key >> (index_size_bits * (bucket_depth - next_depth))) & (depth ? bucket_mask : root_bucket_mask);
		if (next_depth < bucket_depth)
		{
			void **b = static_cast<void **>(buckets);
			if (!b[i])
				return NULL;
			return find_bucket_(key, b[i], next_depth);
		}
		else
		{
			data_t_ *d = static_cast<data_t_ *>(buckets);
			return &d[i];
		}
	}
	
	data_t_ &make_bucket_(TKey key)
	{
		if (index_size_bits == 8 && bucket_depth == 2)
		{
			uint16_t k = key & 0xFFFF;
			data_t_ **d1 = static_cast<data_t_**>(buckets_);
			data_t_ *d0 = d1[(k >> 8) & root_bucket_mask];
			if (!d0) return make_bucket_(key, d1, 0);
			return d0[k & bucket_mask];
		}
		else if (index_size_bits == 8 && bucket_depth == 4)
		{
			uint32_t k = key & 0xFFFFFFFF;
			data_t_ ****d3 = static_cast<data_t_****>(buckets_);
			data_t_ ***d2 = d3[(k >> 24) & root_bucket_mask];
			if (!d2) return make_bucket_(key, d3, 0);
			data_t_ **d1 = d2[(k >> 16) & bucket_mask];
			if (!d1) return make_bucket_(key, d2, 1);
			data_t_ *d0 = d1[(k >> 8) & bucket_mask];
			if (!d0) return make_bucket_(key, d1, 2);
			return d0[k & bucket_mask];
		}
		else if (index_size_bits == 8 && bucket_depth == 8)
		{
			uint64_t k = key;
			data_t_ ********d7 = static_cast<data_t_********>(buckets_);
			data_t_ *******d6 = d7[(k >> 56) & root_bucket_mask];
			if (!d6) return make_bucket_(key, d7, 0);
			data_t_ ******d5 = d6[(k >> 48) & bucket_mask];
			if (!d5) return make_bucket_(key, d6, 1);
			data_t_ *****d4 = d5[(k >> 40) & bucket_mask];
			if (!d4) return make_bucket_(key, d5, 2);
			data_t_ ****d3 = d4[(k >> 32) & bucket_mask];
			if (!d3) return make_bucket_(key, d4, 3);
			data_t_ ***d2 = d3[(k >> 24) & bucket_mask];
			if (!d2) return make_bucket_(key, d3, 4);
			data_t_ **d1 = d2[(k >> 16) & bucket_mask];
			if (!d1) return make_bucket_(key, d2, 5);
			data_t_ *d0 = d1[(k >> 8) & bucket_mask];
			if (!d0) return make_bucket_(key, d1, 6);
			return d0[k & bucket_mask];
		}
		else
		{
			return make_bucket_(key, buckets_, 0);
		}
	}

	static_assert(sizeof(TKey) <= sizeof(uint64_t), "Key size too large");
	data_t_ &make_bucket_(uint64_t key, void *buckets, size_t depth)
	{
		size_t next_depth = depth + 1;
		size_t i = (key >> (index_size_bits * (bucket_depth - next_depth))) & (depth ? bucket_mask : root_bucket_mask);
		if (next_depth < bucket_depth)
		{
			void **b = static_cast<void **>(buckets);
			if (!b[i])
			{
				// Doesn't exist yet, create
				void *ptr;
				if (next_depth + 1 < bucket_depth)
				{
					ptr = static_cast<void *>(new void *[bucket_size]);
					memset(ptr, 0, sizeof(void *) * bucket_size);
				}
				else
				{
					ptr = static_cast<void *>(new data_t_[bucket_size]);
				}
				void *volatile *d = static_cast<void *volatile *>(&b[i]);
				// Set safely
#if defined(_MSC_VER)
				void *old = _InterlockedCompareExchangePointer(d, ptr, nullptr); // TODO: GCC
#elif defined(__GNUC__) || defined(__clang__)
				void *old = __sync_val_compare_and_swap(d, ptr, nullptr);
#else
				static_assert(false, "Interlocked compare exchange not implemented");
#endif
				if (old)
				{
					// Already set by another thread, remove, and use old one
					if (next_depth + 1 < bucket_depth)
						delete[] static_cast<void **>(ptr);
					else
						delete[] static_cast<data_t_ *>(ptr);
					return make_bucket_(key, old, next_depth);
				}
				else
				{
					// Use the new one
					return make_bucket_(key, ptr, next_depth);
				}
			}
			return make_bucket_(key, b[i], next_depth);
		}
		else
		{
			data_t_ *d = static_cast<data_t_ *>(buckets);
			return d[i];
		}
	}
	
private:
	void *buckets_;

}; /* class concurrent_bucket_map */

#endif /* #ifndef ADDTL_CONCURRENT_BUCKET_MAP_H */

} /* namespace addtl */

// #endif /* #ifndef ADDTL_SUPPRESS */

/* end of file */
