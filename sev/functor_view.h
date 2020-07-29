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
#ifndef SEV_FUNCTOR_VIEW_H
#define SEV_FUNCTOR_VIEW_H

#include "platform.h"
#include "exception.h"
#include "functor.h"

#ifdef __cplusplus

/*

TODO: `PlacementFunctor`: Functor with external storage that operates like a placement new operator?

NOTE: In the functor queue, multiple functors maybe calling simultaneously. It will directly use the memory of the queue, so the block will only be recycled when none of them are 'in call'.

*/

namespace sev {

template<class TFn>
struct FunctorView;
template<class TRes, class... TArgs>
struct FunctorView<TRes(TArgs...)>
{
private:
	using TVt = FunctorVt<TRes(TArgs...)>;
	using TFunctor = Functor<TRes(TArgs...)>;

public:
#pragma warning(push)
#pragma warning(disable: 26495)
#ifdef __INTELLISENSE__
#pragma diag_suppress 2398
#endif
	inline FunctorView() noexcept
	{
		static const auto vtable = TVt();
		m_Vt = vtable;
	}
#pragma warning(pop)

	template<class TFn>
	FunctorView(const TFn &fn) noexcept
	{
		static const auto vtable = TVt(fn);
		m_Vt = &vtable;
		m_Ptr = reinterpret_cast<void *>(const_cast<TFn *>(&fn));
		m_Movable = false;
	}

	template<class TFn>
	FunctorView(TFn &fn) noexcept
	{
		static const auto vtable = TVt(fn);
		m_Vt = &vtable;
		m_Ptr = reinterpret_cast<void *>(&fn);
		m_Movable = false;
	}

	template<class TFn>
	FunctorView(TFn &&fn) noexcept
	{
		static const auto vtable = TVt(fn);
		m_Vt = &vtable;
		m_Ptr = reinterpret_cast<void *>(&fn);
		m_Movable = true;
	}

	inline FunctorView(const TFunctor &fn) noexcept
	{
		m_Vt = fn.vt();
		m_Ptr = fn.ptr();
		m_Movable = false;
	}

	inline FunctorView(TFunctor &fn) noexcept
	{
		m_Vt = fn.vt();
		m_Ptr = fn.ptr();
		m_Movable = false;
	}

	inline FunctorView(TFunctor &&fn) noexcept
	{
		m_Vt = fn.vt();
		m_Ptr = fn.ptr();
		m_Movable = true;
	}

	inline TFunctor toFunctor(const bool forward = false)
	{
		bool movable = m_Movable && forward;
		TFunctor fn(m_Vt, m_Ptr, movable);
		if (movable)
		{
			static const auto vtable = TVt();
			m_Vt = &vtable; // This view is no longer valid
		}
		return fn;
	}

	inline TFunctor toFunctor() const
	{
		return TFunctor(m_Vt, m_Ptr, false);
	}

	inline void extract(const TVt *&vt, void *&ptr, bool &movable, bool forward = false)
	{
		movable = m_Movable && forward;
		vt = m_Vt;
		ptr = m_Ptr;
		if (movable)
		{
			static const auto vtable = TVt();
			m_Vt = &vtable; // This view is no longer valid
		}
	}

	inline void extract(const TVt *&vt, const void *&ptr) const // Movable is always false in this case, only copyable
	{
		vt = m_Vt;
		ptr = m_Ptr;
	}

	inline TRes operator()(TArgs... value)
	{
		return m_Vt->invoke(m_Ptr, value...);
	}

	inline bool movable() const
	{
		return m_Movable;
	}

	inline void *ptr()
	{
		return m_Ptr;
	}

	inline const void *ptr() const
	{
		return m_Ptr;
	}

	inline const TVt *vt() const
	{
		return m_Vt;
	}

	inline FunctorView(const FunctorView &other) noexcept
		: m_Vt(other.m_Vt), m_Ptr(other.m_Ptr)
	{
		// When copying a movable, it's no longer movable
		other.m_Movable = false;
	}

	inline FunctorView(FunctorView &other) noexcept
		: m_Vt(other.m_Vt), m_Ptr(other.m_Ptr)
	{
		// When copying a movable, it's no longer movable
		other.m_Movable = false;
	}

	inline FunctorView(FunctorView &&other) noexcept
		: m_Vt(other.m_Vt), m_Ptr(other.m_Ptr), m_Movable(other.m_Movable)
	{
		// When moving a movable, the other vtable gets reset to the default
		static const auto vtable = TVt();
		other.m_Vt = &vtable;
	}

	template<typename T = FunctorView>
	T &operator= (T &&other) noexcept
	{
		if (*this != other)
		{
			~FunctorView();
			new (this) FunctorView(forward(other));
		}
	}

private:
	const TVt *m_Vt;
	void *m_Ptr;
	mutable bool m_Movable;

};

}

#endif

#endif /* #ifndef SEV_FUNCTOR_VIEW_H */

/* end of file */
