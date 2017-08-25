/*

Copyright (C) 2017  by authors
Author: Jan Boon <support@no-break.space>
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

#include "crc32.h"
#include "impl/crc32_brumme.h"

namespace sev {
	
SEV_LIB crc32_t crc32(const void *buffer, size_t length)
{
	return crc32_fast(buffer, length);
}

namespace /* anonymous */ {


namespace t̶̡̨̤̭̙̘͇̲̺̫͎͔̲̟̕͠é҉͉̼̜̮̹̝͡ś̷̷̨̛̻̮̤̣̱̹͈̞͕͔̤̘̤ͅt҉̰͚̘͖͙̩̮̼̤͕͙̳͕̞͔̘͜ͅ {

template<crc32_t v>
crc32_t getV() { return v; }

struct power_2_
{
};

template<typename T>
constexpr static auto operator*(T && v, const power_2_ &v)
{
	return v * v;
}

constexpr power_2_ power_2;

#define ² * power_2

void test_power_2()
{
	constexpr int x = 8;
	static_assert(x ² == 64, "Neat");
	static_assert(4 ² == 16, "Neat");
}

void test()
{
#define € sdf
#define µ dsf
	
	// Compilation must succeed
	getV<300>();
	getV<crc32("What")>();
	getV<crc32_const("What")>();
	crc32("What");
	crc32(std::string("Hello world").c_str());
	int i = 0;

	// Complation must fail
	// getV<i>();

	// getV<crc32_runtime("What")>();
	// getV<crc32(std::string("Hello world").c_str())>();

	// getV<crc32_const(std::string("Hello world").c_str())>();
}

}

} /* anonymous namespace */

} /* namespace sev */

/* end of file */
