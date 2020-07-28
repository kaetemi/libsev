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
#ifndef SEV_EVENT_FUNCTION_H
#define SEV_EVENT_FUNCTION_H

#include "platform.h"

/*

TODO: Rather than this mechanism, it might be more interesting to directly implement a function_ring_buffer...
that way there's no random allocations involved! (Post on this!)
Start with a proof of concept FunctionVector.

*/

#ifdef __cplusplus

#define SEV_EVENT_FUNCTION_ALIGN 64

namespace sev {

namespace impl {

/* Wraps the type into an empty struct so it can be passed blankly as an argment to a template function. */
template<class T>
struct EventFunctionWrapper
{
	using type = T;
};

class SEV_LIB EventFunctionVTable
{
public:
	using TInvoke = void(*)(void *ptr);
	using TDestroy = void(*)(void *ptr);
	using TCopyConstruct = void(*)(void *ptr, const void *other);
	using TMoveConstruct = void(*)(void *ptr, void *other);
	
	explicit constexpr EventFunctionVTable() noexcept
		: Size(0)
		, Invoke([](void *) -> void { throw std::bad_function_call(); })
		, Destroy([](void *) -> void { })
		, CopyConstruct([](void *, const void *) -> void { })
		, MoveConstruct([](void *, void *) -> void { })
	{
		// ...
	}
	
	template<class TFn>
	explicit constexpr EventFunctionVTable(EventFunctionWrapper<TFn>) noexcept
		: Size(sizeof(TFn))
		, Invoke([](void *ptr) -> void {
			TFn *f = reinterpret_cast<TFn *>(ptr);
			(*f)();
		}), Destroy([](void *ptr) -> void {
			TFn *f = reinterpret_cast<TFn *>(ptr);
			f->~TFn();
		}), CopyConstruct([](void *ptr, const void *other) -> void {
			TFn *f = reinterpret_cast<TFn *>(ptr);
			const TFn *o = reinterpret_cast<const TFn *>(other);
			new (f) TFn(*o);
		})
		, MoveConstruct([](void *ptr, void *other) -> void {
			TFn *f = reinterpret_cast<TFn *>(ptr);
			TFn *o = reinterpret_cast<TFn *>(other);
			new (f) TFn(move(*o));
		})
	{
		static_assert(alignof(TFn) <= SEV_EVENT_FUNCTION_ALIGN);
	}

	const ptrdiff_t Size;

	const TInvoke Invoke;
	const TDestroy Destroy;

	const TCopyConstruct CopyConstruct;
	const TMoveConstruct MoveConstruct;

	EventFunctionVTable(const EventFunctionVTable &) = delete;
	EventFunctionVTable(EventFunctionVTable &&) = delete;

	EventFunctionVTable& operator= (const EventFunctionVTable &) = delete;
	EventFunctionVTable& operator= (EventFunctionVTable &&) = delete;

	~EventFunctionVTable() = default;
};

} /* namespace impl */

class SEV_LIB alignas(SEV_EVENT_FUNCTION_ALIGN) EventFunction
{
private:
	static constexpr int c_Capacity = sizeof(void *) * 7;
	static const impl::EventFunctionVTable c_VTable;

public:
	inline constexpr EventFunction() : m_VTable(&c_VTable), m_Storage{}
	{
		
	}

	template<class TFn>
	EventFunction(TFn &&fn)
	{
		static const impl::EventFunctionVTable vtable = impl::EventFunctionVTable(impl::EventFunctionWrapper<TFn>());
		m_VTable = &vtable;
		TFn *f = reinterpret_cast<TFn *>(p_allocPtr(sizeof(TFn))); // Allocate space
		new (f) TFn(forward<decltype(fn)>(fn)); // Move or copy construct
	}

	inline ~EventFunction() noexcept
	{
		p_destroyPtr();
	}

	inline void operator()()
	{
		return m_VTable->Invoke(p_ptr());
	}

	inline EventFunction(const EventFunction &other)
	{
		m_VTable = other.m_VTable;
		void *ptr = p_allocPtr(m_VTable->Size); // Allocate space
		m_VTable->CopyConstruct(ptr, other.p_ptr());
	}

	inline EventFunction(EventFunction &&other) noexcept
	{
		m_VTable = other.m_VTable;
		if (m_VTable->Size > c_Capacity)
			m_Storage.Ptr = other.m_Storage.Ptr; // Just move the ptr
		else
			m_VTable->MoveConstruct(m_Storage.Data, other.m_Storage.Data); // Move the data
		other.m_VTable = null;
	}

	inline EventFunction &operator= (const EventFunction &other)
	{
		if (this != &other)
		{
			p_destroyPtr();
			new (this) EventFunction(other);
		}
		return *this;
	}

	inline EventFunction &operator= (EventFunction &&other) noexcept
	{
		if (this != &other)
		{
			p_destroyPtr();
			new (this) EventFunction(move(other));
		}
		return *this;
	}

private:
	inline void *p_ptr() noexcept
	{
		return m_VTable->Size > c_Capacity ? m_Storage.Ptr : m_Storage.Data;
	}

	inline const void *p_ptr() const noexcept
	{
		return m_VTable->Size > c_Capacity ? m_Storage.Ptr : m_Storage.Data;
	}

	inline void *p_allocPtr(ptrdiff_t size)
	{
		if (size > c_Capacity)
		{
			void *ptr = _aligned_malloc(size, SEV_EVENT_FUNCTION_ALIGN);
			return m_Storage.Ptr;
		}
		else
		{
			return m_Storage.Data;
		}
	}

	inline void p_destroyPtr() noexcept
	{
		if (m_VTable->Size > c_Capacity)
		{
			m_VTable->Destroy(m_Storage.Ptr);
			_aligned_free(m_Storage.Ptr);
		}
		else
		{
			m_VTable->Destroy(m_Storage.Data);
		}
	}

	union
	{
		void *Ptr;
		uint8_t Data[c_Capacity];

	} m_Storage;
	const impl::EventFunctionVTable *m_VTable;

};

}

typedef sev::EventFunction SEV_EventFunction;

#endif

#endif /* #ifndef SEV_EVENT_FUNCTION_H */

/* end of file */
