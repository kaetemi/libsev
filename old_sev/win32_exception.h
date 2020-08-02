/*

Copyright (C) 2019-2020  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
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

throw Win32Exception(...);
catch (Win32Exception &ex) { char *what = ex.what(); ... }

Only throw exceptions in exceptional cases that shouldn't happen.
Regular error handling, and non-critical errors, should use error return values.
Exception messages are in UTF-8.
Under normal circumstances, your application should *never* throw an exception.

*/

#pragma once
#ifndef SEV_WIN32_EXCEPTION_H
#define SEV_WIN32_EXCEPTION_H

#include "platform.h"
#include "exception.h"

#ifdef __cplusplus

namespace sev {

struct SEV_LIB Win32Exception : Exception
{
public:
	using base = Exception;

	Win32Exception(const HRESULT hr, const DWORD errorCode, const StringView file, const int line) noexcept;
	inline Win32Exception(const HRESULT hr, const DWORD errorCode, const std::string_view file, const int line) noexcept : Win32Exception(hr, errorCode, StringView(file), line) { }
	virtual ~Win32Exception() noexcept;

	Win32Exception(const Win32Exception &other) noexcept;
	Win32Exception &operator=(Win32Exception const &other) noexcept;

	[[nodiscard]] virtual char const *what() const;

	inline std::string_view file() const { return m_File.sv(); };
	inline int line() const { return m_Line; };
	inline std::string_view systemMessage() const { return m_SystemMessage.sv(); };

	// Either pass hr or errorCode (and hr = S_OK)
	inline static std::string systemMessage(const HRESULT hr, const DWORD errorCode = 0)
	{
		std::string res;
		StringView str = systemMessageImpl(hr, errorCode);
		auto d = gsl::finally(std::bind(destroyStringView, str));
		res = str.sv(); // Could throw out-of-memory
		return res;
	}
	
private:
	static StringView systemMessageImpl(const HRESULT hr, DWORD errorCode);
	static void destroyStringView(StringView sv); // Can't share new and delete over boundaries

private:
	HRESULT m_HResult;
	DWORD m_ErrorCode;
	StringView m_File; // String view, but guaranteed NUL-terminated
	int m_Line;
	StringView m_SystemMessage; // String view, but guaranteed empty or NUL-terminated
	StringView m_Message; // String view, but guaranteed empty or NUL-terminated
	
};

}

#define SEV_THROW_HRESULT(hr) throw Win32Exception((hr), 0, __FILE__, __LINE__)
#define SEV_THROW_LAST_ERROR() throw Win32Exception(S_OK, GetLastError(), __FILE__, __LINE__)
#define SEV_IF_HRESULT_THROW(hr) if (hr) throw Win32Exception((hr), 0, __FILE__, __LINE__)

#endif

#endif /* #ifndef SEV_WIN32_EXCEPTION_H */

/* end of file */
