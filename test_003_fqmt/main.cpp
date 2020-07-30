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

#include <sev/win32_exception.h>
#include <sev/event_flag.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <queue>
#include <concurrent_queue.h>
#include <sev/functor_vt.h>
#include <sev/functor_view.h>
#include <sev/concurrent_functor_queue.h>

/*
*/

static std::atomic_ptrdiff_t s_AllocationCount;

// Override global C++ allocation for debug purpose
void *operator new(size_t sz)
{
	// printf("-[[[Allocate %zu bytes]]]-", sz);
	s_AllocationCount += 1;
	void *ptr = _aligned_malloc(sz, 32);
	if (ptr)
		return ptr;
	else
		throw std::bad_alloc{};
}

void* operator new(size_t sz, const std::nothrow_t& tag) noexcept
{
	// printf("-[[[Allocate %zu bytes]]]-", sz);
	s_AllocationCount += 1;
	return _aligned_malloc(sz, 32);
}

void operator delete(void *ptr) noexcept
{
	// printf("-[[[Free pointer]]]-");
	_aligned_free(ptr);
	s_AllocationCount -= 1;
}

int main()
{
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << std::endl << std::endl;
	}
	std::chrono::time_point t0 = std::chrono::steady_clock::now();
	int64_t ms;
	auto delta = [&]() -> int64_t {
		std::chrono::time_point t1 = std::chrono::steady_clock::now();
		int64_t res = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
		t0 = t1;
		return res;
	};
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
#if 0
	{
		std::cout << "Test std::queue<std::function<int(int,int)>>::push(f) single threaded with 1048576 entries and 2 strings"sv << std::endl;
		std::string s = "This is really a very long string that definitely won't fit inside the builtin storage";
		std::string t = "This is really a very long string that also won't fit inside the builtin storage";
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		std::queue<std::function<int(int,int)>> q;
		delta();
		for (int i = 0; i < (1024 * 1024); ++i)
			q.push(f);
		ms = delta();
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << "\n"sv;
		delta();
	}
	{
		ms = delta();
		std::cout << "Deallocation: "sv << ms << "ms"sv << std::endl;
		delta();
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << std::endl << std::endl;
	}
#endif
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	{
		std::cout << "Test concurrency::concurrent_queue<std::function<int(int,int)>>::push(f) single threaded with 1048576 entries and 2 strings"sv << std::endl;
		std::string s = "This is really a very long string that definitely won't fit inside the builtin storage";
		std::string t = "This is really a very long string that also won't fit inside the builtin storage";
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		concurrency::concurrent_queue<std::function<int(int,int)>> q;
		delta();
		for (int i = 0; i < (1024 * 1024); ++i)
			q.push(f);
		ms = delta();
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << "\n"sv;
		delta();
	}
	{
		ms = delta();
		std::cout << "Deallocation: "sv << ms << "ms"sv << std::endl;
		delta();
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << std::endl << std::endl;
	}
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	{
		std::cout << "Test sev::ConcurrentFunctorQueue<int(int,int)>::push(f) single threaded with 1048576 entries and 2 strings"sv << std::endl;
		std::string s = "This is really a very long string that definitely won't fit inside the builtin storage";
		std::string t = "This is really a very long string that also won't fit inside the builtin storage";
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		sev::ConcurrentFunctorQueue<int(int,int)> q;
		delta();
		for (int i = 0; i < (1024 * 1024); ++i)
			q.push(f);
		ms = delta();
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << "\n"sv;
		delta();
	}
	{
		ms = delta();
		std::cout << "Deallocation: "sv << ms << "ms"sv << std::endl;
		delta();
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << std::endl << std::endl;
	}
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
#if 0
	{
		std::cout << "Test concurrency::concurrent_queue<std::function<int(int,int)>>::push(f) 6 threaded with 1048576 entries each and 2 strings"sv << std::endl;
		std::string s = "This is really a very long string that definitely won't fit inside the builtin storage";
		std::string t = "This is really a very long string that also won't fit inside the builtin storage";
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		concurrency::concurrent_queue<std::function<int(int,int)>> q;
		delta();
		std::thread threads[6];
		for (int t = 0; t < 6; ++t)
		{
			threads[t] = std::thread([&]() -> void {
				for (int i = 0; i < (1024 * 1024); ++i)
					q.push(f);
			});
		}
		for (int t = 0; t < 6; ++t)
		{
			threads[t].join();
		}
		ms = delta();
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << "\n"sv;
		delta();
	}
	{
		ms = delta();
		std::cout << "Deallocation: "sv << ms << "ms"sv << std::endl;
		delta();
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << std::endl << std::endl;
	}
#endif
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
#if 1
	{
		std::cout << "Test sev::ConcurrentFunctorQueue<std::function<int(int,int)>::push(f) 6 threaded with 1048576 entries each and 2 strings"sv << std::endl;
		std::string s = "This is really a very long string that definitely won't fit inside the builtin storage";
		std::string t = "This is really a very long string that also won't fit inside the builtin storage";
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		sev::ConcurrentFunctorQueue<int(int,int)> q;
		delta();
		std::thread threads[6];
		for (int t = 0; t < 6; ++t)
		{
			threads[t] = std::thread([&]() -> void {
				for (int i = 0; i < (1024 * 1024); ++i)
					q.push(f);
				});
		}
		for (int t = 0; t < 6; ++t)
		{
			threads[t].join();
		}
		ms = delta();
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << "\n"sv;
		delta();
	}
	{
		ms = delta();
		std::cout << "Deallocation: "sv << ms << "ms"sv << std::endl;
		delta();
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << std::endl << std::endl;
	}
#endif
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
}

/* end of file */
