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

#pragma once
#ifndef SEV_EXCEPTION_H
#define SEV_EXCEPTION_H

#include "platform.h"

#ifdef __cplusplus

namespace sev {

struct SEV_LIB Exception
{
public:
	struct SEV_LIB StringView
	{
	public:
		StringView() : Data(null), Size(0) { }
		inline StringView(std::string_view str) : Data(str.data()), Size(str.size()) { }
		inline std::string_view sv() const { return std::string_view(Data, Size); }
		const char *Data;
		ptrdiff_t Size;
	};

	Exception() noexcept;
	Exception(std::string_view str) noexcept;
	Exception(std::string_view litStr, int) noexcept;
	virtual ~Exception() noexcept;

	Exception(const Exception &other) noexcept;
	Exception &operator=(Exception const &other) noexcept;

	[[nodiscard]] virtual char const *what() const;

private:
	StringView m_What;
	bool m_Delete;

};

}

#endif

#endif /* #ifndef SEV_EXCEPTION_H */

/* end of file */
