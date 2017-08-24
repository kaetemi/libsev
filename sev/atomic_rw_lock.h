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

#ifndef SEV_ATOMIC_RW_LOCK_H
#define SEV_ATOMIC_RW_LOCK_H

#include "config.h"
#ifdef SEV_MODULE_ATOMIC_LOCK

#include <atomic>
#include <thread>

namespace sev {

// TODO: Rename to AtomicSharedLock and follow std::shared_mutex interface (allow std::unique_lock and std::shared_lock access)
// TODO: Rename to AtomicSharedMutex 
//! Lock allowing one writer and multiple readers
class SEV_LIB AtomicRWLock
{
public:
	inline AtomicRWLock() : m_Writing(false), m_Reading(0)
	{

	}

	inline bool tryLockWrite()
	{
		if (m_Writing.exchange(true))
			return false; // Already locked for write
		if (m_Reading) // Successfully locked for write, but busy read
		{
			m_Writing.exchange(false); // Unlock write
			return false; // Already busy for read
		}
		return true; // Successfully locked for write and read
	}

	inline void lockWrite()
	{
		while (m_Writing.exchange(true))
			std::this_thread::yield();
		while (m_Reading)
			std::this_thread::yield();
	}

	inline void unlockWrite()
	{
		m_Writing.exchange(false);
	}

	inline bool tryLockRead()
	{
		++m_Reading;
		if (m_Writing)
		{
			--m_Reading;
			return false;
		}
		return true;
	}

	inline void lockRead()
	{
		++m_Reading;
		while (m_Writing)
		{
			--m_Reading;
			while (m_Writing)
				std::this_thread::yield();
			++m_Reading;
		}
	}

	inline void unlockRead()
	{
		--m_Reading;
	}

private:
	std::atomic_bool m_Writing;
	std::atomic_int m_Reading;

private:
	AtomicRWLock &operator=(const AtomicRWLock&) = delete;
	AtomicRWLock(const AtomicRWLock&) = delete;

}; /* class AtomicRWLock */

} /* namespace sev */

#else /* #ifdef SEV_MODULE_ATOMIC_LOCK */

#include <shared_mutex>

namespace sev {

typedef std::shared_mutex AtomicSharedLock;

} /* namespace sev */

#endif /* #ifdef SEV_MODULE_ATOMIC_LOCK */

#endif /* SEV_ATOMIC_RW_LOCK_H */

/* end of file */
