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

#ifndef NONSTD_ATOMIC_MUTEX_H
#define NONSTD_ATOMIC_MUTEX_H

#ifndef NONSTD_SUPPRESS

#include <atomic>
#include <thread>

#include "debug_break.h"

namespace nonstd {

class atomic_mutex
{
public:
	inline atomic_mutex()
	{
		m_Atomic.clear();
	}

#ifdef SEV_DEBUG
	inline ~atomic_mutex()
	{
		if (!tryLock())
			debug_break(); // Must be unlocked before destroying
	}
#endif

	inline void lock()
	{
		while (m_Atomic.test_and_set())
			std::this_thread::yield();
	}

	inline bool try_lock()
	{
		return !m_Atomic.test_and_set();
	}

	inline void unlock()
	{
		m_Atomic.clear();
	}

private:
	std::atomic_flag m_Atomic;

private:
	atomic_mutex &operator=(const atomic_mutex&) = delete;
	atomic_mutex(const atomic_mutex&) = delete;

}; /* class atomic_mutex */

} /* namespace nonstd */

#else /* #ifndef NONSTD_SUPPRESS */

#include <mutex>

namespace nonstd {

using atomic_mutex = std::mutex;

} /* namespace nonstd */

#endif /* #ifndef NONSTD_SUPPRESS */

#endif /* #ifndef NONSTD_ATOMIC_MUTEX_H */

/* end of file */
