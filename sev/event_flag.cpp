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

namespace sev {

struct EventFlagImpl
{
public:
	std::mutex Mutex;
	std::condition_variable CondVar;
	bool Flag;
	bool ResetValue;
	bool Reset;
};

#ifdef SEV_EVENT_FLAG_STL

EventFlagImpl *EventFlag::createImpl(bool manualReset, bool initialState)
{
	EventFlagImpl *m = new EventFlagImpl();
	m->Flag = initialState;
	m->ResetValue = manualReset;
	m->Reset = false;
	return m;
}

void EventFlag::destroyImpl(EventFlagImpl *m)
{
	delete m;
}

void EventFlag::waitImpl(EventFlagImpl *m)
{
	std::unique_lock<std::mutex> lock(m->Mutex);
	if (m->Reset) // Reset cannot keep an already-waiting thread blocking
		m->Flag = false;
	while (!m->Flag)
	{
		m->CondVar.wait(lock); // Mutex is unlocked while waiting, relocked when back
	}
	m->Flag = m->ResetValue;
}

void EventFlag::setImpl(EventFlagImpl *m)
{
	std::unique_lock<std::mutex> lock(m->Mutex);
	m->Reset = false;
	m->Flag = true;
	m->CondVar.notify_one();
}

void EventFlag::resetImpl(EventFlagImpl *m)
{
	std::unique_lock<std::mutex> lock(m->Mutex);
	m->Reset = true;
}

#endif

}

SEV_EventFlag SEV_EventFlag_create()
{
	try
	{
		SEV_EventFlag eventFlag;
		new (&eventFlag) sev::EventFlag();
		return eventFlag;
	}
	catch (...)
	{
		return null;
	}
}

void SEV_EventFlag_destroy(SEV_EventFlag eventFlag)
{
	reinterpret_cast<sev::EventFlag &>(eventFlag).~EventFlag();
}

bool SEV_EventFlag_wait(SEV_EventFlag eventFlag)
{
	try
	{
		reinterpret_cast<sev::EventFlag &>(eventFlag).wait();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool SEV_EventFlag_set(SEV_EventFlag eventFlag)
{
	try
	{
		reinterpret_cast<sev::EventFlag &>(eventFlag).set();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool SEV_EventFlag_reset(SEV_EventFlag eventFlag)
{
	try
	{
		reinterpret_cast<sev::EventFlag &>(eventFlag).reset();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

/* end of file */
