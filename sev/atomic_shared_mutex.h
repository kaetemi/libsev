/*

Copyright (C) 2017-2020  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
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

#pragma once
#ifndef SEV_ATOMIC_SHARED_MUTEX_H
#define SEV_ATOMIC_SHARED_MUTEX_H

#include "platform.h"

#ifdef __cplusplus
#include <atomic>
#include <thread>
#endif

// #define SEV_SUPPRESS_ATOMIC_MUTEX

#ifdef __cplusplus
extern "C" {
#endif

typedef volatile LONG SEV_AtomicInt32;
typedef volatile ptrdiff_t SEV_AtomicPtrDiff;
typedef volatile PVOID SEV_AtomicPtr;

static_assert(sizeof(int32_t) == sizeof(SEV_AtomicInt32));
static_assert(sizeof(ptrdiff_t) == sizeof(SEV_AtomicPtrDiff));
static_assert(sizeof(void *) == sizeof(SEV_AtomicPtr));

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
static_assert(sizeof(std::atomic_int32_t) == sizeof(SEV_AtomicInt32));
static_assert(sizeof(std::atomic_ptrdiff_t) == sizeof(SEV_AtomicPtrDiff));
static_assert(sizeof(std::atomic_ptrdiff_t) == sizeof(SEV_AtomicPtr));
#endif

static inline int32_t SEV_AtomicInt32_load(SEV_AtomicInt32 *src)
{
#if defined(__cplusplus)
	return ((std::atomic_int32_t *)src)->load();
#elif defined(_WIN32)
	return InterlockedCompareExchange(src, 0, 0);
#else
	static_assert(false);
#endif
}

static inline void SEV_AtomicInt32_store(SEV_AtomicInt32 *dst, int32_t val)
{
#if defined(__cplusplus)
	((std::atomic_int32_t *)dst)->store(val);
#elif defined(_WIN32)
	InterlockedExchange(dst, val);
#else
	static_assert(false);
#endif
}

static inline int32_t SEV_AtomicInt32_exchange(SEV_AtomicInt32 *dst, int32_t val)
{
#ifdef _WIN32
	return InterlockedExchange(dst, val);
#else
	static_assert(false);
#endif
}

static inline int32_t SEV_AtomicInt32_compareExchange(SEV_AtomicInt32 *dst, int32_t exch, int32_t comp)
{
#ifdef _WIN32
	return InterlockedCompareExchange(dst, exch, comp);
#else
	static_assert(false);
#endif
}

static inline int32_t SEV_AtomicInt32_increment(SEV_AtomicInt32 *var)
{
#ifdef _WIN32
	return InterlockedIncrement(var);
#else
	static_assert(false);
#endif
}

static inline int32_t SEV_AtomicInt32_decrement(SEV_AtomicInt32 *var)
{
#ifdef _WIN32
	return InterlockedDecrement(var);
#else
	static_assert(false);
#endif
}

static inline ptrdiff_t SEV_AtomicPtrDiff_load(SEV_AtomicPtrDiff *src)
{
#if defined(__cplusplus)
	return ((std::atomic_ptrdiff_t *)src)->load();
#elif defined(_WIN32)
	return (ptrdiff_t)InterlockedCompareExchangePointer((volatile PVOID *)src, null,null);
#else
	static_assert(false);
#endif
}

static inline void SEV_AtomicPtrDiff_store(SEV_AtomicPtrDiff *dst, ptrdiff_t val)
{
#if defined(__cplusplus)
	((std::atomic_ptrdiff_t *)dst)->store(val);
#elif defined(_WIN32)
	InterlockedExchangePointer((volatile PVOID *)dst, (PVOID)val);
#else
	static_assert(false);
#endif
}

static inline ptrdiff_t SEV_AtomicPtrDiff_exchange(SEV_AtomicPtrDiff *dst, ptrdiff_t val)
{
#ifdef _WIN32
	return (ptrdiff_t)InterlockedExchangePointer((volatile PVOID *)dst, (PVOID)val);
#else
	static_assert(false);
#endif
}

static inline ptrdiff_t SEV_AtomicPtrDiff_compareExchange(SEV_AtomicPtrDiff *dst, ptrdiff_t exch, ptrdiff_t comp)
{
#ifdef _WIN32
	return (ptrdiff_t)InterlockedCompareExchangePointer((volatile PVOID *)dst, (PVOID)exch, (PVOID)comp);
#else
	static_assert(false);
#endif
}

static inline ptrdiff_t SEV_AtomicPtrDiff_increment(SEV_AtomicPtrDiff *var)
{
#ifdef _WIN32
	return InterlockedIncrementSizeT(var);
#else
	static_assert(false);
#endif
}

static inline ptrdiff_t SEV_AtomicPtrDiff_decrement(SEV_AtomicPtrDiff *var)
{
#ifdef _WIN32
	return InterlockedDecrementSizeT(var);
#else
	static_assert(false);
#endif
}

static inline void *SEV_AtomicPtr_load(SEV_AtomicPtr *src)
{
#if defined(__cplusplus)
	return (void *)((std::atomic_ptrdiff_t *)src)->load();
#elif defined(_WIN32)
	return (void *)InterlockedCompareExchangePointer(src, null,null);
#else
	static_assert(false);
#endif
}

static inline void SEV_AtomicPtr_store(SEV_AtomicPtr *dst, void *val)
{
#if defined(__cplusplus)
	((std::atomic_ptrdiff_t *)dst)->store((ptrdiff_t)val);
#elif defined(_WIN32)
	InterlockedExchangePointer(dst, val);
#else
	static_assert(false);
#endif
}

static inline void *SEV_AtomicPtr_exchange(SEV_AtomicPtr *dst, void *val)
{
#ifdef _WIN32
	return InterlockedExchangePointer(dst, val);
#else
	static_assert(false);
#endif
}

static inline void *SEV_AtomicPtr_compareExchange(SEV_AtomicPtr *dst, void *exch, void *comp)
{
#ifdef _WIN32
	return InterlockedCompareExchangePointer(dst, exch, comp);
#else
	static_assert(false);
#endif
}

static inline void SEV_Thread_yield()
{
#if defined(__cplusplus)
	std::this_thread::yield();
#elif defined(_WIN32)
	SwitchToThread();
#else
	static_assert(false);
#endif
}

#ifdef __cplusplus
extern "C" {
#endif

struct SEV_AtomicSharedMutex
{
	SEV_AtomicInt32 Unique; // 0 or 1
	SEV_AtomicInt32 Sharing; // 0 to infinity-ish

};

static inline void SEV_AtomicSharedMutex_init(SEV_AtomicSharedMutex *me)
{
	(*me) = { 0, 0 };
}

static inline bool SEV_AtomicSharedMutex_tryLock(SEV_AtomicSharedMutex *me)
{
	if (SEV_AtomicInt32_exchange(&me->Unique, 1))
		return false; // Already locked for unique
	if (SEV_AtomicInt32_load(&me->Sharing)) // Successfully locked for unique, but busy sharing
	{
		SEV_AtomicInt32_store(&me->Unique, 0); // Unlock unique
		return false; // Already busy for sharing
	}
	return true; // Successfully locked for unique and sharing
}

static inline void SEV_AtomicSharedMutex_lock(SEV_AtomicSharedMutex *me)
{
	while (SEV_AtomicInt32_exchange(&me->Unique, 1))
		SEV_Thread_yield();
	while (SEV_AtomicInt32_load(&me->Sharing))
		SEV_Thread_yield();
}

static inline void SEV_AtomicSharedMutex_unlock(SEV_AtomicSharedMutex *me)
{
#ifdef SEV_DEBUG
	if (!_InterlockedExchange(&me->Unique, 0))
		SEV_DEBUG_BREAK();
#else
	me->Unique = 0;
#endif
}

static inline bool SEV_AtomicSharedMutex_tryLockShared(SEV_AtomicSharedMutex *me)
{
	SEV_AtomicInt32_increment(&me->Sharing);
	if (me->Unique)
	{
		SEV_AtomicInt32_decrement(&me->Sharing);
		return false;
	}
	return true;
}

static inline void SEV_AtomicSharedMutex_lockShared(SEV_AtomicSharedMutex *me)
{
	SEV_AtomicInt32_increment(&me->Sharing);
	while (me->Unique)
	{
		SEV_AtomicInt32_decrement(&me->Sharing);
		while (me->Unique)
			SEV_Thread_yield();
		SEV_AtomicInt32_increment(&me->Sharing);
	}
}

static inline void SEV_AtomicSharedMutex_unlockShared(SEV_AtomicSharedMutex *me)
{
	SEV_AtomicInt32_decrement(&me->Sharing);
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#ifndef SEV_SUPPRESS_ATOMIC_MUTEX

#include <atomic>
#include <thread>

#include "debugbreak.h"

namespace sev {

#if 1
//! Lock allowing one unique writer and multiple shared readers
class AtomicSharedMutex
{
public:
	inline AtomicSharedMutex() noexcept : m{ 0, 0 }
	{

	}

#ifdef SEV_DEBUG
	inline ~AtomicSharedMutex() noexcept
	{
		if (!try_lock())
			debug_break(); // Must be unlocked before destroying
	}
#endif

	inline bool try_lock() noexcept
	{
		return SEV_AtomicSharedMutex_tryLock(&m);
	}

	inline bool tryLock() noexcept
	{
		return try_lock();
	}

	inline void lock() noexcept
	{
		SEV_AtomicSharedMutex_lock(&m);
	}

	inline void unlock() noexcept
	{
		SEV_AtomicSharedMutex_unlock(&m);
	}

	inline bool try_lock_shared() noexcept
	{
		return SEV_AtomicSharedMutex_tryLockShared(&m);
	}
	
	inline bool tryLockShared() noexcept
	{
		return try_lock_shared();
	}

	inline void lock_shared() noexcept
	{
		SEV_AtomicSharedMutex_lockShared(&m);
	}
	
	inline void lockShared() noexcept
	{
		lock_shared();
	}
	
	inline void unlock_shared() noexcept
	{
		SEV_AtomicSharedMutex_unlockShared(&m);
	}
	
	inline void unlockShared() noexcept
	{
		unlock_shared();
	}

private:
	SEV_AtomicSharedMutex m;

private:
	AtomicSharedMutex &operator=(const AtomicSharedMutex&) = delete;
	AtomicSharedMutex(const AtomicSharedMutex&) = delete;

}; /* class AtomicSharedMutex */
#else
//! Lock allowing one unique writer and multiple shared readers
class AtomicSharedMutex
{
public:
	inline AtomicSharedMutex() noexcept : m_Unique(false), m_Sharing(0)
	{

	}

#ifdef SEV_DEBUG
	inline ~AtomicSharedMutex() noexcept
	{
		if (!try_lock())
			debug_break(); // Must be unlocked before destroying
	}
#endif

	inline bool try_lock() noexcept
	{
		if (m_Unique.exchange(true))
			return false; // Already locked for unique
		if (m_Sharing) // Successfully locked for unique, but busy sharing
		{
			m_Unique.exchange(false); // Unlock unique
			return false; // Already busy for sharing
		}
		return true; // Successfully locked for unique and sharing
	}

	inline bool tryLock() noexcept
	{
		return try_lock();
	}

	inline void lock() noexcept
	{
		while (m_Unique.exchange(true))
			std::this_thread::yield();
		while (m_Sharing)
			std::this_thread::yield();
	}

	inline void unlock() noexcept
	{
#ifndef NDEBUG
		if (!m_Unique.exchange(false))
			debug_break(); // Already unlocked before
#else
		m_Unique.exchange(false);
#endif
	}

	inline bool try_lock_shared() noexcept
	{
		++m_Sharing;
		if (m_Unique)
		{
			--m_Sharing;
			return false;
		}
		return true;
	}

	inline bool tryLockShared() noexcept
	{
		return try_lock_shared();
	}

	inline void lock_shared() noexcept
	{
		++m_Sharing;
		while (m_Unique)
		{
			--m_Sharing;
			while (m_Unique)
				std::this_thread::yield();
			++m_Sharing;
		}
	}

	inline void lockShared() noexcept
	{
		lock_shared();
	}

	inline void unlock_shared() noexcept
	{
		--m_Sharing;
	}

	inline void unlockShared() noexcept
	{
		unlock_shared();
	}

private:
	std::atomic_bool m_Unique;
	std::atomic_int m_Sharing;

private:
	AtomicSharedMutex &operator=(const AtomicSharedMutex&) = delete;
	AtomicSharedMutex(const AtomicSharedMutex&) = delete;

}; /* class AtomicSharedMutex */
#endif

} /* namespace sev */

#else /* #ifndef SEV_SUPPRESS_ATOMIC_MUTEX */

#include <shared_mutex>

namespace sev {

//! Lock allowing one unique writer and multiple shared readers
class AtomicSharedMutex
{
public:
	inline AtomicSharedMutex() noexcept
	{

	}

	inline bool try_lock() noexcept
	{
		return m_Mutex.try_lock();
	}

	inline bool tryLock() noexcept
	{
		return try_lock();
	}

	inline void lock() noexcept
	{
		m_Mutex.lock();
	}

	inline void unlock() noexcept
	{
		m_Mutex.unlock();
	}

	inline bool try_lock_shared() noexcept
	{
		return m_Mutex.try_lock_shared();
	}

	inline bool tryLockShared() noexcept
	{
		return try_lock_shared();
	}

	inline void lock_shared() noexcept
	{
		m_Mutex.lock_shared();
	}

	inline void lockShared() noexcept
	{
		lock_shared();
	}

	inline void unlock_shared() noexcept
	{
		m_Mutex.unlock_shared();
	}

	inline void unlockShared() noexcept
	{
		unlock_shared();
	}

private:
	std::shared_mutex m_Mutex;

private:
	AtomicSharedMutex &operator=(const AtomicSharedMutex&) = delete;
	AtomicSharedMutex(const AtomicSharedMutex&) = delete;

}; /* class AtomicSharedMutex */

} /* namespace sev */

#endif /* #ifndef SEV_SUPPRESS_ATOMIC_MUTEX */

#endif /* #ifdef __cplusplus */

#endif /* SEV_ATOMIC_SHARED_MUTEX_H */

/* end of file */
