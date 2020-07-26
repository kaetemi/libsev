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

// #define SEV_EVENT_FLAG_MOVABLE
// #define SEV_EVENT_FLAG_STL

#if !defined(SEV_EVENT_FLAG_WIN32) && !defined(SEV_EVENT_FLAG_EVENTFD) && !defined(SEV_EVENT_FLAG_STL)
#ifdef _WIN32
#define SEV_EVENT_FLAG_WIN32
#else
#define SEV_EVENT_FLAG_STL
#endif
#endif

#if defined(SEV_EVENT_FLAG_WIN32)
#include "win32_exception.h"
#endif

#if defined(SEV_EVENT_FLAG_WIN32) || defined(SEV_EVENT_FLAG_EVENTFD)
#define SEV_EVENT_FLAG_INLINE inline
#else
#define SEV_EVENT_FLAG_INLINE
#endif

#ifdef __cplusplus

namespace sev {

#ifdef SEV_EVENT_FLAG_STL
struct EventFlagImpl;
#endif

struct SEV_LIB EventFlag /* NOTE: Use struct for objects which should generally be used by value, class for objects which should generally be used by pointer. */
{
public:
	inline EventFlag(bool manualReset = false, bool initialState = false)
#ifdef SEV_EVENT_FLAG_WIN32
		: m_Event(CreateEventW(null, manualReset, manualReset, null))
#else
		: m_Impl(createImpl(manualReset, manualReset))
#endif
	{
		static_assert(sizeof(EventFlag) == sizeof(void *));
#ifdef SEV_EVENT_FLAG_WIN32
		static_assert(sizeof(EventFlag) == sizeof(HANDLE));
		if (!m_Event)
			SEV_THROW_LAST_ERROR();
#else
		static_assert(sizeof(EventFlag) == sizeof(EventFlagImpl *));
#endif
	}

	inline ~EventFlag()
	{
#ifdef SEV_EVENT_FLAG_WIN32
		HANDLE hEvent = m_Event;
#ifdef SEV_EVENT_FLAG_MOVABLE
		if (hEvent != INVALID_HANDLE_VALUE)
#endif
		{
			m_Event = INVALID_HANDLE_VALUE;
			SetEvent(hEvent);
			CloseHandle(hEvent);
		}
#else
		destroyImpl(m_Impl);
#endif
	}

	inline EventFlag(const EventFlag &other) = delete;
	inline EventFlag &operator=(const EventFlag &other) = delete;

#ifdef SEV_EVENT_FLAG_MOVABLE
	inline EventFlag(EventFlag &&other) noexcept
	{
#ifdef SEV_EVENT_FLAG_WIN32
		m_Event = other.m_Event;
		other.m_Event = INVALID_HANDLE_VALUE;
#else
		m_Impl = other.m_Impl;
		other.m_Impl = null;
#endif
	}

	inline EventFlag &operator=(EventFlag &&other) noexcept
	{
		if (this != &other)
		{
			this->~EventFlag();
			new (this) EventFlag(move(other));
		}
		return *this;
	}
#endif

	SEV_EVENT_FLAG_INLINE void wait()
	{
#ifdef SEV_EVENT_FLAG_WIN32
		HANDLE hEvent = m_Event;
		DWORD res = WaitForSingleObject(hEvent, INFINITE);
		if (res != WAIT_OBJECT_0)
			SEV_THROW_LAST_ERROR();
		if (m_Event != hEvent) // This will cause either a memory access violation or throw the following exception
			throw Exception("sev::EventFlag deleted while waiting"sv, 1); // Thread that wasn't awakened should crash and unwind
#else
		waitImpl(m_Impl);
#endif
	}

	SEV_EVENT_FLAG_INLINE void set()
	{
#ifdef SEV_EVENT_FLAG_WIN32
		if (!SetEvent(m_Event))
			SEV_THROW_LAST_ERROR();
#else
		setImpl(m_Impl);
#endif
	}

	SEV_EVENT_FLAG_INLINE void reset()
	{
#ifdef SEV_EVENT_FLAG_WIN32
		if (!ResetEvent(m_Event))
			SEV_THROW_LAST_ERROR();
#else
		resetImpl(m_Impl);
#endif
	}

private:
#ifdef SEV_EVENT_FLAG_WIN32
	HANDLE m_Event;
#else
	EventFlagImpl *m_Impl;

	static EventFlagImpl *createImpl(bool manualReset, bool initialState);
	static void destroyImpl(EventFlagImpl *m);

	static void waitImpl(EventFlagImpl *m);
	static void setImpl(EventFlagImpl *m);
	static void resetImpl(EventFlagImpl *m);
#endif

};

}

#ifdef SEV_EVENT_FLAG_WIN32
typedef HANDLE SEV_EventFlag;
#else
typedef sev::EventFlagImpl *SEV_EventFlag;
#endif

#else

typedef void *SEV_EventFlag;

#endif

SEV_EventFlag SEV_EventFlag_create();
void SEV_EventFlag_destroy(SEV_EventFlag eventFlag);

bool SEV_EventFlag_wait(SEV_EventFlag eventFlag);
bool SEV_EventFlag_set(SEV_EventFlag eventFlag);
bool SEV_EventFlag_reset(SEV_EventFlag eventFlag);

#endif /* #ifndef SEV_EVENT_FLAG_H */

/* end of file */
