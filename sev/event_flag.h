/*

Copyright (C) 2020  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
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

/*

Event flag. Thread waits if flag is false, one thread continues when the flag is set.
Setting the flag multiple times has no effect, only one thread will continue.
Only one thread is supposed to wait for this flag.

TODO: Support eventfd

*/

#pragma once
#ifndef SEV_EVENT_FLAG_H
#define SEV_EVENT_FLAG_H

#include "platform.h"
#include "atomic.h"

// #define SEV_EVENT_FLAG_STL
#define SEV_EVENT_FLAG_OPTIMIZE

#if !defined(SEV_EVENT_FLAG_WIN32) && !defined(SEV_EVENT_FLAG_EVENTFD) && !defined(SEV_EVENT_FLAG_STL)
#ifdef _WIN32
#define SEV_EVENT_FLAG_WIN32
#else
#define SEV_EVENT_FLAG_STL
#endif
#endif

#ifdef SEV_EVENT_FLAG_STL
#ifdef __cplusplus
namespace sev::impl {
struct EventFlag;
}
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct SEV_EventFlag
{
#if defined(SEV_EVENT_FLAG_WIN32)
	HANDLE Event;
#elif defined(__cplusplus)
	sev::impl::EventFlag *Impl;
#else
	void *Impl;
#endif

#if defined(SEV_EVENT_FLAG_OPTIMIZE)
	SEV_AtomicInt32 Waiting;
	SEV_AtomicInt32 Set;
#else
	int ReservedInt[2];
#endif

};

SEV_LIB errno_t SEV_EventFlag_init(SEV_EventFlag *ef, bool manualReset, bool initialState);
SEV_LIB void SEV_EventFlag_release(SEV_EventFlag *ef);

SEV_LIB void SEV_EventFlag_wait(SEV_EventFlag *ef);
SEV_LIB bool SEV_EventFlag_waitFor(SEV_EventFlag *ef, int timeoutMs);
SEV_LIB void SEV_EventFlag_set(SEV_EventFlag *ef);
SEV_LIB void SEV_EventFlag_reset(SEV_EventFlag *ef);

SEV_LIB void SEV_terminate();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

namespace sev {

struct EventFlag /* NOTE: Use struct for objects which should generally be used by value, class for objects which should generally be used by pointer. */
{
public:
	SEV_FORCE_INLINE EventFlag(bool manualReset = false, bool initialState = false)
	{
		errno_t err = SEV_EventFlag_init(&m, manualReset, initialState);
		if (err)
		{
			if (err == ENOMEM) throw std::bad_alloc();
			else throw std::exception();
		}
	}

	SEV_FORCE_INLINE ~EventFlag() noexcept
	{
		SEV_EventFlag_release(&m);
	}

	SEV_FORCE_INLINE EventFlag(const EventFlag &other) = delete;
	SEV_FORCE_INLINE EventFlag &operator=(const EventFlag &other) = delete;

	SEV_FORCE_INLINE EventFlag(EventFlag &&other) noexcept = delete;
	SEV_FORCE_INLINE EventFlag &operator=(EventFlag &&other) noexcept = delete;

	SEV_FORCE_INLINE void wait() noexcept
	{
		SEV_EventFlag_wait(&m);
	}

	SEV_FORCE_INLINE bool wait(int timeoutMs) noexcept // Returns false on timeout
	{
		return SEV_EventFlag_waitFor(&m, timeoutMs);
	}

	SEV_FORCE_INLINE void set() noexcept
	{
		SEV_EventFlag_set(&m);
	}

	SEV_FORCE_INLINE void reset() noexcept
	{
		SEV_EventFlag_reset(&m);
	}

private:
	SEV_EventFlag m;
	
};

}

#endif /* #ifdef __cplusplus */

#endif /* #ifndef SEV_EVENT_FLAG_H */

/* end of file */
