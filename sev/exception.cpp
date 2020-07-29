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

#include "exception.h"

namespace sev {

namespace /* anonymous */ {

std::string_view copyString(const std::string_view str) noexcept
{
	if (str.empty())
		return std::string_view();
	char *buf = new (nothrow) char[str.size() + 1];
	if (!buf)
		return std::string_view();
	memcpy(buf, str.data(), str.size());
	buf[str.size()] = '\0';
	return std::string_view(buf, str.size());
}

}

Exception::Exception() noexcept : m_Delete(false)
{

}

Exception::Exception(std::string_view str) noexcept : m_What(copyString(std::string_view(str.data(), strlen(str.data())))), m_Delete(true)
{

}

Exception::Exception(std::string_view litStr, int) noexcept : m_What(litStr), m_Delete(false)
{

}

Exception::~Exception() noexcept
{
	if (m_Delete)
		delete[] m_What.Data;
}

Exception::Exception(const Exception &other) noexcept
{
	if (other.m_Delete)
	{
		m_What = copyString(other.m_What.sv());
		m_Delete = true;
	}
	else
	{
		m_What.Data = other.m_What.Data;
		m_What.Size = other.m_What.Size;
		m_Delete = false;
	}
}

Exception &Exception::operator=(Exception const &other) noexcept
{
	if (this != &other) 
	{
		this->~Exception();
		new (this) Exception(other);
	}
	return *this;
}

[[nodiscard]] char const *Exception::what() const
{
	return m_What.Data ? m_What.Data : "Unknown SEv exception";
}

}

/* end of file */
