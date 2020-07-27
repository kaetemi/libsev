/*

Copyright (C) 2016-2020  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
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
#ifndef SEV_ATOMIC_MUTEX_H
#define SEV_ATOMIC_MUTEX_H

#include "platform.h"

// #define SEV_SUPPRESS_ATOMIC_MUTEX

#ifndef SEV_SUPPRESS_ATOMIC_MUTEX

#include <atomic>
#include <thread>

namespace sev {

class AtomicMutex
{
public:
	inline AtomicMutex() noexcept
	{
		m_Atomic.clear();
	}

#ifdef SEV_DEBUG
	inline ~AtomicMutex() noexcept
	{
		if (!tryLock())
			debug_break(); // Must be unlocked before destroying
	}
#endif

	inline void lock() noexcept
	{
		while (m_Atomic.test_and_set())
			std::this_thread::yield();
	}

	inline bool try_lock() noexcept
	{
		return !m_Atomic.test_and_set();
	}

	inline bool tryLock() noexcept
	{
		return try_lock();
	}

	inline void unlock() noexcept
	{
		m_Atomic.clear();
	}

private:
	std::atomic_flag m_Atomic;

private:
	AtomicMutex &operator=(const AtomicMutex&) = delete;
	AtomicMutex(const AtomicMutex&) = delete;

}; /* class AtomicMutex */

} /* namespace sev */

#else /* #ifndef SEV_SUPPRESS_ATOMIC_MUTEX */

#include <mutex>

namespace sev {

class AtomicMutex
{
public:
	inline AtomicMutex()
	{
		
	}

	inline void lock()
	{
		m_Mutex.lock();
	}

	inline bool try_lock()
	{
		return m_Mutex.try_lock();
	}

	inline bool tryLock()
	{
		return try_lock();
	}

	inline void unlock()
	{
#pragma warning(push)
#pragma warning(disable: 26110)
		m_Mutex.unlock();
#pragma warning(pop)
	}

private:
	std::mutex m_Mutex;

private:
	AtomicMutex &operator=(const AtomicMutex&) = delete;
	AtomicMutex(const AtomicMutex&) = delete;

}; /* class AtomicMutex */

} /* namespace sev */

#endif /* #ifndef SEV_SUPPRESS_ATOMIC_MUTEX */

#endif /* #ifndef SEV_ATOMIC_MUTEX_H */

/* end of file */
