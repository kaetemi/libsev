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

#ifndef SEV_EVENT_LOOP_H
#define SEV_EVENT_LOOP_H

#include "self_config.h"
#ifdef SEV_MODULE_EVENT_LOOP

#include <thread>
#include <mutex>
#include <condition_variable>

#include <functional>

#include <queue>
#include <set>

#ifdef SEV_DEPEND_MSVC_CONCURRENT
#	include <concurrent_queue.h>
#	include <concurrent_priority_queue.h>
#else
#	include "atomic_mutex.h"
#endif

#ifdef WIN32
typedef void *HANDLE;
#endif

namespace sev {

/*
struct EventLoopOptions
{
	bool EnableFibers = true;
	bool EnableTimers = true;
	bool EnableIO = true;
}
*/

typedef std::function<void()> EventFunction;

class SEV_LIB EventLoop
{
public:
	EventLoop();
	virtual ~EventLoop();

	void run()
	{
		stop();
		m_Running = true;
		m_Thread = std::move(std::thread(&EventLoop::loop, this));
	}

	void runSync()
	{
		stop();
		m_Running = true;
		loop();
	}

	void stop() // thread-safe
	{
		m_Running = false;
		poke();
		if (m_Thread.joinable())
			m_Thread.join();
	}

	void clear() // semi-thread-safe
	{
#ifdef SEV_DEPEND_MSVC_CONCURRENT
		m_ImmediateConcurrent.clear();
		m_TimeoutConcurrent.clear();
#else
		std::unique_lock<AtomicMutex> lock(m_QueueLock);
		std::unique_lock<AtomicMutex> tlock(m_QueueTimeoutLock);
		m_Immediate = std::move(std::queue<EventFunction>());
		m_Timeout = std::move(std::priority_queue<timeout_func>());
#endif
	}

	//! Call from inside an interval function to prevent it from being called again
	void cancel()
	{
		m_Cancel = true;
	}

	//! Block call until the queued functions  finished processing. Set empty to repeat the wait until the queue is empty
	void join(bool empty = false) // thread-safe
	{
		std::mutex syncLock;
		std::condition_variable syncCond;
		std::unique_lock<std::mutex> lock(syncLock);
		EventFunction syncFunc = [this, &syncLock, &syncCond, &syncFunc, empty]() -> void {
#ifdef SEV_DEPEND_MSVC_CONCURRENT
			if (empty && !m_ImmediateConcurrent.empty())
#else
			bool immediateEmpty;
			; {
				std::unique_lock<AtomicMutex> lock(m_QueueLock);
				immediateEmpty = m_Immediate.empty();
			}
			if (empty && !immediateEmpty)
#endif
			{
				immediate(syncFunc);
			}
			else
			{
				std::unique_lock<std::mutex> lock(syncLock);
				syncCond.notify_one();
			}
		};
		immediate(syncFunc);
		syncCond.wait(lock);
	}

	/*
private:
	template<typename T>
	inline void immediate_(T &&f) 
	{
#ifdef SEV_DEPEND_MSVC_CONCURRENT
		m_ImmediateConcurrent.push(std::forward<T>(f));
#else
		std::unique_lock<AtomicMutex> lock(m_QueueLock);
		m_Immediate.push(std::forward<T>(f));
#endif
		poke();
	}

public:
	inline void immediate(const EventFunction &f) // thread-safe
	{
		immediate_(f);
	}

	void immediate(EventFunction &&f) // thread-safe
	{
		immediate_(f);
	}
	*/

	template<typename T = EventFunction>
	inline void immediate(T &&f)
	{
#ifdef SEV_DEPEND_MSVC_CONCURRENT
		m_ImmediateConcurrent.push(std::forward<EventFunction>(f));
#else
		std::unique_lock<AtomicMutex> lock(m_QueueLock);
		m_Immediate.push(std::forward<EventFunction>(f));
#endif
		poke();
	}

	template<class rep, class period, typename T = EventFunction>
	void timeout(T &&f, const std::chrono::duration<rep, period>& delta) // thread-safe
	{
		timeout_func tf;
		tf.f = std::forward<EventFunction>(f);
		tf.time = std::chrono::steady_clock::now() + delta;
		tf.interval = std::chrono::nanoseconds::zero();
		; {
#ifdef SEV_DEPEND_MSVC_CONCURRENT
			m_TimeoutConcurrent.push(std::move(tf));
#else
			std::unique_lock<AtomicMutex> lock(m_QueueTimeoutLock);
			m_Timeout.push(std::move(tf));
#endif
		}
		poke();
	}

	template<class rep, class period, typename T = EventFunction> 
	void interval(T &&f, const std::chrono::duration<rep, period>& interval) // thread-safe
	{
		timeout_func tf;
		tf.f = std::forward<EventFunction>(f);
		tf.time = std::chrono::steady_clock::now() + interval;
		tf.interval = interval;
		; {
#ifdef SEV_DEPEND_MSVC_CONCURRENT
			m_TimeoutConcurrent.push(std::move(tf));
#else
			std::unique_lock<AtomicMutex> lock(m_QueueTimeoutLock);
			m_Timeout.push(std::move(tf));
#endif
		}
		poke();
	}

	template<typename T = EventFunction>
	void timed(T &&f, const std::chrono::steady_clock::time_point &point) // thread-safe
	{
		timeout_func tf;
		tf.f = std::forward<EventFunction>(f);
		tf.time = point;
		tf.interval = std::chrono::steady_clock::duration::zero();
		; {
#ifdef SEV_DEPEND_MSVC_CONCURRENT
			m_TimeoutConcurrent.push(std::move(tf));
#else
			std::unique_lock<AtomicMutex> lock(m_QueueTimeoutLock);
			m_Timeout.push(std::move(tf));
#endif
		}
		poke();
	}

public:
	template<typename T = EventFunction>
	void thread(T &&f, T &&callback)
	{
		std::thread t([this, f = std::forward(f), callback = std::forward(f)]() mutable -> void {
			f();
			immediate(std::move(callback));
		});
		t.detach();
	}

private:
	void loop();
	void poke();

private:
	struct timeout_func
	{
		EventFunction f;
		std::chrono::steady_clock::time_point time;
		std::chrono::steady_clock::duration interval;

		bool operator <(const timeout_func &o) const
		{
			return time > o.time;
		}

	};

private:
	bool m_Running;
#ifndef SEV_DEPEND_WIN32_SYNCHRONIZATION_EVENT
	volatile bool m_Poked;
#endif
	std::thread m_Thread;
#ifndef SEV_DEPEND_WIN32_SYNCHRONIZATION_EVENT
	std::mutex m_PokeLock;
	std::condition_variable m_PokeCond;
#else
	HANDLE m_PokeEvent;
#endif

#ifdef SEV_DEPEND_MSVC_CONCURRENT
	concurrency::concurrent_queue<EventFunction> m_ImmediateConcurrent;
	concurrency::concurrent_priority_queue<timeout_func> m_TimeoutConcurrent;
#else
	AtomicMutex m_QueueLock;
	std::queue<EventFunction> m_Immediate;
	AtomicMutex m_QueueTimeoutLock;
	std::priority_queue<timeout_func> m_Timeout;
#endif
	bool m_Cancel;

}; /* class EventLoop */

} /* namespace sev */

#endif /* #ifdef SEV_MODULE_EVENT_LOOP */

#endif /* #ifndef SEV_EVENT_LOOP_H */

/* end of file */
