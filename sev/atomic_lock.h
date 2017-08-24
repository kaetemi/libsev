/*

Copyright (C) 2016-2017  by authors
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

#ifndef SEV_ATOMIC_LOCK_H
#define SEV_ATOMIC_LOCK_H

#include "config.h"
#ifdef SEV_MODULE_ATOMIC_LOCK

#include <atomic>
#include <thread>

namespace sev {

class SEV_LIB AtomicLock
{
public:
	inline AtomicLock()
	{
		m_Atomic.clear();
	}

	inline void lock()
	{
		while (m_Atomic.test_and_set())
			std::this_thread::yield();
	}

	inline bool try_lock()
	{
		return !m_Atomic.test_and_set();
	}

	inline bool tryLock()
	{
		return try_lock();
	}

	inline void unlock()
	{
		m_Atomic.clear();
	}

private:
	std::atomic_flag m_Atomic;

private:
	AtomicLock &operator=(const AtomicLock&) = delete;
	AtomicLock(const AtomicLock&) = delete;

}; /* class AtomicLock */

} /* namespace sev */

#else /* #ifdef SEV_MODULE_ATOMIC_LOCK */

#include <mutex>

namespace sev {

typedef std::mutex AtomicLock;

} /* namespace sev */

#endif /* #ifdef SEV_MODULE_ATOMIC_LOCK */

#endif /* SEV_ATOMIC_LOCK_H */

/* end of file */
