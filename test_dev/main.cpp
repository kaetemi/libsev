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

int main()
{
	std::cout << sev::Win32Exception(0, 0, __FILE__, __LINE__).what() << "\n";
	std::cout << sev::Win32Exception(0, ERROR_PATH_NOT_FOUND, __FILE__, __LINE__).what() << "\n";
	std::cout << sev::Win32Exception(E_OUTOFMEMORY, 0, __FILE__, __LINE__).what() << "\n";

	// throw sev::Win32Exception(E_OUTOFMEMORY, 0, __FILE__, __LINE__);

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

	std::cout << "Y\n"sv;
}
