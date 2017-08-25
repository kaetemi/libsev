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

#ifndef SEV_CRC32_H
#define SEV_CRC32_H

#include "config.h"

#include <string>
#include <cstring>

namespace sev {

typedef uint32_t crc32_t;

namespace impl {

// Author: https://gist.github.com/vivkin/8005639
// ->
constexpr unsigned int CRC32_TABLE[] = {
	0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
	0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C, 0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
};
constexpr unsigned int crc32_4(char c, unsigned int h) { return (h >> 4) ^ CRC32_TABLE[(h & 0xF) ^ c]; }
constexpr unsigned int crc32(const char *s, unsigned int h = ~0) { return !*s ? ~h : crc32(s + 1, crc32_4(*s >> 4, crc32_4(*s & 0xF, h))); }
// <-

}

SEV_LIB crc32_t crc32(const void *buffer, size_t length);

inline crc32_t crc32(const std::string &str)
{
	return crc32(str.c_str(), str.size());
}

inline crc32_t crc32_runtime(const char *str)
{
	return crc32(str, strlen(str)); // OPTIMIZE: May be better to use a modified version of the const to build-in the strlen feature
}

constexpr crc32_t crc32_const(const char * str)
{
	return impl::crc32(str);
}

template<typename T>
constexpr crc32_t crc32(T && X)
{
	return noexcept(X) ? crc32_const(X) : crc32_runtime(X);
}

} /* namespace sev */

#endif /* #ifndef SEV_CRC32_H */

/* end of file */
