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

#include "event_flag.h"

#include <atomic>
#include <condition_variable>

void SEV_terminate()
{
	SEV_DEBUG_BREAK();
	std::terminate();
}

#if defined(SEV_EVENT_FLAG_WIN32)

errno_t SEV_EventFlag_init(SEV_EventFlag *ef, bool manualReset, bool initialState)
{
	SEV_AtomicInt32_store(&ef->Set, 0);
	SEV_AtomicInt32_store(&ef->Waiting, 0);
	ef->Event = CreateEventW(null, manualReset, manualReset, null);
	if (!ef->Event) return ENOMEM;
	return 0;
}

void SEV_EventFlag_release(SEV_EventFlag *ef)
{
	HANDLE hEvent = ef->Event;
	if (ef->Waiting)
		SEV_terminate();
#ifdef SEV_EVENT_FLAG_MOVABLE
	if (hEvent != INVALID_HANDLE_VALUE)
#endif
	{
		CloseHandle(hEvent);
	}

}

void SEV_EventFlag_wait(SEV_EventFlag *ef)
{
	HANDLE hEvent = ef->Event;
#ifdef SEV_EVENT_FLAG_OPTIMIZE
	SEV_AtomicInt32_increment(&ef->Waiting);
	if (SEV_AtomicInt32_exchange(&ef->Set, 0))
	{
		SEV_AtomicInt32_decrement(&ef->Waiting);
		return;
	}
#endif
	DWORD res = WaitForSingleObject(hEvent, INFINITE);
#ifdef SEV_EVENT_FLAG_OPTIMIZE
	SEV_AtomicInt32_decrement(&ef->Waiting);
#endif
	if (res != WAIT_OBJECT_0)
		SEV_terminate();

}

bool SEV_EventFlag_waitFor(SEV_EventFlag *ef, int timeoutMs)
{
	HANDLE hEvent = ef->Event;
#ifdef SEV_EVENT_FLAG_OPTIMIZE
	SEV_AtomicInt32_increment(&ef->Waiting);
	if (SEV_AtomicInt32_exchange(&ef->Set, 0))
	{
		SEV_AtomicInt32_decrement(&ef->Waiting);
		return true;
	}
#endif
	DWORD res = WaitForSingleObject(hEvent, timeoutMs);
#ifdef SEV_EVENT_FLAG_OPTIMIZE
	SEV_AtomicInt32_decrement(&ef->Waiting);
#endif
	if (res == WAIT_TIMEOUT)
		return false;
	if (res != WAIT_OBJECT_0)
		SEV_terminate();
	return true;

}

void SEV_EventFlag_set(SEV_EventFlag *ef)
{
	if (SEV_AtomicInt32_load(&ef->Waiting))
	{
		if (!SetEvent(ef->Event))
			SEV_terminate();
	}
	else
	{
		SEV_AtomicInt32_store(&ef->Set, 1);
		if (SEV_AtomicInt32_load(&ef->Waiting) && SEV_AtomicInt32_exchange(&ef->Set, 0))
		{
			if (!SetEvent(ef->Event))
				SEV_terminate();
		}
	}
}

void SEV_EventFlag_reset(SEV_EventFlag *ef)
{
	SEV_AtomicInt32_store(&ef->Set, 0);
	if (!ResetEvent(ef->Event))
		SEV_terminate();
}

#else

namespace sev::impl {

struct EventFlag
{
public:
	std::mutex Mutex;
	std::condition_variable CondVar;
	bool Flag;
	bool ResetValue;
	bool Reset;
	int Waiting;
	bool Delete;
};

}

errno_t SEV_EventFlag_init(SEV_EventFlag *ef, bool manualReset, bool initialState)
{
	ef->Impl = new (std::nothrow) sev::impl::EventFlag();
	sev::impl::EventFlag *const m = ef->Impl;
	if (!m) return ENOMEM;
	m->Flag = initialState;
	m->ResetValue = manualReset;
	m->Reset = false;
	m->Waiting = 0;
	m->Delete = false;
	return 0;
}

void SEV_EventFlag_release(SEV_EventFlag *ef)
{
	sev::impl::EventFlag *const m = ef->Impl;
	{
		std::unique_lock<std::mutex> lock(m->Mutex);
		if (m->Waiting)
		{
			m->Flag = true;
			m->ResetValue = true;
			m->Reset = false;
			m->Delete = true;
			m->CondVar.notify_all();
			return;
		}
	}
	delete m;
}

void SEV_EventFlag_wait(SEV_EventFlag *ef)
{
	sev::impl::EventFlag *const m = ef->Impl;
	bool exc;
	bool del;
	{
		std::unique_lock<std::mutex> lock(m->Mutex);
		++m->Waiting;
		if (m->Reset) // Reset cannot keep an already-waiting thread blocking
			m->Flag = false;
		while (!m->Flag)
		{
			m->CondVar.wait(lock); // Mutex is unlocked while waiting, relocked when back
		}
		m->Flag = m->ResetValue;
		--m->Waiting;
		exc = m->Delete;
		del = !m->Waiting && exc; // Delete on last thread exit
	}
	if (del)
		delete m; // Delayed deletion while waiting for a thread
	if (exc)
		SEV_terminate(); // throw Exception("sev::EventFlag deleted while waiting", 1);
}

bool SEV_EventFlag_waitFor(SEV_EventFlag *ef, int timeoutMs)
{
	sev::impl::EventFlag *const m = ef->Impl;
	bool exc;
	bool del;
	bool res = true;
	{
		std::unique_lock<std::mutex> lock(m->Mutex);
		++m->Waiting;
		if (m->Reset) // Reset cannot keep an already-waiting thread blocking
			m->Flag = false;
		while (!m->Flag)
		{
			res = m->CondVar.wait_for(lock, std::chrono::milliseconds(timeoutMs)) != std::cv_status::timeout; // Mutex is unlocked while waiting, relocked when back
		}
		m->Flag = m->ResetValue;
		--m->Waiting;
		exc = m->Delete;
		del = !m->Waiting && exc; // Delete on last thread exit
	}
	if (del)
		delete m; // Delayed deletion while waiting for a thread
	if (exc)
		SEV_terminate(); // throw Exception("sev::EventFlag deleted while waiting", 1);
	return res;
}

void SEV_EventFlag_set(SEV_EventFlag *ef)
{
	sev::impl::EventFlag *const m = ef->Impl;
	std::unique_lock<std::mutex> lock(m->Mutex);
	m->Reset = false;
	m->Flag = true;
	m->CondVar.notify_one();
}

void SEV_EventFlag_reset(SEV_EventFlag *ef)
{
	sev::impl::EventFlag *const m = ef->Impl;
	std::unique_lock<std::mutex> lock(m->Mutex);
	m->Reset = true;
}

#endif 

/* end of file */
