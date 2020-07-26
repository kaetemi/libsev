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
TODO: Move stl stuff into a struct, use by pointer, due to dll boundary

*/

#pragma once
#ifndef SEV_EVENT_FLAG_H
#define SEV_EVENT_FLAG_H

#include "platform.h"
#include "win32_exception.h"

#include <atomic>
#include <condition_variable>

#if !defined(SEV_EVENT_FLAG_WIN32) && !defined(SEV_EVENT_FLAG_STL) && !defined(SEV_EVENT_FLAG_EVENTFD)
#ifdef _WIN32
#define SEV_EVENT_FLAG_WIN32
#else
#define SEV_EVENT_FLAG_STL
#endif
#endif

#ifdef __cplusplus

namespace sev {

struct SEV_LIB EventFlag /* NOTE: Use struct for objects which should generally be used by value, class for objects which should generally be used by pointer. */
{
public:
	EventFlag(bool manualReset = false, bool initialState = false)
#ifdef SEV_EVENT_FLAG_WIN32
		: m_Event(CreateEventW(null, manualReset, manualReset, null))
#else
		: m_ResetValue(manualReset), m_Flag(initialState)
#endif
	{
#ifdef SEV_EVENT_FLAG_WIN32
		static_assert(sizeof(EventFlag) == sizeof(HANDLE));
		if (!m_Event)
			SEV_THROW_LAST_ERROR();
#endif
	}

	~EventFlag()
	{
#ifdef SEV_EVENT_FLAG_WIN32
		CloseHandle(m_Event);
#endif
	}

	void wait()
	{
#ifdef SEV_EVENT_FLAG_WIN32
		DWORD res = WaitForSingleObject(m_Event, INFINITE);
		if (res != WAIT_OBJECT_0)
			SEV_THROW_LAST_ERROR();
#else
		std::unique_lock<std::mutex> lock(m_Mutex);
		bool flag = true;
		while (!m_Flag.compare_exchange_weak(flag, m_ResetValue))
		{
			flag = true;
			m_CondVar.wait(lock);
		}
#endif
	}

	void set()
	{
#ifdef SEV_EVENT_FLAG_WIN32
		if (!SetEvent(m_Event))
			SEV_THROW_LAST_ERROR();
#else
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_Flag = true;
		m_CondVar.notify_one();
#endif
	}

	void reset()
	{
#ifdef SEV_EVENT_FLAG_WIN32
		if (!ResetEvent(m_Event))
			SEV_THROW_LAST_ERROR();
#else
		m_Flag = false;
#endif
	}

private:
#ifdef SEV_EVENT_FLAG_WIN32
	HANDLE m_Event;
#else
	std::condition_variable m_CondVar;
	std::mutex m_Mutex;
	bool m_ResetValue;
	std::atomic_bool m_Flag;
#endif

};

}

#ifdef SEV_EVENT_FLAG_WIN32
typedef HANDLE SEV_EventFlag;
#else
typedef sev::EventFlag *SEV_EventFlag;
#endif

#else

typedef void SEV_EventFlag;

#endif

SEV_EventFlag SEV_EventFlag_create();
void SEV_EventFlag_destroy(SEV_EventFlag eventFlag);

#endif /* #ifndef SEV_EVENT_FLAG_H */

/* end of file */
