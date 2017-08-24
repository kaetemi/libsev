/*

Copyright (C) 2017  by authors
Author: Jan Boon <jan.boon@kaetemi.be>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

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

#ifndef SEV_ATOMIC_SHARED_MUTEX_H
#define SEV_ATOMIC_SHARED_MUTEX_H

#include "config.h"
#ifdef SEV_MODULE_ATOMIC_MUTEX

#include <atomic>
#include <thread>

namespace sev {

//! Lock allowing one unique writer and multiple shared readers
class SEV_LIB AtomicSharedMutex
{
public:
	inline AtomicSharedMutex() : m_Unique(false), m_Sharing(0)
	{

	}

#ifdef SEV_DEBUG
	inline ~AtomicSharedMutex()
	{
		if (!tryLock())
			SEV_DEBUG_BREAK(); // Must be unlocked before destroying
	}
#endif

	inline bool try_lock()
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

	inline bool tryLock()
	{
		return try_lock();
	}

	inline void lock()
	{
		while (m_Unique.exchange(true))
			std::this_thread::yield();
		while (m_Sharing)
			std::this_thread::yield();
	}

	inline void unlock()
	{
#ifdef SEV_DEBUG
		if (!m_Unique.exchange(false))
			SEV_DEBUG_BREAK(); // Already unlocked before
#else
		m_Unique.exchange(false);
#endif
	}

	inline bool try_lock_shared()
	{
		++m_Sharing;
		if (m_Unique)
		{
			--m_Sharing;
			return false;
		}
		return true;
	}

	inline bool tryLockShared()
	{
		return try_lock_shared();
	}

	inline void lock_shared()
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

	inline void lockShared()
	{
		lock_shared();
	}

	inline void unlock_shared()
	{
		--m_Sharing;
	}

	inline void unlockShared()
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

#else /* #ifdef SEV_MODULE_ATOMIC_MUTEX */

#include <shared_mutex>

namespace sev {

class SEV_LIB AtomicMutex : public std::shared_mutex
{
public:
	inline bool tryLock()
	{
		return try_lock();
	}

	inline bool tryLockShared()
	{
		return try_lock_shared();
	}

	inline void lockShared()
	{
		lock_shared();
	}

	inline void unlockShared()
	{
		unlock_shared();
	}

}; /* class AtomicMutex */

} /* namespace sev */

#endif /* #ifdef SEV_MODULE_ATOMIC_MUTEX */

#endif /* SEV_ATOMIC_SHARED_MUTEX_H */

/* end of file */
