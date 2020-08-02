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
#ifndef SEV_ATOMIC_H
#define SEV_ATOMIC_H

#include "platform.h"

#ifdef __cplusplus
#include <atomic>
#include <thread>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef volatile LONG SEV_AtomicInt32;
typedef volatile ptrdiff_t SEV_AtomicPtrDiff;
typedef volatile PVOID SEV_AtomicPtr;

static_assert(sizeof(int32_t) == sizeof(SEV_AtomicInt32));
static_assert(sizeof(ptrdiff_t) == sizeof(SEV_AtomicPtrDiff));
static_assert(sizeof(void *) == sizeof(SEV_AtomicPtr));

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
static_assert(sizeof(std::atomic_int32_t) == sizeof(SEV_AtomicInt32));
static_assert(sizeof(std::atomic_ptrdiff_t) == sizeof(SEV_AtomicPtrDiff));
static_assert(sizeof(std::atomic_ptrdiff_t) == sizeof(SEV_AtomicPtr));
#endif

static SEV_FORCE_INLINE int32_t SEV_AtomicInt32_load(SEV_AtomicInt32 *src)
{
#if defined(__cplusplus)
	return ((std::atomic_int32_t *)src)->load();
#elif defined(_WIN32)
	return InterlockedCompareExchange(src, 0, 0);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE void SEV_AtomicInt32_store(SEV_AtomicInt32 *dst, int32_t val)
{
#if defined(__cplusplus)
	((std::atomic_int32_t *)dst)->store(val);
#elif defined(_WIN32)
	InterlockedExchange(dst, val);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE int32_t SEV_AtomicInt32_exchange(SEV_AtomicInt32 *dst, int32_t val)
{
#ifdef _WIN32
	return InterlockedExchange(dst, val);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE int32_t SEV_AtomicInt32_compareExchange(SEV_AtomicInt32 *dst, int32_t exch, int32_t comp)
{
#ifdef _WIN32
	return InterlockedCompareExchange(dst, exch, comp);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE int32_t SEV_AtomicInt32_increment(SEV_AtomicInt32 *var)
{
#ifdef _WIN32
	return InterlockedIncrement(var);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE int32_t SEV_AtomicInt32_decrement(SEV_AtomicInt32 *var)
{
#ifdef _WIN32
	return InterlockedDecrement(var);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE ptrdiff_t SEV_AtomicPtrDiff_load(SEV_AtomicPtrDiff *src)
{
#if defined(__cplusplus)
	return ((std::atomic_ptrdiff_t *)src)->load();
#elif defined(_WIN32)
	return (ptrdiff_t)InterlockedCompareExchangePointer((volatile PVOID *)src, null,null);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE void SEV_AtomicPtrDiff_store(SEV_AtomicPtrDiff *dst, ptrdiff_t val)
{
#if defined(__cplusplus)
	((std::atomic_ptrdiff_t *)dst)->store(val);
#elif defined(_WIN32)
	InterlockedExchangePointer((volatile PVOID *)dst, (PVOID)val);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE ptrdiff_t SEV_AtomicPtrDiff_exchange(SEV_AtomicPtrDiff *dst, ptrdiff_t val)
{
#ifdef _WIN32
	return (ptrdiff_t)InterlockedExchangePointer((volatile PVOID *)dst, (PVOID)val);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE ptrdiff_t SEV_AtomicPtrDiff_compareExchange(SEV_AtomicPtrDiff *dst, ptrdiff_t exch, ptrdiff_t comp)
{
#ifdef _WIN32
	return (ptrdiff_t)InterlockedCompareExchangePointer((volatile PVOID *)dst, (PVOID)exch, (PVOID)comp);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE ptrdiff_t SEV_AtomicPtrDiff_increment(SEV_AtomicPtrDiff *var)
{
#ifdef _WIN32
	return InterlockedIncrementSizeT(var);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE ptrdiff_t SEV_AtomicPtrDiff_decrement(SEV_AtomicPtrDiff *var)
{
#ifdef _WIN32
	return InterlockedDecrementSizeT(var);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE void *SEV_AtomicPtr_load(SEV_AtomicPtr *src)
{
#if defined(__cplusplus)
	return (void *)((std::atomic_ptrdiff_t *)src)->load();
#elif defined(_WIN32)
	return (void *)InterlockedCompareExchangePointer(src, null,null);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE void SEV_AtomicPtr_store(SEV_AtomicPtr *dst, void *val)
{
#if defined(__cplusplus)
	((std::atomic_ptrdiff_t *)dst)->store((ptrdiff_t)val);
#elif defined(_WIN32)
	InterlockedExchangePointer(dst, val);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE void *SEV_AtomicPtr_exchange(SEV_AtomicPtr *dst, void *val)
{
#ifdef _WIN32
	return InterlockedExchangePointer(dst, val);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE void *SEV_AtomicPtr_compareExchange(SEV_AtomicPtr *dst, void *exch, void *comp)
{
#ifdef _WIN32
	return InterlockedCompareExchangePointer(dst, exch, comp);
#else
	static_assert(false);
#endif
}

static SEV_FORCE_INLINE void SEV_Thread_yield()
{
#if defined(__cplusplus)
	std::this_thread::yield();
#elif defined(_WIN32)
	SwitchToThread();
#else
	static_assert(false);
#endif
}

#endif /* #ifndef SEV_ATOMIC_H */

/* end of file */
