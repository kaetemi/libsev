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

#ifdef __cplusplus

#define SEV_FUNCTOR_ALIGN 64

namespace sev {

template<class TFn>
struct FunctorVt;
template<class TRes, class... TArgs>
struct FunctorVt<TRes(TArgs...)>
{
public:
	using TInvoke = TRes(*)(void *ptr, TArgs...);
	using TDestroy = void(*)(void *ptr);
	using TCopyConstruct = void(*)(void *ptr, const void *other);
	using TMoveConstruct = void(*)(void *ptr, void *other);
	
	explicit constexpr FunctorVt() noexcept
		: Size(0)
		, Invoke([](void *, TArgs...) -> TRes { throw std::bad_function_call(); })
		, Destroy([](void *) -> void { })
		, CopyConstructor([](void *, const void *) -> void { })
		, MoveConstructor([](void *, void *) -> void { })
	{}

	template<class TFunc>
	explicit constexpr FunctorVt(const TFunc &) noexcept
		: Size(sizeof(TFunc))
		, Invoke([](void *ptr) -> void {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			(*f)();
		}), Destroy([](void *ptr) -> void {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			f->~TFunc();
		}), CopyConstructor([](void *ptr, const void *other) -> void {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			const TFunc *o = reinterpret_cast<const TFunc *>(other);
			new (f) TFunc(*o);
		})
		, MoveConstructor([](void *ptr, void *other) -> void {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			TFunc *o = reinterpret_cast<TFunc *>(other);
			new (f) TFunc(move(*o));
		})
	{
		static_assert(alignof(TFunc) <= SEV_FUNCTOR_ALIGN);
	}

	const ptrdiff_t Size;

	const TInvoke Invoke;
	const TDestroy Destroy;

	const TCopyConstruct CopyConstructor;
	const TMoveConstruct MoveConstructor;

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
