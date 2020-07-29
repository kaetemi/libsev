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
#ifndef SEV_FUNCTOR_VT_H
#define SEV_FUNCTOR_VT_H

#include "platform.h"

#define SEV_FUNCTOR_ALIGN 64

#ifdef __cplusplus
extern "C" {
#endif

struct SEV_FunctorVtRaw
{
public:
	ptrdiff_t Size;
	void *Invoke;
	void(*Destroy)(void *ptr);
	void(*ConstCopyConstructor)(void *ptr, const void *other);
	void(*CopyConstructor)(void *ptr, void *other);
	void(*MoveConstructor)(void *ptr, void *other);

};

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef __cplusplus

namespace sev {

using FunctorVtRaw = SEV_FunctorVtRaw;

template<class TFn>
struct FunctorVt;
template<class TRes, class... TArgs>
struct FunctorVt<TRes(TArgs...)>
{
public:
	using TInvoke = TRes(*)(void *ptr, TArgs...);
	using TDestroy = void(*)(void *ptr);
	using TConstCopyConstructor = void(*)(void *ptr, const void *other);
	using TCopyConstructor = void(*)(void *ptr, void *other);
	using TMoveConstructor = void(*)(void *ptr, void *other);
	
	explicit constexpr FunctorVt() noexcept
		: m{ /*Size*/(0)
		, /*Invoke*/((TInvoke)([](void *, TArgs...) -> TRes { throw std::bad_function_call(); }))
		, /*Destroy*/([](void *) -> void {})
		, /*ConstCopyConstructor*/([](void *, const void *) -> void {})
		, /*CopyConstructor*/([](void *, void *) -> void {})
		, /*MoveConstructor*/([](void *, void *) -> void {}) }
	{
		static_assert(sizeof(FunctorVt) == sizeof(FunctorVtRaw));
	}

	explicit constexpr FunctorVt(FunctorVtRaw &&raw) noexcept
		: m(raw)
	{
		static_assert(sizeof(FunctorVt) == sizeof(FunctorVtRaw));
	}

	template<class TFunc>
	explicit constexpr FunctorVt(const TFunc &) noexcept
		: m{ /*Size*/(sizeof(TFunc))
		, /*Invoke*/(TInvoke)([](void *ptr) -> void {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			(*f)();
		}), /*Destroy*/([](void *ptr) -> void {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			f->~TFunc();
		}), /*ConstCopyConstructor*/([](void *ptr, const void *other) -> void {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			const TFunc *o = reinterpret_cast<const TFunc *>(other);
			new (f) TFunc(*o);
		}), /*CopyConstructor*/([](void *ptr, void *other) -> void {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			TFunc *o = reinterpret_cast<TFunc *>(other);
			new (f) TFunc(*o);
			})
		, /*MoveConstructor*/([](void *ptr, void *other) -> void {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			TFunc *o = reinterpret_cast<TFunc *>(other);
			new (f) TFunc(move(*o));
		}) }
	{
		static_assert(alignof(TFunc) <= SEV_FUNCTOR_ALIGN);
		static_assert(sizeof(FunctorVt) == sizeof(FunctorVtRaw));
	}

	inline FunctorVtRaw &raw() const { return m; }

	inline ptrdiff_t size() const { return m.Size; }
	inline TRes invoke(void *ptr, TArgs... value) const { return ((TInvoke)m.Invoke)(ptr, value...); }
	inline void destroy(void *ptr) const { return m.Destroy(ptr); }
	inline void constCopyConstructor(void *ptr, const void *other) const { return m.ConstCopyConstructor(ptr, other); }
	inline void copyConstructor(void *ptr, void *other) const { return m.CopyConstructor(ptr, other); }
	inline void moveConstructor(void *ptr, void *other) const { return m.MoveConstructor(ptr, other); }

	/*
	const ptrdiff_t Size;

	const TInvoke Invoke;
	const TDestroy Destroy;
	const TConstCopyConstructor ConstCopyConstructor;
	const TCopyConstructor CopyConstructor;
	const TMoveConstructor MoveConstructor;
	*/

private:
	const FunctorVtRaw m;

public:
	FunctorVt(const FunctorVt &) = delete;
	FunctorVt(FunctorVt &&) = delete;

	FunctorVt& operator= (const FunctorVt &) = delete;
	FunctorVt& operator= (FunctorVt &&) = delete;

	~FunctorVt() = default;

};

}

#endif

#endif /* #ifndef SEV_FUNCTOR_VT_H */

/* end of file */
