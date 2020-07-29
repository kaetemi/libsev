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
#ifndef SEV_CONCURRENT_FUNCTOR_QUEUE_H
#define SEV_CONCURRENT_FUNCTOR_QUEUE_H

#include "platform.h"
#include "functor_view.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SEV_ConcurrentFunctorQueue
{
	ptrdiff_t BlockSize;
	uint8_t *volatile ReadBlock;
	uint8_t *volatile WriteBlock;
	uint8_t *volatile SpareBlock;

	/*
	volatile ptrdiff_t ReadIdx;
	volatile ptrdiff_t PreWriteIdx; // While PreWriteIdx > 
	volatile ptrdiff_t WriteIdx;
	*/

	volatile ptrdiff_t ReadIdx;
	volatile ptrdiff_t PreWriteIdx;

};

SEV_ConcurrentFunctorQueue *SEV_ConcurrentFunctorQueue_create();
void SEV_ConcurrentFunctorQueue_destroy(SEV_ConcurrentFunctorQueue *concurrentFunctorQueue);

void SEV_ConcurrentFunctorQueue_init(SEV_ConcurrentFunctorQueue *me);
void SEV_ConcurrentFunctorQueue_release(SEV_ConcurrentFunctorQueue *me);

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
	inline ConcurrentFunctorQueue() { SEV_ConcurrentFunctorQueue_init(&m); }
	inline ~ConcurrentFunctorQueue() { SEV_ConcurrentFunctorQueue_release(&m); }
	
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
