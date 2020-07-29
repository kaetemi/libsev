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
#ifndef SEV_FUNCTOR_H
#define SEV_FUNCTOR_H

#include "platform.h"
#include "functor_vt.h"

#ifdef __cplusplus
extern "C" {
#endif

SEV_LIB void *SEV_alignedMAlloc(ptrdiff_t size, size_t alignment);
SEV_LIB void SEV_alignedFree(void *ptr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef __cplusplus

namespace sev {

inline void *alignedMAlloc(ptrdiff_t size, size_t alignment)
{
	return SEV_alignedMAlloc(size, alignment);
}

inline void alignedFree(void *ptr)
{
	return SEV_alignedFree(ptr);
}

template<class TFn>
struct alignas(SEV_FUNCTOR_ALIGN) Functor;
template<class TRes, class... TArgs>
struct alignas(SEV_FUNCTOR_ALIGN) Functor<TRes(TArgs...)>
{
private:
	using TVt = FunctorVt<TRes(TArgs...)>;
	static constexpr int c_Capacity = sizeof(void *) * 7;

public:
#pragma warning(push)
#pragma warning(disable: 26495)
#ifdef __INTELLISENSE__
#pragma diag_suppress 2398
#endif
	inline Functor()
	{
		static const TVt vtable;
		m_Vt = &vtable;
	}
#pragma warning(pop)
	
	template<class TFn>
	Functor(TFn &&fn)
	{
		static const TVt vtable = TVt(fn);
		m_Vt = &vtable;
		TFn *f = reinterpret_cast<TFn *>(p_allocPtr(sizeof(TFn))); // Allocate space
		new (f) TFn(forward<decltype(fn)>(fn)); // Move or copy construct
	}

	// Construct from vtable and data pointer
	inline Functor(const TVt *vt, const void *ptr)
	{
		SEV_ASSERT(vt);
		m_Vt = vt;
		void *p = p_allocPtr(sizeof(vt->size())); // Allocate space
		vt->copyConstructor(p, ptr);
	}

	// Construct from vtable and data pointer, use move constructor when movable is set
	inline Functor(const TVt *vt, void *ptr, bool movable = false)
	{
		SEV_ASSERT(vt);
		m_Vt = vt;
		void *p = p_allocPtr(sizeof(vt->size())); // Allocate space
		if (movable) vt->moveConstructor(p, ptr);
		else vt->copyConstructor(p, ptr);
	}

	inline ~Functor() noexcept
	{
		SEV_ASSERT(m_Vt);
		p_destroyPtr();
	}

	inline TRes operator()(TArgs... value)
	{
		return m_Vt->invoke(p_ptr(), value...);
	}

	inline Functor(const Functor &other)
	{
		m_Vt = other.m_Vt;
		void *ptr = p_allocPtr(m_Vt->size()); // Allocate space
		m_Vt->copyConstructor(ptr, other.p_ptr());
	}

	inline Functor(Functor &other)
	{
		m_Vt = other.m_Vt;
		void *ptr = p_allocPtr(m_Vt->size()); // Allocate space
		m_Vt->copyConstructor(ptr, other.p_ptr());
	}

	inline Functor(Functor &&other) noexcept
	{
		static const TVt vtable;
		m_Vt = other.m_Vt;
		if (m_Vt->size() > c_Capacity)
			m_Storage.Ptr = other.m_Storage.Ptr; // Just move the ptr
		else
			m_Vt->moveConstructor(m_Storage.Data, other.m_Storage.Data); // Move the data
		other.m_Vt = &vtable; // Empty other vtable
	}

	template<typename T = Functor>
	inline T &operator= (T &&other) noexcept
	{
		if (this != &other)
		{
			p_destroyPtr();
			new (this) T(forward(other));
		}
		return *this;
	}

	inline void *ptr()
	{
		return p_ptr();
	}

	inline const void *ptr() const
	{
		return p_ptr();
	}

	inline const TVt *vt() const
	{
		return m_Vt;
	}

private:
	inline void *p_ptr() noexcept
	{
		return m_Vt->size() > c_Capacity ? m_Storage.Ptr : m_Storage.Data;
	}

	inline const void *p_ptr() const noexcept
	{
		return m_Vt->size() > c_Capacity ? m_Storage.Ptr : m_Storage.Data;
	}

	inline void *p_allocPtr(ptrdiff_t size)
	{
		if (size > c_Capacity)
		{
			void *ptr = alignedMAlloc(size, SEV_FUNCTOR_ALIGN);
			return m_Storage.Ptr;
		}
		else
		{
			return m_Storage.Data;
		}
	}

	inline void p_destroyPtr() noexcept
	{
		if (m_Vt->size() > c_Capacity)
		{
			m_Vt->destroy(m_Storage.Ptr);
			alignedFree(m_Storage.Ptr);
		}
		else
		{
			m_Vt->destroy(m_Storage.Data);
		}
	}

private:
	union
	{
		void *Ptr;
		uint8_t Data[c_Capacity];

	} m_Storage;

	const TVt *m_Vt;

};

}

#endif

#endif /* #ifndef SEV_FUNCTOR_H */

/* end of file */
