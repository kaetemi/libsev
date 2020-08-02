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

/*
Allocation count: 0
The operation completed successfully.
File: X:\source\libsev\test_dev\main.cpp, line: 73
The system cannot find the path specified.
DWORD: 0x00000003
File: X:\source\libsev\test_dev\main.cpp, line: 74
Not enough memory resources are available to complete this operation.
HRESULT: 0x8007000e
File: X:\source\libsev\test_dev\main.cpp, line: 75
Allocation count: 6
Allocation count: 0
1
2
3
4
5
.
1
2
3
.
1
2
3
4 (Exception OK)
sev::EventFlag deleted while waiting
5
.
Y
Allocation count: 3
Allocation count: 0
Allocation count: 0
Allocation count: 0
Access is denied.
The operation completed successfully.
Not implemented
The local WINS cannot be deleted.
The attribute type specified to the directory service is not defined.
The specified quick mode policy was not found.
Allocation count: 0
*/

// #include <sev/win32_exception.h>
#include <sev/event_flag.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

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
		ptrdiff_t z = s_AllocationCount; // Needs static link to work
		std::cout << "Allocation count: "sv << z << "\n"sv;
	}
#if 0
	{
		std::cout << sev::Win32Exception(0, 0, __FILE__, __LINE__).what() << "\n";
		std::cout << sev::Win32Exception(0, ERROR_PATH_NOT_FOUND, __FILE__, __LINE__).what() << "\n";
		std::cout << sev::Win32Exception(E_OUTOFMEMORY, 0, __FILE__, __LINE__).what() << "\n";

		// throw sev::Win32Exception(E_OUTOFMEMORY, 0, __FILE__, __LINE__);
	}

	{
		sev::Win32Exception a(E_OUTOFMEMORY, 0, __FILE__, __LINE__);
		sev::Win32Exception b(a);
		sev::Win32Exception c = a;
		ptrdiff_t z = s_AllocationCount; // Needs static link to work
		std::cout << "Allocation count: "sv << z << "\n"sv;
	}
#endif
	{
		ptrdiff_t z = s_AllocationCount; // Needs static link to work
		std::cout << "Allocation count: "sv << z << "\n"sv;
	}

	{
		sev::EventFlag a;
		sev::EventFlag b;

		std::thread t([&]() -> void {
			a.wait();
			std::cout << "2\n"sv;
			b.set();
			a.wait();
			std::cout << "4\n"sv;
			b.set();
		});

		std::cout << "1\n"sv;
		a.set();
		b.wait();
		std::cout << "3\n"sv;
		a.set();
		b.wait();
		std::cout << "5\n"sv;

		t.join();
		std::cout << ".\n"sv;
	}

	{
		sev::EventFlag a;
		sev::EventFlag b;
		sev::EventFlag c;

		std::thread t([&]() -> void {
			c.wait();
			b.set();
			a.wait();
			std::cout << "1\n"sv;
			c.set();
			a.wait();
		});

		a.set();
		a.reset();
		c.set();
		b.wait();
		a.set();
		c.wait();
		std::cout << "2\n"sv;
		a.set();
		std::cout << "3\n"sv;

		t.join();
		std::cout << ".\n"sv;
	}

	{
		sev::EventFlag d;
		sev::EventFlag e;
		{
			sev::EventFlag c;
			sev::EventFlag a;
			sev::EventFlag b;

			std::thread t([&]() -> void {
				c.set();
				a.wait();
				std::cout << "1\n"sv;
				c.set();
				e.set();
				});

			std::thread u([&]() -> void {
#if 1
				b.wait();
				std::cout << "3\n"sv;
				c.set();
				std::cout << "4\n"; // Not testing crash anymore
#else
				try
				{
					b.wait();
					std::cout << "3\n"sv;
					c.set();
					b.wait(); // TEST
					std::cout << "SHOULD NOT HAPPEN\n"sv;
				}
				catch (sev::Exception &ex)
				{
					std::cout << "4 (Exception OK)\n"sv;
					std::cout << ex.what() << "\n"sv;
				}
#endif
				d.set();
				});

			c.wait();
			a.set();
			a.reset();
			c.wait();
			std::cout << "2\n"sv;
			b.set();
			c.wait();

			//t.join();
			t.detach();
			u.detach();
			//u.join();
		}
		d.wait();
		e.wait();
		std::cout << "5\n"sv;
		std::cout << ".\n"sv;
	}

	{
		std::cout << "Y\n"sv;
	}
#if 0
	{
		sev::Exception a("Hello world");
		sev::Exception b(a);
		sev::Exception c = b;
		ptrdiff_t z = s_AllocationCount; // Needs static link to work
		std::cout << "Allocation count: "sv << z << "\n"sv;
	}
#endif
	{
		ptrdiff_t z = s_AllocationCount; // Needs static link to work
		std::cout << "Allocation count: "sv << z << "\n"sv;
	}

#if 0
	{
		sev::Exception a("Hello world", 1);
		sev::Exception b(a);
		sev::Exception c = b;
		ptrdiff_t z = s_AllocationCount; // Needs static link to work
		std::cout << "Allocation count: "sv << z << "\n"sv;
	}
#endif

	{
		ptrdiff_t z = s_AllocationCount; // Needs static link to work
		std::cout << "Allocation count: "sv << z << "\n"sv;
	}

#if 0
	{
		std::cout << sev::Win32Exception::systemMessage(E_ACCESSDENIED);
		std::cout << sev::Win32Exception::systemMessage(S_OK);
		std::cout << sev::Win32Exception::systemMessage(E_NOTIMPL);
		std::cout << sev::Win32Exception::systemMessage(S_OK, ERROR_CAN_NOT_DEL_LOCAL_WINS);
		std::cout << sev::Win32Exception::systemMessage(S_OK, ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED);
		std::cout << sev::Win32Exception::systemMessage(S_OK, ERROR_IPSEC_QM_POLICY_NOT_FOUND);
	}
#endif

	{
		ptrdiff_t z = s_AllocationCount; // Needs static link to work
		std::cout << "Allocation count: "sv << z << "\n"sv;
	}
}
