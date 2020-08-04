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

#pragma once
#ifndef SEV_EVENT_LOOP_IMPL_H
#define SEV_EVENT_LOOP_IMPL_H

#include "platform.h"

#ifdef _MSC_VER
#define SEV_EVENT_LOOP_MSVC_CONCURRENT
#endif

#include "event_loop.h"
#include "concurrent_functor_queue.h"

#include <mutex>
#include <thread>
#include <vector>
#include <map>

#ifdef SEV_EVENT_LOOP_MSVC_CONCURRENT
#include <concurrent_priority_queue.h>
#else
#include <queue>
#endif

namespace sev::impl::el {

extern SEV_EventLoopVt EventLoopVt;

struct TimeoutFunctor
{
	EventFunctor Functor; // Return ECANCELED to stop an interval
	std::chrono::steady_clock::time_point Time;
	std::chrono::steady_clock::duration Interval;

	bool operator <(const TimeoutFunctor &o) const
	{
		return Time > o.Time;
	}

};

#if 0 // TODO
struct TimeoutFunctorWin32
{
	EventFunctor Functor; // Return ECANCELED to stop an interval
	bool Interval;

};
#endif

class EventLoopBase : public SEV_EventLoop
{
public:
	EventLoopBase(SEV_EventLoopVt *vt) : SEV_EventLoop{ vt }, QueueItems(0), Running(false), Threads(0), ThreadsWaiting(0), Stopping(false)
	{

	}

	ConcurrentFunctorQueue<errno_t(EventLoop &)> Queue;
	std::atomic_int QueueItems;
	std::atomic_bool Running;
	std::atomic_int Threads;
	std::atomic_int ThreadsWaiting;

	std::mutex ManagedThreadsMutex;
	std::vector<std::thread> ManagedThreads;
	std::atomic_bool Stopping;
	EventFlag LoopEndedFlag;

	EventLoopBase(const EventLoop &) = delete;
	EventLoopBase(EventLoop &&) = delete;

};

class EventLoop : public EventLoopBase
{
public:
	EventLoop() : EventLoopBase(&EventLoopVt)
	{
	}

	EventFlag Flag;

#ifdef SEV_EVENT_LOOP_MSVC_CONCURRENT
	concurrency::concurrent_priority_queue<TimeoutFunctor> TimeoutConcurrent;
#else
	AtomicMutex TimeoutMutex;
	std::priority_queue<TimeoutFunctor> Timeout;
#endif

};

#if 0 // TODO
class EventLoopWin32 : public EventLoopBase
{
public:
	EventLoopWin32() : EventLoopBase(&EventLoopVt) // TODO
	{
	}

	std::mutex TimeoutMutex;
	std::map<INT_PTR, TimeoutFunctorWin32> TimeoutMap; // INT_PTR timerId = SetTimer

};
#endif

}

#endif /* #ifndef SEV_EVENT_LOOP_IMPL_H */

/* end of file */
