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

Concurrent queue for arbitrary-sized functors with a fixed-size block allocator.

*/

#pragma once
#ifndef SEV_CONCURRENT_FUNCTOR_QUEUE_H
#define SEV_CONCURRENT_FUNCTOR_QUEUE_H

#include "platform.h"
#include "functor_view.h"
#include "atomic_shared_mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline uint64_t SEV_nextPow2U64(uint64_t v)
{
	v -= 1;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	v += 1;
	return v;
}

static inline ptrdiff_t SEV_nextPow2PtrDiff(ptrdiff_t v)
{
	v -= 1;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	if (sizeof(ptrdiff_t) == sizeof(uint64_t))
		v |= v >> 32;
	v += 1;
	return v;
}

struct SEV_ConcurrentFunctorQueue
{
	ptrdiff_t BlockSize;

	SEV_AtomicPtr ReadBlock;
	void *WriteBlock;

	SEV_AtomicPtr SpareBlockA;
	SEV_AtomicPtr SpareBlockB;

	SEV_AtomicPtrDiff PreWriteIdx; // 6* void*

	ptrdiff_t ReservedPtr[2]; // Fix structure size to multiples of 32 for ABI stability

	SEV_AtomicSharedMutex AtomicWriteSwap;
	SEV_AtomicSharedMutex DeleteLock; // 4* int

	int32_t ReservedInt[4]; // Fix structure size to multiples of 32 for ABI stability
	// TODO: Add some malloc/free counters for perf
	// SEV_AtomicInt32 PerfMAllocCounter;
	// SEV_AtomicInt32 PerfFreeCounter;

};

SEV_LIB SEV_ConcurrentFunctorQueue *SEV_ConcurrentFunctorQueue_create(ptrdiff_t blockSize);
SEV_LIB void SEV_ConcurrentFunctorQueue_destroy(SEV_ConcurrentFunctorQueue *concurrentFunctorQueue);

SEV_LIB errno_t SEV_ConcurrentFunctorQueue_init(SEV_ConcurrentFunctorQueue *me, ptrdiff_t blockSize);
SEV_LIB void SEV_ConcurrentFunctorQueue_release(SEV_ConcurrentFunctorQueue *me);

SEV_LIB errno_t SEV_ConcurrentFunctorQueue_push(SEV_ConcurrentFunctorQueue *me, void(*f)(void *ptr, void *args), void *ptr, ptrdiff_t size); // Does a memcpy of the data ptr // TODO: errno_t return value on f
SEV_LIB errno_t SEV_ConcurrentFunctorQueue_pushFunctor(SEV_ConcurrentFunctorQueue *me, const SEV_FunctorVt *vt, void *ptr, void(*forwardConstructor)(void *ptr, void *other)); // Returns EOTHER if forwardConstructor throws, returns ENOMEM in case of memory allocation failure, 0 if OK
#ifdef __cplusplus
SEV_LIB errno_t SEV_ConcurrentFunctorQueue_pushFunctorEx(SEV_ConcurrentFunctorQueue *me, const SEV_FunctorVt *vt, ptrdiff_t size, void *ptr, void(*forwardConstructor)(void *ptr, void *other)); // Throws only if forwardConstructor throws
#endif

// SEV_LIB errno_t SEV_ConcurrentFunctorQueue_tryCallAndPop(SEV_ConcurrentFunctorQueue *me, void *args); // Returns ENODATA if nothing to pop, EOTHER if function threw an exception; ENOMEM, 0 if OK
// SEV_LIB errno_t SEV_ConcurrentFunctorQueue_tryCallAndPopFunctor(SEV_ConcurrentFunctorQueue *me, errno_t(*caller)(void *args, void *ptr,const SEV_FunctorVt *vt), void *args); // res = f(ptr, args...)
#ifdef __cplusplus
SEV_LIB errno_t SEV_ConcurrentFunctorQueue_tryCallAndPopFunctorEx(SEV_ConcurrentFunctorQueue *me, errno_t(*caller)(void *args, void *ptr, const SEV_FunctorVt *vt), void *args); // (res = vt->Invoke(ptr, err, args...)) err is exception, it must be freed if not a SEV_throw* reference
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef __cplusplus

namespace sev {

namespace impl::q {

// template<class TFn>
// struct ConcurrentFunctorQueue;
template<class TRes, class... TArgs>
struct ConcurrentFunctorQueue
{
public:
	inline ConcurrentFunctorQueue(ptrdiff_t blockSize = (64 * 1024)) { if (SEV_ConcurrentFunctorQueue_init(&m, blockSize)) throw std::bad_alloc(); }
	inline ConcurrentFunctorQueue(nothrow_t, ptrdiff_t blockSize = (64 * 1024)) noexcept { SEV_ConcurrentFunctorQueue_init(&m, blockSize); }
	inline ~ConcurrentFunctorQueue() { SEV_ConcurrentFunctorQueue_release(&m); }

	inline void push(const FunctorView<TRes(TArgs...)> &fv)
	{
		const FunctorVt<TRes(TArgs...)> *vt;
		void *ptr;
		fv.extract(vt, ptr);
		ExceptionHandle::rethrow(SEV_ConcurrentFunctorQueue_pushFunctorEx(&m, vt->get(), vt->size(), ptr, vt->get()->ConstCopyConstructor));
	}

	inline void push(FunctorView<TRes(TArgs...)> &fv)
	{
		const FunctorVt<TRes(TArgs...)> *vt;
		void *ptr;
		bool movable;
		fv.extract(vt, ptr, movable, false);
		ExceptionHandle::rethrow(SEV_ConcurrentFunctorQueue_pushFunctorEx(&m, vt->get(), vt->size(), ptr, vt->get()->CopyConstructor));
	}

	inline void push(FunctorView<TRes(TArgs...)> &&fv)
	{
		const FunctorVt<TRes(TArgs...)> *vt;
		void *ptr;
		bool movable;
		fv.extract(vt, ptr, movable, true);
		ExceptionHandle::rethrow(SEV_ConcurrentFunctorQueue_pushFunctorEx(&m, vt->get(), vt->size(), ptr, movable ? vt->get()->MoveConstructor : vt->get()->CopyConstructor));
	}

	inline errno_t push(nothrow_t, const FunctorView<TRes(TArgs...)> &fv) noexcept
	{
		const FunctorVt<TRes(TArgs...)> *vt;
		void *ptr;
		fv.extract(vt, ptr);
		return SEV_ConcurrentFunctorQueue_pushFunctorEx(&m, vt->get(), vt->size(), ptr, vt->get()->ConstCopyConstructor);
	}

	inline errno_t push(nothrow_t, FunctorView<TRes(TArgs...)> &fv) noexcept
	{
		const FunctorVt<TRes(TArgs...)> *vt;
		void *ptr;
		bool movable;
		fv.extract(vt, ptr, movable, false);
		return SEV_ConcurrentFunctorQueue_pushFunctorEx(&m, vt->get(), vt->size(), ptr, vt->get()->CopyConstructor);
	}

	inline errno_t push(nothrow_t, FunctorView<TRes(TArgs...)> &&fv) noexcept
	{
		const FunctorVt<TRes(TArgs...)> *vt;
		void *ptr;
		bool movable;
		fv.extract(vt, ptr, movable, true);
		return SEV_ConcurrentFunctorQueue_pushFunctorEx(&m, vt->get(), vt->size(), ptr, movable ? vt->get()->MoveConstructor : vt->get()->CopyConstructor);
	}

	inline SEV_ConcurrentFunctorQueue *get() noexcept { return &m; }

protected:
	SEV_ConcurrentFunctorQueue m;

public:
	ConcurrentFunctorQueue(const ConcurrentFunctorQueue &) = delete;
	ConcurrentFunctorQueue(ConcurrentFunctorQueue &&) = delete;

	ConcurrentFunctorQueue &operator= (const ConcurrentFunctorQueue &) = delete;
	ConcurrentFunctorQueue &operator= (ConcurrentFunctorQueue &&) = delete;

};

}

template<class TFn>
struct ConcurrentFunctorQueue;

template<class TRes, class... TArgs>
struct ConcurrentFunctorQueue<TRes(TArgs...)> : public impl::q::ConcurrentFunctorQueue<TRes, TArgs...>
{
public:
	inline ConcurrentFunctorQueue(ptrdiff_t blockSize = (64 * 1024)) : impl::q::ConcurrentFunctorQueue<TRes, TArgs...>(blockSize) { }
	inline ConcurrentFunctorQueue(nothrow_t, ptrdiff_t blockSize = (64 * 1024)) noexcept : impl::q::ConcurrentFunctorQueue<TRes, TArgs...>(nothrow, blockSize) { }

	inline TRes tryCallAndPop(ExceptionHandle &eh, bool &success, TArgs... args) noexcept
	{
		TRes res;
		const SEV_FunctorVt *rvt = null;
		auto invokeData = [&](void *ptr, const SEV_FunctorVt *vt) -> errno_t {
			typedef FunctorVt<TRes(TArgs...)>::TTryInvoke TFn; // typedef TRes(*TFn)(void *ptr, void **err, TArgs...);
			rvt = vt;
			res = ((TFn)vt->TryInvoke)(ptr, eh, args...);
			return eh.raised() ? eh.errNo() : SEV_ESUCCESS;
		};
		static const FunctorVt<errno_t(void *, const SEV_FunctorVt *vt)> wrapvt(invokeData);
		typedef FunctorVt<errno_t(void *, const SEV_FunctorVt *vt)>::TInvoke TInvoke;
		static const TInvoke invokeCall = (TInvoke)wrapvt.get()->Invoke;
		errno_t ec = SEV_ConcurrentFunctorQueue_tryCallAndPopFunctorEx(&m, invokeCall, (void *)(&invokeData));
		success = rvt;
		if (!eh.raised() && ec)
		{
			if (success) eh.capture(ec);
			else if (ec != ENODATA) eh.capture(ec);
		}
		return res;
	}

	inline TRes tryCallAndPop(bool &success, TArgs... args)
	{
		// This turns a lambda call into a function with three pointers (arguments, function, capture list)
		ExceptionHandle eh;
		TRes res = tryCallAndPop(eh, success, args...);
		eh.rethrow();
		return std::move(res);
	}

	inline TRes tryCallAndPop(errno_t &eno, bool &success, TArgs... args) noexcept
	{
		// This turns a lambda call into a function with three pointers (arguments, function, capture list)
		ExceptionHandle eh;
		auto fin = gsl::finally([&]() -> void { eno = eh.rethrow(nothrow); });
		return tryCallAndPop(eh, success, args...);
	}
};

template<class... TArgs>
struct ConcurrentFunctorQueue<void(TArgs...)> : public impl::q::ConcurrentFunctorQueue<void, TArgs...>
{
public:
	inline ConcurrentFunctorQueue(ptrdiff_t blockSize = (64 * 1024)) : impl::q::ConcurrentFunctorQueue<void, TArgs...>(blockSize) { }
	inline ConcurrentFunctorQueue(nothrow_t, ptrdiff_t blockSize = (64 * 1024)) noexcept : impl::q::ConcurrentFunctorQueue<void, TArgs...>(nothrow, blockSize) { }

	inline void tryCallAndPop(ExceptionHandle &eh, bool &success, TArgs... args) noexcept
	{
		const SEV_FunctorVt *rvt = null;
		auto invokeData = [&](void *ptr, const SEV_FunctorVt *vt) -> errno_t {
			typedef FunctorVt<void(TArgs...)>::TTryInvoke TFn; // typedef TRes(*TFn)(void *ptr, void **err, TArgs...);
			rvt = vt;
			((TFn)vt->TryInvoke)(ptr, eh, args...);
			return eh.raised() ? eh.errNo() : SEV_ESUCCESS;
		};
		static const FunctorVt<errno_t(void *, const SEV_FunctorVt *vt)> wrapvt(invokeData);
		typedef FunctorVt<errno_t(void *, const SEV_FunctorVt *vt)>::TInvoke TInvoke;
		static const TInvoke invokeCall = (TInvoke)wrapvt.get()->Invoke;
		errno_t ec = SEV_ConcurrentFunctorQueue_tryCallAndPopFunctorEx(&m, invokeCall, (void *)(&invokeData));
		success = rvt;
		if (!eh.raised() && ec)
		{
			if (success) eh.capture(ec);
			else if (ec != ENODATA) eh.capture(ec);
		}
	}

	inline void tryCallAndPop(bool &success, TArgs... args)
	{
		// This turns a lambda call into a function with three pointers (arguments, function, capture list)
		ExceptionHandle eh;
		tryCallAndPop(eh, success, args...);
		eh.rethrow();
	}

	inline void tryCallAndPop(errno_t &eno, bool &success, TArgs... args) noexcept
	{
		// This turns a lambda call into a function with three pointers (arguments, function, capture list)
		ExceptionHandle eh;
		auto fin = gsl::finally([&]() -> void { eno = eh.rethrow(nothrow); });
		tryCallAndPop(eh, success, args...);
	}
};

}

#endif

#endif /* #ifndef SEV_CONCURRENT_FUNCTOR_QUEUE_H */

/* end of file */
