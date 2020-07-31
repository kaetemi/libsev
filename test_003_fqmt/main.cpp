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

std::string s_S = "This is really a very long string that definitely won't fit inside the builtin storage"s;
std::string s_T = "This is really a very long string that also won't fit inside the builtin storage"s;
std::string s_Y = "!"s;

int main()
{
#define DO_POPS
	const int rounds = (1024 * 1024) * 8 * 4;
	const int tc = 8;
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
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
		std::cout << "Test std::queue<std::function<int(int,int)>>::push(f) single threaded with "sv << rounds << " entries and 2 strings"sv << std::endl;
		std::string s = s_S + s_Y;
		std::string t = s_T + s_Y;
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		std::queue<std::function<int(int,int)>> q;
		delta();
		for (int i = 0; i < rounds; ++i)
			q.push(f);
		ms = delta();
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << std::endl;
#ifdef DO_POPS
		std::cout << "Test pop()"sv << std::endl;
		delta();
		int i = 0;
		intptr_t ref = 0;
		intptr_t res = 0;
		while (!q.empty())
		{
			std::function<int(int, int)> &fp = q.front();
			ref += (-1024 + (intptr_t)i);
			res += fp(-1024, i);
			q.pop();
			++i;
		}
		ms = delta();
		std::cout << "Check: "sv << ref << " = "sv << res << " ("sv << i << ")"sv << std::endl;
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << std::endl;
#endif
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
#if 0
	{
		std::cout << "Test concurrency::concurrent_queue<std::function<int(int,int)>>::push(f) single threaded with "sv << rounds << " entries and 2 strings"sv << std::endl;
		std::string s = s_S + s_Y;
		std::string t = s_T + s_Y;
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		concurrency::concurrent_queue<std::function<int(int,int)>> q;
		delta();
		for (int i = 0; i < rounds; ++i)
			q.push(f);
		ms = delta();
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << "\n"sv;
		// Check committed memory consumption!!
#ifdef DO_POPS
		std::cout << "Test pop()"sv << std::endl;
		delta();
		int i = 0;
		intptr_t ref = 0;
		intptr_t res = 0;
		std::function<int(int, int)> fp;
		while (q.try_pop(fp))
		{
			ref += (-1024 + (intptr_t)i);
			res += fp(-1024, i);
			++i;
		}
		ms = delta();
		std::cout << "Check: "sv << ref << " = "sv << res << " ("sv << i << ")"sv << std::endl;
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << std::endl;
#endif
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
#if 0
	{
		std::cout << "Test sev::ConcurrentFunctorQueue<int(int,int)>::push(f) single threaded with "sv << rounds << " entries and 2 strings"sv << std::endl;
		std::string s = s_S + s_Y;
		std::string t = s_T + s_Y;
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		sev::ConcurrentFunctorQueue<int(int,int)> q;
		delta();
		for (int i = 0; i < rounds; ++i)
			q.push(f);
		ms = delta();
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << "\n"sv;
#ifdef DO_POPS
		std::cout << "Test pop()"sv << std::endl;
		delta();
		int i = 0;
		intptr_t ref = 0;
		intptr_t res = 0;
		bool success;
		for (;;)
		{
			int r = q.tryCallAndPop(success, -1024, i);
			if (!success) break;
			ref += (-1024 + (intptr_t)i);
			res += r;
			++i;
		}
		ms = delta();
		std::cout << "Check: "sv << ref << " = "sv << res << " ("sv << i << ")"sv << std::endl;
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << std::endl;
#endif
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
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
#if 0
	{
		std::cout << "Test concurrency::concurrent_queue<std::function<int(int,int)>>::push(f) single threaded with "sv << rounds << " entries plain"sv << std::endl;
		auto f = [](int x, int y) -> int {
			return x + y;
		};
		concurrency::concurrent_queue<std::function<int(int,int)>> q;
		delta();
		for (int i = 0; i < rounds; ++i)
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
		std::cout << "Test sev::ConcurrentFunctorQueue<int(int,int)>::push(f) single threaded with "sv << rounds << " entries and plain"sv << std::endl;
		auto f = [](int x, int y) -> int {
			return x + y;
		};
		sev::ConcurrentFunctorQueue<int(int,int)> q;
		delta();
		for (int i = 0; i < rounds; ++i)
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
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
#if 0
	{
		std::cout << "Test concurrency::concurrent_queue<std::function<int(int,int)>>::push(f) "sv << tc << " threaded with "sv << rounds << " entries each and 2 strings"sv << std::endl;
		std::string s = s_S + s_Y;
		std::string t = s_T + s_Y;
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		concurrency::concurrent_queue<std::function<int(int,int)>> q;
		delta();
		std::thread threads[tc];
		for (int t = 0; t < tc; ++t)
		{
			threads[t] = std::thread([&]() -> void {
				for (int i = 0; i < rounds / tc; ++i)
					q.push(f);
			});
		}
		for (int t = 0; t < tc; ++t)
		{
			threads[t].join();
		}
		ms = delta();
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << "\n"sv;
#ifdef DO_POPS
		std::cout << "Test pop()"sv << std::endl;
		delta();
		long i = 0;
		int64_t ref = 0;
		int64_t res = 0;
		for (int t = 0; t < tc; ++t)
		{
			threads[t] = std::thread([&]() -> void {
				int64_t tref = 0;
				int64_t tres = 0;
				std::function<int(int, int)> fp;
				while (q.try_pop(fp))
				{
					long j = _InterlockedIncrement(&i) - 1;
					tref += (-1024 + (intptr_t)j);
					tres += fp(-1024, j);
				}
				InterlockedAdd64(&ref, tref);
				InterlockedAdd64(&res, tres);
			});
		}
		for (int t = 0; t < tc; ++t)
		{
			threads[t].join();
		}
		ms = delta();
		std::cout << "Check: "sv << ref << " = "sv << res << " ("sv << i << ")"sv << std::endl;
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << std::endl;
#endif
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
#if 0
	{
		std::cout << "Test sev::ConcurrentFunctorQueue<std::function<int(int,int)>::push(f) "sv << tc << " threaded with "sv << rounds << " entries and 2 strings"sv << std::endl;
		std::string s = s_S + s_Y;
		std::string t = s_T + s_Y;
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		sev::ConcurrentFunctorQueue<int(int,int)> q;
		delta();
		std::thread threads[tc];
		for (int t = 0; t < tc; ++t)
		{
			threads[t] = std::thread([&]() -> void {
				for (int i = 0; i < rounds / tc; ++i)
					q.push(f);
			});
		}
		for (int t = 0; t < tc; ++t)
		{
			threads[t].join();
		}
		ms = delta();
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << "\n"sv;
#ifdef DO_POPS
#if 1
		std::cout << "Test pop()"sv << std::endl;
		delta();
		long i = 0;
		int64_t ref = 0;
		int64_t res = 0;
		for (int t = 0; t < tc; ++t)
		{
			threads[t] = std::thread([&]() -> void {
				int64_t tref = 0;
				int64_t tres = 0;
				bool success;
				for (;;)
				{
					long j = _InterlockedIncrement(&i) - 1;
					if (j >= rounds) break;
					int r = q.tryCallAndPop(success, -1024, j);
					if (!success) break;
					tref += (-1024 + (intptr_t)j);
					tres += r;
				}
				InterlockedAdd64(&ref, tref);
				InterlockedAdd64(&res, tres);
				_InterlockedDecrement(&i);
			});
		}
		for (int t = 0; t < tc; ++t)
		{
			threads[t].join();
		}
		ms = delta();
		std::cout << "Check: "sv << ref << " = "sv << res << " ("sv << i << ")"sv << std::endl;
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << std::endl;
#else
		std::cout << "Test pop()"sv << std::endl;
		delta();
		int i = 0;
		intptr_t ref = 0;
		intptr_t res = 0;
		bool success;
		for (;;)
		{
			int r = q.tryCallAndPop(success, -1024, i);
			if (!success) break;
			ref += (-1024 + (intptr_t)i);
			res += r;
			++i;
		}
		ms = delta();
		std::cout << "Check: "sv << ref << " = "sv << res << " ("sv << i << ")"sv << std::endl;
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << std::endl;
#endif
#endif
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
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
#if 1
	{
		std::cout << "Test concurrency::concurrent_queue<std::function<int(int,int)>>::push(f) and pop(f) "sv << tc << " threaded total with "sv << rounds << " entries each and 2 strings"sv << std::endl;
		std::string s = s_S + s_Y;
		std::string t = s_T + s_Y;
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		concurrency::concurrent_queue<std::function<int(int,int)>> q;
		sev::EventFlag more;
		volatile bool written = false;
		delta();
		std::thread writeThreads[(tc / 2)];
		for (int t = 0; t < (tc / 2); ++t)
		{
			writeThreads[t] = std::thread([&]() -> void {
				for (int i = 0; i < rounds / (tc / 2); ++i)
				{
					q.push(f);
					more.set();
				}
			});
		}
		std::thread readThreads[(tc / 2)];
		long i = 0;
		int64_t ref = 0;
		int64_t res = 0;
		for (int t = 0; t < (tc / 2); ++t)
		{
			readThreads[t] = std::thread([&]() -> void {
				int64_t tref = 0;
				int64_t tres = 0;
				std::function<int(int, int)> fp;
				while (!written)
				{
					while (q.try_pop(fp))
					{
						long j = _InterlockedIncrement(&i) - 1;
						tref += (-1024 + (intptr_t)j);
						tres += fp(-1024, j);
					}
					more.wait();
				}
				InterlockedAdd64(&ref, tref);
				InterlockedAdd64(&res, tres);
				more.set();
			});
		}
		for (int t = 0; t < (tc / 2); ++t)
		{
			writeThreads[t].join();
		}
		written = true;
		more.set();
		for (int t = 0; t < (tc / 2); ++t)
		{
			readThreads[t].join();
		}
		ms = delta();
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << "\n"sv;
#ifdef DO_POPS
		std::cout << "Test pop()"sv << std::endl;
		delta();
		ms = delta();
		std::cout << "Check: "sv << ref << " = "sv << res << " ("sv << i << ")"sv << std::endl;
		std::cout << "Total: "sv << ms << "ms"sv << std::endl;
		std::cout << "Local allocation count: "sv << s_AllocationCount << std::endl;
#endif
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
		std::cout << "Test sev::ConcurrentFunctorQueue<std::function<int(int,int)>::push(f) and pop(f) "sv << tc << " threaded total with "sv << rounds << " entries and 2 strings"sv << std::endl;
		std::string s = s_S + s_Y;
		std::string t = s_T + s_Y;
		auto f = [s, t](int x, int y) -> int {
			if (s[0] == 'T' && t[0] == 'T') return x + y;
			return -1;
		};
		sev::ConcurrentFunctorQueue<int(int,int)> q;
		sev::EventFlag more;
		volatile bool written = false;
		delta();
		std::thread writeThreads[(tc / 2)];
		for (int t = 0; t < (tc / 2); ++t)
		{
			writeThreads[t] = std::thread([&]() -> void {
				for (int i = 0; i < rounds / (tc / 2); ++i)
				{
					q.push(f);
					more.set();
				}
			});
		}
		std::thread readThreads[(tc / 2)];
		long i = 0;
		int64_t ref = 0;
		int64_t res = 0;
		for (int t = 0; t < (tc / 2); ++t)
		{
			readThreads[t] = std::thread([&]() -> void {
				int64_t tref = 0;
				int64_t tres = 0;
				bool success;
				long j = _InterlockedIncrement(&i) - 1;
				for (;;)
				{
					if (j >= rounds) break;
					int r = q.tryCallAndPop(success, -1024, j);
					if (!success)
					{
						if (written) break;
						else
						{
							more.wait();
							continue;
						}
					}
					tref += (-1024 + (intptr_t)j);
					tres += r;
					j = _InterlockedIncrement(&i) - 1;
				}
				InterlockedAdd64(&ref, tref);
				InterlockedAdd64(&res, tres);
				_InterlockedDecrement(&i);
				more.set();
			});
		}
		for (int t = 0; t < (tc / 2); ++t)
		{
			writeThreads[t].join();
		}
		written = true;
		more.set();
		for (int t = 0; t < (tc / 2); ++t)
		{
			readThreads[t].join();
		}
		ms = delta();
		std::cout << "Check: "sv << ref << " = "sv << res << " ("sv << i << ")"sv << std::endl;
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
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
}

/* end of file */
