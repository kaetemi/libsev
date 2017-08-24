/*

Copyright (C) 2017  by authors
Author: Jan Boon <jan.boon@kaetemi.be>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

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

#ifndef SEV_SHARED_SINGLETON_H
#define SEV_SHARED_SINGLETON_H

#include "config.h"
#ifdef SEV_MODULE_SINGLETON

#include "atomic_rw_lock.h"

namespace sev {

//! Purpose is to have a singleton which is reference counted, so it gets destroyed when no longer in use
template<class TClass>
class SEV_LIB SharedSingleton
{
public:
	struct Instance
	{
	public:
		inline Instance() : m_Pointer(NULL)
		{

		}

		inline Instance(Instance &other) : m_Pointer(other.m_Pointer)
		{
			s_SingletonRefCount.fetch_add(1);
		}

		Instance &operator=(const Instance &other)
		{
			if (this != &other)
			{
				if (!m_Pointer)
				{
					s_SingletonRefCount.fetch_add(1);
				}
				m_Pointer = other.m_Pointer;
				if (!m_Pointer)
				{
					s_SingletonRefCount.fetch_sub(1);
				}
			}
			return *this;
		}

		~Instance()
		{
			if (m_Pointer)
			{
				if (s_SingletonRefCount.fetch_sub(1) == 1)
				{
					s_SingletonLock.lockWrite();
					s_SingletonInstance = NULL;
					s_SingletonLock.unlockWrite();
					delete m_Pointer;
				}
			}
		}

		inline TClass *pointer() { return m_Pointer; }
		inline const TClass *pointer() const { return m_Pointer; }

		inline TClass& operator*() { return *m_Pointer; }
		inline TClass* operator->() { return m_Pointer; }

		inline const TClass& operator*() const { return *m_Pointer; }
		inline const TClass* operator->() const { return m_Pointer; }

		inline operator const TClass *() const { return m_Pointer; }
		inline operator TClass *() { return m_Pointer; }

	public: // For some reason SharedSingleton::instance doesn't have access to private
		inline Instance(TClass *pointer, int dummy) : m_Pointer(pointer)
		{

		}

	private:
		TClass *m_Pointer;

	};

	inline SharedSingleton()
	{
		
	}

	template<typename ... TArgs>
	static Instance instance(TArgs ... args)
	{
		s_SingletonRefCount.fetch_add(1);
		s_SingletonLock.lockRead();
		TClass *ptr = s_SingletonInstance;
		s_SingletonLock.unlockRead();
		if (!ptr)
		{
			s_SingletonLock.lockWrite();
			ptr = s_SingletonInstance;
			if (!ptr)
			{
				ptr = new TClass(args ...);
				s_SingletonInstance = ptr;
			}
			s_SingletonLock.unlockWrite();
		}
		return Instance(ptr, 0);
	}

private:
	static std::atomic_int s_SingletonRefCount;
	static std::atomic<TClass *> s_SingletonInstance;
	static AtomicRWLock s_SingletonLock;

}; /* class SharedSingleton */

template <typename TClass>
std::atomic_int SharedSingleton<TClass>::s_SingletonRefCount = 0;

template <typename TClass>
std::atomic<TClass *> SharedSingleton<TClass>::s_SingletonInstance = NULL;

template <typename TClass>
AtomicRWLock SharedSingleton<TClass>::s_SingletonLock;

} /* namespace sev */

#endif /* #ifdef SEV_MODULE_SINGLETON */

#endif /* SEV_SHARED_SINGLETON_H */

/* end of file */
