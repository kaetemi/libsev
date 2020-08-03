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

/*

Exception magic to transcend C library boundaries, and compiler differences!

*/

#ifdef __cplusplus
#include <stdexcept>
namespace sev::impl {
struct Exception;
}
extern "C" {
typedef sev::impl::Exception *SEV_ExceptionHandle; // We pass just a pointer!
#else
typedef void *SEV_ExceptionHandler;
#endif

#define SEV_ESUCCESS 0 // Valid as both errno_t and SEV_ExceptionHandle. For clarity. Guaranteed 0, null or NULL.

SEV_LIB SEV_ExceptionHandle SEV_Exception_capture(errno_t eno); // Captures a C error into an exception handle
SEV_LIB errno_t SEV_Exception_rethrow(SEV_ExceptionHandle eh); // Rethrows the exception as a C error result, releases the internal exception state. Handle is no longer valid after this!

SEV_LIB SEV_ExceptionHandle SEV_Exception_captureEx(void *exception, const char *what, void (*destroy)(void *exception), void *rethrower, errno_t eno); // Creates a new exception handle from a captured exception.
SEV_LIB void SEV_Exception_extractEx(SEV_ExceptionHandle eh, void **exception, const char** what, void (**destroy)(void *exception), void **rethrower, errno_t *eno); // Extracts data from exception. Call discardEx manually when done with the data.
SEV_LIB void SEV_Exception_discardEx(SEV_ExceptionHandle eh); // Releases internal exception state without handling it. Handle is no longer valid after this! No-op when calling with null, 0, or SEV_ESUCCESS.

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef __cplusplus

// ERANGE - std::range_error
// EOVERFLOW - std::overflow_error
// EINVAL - std::invalid_argument
// EDOM - std::domain_error
// ENOMEM - std::bad_alloc
// EOTHER - std::exception

// EOTHER - std::bad_exception - failed to capture

namespace sev {

namespace impl {

constexpr void *rethrower() { return __ExceptionPtrRethrow; }

// Implemented as a template to ensure it gets compiled and linked into the local library or application.
template<typename TExceptionHandle>
class ExceptionHandle
{
public:
	SEV_FORCE_INLINE ExceptionHandle(SEV_ExceptionHandle e) : eh(e)
	{
		// no-op
	}

	SEV_FORCE_INLINE ExceptionHandle() noexcept : eh(SEV_ESUCCESS)
	{
		// no-op
	}

	SEV_FORCE_INLINE ~ExceptionHandle() noexcept
	{
		if (eh)
		{
			SEV_DEBUG_BREAK(); // Captured exceptions must be rethrown!
			rethrow();
		}
	}

	SEV_FORCE_INLINE bool raised() noexcept
	{
		return eh;
	}

	void rethrow()
	{
		if (!eh) return;

		void *exception;
		const char *what;
		void (*destroy)(void *exception);
		void *rethrower;
		errno_t eno;
		SEV_Exception_extractEx(eh, &exception, &what, &exception, &rethrower, &eno);
		gsl::finally([this]() -> { discard(); }); // Release when throwing!
		if (rethrower == impl::rethrower())
		{
			// Same standard library, we can rethrow this as-is!
			std::exception_ptr ep = *(std::exception_ptr *)exception;
			std::rethrow_exception(ep);
		}
		else
		{
			switch (eno)
			{
			case ERANGE:
				throw std::range_error(what ? what : "Failed to capture exception message");
				break;
			case EOVERFLOW:
				throw std::overflow_error(what ? what : "Failed to capture exception message");
				break;
			case EINVAL:
				throw std::invalid_argument(what ? what : "Failed to capture exception message");
				break;
			case EDOM:
				throw std::domain_error(what ? what : "Failed to capture exception message");
				break;
			case ENOMEM:
				throw std::bad_alloc(what ? what : "Failed to capture exception message");
				break;
			default: // case EOTHER:
				throw std::exception(what ? what : "Failed to capture exception message");
				break;
			}
		}
	}

	SEV_FORCE_INLINE void discard() noexcept
	{
		SEV_Exception_discardEx(eh);
		eh = SEV_ESUCCESS;
	}

	template<typename TRes, typename TFn = TRes()>
	TRes capture(TFn &&f) noexcept
	{
		try
		{
			return f();
		}
		catch (std::range_error &ex)
		{
			p_capture(std::current_exception(), ex.what(), ERANGE);
		}
		catch (std::overflow_error &ex)
		{
			p_capture(std::current_exception(), ex.what(), EOVERFLOW);
		}
		catch (std::invalid_argument &ex)
		{
			p_capture(std::current_exception(), ex.what(), EOVERFLOW);
		}
		catch (std::domain_error &ex)
		{
			p_capture(std::current_exception(), ex.what(), EDOM);
		}
		catch (std::bad_alloc &ex)
		{
			p_capture(std::current_exception(), ex.what(), ENOMEM);
		}
		catch (std::exception &ex)
		{
			p_capture(std::current_exception(), ex.what(), EOTHER);
		}
		catch (...)
		{
			p_capture(std::current_exception(), "Unknown exception", EOTHER);
		}
		return TRes();
	}

	SEV_FORCE_INLINE void capture(errno_t eno) noexcept
	{
		if (eh)
		{
			SEV_DEBUG_BREAK(); // Captured exceptions must be rethrown!
			discard();
		}
		eh = SEV_Exception_capture(eno);
	}

private:
	void p_capture(std::exception_ptr ptr, const char *what, errno_t eno)
	{
		void *exception = new (nothrow) std::exception_ptr(ptr);
		if (!exception) // Unable to allocate
		{
			eh = SEV_Exception_capture(eno); // Get a generic if possible
		}
		auto destroy = [](void *exception) {
			delete exception;
		};
		eh = SEV_Exception_captureEx(exception, what, destroy, impl::rethrower(), eno);
	}

	TExceptionHandle eh;

};

}

using ExceptionHandle = impl::ExceptionHandle<SEV_ExceptionHandle>;

}

#endif /* __cplusplus */

#endif /* #ifndef SEV_EXCEPTION_HANDLE_H */

/* end of file */
