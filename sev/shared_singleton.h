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

#include "atomic_shared_mutex.h"

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

		inline Instance(const Instance &other) : m_Pointer(other.m_Pointer)
		{
			s_SingletonStatic.RefCount.fetch_add(1);
		}

		Instance &operator=(const Instance &other)
		{
			if (this != &other)
			{
				if (!m_Pointer)
				{
					s_SingletonStatic.RefCount.fetch_add(1);
				}
				m_Pointer = other.m_Pointer;
				if (!m_Pointer)
				{
					s_SingletonStatic.RefCount.fetch_sub(1);
				}
			}
			return *this;
		}

		~Instance()
		{
			if (m_Pointer)
			{
				if (s_SingletonStatic.RefCount.fetch_sub(1) == 1)
				{
					bool remove;
					s_SingletonStatic.Mutex.lock();
					if (remove = (s_SingletonStatic.RefCount.load() == 0))
						s_SingletonStatic.Instance = NULL;
					s_SingletonStatic.Mutex.unlock();
					if (remove)
						delete m_Pointer;
				}
			}
		}

		inline TClass *get() { return m_Pointer; }
		inline const TClass *get() const { return m_Pointer; }

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
		s_SingletonStatic.RefCount.fetch_add(1);
		s_SingletonStatic.Mutex.lockShared();
		TClass *ptr = s_SingletonStatic.Instance;
		s_SingletonStatic.Mutex.unlockShared();
		if (!ptr)
		{
			s_SingletonStatic.Mutex.lock();
			ptr = s_SingletonStatic.Instance;
			if (!ptr)
			{
				ptr = new TClass(args ...);
				s_SingletonStatic.Instance = ptr;
			}
			s_SingletonStatic.Mutex.unlock();
		}
		return Instance(ptr, 0);
	}

private:
	class SingletonStatic
	{
	public:
		SingletonStatic()
		{
			RefCount = 0;
			Instance = NULL;
		}

		std::atomic_int RefCount;
		std::atomic<TClass *> Instance;
		AtomicSharedMutex Mutex;
	};

	static SingletonStatic s_SingletonStatic;

};

template <typename TClass>
typename SharedSingleton<TClass>::SingletonStatic SharedSingleton<TClass>::s_SingletonStatic;

} /* namespace sev */

#endif /* #ifdef SEV_MODULE_SINGLETON */

#endif /* SEV_SHARED_SINGLETON_H */

/* end of file */
