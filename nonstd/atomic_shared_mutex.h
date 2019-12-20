/*

Copyright (C) 2017-2019  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
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

#ifndef NONSTD_ATOMIC_SHARED_MUTEX_H
#define NONSTD_ATOMIC_SHARED_MUTEX_H

#ifndef NONSTD_SUPPRESS

#include <atomic>
#include <thread>

#include "debug_break.h"

namespace nonstd {

//! Lock allowing one unique writer and multiple shared readers
class atomic_shared_mutex
{
public:
	inline atomic_shared_mutex() : unique_(false), sharing_(0)
	{

	}

#ifndef NDEBUG
	inline ~atomic_shared_mutex()
	{
		if (!try_lock())
			debug_break(); // Must be unlocked before destroying
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


	inline void lock()
	{
		while (unique_.exchange(true))
			std::this_thread::yield();
		while (sharing_)
			std::this_thread::yield();
	}

	inline void unlock()
	{
#ifndef NDEBUG
		if (!unique_.exchange(false))
			debug_break(); // Already unlocked before
#else
		unique_.exchange(false);
#endif
	}

	inline bool try_lock_shared()
	{
		++sharing_;
		if (unique_)
		{
			--sharing_;
			return false;
		}
		return true;
	}

	inline void lock_shared()
	{
		++sharing_;
		while (unique_)
		{
			--sharing_;
			while (unique_)
				std::this_thread::yield();
			++sharing_;
		}
	}
	inline void unlock_shared()
	{
		--sharing_;
	}

private:
	std::atomic_bool unique_;
	std::atomic_int sharing_;

private:
	atomic_shared_mutex &operator=(const atomic_shared_mutex&) = delete;
	atomic_shared_mutex(const atomic_shared_mutex&) = delete;

}; /* class atomic_shared_mutex */

} /* namespace nonstd */

#else /* #ifndef NONSTD_SUPPRESS */

#include <shared_mutex>

namespace nonstd {

using atomic_shared_mutex = std::shared_mutex;

} /* namespace nonstd */

#endif /* #ifndef NONSTD_SUPPRESS */

#endif /* NONSTD_ATOMIC_SHARED_MUTEX_H */

/* end of file */
