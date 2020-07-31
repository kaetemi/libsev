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

struct SEV_ConcurrentFunctorQueue
{
	ptrdiff_t BlockSize;
	uint8_t *volatile ReadBlock;
	uint8_t *volatile WriteBlock;
	uint8_t *volatile SpareBlock;

	volatile ptrdiff_t PreWriteIdx;

	SEV_AtomicSharedMutex DeleteLock;

};

SEV_LIB SEV_ConcurrentFunctorQueue *SEV_ConcurrentFunctorQueue_create(ptrdiff_t blockSize);
SEV_LIB void SEV_ConcurrentFunctorQueue_destroy(SEV_ConcurrentFunctorQueue *concurrentFunctorQueue);

SEV_LIB errno_t SEV_ConcurrentFunctorQueue_init(SEV_ConcurrentFunctorQueue *me, ptrdiff_t blockSize);
SEV_LIB void SEV_ConcurrentFunctorQueue_release(SEV_ConcurrentFunctorQueue *me);

SEV_LIB errno_t SEV_ConcurrentFunctorQueue_push(SEV_ConcurrentFunctorQueue *me, void(*f)(void *ptr, void *args), void *ptr, ptrdiff_t size); // Does a memcpy of the data ptr
SEV_LIB errno_t SEV_ConcurrentFunctorQueue_pushFunctor(SEV_ConcurrentFunctorQueue *me, const SEV_FunctorVt *vt, void *ptr, void(*forwardConstructor)(void *ptr, void *other)); // Returns EOTHER if forwardConstructor throws, returns ENOMEM in case of memory allocation failure, 0 if OK
#ifdef __cplusplus
SEV_LIB errno_t SEV_ConcurrentFunctorQueue_pushFunctorEx(SEV_ConcurrentFunctorQueue *me, const SEV_FunctorVt *vt, ptrdiff_t size, void *ptr, void(*forwardConstructor)(void *ptr, void *other)); // Throws only if forwardConstructor throws
#endif

SEV_LIB errno_t SEV_ConcurrentFunctorQueue_tryCallAndPop(SEV_ConcurrentFunctorQueue *me, void *args); // Returns ENODATA if nothing to pop, EOTHER if function threw an exception, 0 if OK
SEV_LIB errno_t SEV_ConcurrentFunctorQueue_tryCallAndPopFunctor(SEV_ConcurrentFunctorQueue *me, void(*caller)(void *args, void *ptr, void *f), void *args);
#ifdef __cplusplus
SEV_LIB bool SEV_ConcurrentFunctorQueue_tryCallAndPopFunctorEx(SEV_ConcurrentFunctorQueue *me, void(*caller)(void *args, void *ptr, void *f), void *args); // Throws when function throws and exception
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef __cplusplus

namespace sev {

template<class TFn>
struct ConcurrentFunctorQueue;
template<class TRes, class... TArgs>
struct ConcurrentFunctorQueue<TRes(TArgs...)>
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
		if (SEV_ConcurrentFunctorQueue_pushFunctorEx(&m, vt->raw(), vt->size(), ptr, vt->raw()->ConstCopyConstructor))
			throw std::bad_alloc();
	}

	inline void push(FunctorView<TRes(TArgs...)> &fv)
	{
		const FunctorVt<TRes(TArgs...)> *vt;
		void *ptr;
		bool movable;
		fv.extract(vt, ptr, movable, false);
		if (SEV_ConcurrentFunctorQueue_pushFunctorEx(&m, vt->raw(), vt->size(), ptr, vt->raw()->CopyConstructor))
			throw std::bad_alloc();
	}
	
	inline void push(FunctorView<TRes(TArgs...)> &&fv)
	{
		const FunctorVt<TRes(TArgs...)> *vt;
		void *ptr;
		bool movable;
		fv.extract(vt, ptr, movable, true);
		if (SEV_ConcurrentFunctorQueue_pushFunctorEx(&m, vt->raw(), vt->size(), ptr, movable ? vt->raw()->MoveConstructor : vt->raw()->CopyConstructor))
			throw std::bad_alloc();
	}

	inline TRes tryCallAndPop(bool &success, TArgs... args)
	{
		// This turns a lambda call into a function with three pointers (arguments, function, capture list)

		TRes res;
		auto caller = [=, &res](void *ptr, void *f) -> void {
			typedef TRes(*TFn)(void *, TArgs...);
			res = ((TFn)f)(ptr, args...);
		};
		static const FunctorVt<void(void *, void *)> vt(caller);
		typedef FunctorVt<void(void *, void *)>::TInvoke TInvoke;
		static const TInvoke inv = (TInvoke)vt.raw()->Invoke;
		success = SEV_ConcurrentFunctorQueue_tryCallAndPopFunctorEx(&m, inv, (void *)(&caller));
		return res;
	}
	
private:
	SEV_ConcurrentFunctorQueue m;

public:
	ConcurrentFunctorQueue(const ConcurrentFunctorQueue &) = delete;
	ConcurrentFunctorQueue(ConcurrentFunctorQueue &&) = delete;

	ConcurrentFunctorQueue& operator= (const ConcurrentFunctorQueue &) = delete;
	ConcurrentFunctorQueue& operator= (ConcurrentFunctorQueue &&) = delete;
	
};

}

#endif

#endif /* #ifndef SEV_CONCURRENT_FUNCTOR_QUEUE_H */

/* end of file */
