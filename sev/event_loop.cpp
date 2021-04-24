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
#include "event_loop_impl.h"

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

errno_t SEV_EventLoop_join(SEV_EventLoop *el, bool empty)
{
	return el->Vt->Join(el, empty);
}

errno_t SEV_EventLoop_run(SEV_EventLoop *el, const SEV_FunctorVt *onError, void *ptr, void(*forwardConstructor)(void *ptr, void *other))
{
	return el->Vt->Run(el, onError, ptr, forwardConstructor);
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

namespace sev::impl::el {

SEV_EventLoopVt EventLoopVt = {
	SEV_IMPL_EventLoop_destroy,

	SEV_IMPL_EventLoopBase_post,
	SEV_IMPL_EventLoopBase_invoke,
	SEV_IMPL_EventLoopBase_timeout,
	SEV_IMPL_EventLoopBase_interval,

	SEV_IMPL_EventLoop_postFunctor,
	SEV_IMPL_EventLoop_invokeFunctor,
	null, // TimeoutFunctor
	null, // IntervalFunctor

	null, // Join

	SEV_IMPL_EventLoop_run, // Run
	SEV_IMPL_EventLoop_loop, // Loop
	SEV_IMPL_EventLoop_stop, // Stop

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
	el->Vt->Stop(el);
	delete el;
}

errno_t SEV_IMPL_EventLoop_postFunctor(SEV_EventLoop *el, const SEV_FunctorVt *vt, void *ptr, void(*forwardConstructor)(void *ptr, void *other))
{
	sev::impl::el::EventLoop *elp = (sev::impl::el::EventLoop *)el;
	++elp->QueueItems;
	errno_t res = SEV_ConcurrentFunctorQueue_pushFunctor(elp->Queue.get(), vt, ptr, forwardConstructor);
	if (res) --elp->QueueItems;
	else elp->Flag.set();
	return res;
}

void SEV_IMPL_EventLoop_invokeFunctor(SEV_EventLoop *el, SEV_ExceptionHandle *eh, const SEV_FunctorVt *vt, void *ptr)
{
	// NOTE: Invoke catches any errors, and passes them down!
	SEV_ASSERT(eh);
	SEV_ASSERT(!*eh);
	sev::impl::el::EventLoop *elp = (sev::impl::el::EventLoop *)el;
	sev::EventFlag flag;
	++elp->QueueItems;
	errno_t eno = elp->Queue.push(std::nothrow, [=, &flag](sev::EventLoop &elref) -> errno_t {
		errno_t res = ((sev::EventFunctorVt *)vt)->invoke(ptr, *(sev::ExceptionHandle *)eh, elref);
		if (!*eh && res) *eh = SEV_Exception_capture(res);
		flag.set();
		return SEV_ESUCCESS;
		});
	if (eno)
	{
		--elp->QueueItems;
		*eh = SEV_Exception_capture(eno);
	}
	else
	{
		elp->Flag.set();
		flag.wait();
	}
}

errno_t SEV_IMPL_EventLoop_run(SEV_EventLoop *el, const SEV_FunctorVt *onError, void *ptr, void(*forwardConstructor)(void *ptr, void *other))
{
	sev::ExceptionHandle ehr;
	ehr.capture<void>([=]() -> void {
		sev::impl::el::EventLoop *elp = (sev::impl::el::EventLoop *)el;
		sev::Functor<void(SEV_ExceptionHandle *)> onErrorF((sev::FunctorVt<void(SEV_ExceptionHandle *)> *)onError, ptr, forwardConstructor == onError->MoveConstructor);
		std::unique_lock<std::mutex> lock(elp->ManagedThreadsMutex);
		elp->ManagedThreads.push_back(std::move(std::thread([=, onErrorMv = std::move(onErrorF)]() -> void { // FIXME: MOVE
			sev::ExceptionHandle eh;
			sev::Functor<void(SEV_ExceptionHandle *)> onErr(onErrorMv); // FIXME: MOVE
			while (elp->Running)
			{
				el->Vt->Loop(el, (SEV_ExceptionHandle *)(&eh));
				if (eh.raised())
				{
					sev::ExceptionHandle ehc;
					onErr(eh, (SEV_ExceptionHandle *)&eh);
					if (ehc.raised())
					{
						ehc.discard();
						SEV_terminate(); // Ok, bye. Don't throw in the exception handler. Unhandled exception.
					}
				}
			}
		})));
		});
	return ehr.rethrow(std::nothrow);
}

void SEV_IMPL_EventLoop_loop(SEV_EventLoop *el, SEV_ExceptionHandle *eh)
{
	sev::impl::el::EventLoop *elp = (sev::impl::el::EventLoop *)el;
	if (elp->Stopping)
		return;
	elp->Running = true;
	if (elp->Stopping)
	{
		elp->Running = false;
		return;
	}
	++elp->Threads;
	while (elp->Running)
	{
		// Check queue
		if (elp->QueueItems)
		{
			bool success;
			do
			{
				errno_t eno = elp->Queue.tryCallAndPop(*(sev::ExceptionHandle *)eh, success, *elp);
				if (success) --elp->QueueItems;
				if (!*eh && eno) *eh = SEV_Exception_capture(eno);
			} while (success && !*eh); // Popped a function and no errors
			if (*eh) break; // Break out of loop due to error!
		}

		// Check timer queue. Temporary, recycled code.
		// TODO: It might be better (more generic) to put the timer queue onto a separate thread, and simply post to the event loop.
		((sev::ExceptionHandle *)eh)->capture<void>([&]() -> void {
			for (;;)
			{
#ifdef SEV_EVENT_LOOP_MSVC_CONCURRENT
				sev::impl::el::TimeoutFunctor tf;
				if (!elp->TimeoutConcurrent.try_pop(tf))
					break;
				const sev::impl::el::TimeoutFunctor &tfr = tf;
#else
				elp->TimeoutMutex.lock();
				if (!elp->Timeout.size())
				{
					elp->TimeoutMutex.unlock();
					break;
				}
				const sev::impl::el::TimeoutFunctor &tfr = elp->Timeout.top();
#endif
				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
				int64_t wt = std::chrono::duration_cast<std::chrono::milliseconds>(tfr.Time - now).count();
				if (tfr.Time > now) // Wait
				{
#ifdef SEV_EVENT_LOOP_MSVC_CONCURRENT
					elp->TimeoutConcurrent.push(tf);
#else
					elp->TimeoutMutex.unlock();
#endif
					++elp->ThreadsWaiting;
					elp->Flag.wait(wt & 0xFFFF); // Mask to 65 seconds, it's fine to break out earlier, the loop re-checks
					--elp->ThreadsWaiting;
					if (elp->QueueItems > 1 && elp->ThreadsWaiting > 1)
						elp->Flag.set(); // Wake up more threads if there's more than one item in the queue
					break;
				}
#ifndef SEV_EVENT_LOOP_MSVC_CONCURRENT
				sev::impl::el::TimeoutFunctor tf = tfr;
				elp->Timeout.pop();
				elp->TimeoutMutex.unlock();
#endif
				errno_t eno = tf.Functor(*(sev::ExceptionHandle *)eh, *elp);
				bool cancel = eno == ECANCELED;
				if (!*eh && eno && eno != ECANCELED) *eh = SEV_Exception_capture(eno);
				--elp->QueueItems;
				if (!cancel && (tf.Interval > std::chrono::nanoseconds::zero())) // repeat
				{
					tf.Time += tf.Interval;
					;
					{
#ifdef SEV_EVENT_LOOP_MSVC_CONCURRENT
						elp->TimeoutConcurrent.push(std::move(tf));
						++elp->QueueItems;
#else
						std::unique_lock<std::mutex> lock(elp->TimeoutMutex);
						elp->Timeout.push(std::move(tf));
#endif
					}
				}
				if (*eh) break; // Break out of loop due to error!
			}
		});
		if (*eh) break; // Break out of loop due to error!

		// Wait
		++elp->ThreadsWaiting;
		elp->Flag.wait();
		--elp->ThreadsWaiting;
		if (elp->QueueItems > 1 && elp->ThreadsWaiting > 1)
			elp->Flag.set(); // Wake up more threads if there's more than one item in the queue
	}
	--elp->Threads;
	elp->LoopEndedFlag.set();
}

void SEV_IMPL_EventLoop_stop(SEV_EventLoop *el)
{
	sev::impl::el::EventLoop *elp = (sev::impl::el::EventLoop *)el;
	elp->Stopping = true;
	{
		std::unique_lock<std::mutex> lock(elp->ManagedThreadsMutex);
		elp->Stopping = true; // Yes.
		elp->Running = false;
		// Wait for managed threads
		for (std::thread &t : elp->ManagedThreads)
		{
			if (t.joinable())
			{
				try
				{
					t.join();
				}
				catch (...)
				{
					// ...
				}
			}
		}
		elp->ManagedThreads.clear();
		while (elp->Threads)
		{
			// Wait for other threads
			// SEV_ASSERT(!elp->Running);
			elp->LoopEndedFlag.wait();
		}
		elp->Stopping = false;
	}
}

/* end of file */
