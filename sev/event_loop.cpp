/*

Copyright (C) 2017  by authors
Author: Jan Boon <support@no-break.space>
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

#include "event_loop.h"
#ifdef SEV_MODULE_EVENT_LOOP

#include "impl/platform_win32.h"

namespace sev {

EventLoop::EventLoop() : m_Running(false), m_Cancel(false)
{
#ifdef SEV_DEPEND_WIN32_SYNCHRONIZATION_EVENT
	m_PokeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
}

EventLoop::~EventLoop()
{
	stop();
	clear();
#ifdef SEV_DEPEND_WIN32_SYNCHRONIZATION_EVENT
	CloseHandle(m_PokeEvent);
#endif
}

void EventLoop::loop()
{
	while (m_Running)
	{
#ifndef SEV_DEPEND_WIN32_SYNCHRONIZATION_EVENT
		m_Poked = false;
#endif

		for (;;)
		{
#ifdef SEV_DEPEND_MSVC_CONCURRENT
			EventFunction f;
			if (!m_ImmediateConcurrent.try_pop(f))
				break;
#else
			m_QueueLock.lock();
			if (!m_Immediate.size())
			{
				m_QueueLock.unlock();
				break;
			}
			EventFunction f = m_Immediate.front();
			m_Immediate.pop();
			m_QueueLock.unlock();
#endif
			f();
		}

		bool poked = false;
		for (;;)
		{
#ifdef SEV_DEPEND_MSVC_CONCURRENT
			timeout_func tf;
			if (!m_TimeoutConcurrent.try_pop(tf))
				break;
			const timeout_func &tfr = tf;
#else
			m_QueueTimeoutLock.lock();
			if (!m_Timeout.size())
			{
				m_QueueTimeoutLock.unlock();
				break;
			}
			const timeout_func &tfr = m_Timeout.top();
#endif
			std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
#ifdef SEV_DEPEND_WIN32_SYNCHRONIZATION_EVENT
			DWORD wt = (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(tfr.time - now).count();
			if (tfr.time > now && wt > 0)
#else
			if (tfr.time > now) // wait
#endif
			{
#ifdef SEV_DEPEND_MSVC_CONCURRENT
				m_TimeoutConcurrent.push(tf);
#else
				m_QueueTimeoutLock.unlock();
#endif
#ifdef SEV_DEPEND_WIN32_SYNCHRONIZATION_EVENT
				WaitForSingleObject(m_PokeEvent, wt);
#else
				; {
					std::unique_lock<std::mutex> lock(m_PokeLock);
					if (!m_Poked)
						m_PokeCond.wait_until(lock, tfr.time);
				}
#endif
				poked = true;
				break;
			}
#ifndef SEV_DEPEND_MSVC_CONCURRENT
			timeout_func tf = tfr;
			m_Timeout.pop();
			m_QueueTimeoutLock.unlock();
#endif
			m_Cancel = false;
			tf.f(); // call
			if (!m_Cancel && (tf.interval > std::chrono::nanoseconds::zero())) // repeat
			{
				tf.time += tf.interval;
				; {
#ifdef SEV_DEPEND_MSVC_CONCURRENT
					m_TimeoutConcurrent.push(std::move(tf));
#else
					std::unique_lock<AtomicMutex> lock(m_QueueTimeoutLock);
					m_Timeout.push(std::move(tf));
#endif
				}
			}
		}

		if (!poked)
		{
#ifdef SEV_DEPEND_WIN32_SYNCHRONIZATION_EVENT
			WaitForSingleObject(m_PokeEvent, INFINITE);
#else
			std::unique_lock<std::mutex> lock(m_PokeLock);
			if (!m_Poked)
				m_PokeCond.wait(lock);
#endif
		}
	}
}

void EventLoop::poke() // private
{
#ifdef SEV_DEPEND_WIN32_SYNCHRONIZATION_EVENT
	SetEvent(m_PokeEvent);
#else
	std::unique_lock<std::mutex> lock(m_PokeLock);
	m_PokeCond.notify_one();
	m_Poked = true;
#endif
}

} /* namespace sev */

namespace sev {

namespace /* anonymous */ {

void test()
{
	
}

} /* anonymous namespace */

} /* namespace sev */

#endif /* #ifdef SEV_MODULE_EVENT_LOOP */

/* end of file */
