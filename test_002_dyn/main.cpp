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

#include <sev/event_flag.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <sev/functor_vt.h>
#include <sev/functor_view.h>
#include <sev/concurrent_functor_queue.h>
#include <sev/atomic_shared_mutex.h>
#include <sev/event_loop.h>
#include <sev/exception.h>

/*
Local allocation count: 0
Hello world 42
Local allocation count: 0
Movable: NO
5 = Five
5 = Five
-
5 = Five
5 = Five
Local allocation count: 0
Movable: YES
5 = Five
-
5 = Five
5 = Five
Local allocation count: 0
Movable: YES
5 = Five
-
5 = Five
Movable: YES
bad function call
Exception OK!
Local allocation count: 0
Local allocation count (string): 1
Local allocation count (string and lambda): 2
Movable: YES
5 = Five
-
Should not yet be empty: 5 = Five
Local allocation count (before): 2
Local allocation count (after): 3
5 = Five
bad function call
Exception OK!
Should be empty: 5 =
Local allocation count: 3
-
Local allocation count: 0
-->
Local allocation count: 1
Added one
Local allocation count: 2
Should be empty:
Added all
Local allocation count: 131076
Called: Nice!
110 = 110
Call: OK
Local allocation count: 131075
Ref: 503300096, Test: 503300096
Local allocation count: 65539
Added more
Local allocation count: 196611
<--
Local allocation count: 0
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
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << "\n"sv;
	}
	{
		try
		{
			sev::ExceptionHandle eh;
			eh.capture<void>([]() -> void {
				throw std::invalid_argument("Exception caught successfully!");
			});
			std::cout << "Exception should follow:" << std::endl;
			eh.rethrow();
		}
		catch (const std::invalid_argument &ex)
		{
			std::cout << ex.what() << std::endl;
		}
	}
	{
		try
		{
			sev::ExceptionHandle eh;
			eh.capture(EINVAL);
			std::cout << "Exception should follow:" << std::endl;
			eh.rethrow();
		}
		catch (const std::invalid_argument &ex)
		{
			std::cout << ex.what() << std::endl;
		}
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << "\n"sv;
	}
	{
		int a = 5;
		int b = 42;
		auto func = [a]() -> void {
			std::cout << "Hello world "sv << a << std::endl;
		};
		static_assert(sizeof(b) == sizeof(func));
		sev::FunctorVt<void()>(func).invoke(&b);
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << "\n"sv;
	}
	{
		std::string a = "Five";
		auto func = [a]() -> void {
			std::cout << "5 = "sv << a << std::endl;
		};
		sev::FunctorView<void()> z(func);
		std::cout << "Movable: "sv << (z.movable() ? "YES"sv : "NO"sv) << std::endl;
		z();
		func();
		std::cout << "-"sv << std::endl;
		sev::FunctorView<void()> y(z);
		y();
		z();
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << "\n"sv;
	}
	{
		std::string a = "Five";
		auto func = [a]() -> void {
			std::cout << "5 = "sv << a << std::endl;
		};
		sev::FunctorView<void()> z(std::move(func));
		std::cout << "Movable: "sv << (z.movable() ? "YES"sv : "NO"sv) << std::endl;
		z();
		std::cout << "-"sv << std::endl;
		sev::FunctorView<void()> y(z);
		y();
		z();
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << "\n"sv;
	}
	{
		std::string a = "Five";
		auto func = [a]() -> void {
			std::cout << "5 = "sv << a << std::endl;
		};
		sev::FunctorView<void()> z(std::move(func));
		std::cout << "Movable: "sv << (z.movable() ? "YES"sv : "NO"sv) << std::endl;
		z();
		std::cout << "-"sv << std::endl;
		sev::FunctorView<void()> y(std::move(z));
		y();
		std::cout << "Movable: "sv << (y.movable() ? "YES"sv : "NO"sv) << std::endl;
		try
		{
			z(); // This one was moved out!
			std::cout << "NOT OK"sv << std::endl;
		}
		catch (std::bad_function_call &ex)
		{
			std::cout << ex.what() << std::endl;
			std::cout << "Exception OK!"sv << std::endl;
		}
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << "\n"sv;
	}
	{
		std::string a = "Five";
		{
			ptrdiff_t z = s_AllocationCount;
			std::cout << "Local allocation count (string): "sv << z << "\n"sv;
		}
		auto func = [a]() -> void {
			std::cout << "5 = "sv << a << std::endl;
		};
		{
			ptrdiff_t z = s_AllocationCount;
			std::cout << "Local allocation count (string and lambda): "sv << z << "\n"sv;
		}
		sev::FunctorView<void()> z(std::move(func));
		std::cout << "Movable: "sv << (z.movable() ? "YES"sv : "NO"sv) << std::endl;
		z();
		std::cout << "-"sv << std::endl;
		std::cout << "Should not yet be empty: "sv;
		func(); // Is not actually moved yet until toFunctor is called
		{
			ptrdiff_t z = s_AllocationCount;
			std::cout << "Local allocation count (before): "sv << z << "\n"sv; // 2
		}
		z.toFunctor(false);
		z.toFunctor(false);
		z.toFunctor(false);
		z.toFunctor(false);
		sev::Functor<void()> y = z.toFunctor(true);
		{
			ptrdiff_t z = s_AllocationCount;
			std::cout << "Local allocation count (after): "sv << z << "\n"sv; // 2
		}
		y();
		try
		{
			z(); // This one was moved out!
			std::cout << "NOT OK"sv << std::endl;
		}
		catch (std::bad_function_call &ex)
		{
			std::cout << ex.what() << std::endl;
			std::cout << "Exception OK!"sv << std::endl;
		}
		std::cout << "Should be empty: "sv;
		func(); // This will now give an empty string, since it was actually moved
		{
			ptrdiff_t z = s_AllocationCount;
			std::cout << "Local allocation count: "sv << z << "\n"sv;
		}
		std::cout << "-"sv << std::endl;
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << "\n"sv;
	}
	// return EXIT_SUCCESS;;
	{
		std::cout << "-->"sv << std::endl;
		std::string s = "Nice!";
		{
			ptrdiff_t z = s_AllocationCount;
			std::cout << "Local allocation count: "sv << z << "\n"sv;
		}
		sev::ConcurrentFunctorQueue<int(int)> q;
		q.push([s = std::move(s)](int y) -> int {
			std::cout << "Called: "sv << s << std::endl;
			return y + 10;
		});
		std::cout << "Added one"sv << std::endl;
		{
			ptrdiff_t z = s_AllocationCount;
			std::cout << "Local allocation count: "sv << z << "\n"sv;
		}
		std::cout << "Should be empty: "sv << s << std::endl;
		// TODO: call
		std::string ls = "This is a super duper very long string";
		for (int i = 0; i < (64 * 1024); ++i)
		{
			q.push([ls, i](int y) -> int {
				if (ls[0] == 'T')
					return y + i;
				return -1;
			});
		}
		std::cout << "Added all"sv << std::endl;
		{
			ptrdiff_t z = s_AllocationCount;
			std::cout << "Local allocation count: "sv << z << "\n"sv;
		}
		bool success;
		std::cout << q.tryCallAndPop(success, 100) << " = 110" << std::endl;
		std::cout << "Call: "sv << (success ? "OK"sv : "NOT OK"sv) << std::endl;
		{
			ptrdiff_t z = s_AllocationCount;
			std::cout << "Local allocation count: "sv << z << "\n"sv;
		}
		int ref = 0;
		int test = 0;
		for (int i = 0; i < (32 * 1024); ++i) // Pop half
		{
			bool success;
			ref += (-1024 + i);
			test += q.tryCallAndPop(success, -1024);
			if (!success) std::cout << "NOT OK";
		}
		std::cout << "Ref: "sv << ref << ", Test: "sv << test << std::endl;
		{
			ptrdiff_t z = s_AllocationCount;
			std::cout << "Local allocation count: "sv << z << "\n"sv;
		}
		for (int i = 0; i < (64 * 1024); ++i)
		{
			q.push([ls, i](int y) -> int {
				if (ls[0] == 'T')
					return y + i;
				return -1;
				});
		}
		std::cout << "Added more"sv << std::endl;
		{
			ptrdiff_t z = s_AllocationCount;
			std::cout << "Local allocation count: "sv << z << "\n"sv;
		}
		std::cout << "<--"sv << std::endl;
	}
	{
		ptrdiff_t z = s_AllocationCount;
		std::cout << "Local allocation count: "sv << z << "\n"sv;
	}
}

/* end of file */
