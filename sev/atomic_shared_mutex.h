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

// #define SEV_SUPPRESS_ATOMIC_MUTEX

#ifdef __cplusplus

#ifndef SEV_SUPPRESS_ATOMIC_MUTEX

#include <atomic>
#include <thread>

#include "debugbreak.h"

namespace sev {

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
