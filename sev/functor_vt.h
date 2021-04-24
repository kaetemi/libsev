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
#include "exception.h"

#define SEV_FUNCTOR_ALIGN 64

#ifdef __cplusplus
extern "C" {
#endif

struct SEV_FunctorVt
{
	ptrdiff_t Size;
	void(*ConstCopyConstructor)(void *ptr, const void *other);
	void(*CopyConstructor)(void *ptr, void *other);
	void(*MoveConstructor)(void *ptr, void *other);
	void(*Destroy)(void *ptr);

	void *Invoke;
	void *TryInvoke;

};

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef __cplusplus

namespace sev {

template<class TFn>
struct FunctorVt;
template<class TRes, class... TArgs>
struct FunctorVt<TRes(TArgs...)>
{
public:
	using TConstCopyConstructor = void(*)(void *ptr, const void *other);
	using TCopyConstructor = void(*)(void *ptr, void *other);
	using TMoveConstructor = void(*)(void *ptr, void *other);
	using TDestroy = void(*)(void *ptr);
	using TInvoke = TRes(*)(void *ptr, TArgs...);
	using TTryInvoke = TRes(*)(void *ptr, ExceptionHandle &, TArgs...);
	using TInvokeCatch = TRes(*)(void *ptr, void **err, TArgs...);
	using TDestroyException = void(*)(void *ptr);
	
	explicit constexpr FunctorVt() noexcept
		: m{ /*Size*/(0)
		, /*ConstCopyConstructor*/([](void *, const void *) -> void {})
		, /*CopyConstructor*/([](void *, void *) -> void {})
		, /*MoveConstructor*/([](void *, void *) -> void {})
		, /*Destroy*/([](void *) -> void {})
		, /*Invoke*/((TInvoke)([](void *, TArgs...) -> TRes { throw std::bad_function_call(); }))
		, /*TryInvoke*/((TTryInvoke)([](void *, ExceptionHandle &, TArgs...) -> TRes { throw std::bad_function_call(); })) }
	{
		static_assert(sizeof(FunctorVt) == sizeof(SEV_FunctorVt));
	}

	explicit constexpr FunctorVt(SEV_FunctorVt &&raw) noexcept
		: m(raw)
	{
		static_assert(sizeof(FunctorVt) == sizeof(SEV_FunctorVt));
	}

	template<class TFunc>
	explicit constexpr FunctorVt(const TFunc &) noexcept
		: m{ /*Size*/(sizeof(TFunc))
		, /*ConstCopyConstructor*/([](void *ptr, const void *other) -> void {
			//printf("[[ConstCopyConstructor]]\n");
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			const TFunc *o = reinterpret_cast<const TFunc *>(other);
			new (f) TFunc(*o);
		}), /*CopyConstructor*/([](void *ptr, void *other) -> void {
			//printf("[[CopyConstructor]]\n");
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			TFunc *o = reinterpret_cast<TFunc *>(other);
			new (f) TFunc(*o);
			})
		, /*MoveConstructor*/([](void *ptr, void *other) -> void {
			//printf("[[MoveConstructor]]\n");
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			TFunc *o = reinterpret_cast<TFunc *>(other);
			new (f) TFunc(std::move(*o));
		}), /*Destroy*/([](void *ptr) -> void {
			//printf("[[Destroy]]\n");
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			f->~TFunc();
		}), /*Invoke*/(TInvoke)([](void *ptr, TArgs... args) -> TRes {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			return (*f)(args...);
		}), /*TryInvoke*/(TTryInvoke)([](void *ptr, ExceptionHandle &eh, TArgs... args) -> TRes {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			return eh.capture<TRes>([&]() -> TRes {
				return (*f)(args...);
			});
		})}
	{
		static_assert(alignof(TFunc) <= SEV_FUNCTOR_ALIGN);
		static_assert(sizeof(FunctorVt) == sizeof(SEV_FunctorVt));
	}

	template<class TFunc>
	explicit constexpr FunctorVt(const TFunc &, std::nothrow_t) noexcept // Use only for wrapping functions that definitely don't throw
		: m{ /*Size*/(sizeof(TFunc))
		, /*ConstCopyConstructor*/([](void *ptr, const void *other) -> void {
			//printf("[[ConstCopyConstructor]]\n");
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			const TFunc *o = reinterpret_cast<const TFunc *>(other);
			new (f) TFunc(*o);
		}), /*CopyConstructor*/([](void *ptr, void *other) -> void {
			//printf("[[CopyConstructor]]\n");
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			TFunc *o = reinterpret_cast<TFunc *>(other);
			new (f) TFunc(*o);
			})
		, /*MoveConstructor*/([](void *ptr, void *other) -> void {
			//printf("[[MoveConstructor]]\n");
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			TFunc *o = reinterpret_cast<TFunc *>(other);
			new (f) TFunc(std::move(*o));
		}), /*Destroy*/([](void *ptr) -> void {
			//printf("[[Destroy]]\n");
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			f->~TFunc();
		}), /*Invoke*/(TInvoke)([](void *ptr, TArgs... args) -> TRes {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			return (*f)(args...);
		}), /*TryInvoke*/(TTryInvoke)([](void *ptr, ExceptionHandle &, TArgs... args) -> TRes {
			TFunc *f = reinterpret_cast<TFunc *>(ptr);
			return (*f)(args...);
			})}
	{
		static_assert(alignof(TFunc) <= SEV_FUNCTOR_ALIGN);
		static_assert(sizeof(FunctorVt) == sizeof(SEV_FunctorVt));
	}

	inline const SEV_FunctorVt *get() const { return &m; }

	inline ptrdiff_t size() const { return m.Size; }
	inline void constCopyConstructor(void *ptr, const void *other) const { return m.ConstCopyConstructor(ptr, other); }
	inline void copyConstructor(void *ptr, void *other) const { return m.CopyConstructor(ptr, other); }
	inline void moveConstructor(void *ptr, void *other) const { return m.MoveConstructor(ptr, other); }
	inline void destroy(void *ptr) const { return m.Destroy(ptr); }

	inline TRes invoke(void *ptr, TArgs... value) const { return ((TInvoke)m.Invoke)(ptr, value...); }
	inline TRes invoke(void *ptr, ExceptionHandle &eh, TArgs... value) const { return ((TTryInvoke)m.TryInvoke)(ptr, eh, value...); }

private:
	const SEV_FunctorVt m;

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
