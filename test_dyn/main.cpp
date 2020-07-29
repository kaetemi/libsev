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
#include <sev/functor_vt.h>
#include <sev/functor_view.h>

int main()
{
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
		std::string a = "Five";
		auto func = [a]() -> void {
			std::cout << "5 = "sv << a << std::endl;
		};
		sev::FunctorView<void()> z(std::move(func));
		std::cout << "Movable: "sv << (z.movable() ? "YES"sv : "NO"sv) << std::endl;
		z();
		std::cout << "-"sv << std::endl;
		std::cout << "Should not yet be empty: "sv;
		func(); // Is not actually moved yet until toFunctor is called
		sev::Functor<void()> y = z.toFunctor(true);
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
		std::cout << "-"sv << std::endl;
	}
}
