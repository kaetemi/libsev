/*

Copyright (C) 2016-2020  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
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

#include "event_loop.h"
#include "concurrent_functor_queue.h"

void SEV_EventLoop_destroy(SEV_EventLoop *el)
{
	el->Vt->Destroy(el);
}

errno_t SEV_EventLoop_post(SEV_EventLoop *el, errno_t(*f)(void *ptr, SEV_EventLoop *el), void *ptr, ptrdiff_t size)
{
	return el->Vt->Post(el, f, ptr, size);
}

void SEV_EventLoop_invoke(SEV_EventLoop *el, SEV_ExceptionHandle *eh, errno_t(*f)(void *ptr, SEV_EventLoop *el), void *ptr)
{
	return el->Vt->Invoke(el, eh, f, ptr);
}

errno_t SEV_EventLoop_timeout(SEV_EventLoop *el, errno_t(*f)(void *ptr, SEV_EventLoop *el), void *ptr, ptrdiff_t size, int timeoutMs)
{
	return el->Vt->Timeout(el, f, ptr, size, timeoutMs);
}

errno_t SEV_EventLoop_interval(SEV_EventLoop *el, errno_t(*f)(void *ptr, SEV_EventLoop *el), void *ptr, ptrdiff_t size, int intervalMs)
{
	return el->Vt->Interval(el, f, ptr, size, intervalMs);
}

errno_t SEV_EventLoop_postFunctor(SEV_EventLoop *el, const SEV_FunctorVt *vt, void *ptr, void(*forwardConstructor)(void *ptr, void *other))
{
	return el->Vt->PostFunctor(el, vt, ptr, forwardConstructor);
}

void SEV_EventLoop_invokeFunctor(SEV_EventLoop *el, SEV_ExceptionHandle *eh, const SEV_FunctorVt *vt, void *ptr)
{
	return el->Vt->InvokeFunctor(el, eh, vt, ptr);
}

errno_t SEV_EventLoop_timeoutFunctor(SEV_EventLoop *el, const SEV_FunctorVt *vt, void *ptr, void(*forwardConstructor)(void *ptr, void *other), int timeoutMs)
{
	return el->Vt->TimeoutFunctor(el, vt, ptr, forwardConstructor, timeoutMs);
}

errno_t SEV_EventLoop_intervalFunctor(SEV_EventLoop *el, const SEV_FunctorVt *vt, void *ptr, void(*forwardConstructor)(void *ptr, void *other), int intervalMs)
{
	return el->Vt->IntervalFunctor(el, vt, ptr, forwardConstructor, intervalMs);
}

void SEV_EventLoop_cancel(SEV_EventLoop *el)
{
	el->Vt->Cancel(el);
}

errno_t SEV_EventLoop_join(SEV_EventLoop *el, bool empty)
{
	return el->Vt->Join(el, empty);
}

void SEV_EventLoop_run(SEV_EventLoop *el, const SEV_FunctorVt *onError, void *ptr, void(*forwardConstructor)(void *ptr, void *other))
{
	el->Vt->Run(el, onError, ptr, forwardConstructor);
}

void SEV_EventLoop_loop(SEV_EventLoop *el, SEV_ExceptionHandle *eh)
{
	el->Vt->Loop(el, eh);
}

void SEV_EventLoop_stop(SEV_EventLoop *el)
{
	el->Vt->Stop(el);
}


errno_t SEV_IMPL_EventLoopBase_post(SEV_EventLoop *el, errno_t(*f)(void *ptr, SEV_EventLoop *el), void *ptr, ptrdiff_t size)
{
	// Generic unoptimized wrapper
	try
	{
		std::vector<uint8_t> v(size);
		memcpy(&v[0], ptr, size);
		sev::EventFunctorView fv = std::move([f, v](sev::EventLoop &el) -> errno_t {
			errno_t err = f((void *)&v[0], &el);
			if (!err) return 0;
			if (err == ENOMEM) throw std::bad_alloc();
			throw std::exception();
			return 0; // FIXME
			});
		const sev::EventFunctorVt *vt;
		void *ptr;
		bool movable;
		fv.extract(vt, ptr, movable, true);
		SEV_ASSERT(movable);
		return el->Vt->PostFunctor(el, vt->get(), ptr, vt->get()->MoveConstructor);
	}
	catch (std::bad_alloc)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EOTHER;
	}
	return 0;
}

void SEV_IMPL_EventLoopBase_invoke(SEV_EventLoop *el, SEV_ExceptionHandle *eh, errno_t(*f)(void *ptr, SEV_EventLoop *el), void *ptr)
{
	((sev::ExceptionHandle *)eh)->capture<void>([=]() -> void {
		sev::EventFunctorView fv = std::move([f, ptr](sev::EventLoop &el) -> errno_t {
			errno_t err = f(ptr, &el);
			if (!err) return 0;
			if (err == ENOMEM) throw std::bad_alloc();
			throw std::exception();
			return 0; // FIXME
			});
		const sev::EventFunctorVt *vt;
		void *ptr;
		bool movable;
		fv.extract(vt, ptr, movable, false);
		SEV_ASSERT(!movable);
		el->Vt->InvokeFunctor(el, eh, vt->get(), ptr);
	});
}

errno_t SEV_IMPL_EventLoopBase_timeout(SEV_EventLoop *el, errno_t(*f)(void *ptr, SEV_EventLoop *el), void *ptr, ptrdiff_t size, int timeoutMs)
{
	// Generic unoptimized wrapper
	try
	{
		std::vector<uint8_t> v(size);
		memcpy(&v[0], ptr, size);
		sev::EventFunctorView fv = std::move([f, v](sev::EventLoop &el) -> errno_t {
			errno_t err = f((void *)&v[0], &el);
			if (!err) return 0;
			if (err == ENOMEM) throw std::bad_alloc();
			throw std::exception();
			return 0; // FIXME
			});
		const sev::EventFunctorVt *vt;
		void *ptr;
		bool movable;
		fv.extract(vt, ptr, movable, true);
		SEV_ASSERT(movable);
		return el->Vt->TimeoutFunctor(el, vt->get(), ptr, vt->get()->MoveConstructor, timeoutMs);
	}
	catch (std::bad_alloc)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EOTHER;
	}
	return 0;
}

errno_t SEV_IMPL_EventLoopBase_interval(SEV_EventLoop *el, errno_t(*f)(void *ptr, SEV_EventLoop *el), void *ptr, ptrdiff_t size, int intervalMs)
{
	// Generic unoptimized wrapper
	try
	{
		std::vector<uint8_t> v(size);
		memcpy(&v[0], ptr, size);
		sev::EventFunctorView fv = std::move([f, v](sev::EventLoop &el) -> errno_t {
			errno_t err = f((void *)&v[0], &el);
			if (!err) return 0;
			if (err == ENOMEM) throw std::bad_alloc();
			throw std::exception();
			return 0; // FIXME
			});
		const sev::EventFunctorVt *vt;
		void *ptr;
		bool movable;
		fv.extract(vt, ptr, movable, true);
		SEV_ASSERT(movable);
		return el->Vt->IntervalFunctor(el, vt->get(), ptr, vt->get()->MoveConstructor, intervalMs);
	}
	catch (std::bad_alloc)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EOTHER;
	}
	return 0;
}

static SEV_EventLoopVt s_EventLoopVt = {
	SEV_IMPL_EventLoop_destroy, // Destroy

	SEV_IMPL_EventLoopBase_post,
	SEV_IMPL_EventLoopBase_invoke,
	SEV_IMPL_EventLoopBase_timeout,
	SEV_IMPL_EventLoopBase_interval,

	SEV_IMPL_EventLoop_postFunctor, // PostFunctor
	null, // InvokeFunctor
	null, // TimeoutFunctor
	null, // IntervalFunctor

	null, // Cancel
	null, // Join

	null, // Run
	null, // Loop
	null, // Stop

};

namespace sev::impl::el {

class EventLoop : public SEV_EventLoop
{
public:
	EventLoop() : SEV_EventLoop{ &s_EventLoopVt }
	{

	}

	EventFlag Flag;
	ConcurrentFunctorQueue<errno_t(EventLoop &)> Queue;
};

}

SEV_EventLoop *SEV_EventLoop_create()
{
	try
	{
		return new sev::impl::el::EventLoop();
	}
	catch (...)
	{
		return null;
	}
}

void SEV_IMPL_EventLoop_destroy(SEV_EventLoop *el)
{
	delete el;
}

errno_t SEV_IMPL_EventLoop_postFunctor(SEV_EventLoop *el, const SEV_FunctorVt *vt, void *ptr, void(*forwardConstructor)(void *ptr, void *other))
{
	sev::impl::el::EventLoop *elp = (sev::impl::el::EventLoop *)el;
	errno_t res = SEV_ConcurrentFunctorQueue_pushFunctor(elp->Queue.get(), vt, ptr, forwardConstructor);
	elp->Flag.set();
	return res;
}

void SEV_IMPL_EventLoop_invokeFunctor(SEV_EventLoop *el, SEV_ExceptionHandle *eh, const SEV_FunctorVt *vt, void *ptr)
{
	// NOTE: Invoke catches any errors, and passes them down!
	SEV_ASSERT(eh);
	SEV_ASSERT(!*eh);
	sev::impl::el::EventLoop *elp = (sev::impl::el::EventLoop *)el;
	sev::EventFlag flag;
	errno_t eno = elp->Queue.push(nothrow, [=, &flag](sev::EventLoop &elref) -> errno_t {
		errno_t res = ((sev::EventFunctorVt *)vt)->invoke(ptr, *(sev::ExceptionHandle *)eh, elref);
		if (!*eh && res) SEV_Exception_capture(res);
		flag.set();
		return SEV_ESUCCESS;
	});
	if (eno) SEV_Exception_capture(eno);
	flag.wait();
}















namespace sev {

#if 0

namespace {

thread_local IEventLoop *l_EventLoop;

}

IEventLoop::~IEventLoop() noexcept
{

}

void IEventLoop::setCurrent(bool current)
{
	l_EventLoop = current ? this : null;
}

bool IEventLoop::current()
{
	return l_EventLoop == this;
}

EventLoop::EventLoop() : m_Running(false), m_Cancel(false)
{
	
}

EventLoop::~EventLoop() noexcept
{
	stop();
	clear();
}

void EventLoop::loop()
{
	while (m_Running)
	{
		for (;;)
		{
#ifdef SEV_EVENT_LOOP_CONCURRENT_QUEUE
			EventFunctor f;
			if (!m_ImmediateConcurrent.try_pop(f))
				break;
#else
			m_QueueLock.lock();
			if (!m_Immediate.size())
			{
				m_QueueLock.unlock();
				break;
			}
			EventFunctor f = m_Immediate.front();
			m_Immediate.pop();
			m_QueueLock.unlock();
#endif
			f(*this);
		}

		bool poked = false;
		for (;;)
		{
#ifdef SEV_EVENT_LOOP_CONCURRENT_QUEUE
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
			int64_t wt = std::chrono::duration_cast<std::chrono::milliseconds>(tfr.time - now).count();
			if (tfr.time > now) // Wait
			{
#ifdef SEV_EVENT_LOOP_CONCURRENT_QUEUE
				m_TimeoutConcurrent.push(tf);
#else
				m_QueueTimeoutLock.unlock();
#endif
				m_Flag.wait(wt & 0xFFFF); // Mask to 65 seconds, it's fine to break out earlier, the loop re-checks
				poked = true;
				break;
			}
#ifndef SEV_EVENT_LOOP_CONCURRENT_QUEUE
			timeout_func tf = tfr;
			m_Timeout.pop();
			m_QueueTimeoutLock.unlock();
#endif
			m_Cancel = false;
			tf.f(*this); // call
			if (!m_Cancel && (tf.interval > std::chrono::nanoseconds::zero())) // repeat
			{
				tf.time += tf.interval;
				; {
#ifdef SEV_EVENT_LOOP_CONCURRENT_QUEUE
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
			m_Flag.wait();
		}
	}
}

} /* namespace sev */

namespace sev {

namespace /* anonymous */ {

void test()
{
	
}

} /* anonymous namespace */

#endif

} /* namespace sev */

/* end of file */
