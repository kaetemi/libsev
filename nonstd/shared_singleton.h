/*

Copyright (C) 2017-2019  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
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

#ifndef NONSTD_SHARED_SINGLETON_H
#define NONSTD_SHARED_SINGLETON_H

// #ifndef NONSTD_SUPPRESS

#include "atomic_shared_mutex.h"

namespace nonstd {

//! Purpose is to have a singleton which is reference counted, so it gets destroyed when no longer in use
template<class TClass>
class shared_singleton
{
public:
	struct instance
	{
	public:
		inline instance() : pointer_(NULL)
		{

		}

		inline instance(const instance &other) : pointer_(other.pointer_)
		{
			s_singleton_static_.ref_count.fetch_add(1);
		}

		instance &operator=(const instance &other)
		{
			if (this != &other)
			{
				if (!pointer_)
				{
					s_singleton_static_.ref_count.fetch_add(1);
				}
				pointer_ = other.pointer_;
				if (!pointer_)
				{
					s_singleton_static_.ref_count.fetch_sub(1);
				}
			}
			return *this;
		}

		~instance()
		{
			if (pointer_)
			{
				if (s_singleton_static_.ref_count.fetch_sub(1) == 1)
				{
					bool remove;
					s_singleton_static_.mutex.lock(); // FIXME: Use unique_lock
					if (remove = (s_singleton_static_.ref_count.load() == 0))
						s_singleton_static_.instance = NULL;
					s_singleton_static_.mutex.unlock();
					if (remove)
						delete pointer_;
				}
			}
		}

		inline TClass *get() { return pointer_; }
		inline const TClass *get() const { return pointer_; }

		inline TClass& operator*() { return *pointer_; }
		inline TClass* operator->() { return pointer_; }

		inline const TClass& operator*() const { return *pointer_; }
		inline const TClass* operator->() const { return pointer_; }

		inline operator const TClass *() const { return pointer_; }
		inline operator TClass *() { return pointer_; }

	public: // For some reason shared_singleton::instance doesn't have access to private
		inline instance(TClass *pointer, int dummy) : pointer_(pointer)
		{

		}

	private:
		TClass *pointer_;

	};

	inline shared_singleton()
	{

	}

	template<typename ... TArgs>
	static instance instance(TArgs ... args)
	{
		s_singleton_static_.ref_count.fetch_add(1);
		s_singleton_static_.mutex.lock_shared(); // FIXME: Use shared_lock
		TClass *ptr = s_singleton_static_.instance;
		s_singleton_static_.mutex.unlock_shared();
		if (!ptr)
		{
			s_singleton_static_.mutex.lock(); // FIXME: Use unique_lock
			ptr = s_singleton_static_.instance;
			if (!ptr)
			{
				ptr = new TClass(args ...);
				s_singleton_static_.instance = ptr;
			}
			s_singleton_static_.mutex.unlock();
		}
		return instance(ptr, 0);
	}

private:
	class singleton_static_
	{
	public:
		singleton_static_()
		{
			ref_count = 0;
			instance = NULL;
		}

		std::atomic_int ref_count;
		std::atomic<TClass *> instance;
		atomic_shared_mutex mutex;
	};

	static singleton_static_ s_singleton_static_;

};

template <typename TClass>
typename shared_singleton<TClass>::singleton_static_ shared_singleton<TClass>::s_singleton_static_;

} /* namespace nonstd */

// #endif /* #ifndef NONSTD_SUPPRESS */

#endif /* NONSTD_SHARED_SINGLETON_H */

/* end of file */
