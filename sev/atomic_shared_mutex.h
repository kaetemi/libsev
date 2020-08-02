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
#include "atomic.h"

#ifdef __cplusplus
#include <thread>
#endif

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

static inline bool SEV_AtomicSharedMutex_tryPartialLock(SEV_AtomicSharedMutex *me) // If no other thread has a unique lock, a partial unique lock is obtained, and new threads are blocked from obtaining a shared lock. Other threads may still hold an existing shared lock
{
	if (SEV_AtomicInt32_exchange(&me->Unique, 1))
		return false; // Already locked for unique
	return true; // Successfully locked for unique
}

static inline void SEV_AtomicSharedMutex_cancelPartialLock(SEV_AtomicSharedMutex *me) // Unlock an obtained partial lock without completing it
{
#ifdef SEV_DEBUG
	if (!SEV_AtomicInt32_exchange(&me->Unique, 0))
		SEV_DEBUG_BREAK();
#else
	SEV_AtomicInt32_store(&me->Unique, 0);
#endif
}

static inline void SEV_AtomicSharedMutex_completePartialLock(SEV_AtomicSharedMutex *me) // Blocks until all shared locks are unlocked, a full unique lock is now obtained
{
	SEV_ASSERT(SEV_AtomicInt32_load(&me->Unique));
	while (SEV_AtomicInt32_load(&me->Sharing))
		SEV_Thread_yield();
}

static inline void SEV_AtomicSharedMutex_downgradeLock(SEV_AtomicSharedMutex *me) // Downgrades a unique lock or a partial unique lock into a shared lock
{
	SEV_AtomicInt32_increment(&me->Sharing);
#ifdef SEV_DEBUG
	if (!SEV_AtomicInt32_exchange(&me->Unique, 0))
		SEV_DEBUG_BREAK();
#else
	SEV_AtomicInt32_store(&me->Unique, 0);
#endif
}

static inline void SEV_AtomicSharedMutex_upgradeLock(SEV_AtomicSharedMutex *me) // Upgrades a shared lock to a unique lock
{
	while (SEV_AtomicInt32_exchange(&me->Unique, 1))
		SEV_Thread_yield();
	int32_t sharing = SEV_AtomicInt32_decrement(&me->Sharing);
	while (sharing)
	{
		SEV_Thread_yield();
		sharing = SEV_AtomicInt32_load(&me->Sharing);
	}
}

static inline bool SEV_AtomicSharedMutex_isLocked(SEV_AtomicSharedMutex *me) // Returns true if a unique or partial unique lock exists
{
	return SEV_AtomicInt32_load(&me->Unique);
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
	if (!SEV_AtomicInt32_exchange(&me->Unique, 0))
		SEV_DEBUG_BREAK();
#else
	SEV_AtomicInt32_store(&me->Unique, 0);
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
