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
#include "atomic_shared_mutex.h"

#include <map>
#include <shared_mutex>

namespace sev {

namespace impl {

struct Exception
{
	void *Exception;
	const char *What;
	void (*Destroy)(void *exception);
	void *Rethrower;
	errno_t ErrNo;
	bool Static;
};

namespace {

AtomicSharedMutex s_ErrNoMutex;
typedef std::map<errno_t, Exception> TErrNoMap;
TErrNoMap s_ErrNo;

Exception s_BadException {
	null,
	"Failed to capture exception",
	null,
	&s_BadException,
	EOTHER,
	true,
};

}

// ERANGE - std::range_error
// EOVERFLOW - std::overflow_error
// EINVAL - std::invalid_argument
// EDOM - std::domain_error
// ENOMEM - std::bad_alloc
// EOTHER - std::exception

// EOTHER - std::bad_exception - failed to capture

}

}

SEV_ExceptionHandle SEV_Exception_capture(errno_t eno)
{
	if (!eno)
	{
		return null;
	}

	// Get existing
	{
		std::shared_lock<sev::AtomicSharedMutex> lock(sev::impl::s_ErrNoMutex);
		sev::impl::TErrNoMap::iterator it = sev::impl::s_ErrNo.find(eno);
		if (it != sev::impl::s_ErrNo.end())
		{
			return &it->second;
		}
	}

	// Write new
	{
		std::unique_lock<sev::AtomicSharedMutex> lock(sev::impl::s_ErrNoMutex);
		sev::impl::TErrNoMap::iterator it = sev::impl::s_ErrNo.find(eno);
		if (it != sev::impl::s_ErrNo.end())
		{
			return &it->second;
		}
		sev::impl::Exception e;
		memset(&e, 0, sizeof(sev::impl::Exception));
		switch (eno)
		{
		case EPERM:
			e.What = "EPERM";
			break;
		case ENOENT:
			e.What = "ENOENT";
			break;
		case ESRCH:
			e.What = "ESRCH";
			break;
		case EINTR:
			e.What = "EINTR";
			break;
		case EIO:
			e.What = "EIO";
			break;
		case ENXIO:
			e.What = "ENXIO";
			break;
		case E2BIG:
			e.What = "E2BIG";
			break;
		case ENOEXEC:
			e.What = "ENOEXEC";
			break;
		case EBADF:
			e.What = "EBADF";
			break;
		case ECHILD:
			e.What = "ECHILD";
			break;
		case EAGAIN:
			e.What = "EAGAIN";
			break;
		case ENOMEM:
			e.What = "ENOMEM";
			break;
		case EACCES:
			e.What = "EACCES";
			break;
		case EFAULT:
			e.What = "EFAULT";
			break;
		case EBUSY:
			e.What = "EBUSY";
			break;
		case EEXIST:
			e.What = "EEXIST";
			break;
		case EXDEV:
			e.What = "EXDEV";
			break;
		case ENODEV:
			e.What = "ENODEV";
			break;
		case ENOTDIR:
			e.What = "ENOTDIR";
			break;
		case EISDIR:
			e.What = "EISDIR";
			break;
		case ENFILE:
			e.What = "ENFILE";
			break;
		case EMFILE:
			e.What = "EMFILE";
			break;
		case ENOTTY:
			e.What = "ENOTTY";
			break;
		case EFBIG:
			e.What = "EFBIG";
			break;
		case ENOSPC:
			e.What = "ENOSPC";
			break;
		case ESPIPE:
			e.What = "ESPIPE";
			break;
		case EROFS:
			e.What = "EROFS";
			break;
		case EMLINK:
			e.What = "EMLINK";
			break;
		case EPIPE:
			e.What = "EPIPE";
			break;
		case EDOM:
			e.What = "EDOM";
			break;
		case EDEADLK:
			e.What = "EDEADLK";
			break;
		case ENAMETOOLONG:
			e.What = "ENAMETOOLONG";
			break;
		case ENOLCK:
			e.What = "ENOLCK";
			break;
		case ENOSYS:
			e.What = "ENOSYS";
			break;
		case ENOTEMPTY:
			e.What = "ENOTEMPTY";
			break;
		case EINVAL:
			e.What = "EINVAL";
			break;
		case ERANGE:
			e.What = "ERANGE";
			break;
		case EILSEQ:
			e.What = "EILSEQ";
			break;
		case STRUNCATE:
			e.What = "STRUNCATE";
			break;
		case EADDRINUSE:
			e.What = "EADDRINUSE";
			break;
		case EADDRNOTAVAIL:
			e.What = "EADDRNOTAVAIL";
			break;
		case EAFNOSUPPORT:
			e.What = "EAFNOSUPPORT";
			break;
		case EALREADY:
			e.What = "EALREADY";
			break;
		case EBADMSG:
			e.What = "EBADMSG";
			break;
		case ECANCELED:
			e.What = "ECANCELED";
			break;
		case ECONNABORTED:
			e.What = "ECONNABORTED";
			break;
		case ECONNREFUSED:
			e.What = "ECONNREFUSED";
			break;
		case ECONNRESET:
			e.What = "ECONNRESET";
			break;
		case EDESTADDRREQ:
			e.What = "EDESTADDRREQ";
			break;
		case EHOSTUNREACH:
			e.What = "EHOSTUNREACH";
			break;
		case EIDRM:
			e.What = "EIDRM";
			break;
		case EINPROGRESS:
			e.What = "EINPROGRESS";
			break;
		case EISCONN:
			e.What = "EISCONN";
			break;
		case ELOOP:
			e.What = "ELOOP";
			break;
		case EMSGSIZE:
			e.What = "EMSGSIZE";
			break;
		case ENETDOWN:
			e.What = "ENETDOWN";
			break;
		case ENETRESET:
			e.What = "ENETRESET";
			break;
		case ENETUNREACH:
			e.What = "ENETUNREACH";
			break;
		case ENOBUFS:
			e.What = "ENOBUFS";
			break;
		case ENODATA:
			e.What = "ENODATA";
			break;
		case ENOLINK:
			e.What = "ENOLINK";
			break;
		case ENOMSG:
			e.What = "ENOMSG";
			break;
		case ENOPROTOOPT:
			e.What = "ENOPROTOOPT";
			break;
		case ENOSR:
			e.What = "ENOSR";
			break;
		case ENOSTR:
			e.What = "ENOSTR";
			break;
		case ENOTCONN:
			e.What = "ENOTCONN";
			break;
		case ENOTRECOVERABLE:
			e.What = "ENOTRECOVERABLE";
			break;
		case ENOTSOCK:
			e.What = "ENOTSOCK";
			break;
		case ENOTSUP:
			e.What = "ENOTSUP";
			break;
		case EOPNOTSUPP:
			e.What = "EOPNOTSUPP";
			break;
		case EOTHER:
			e.What = "EOTHER";
			break;
		case EOVERFLOW:
			e.What = "EOVERFLOW";
			break;
		case EOWNERDEAD:
			e.What = "EOWNERDEAD";
			break;
		case EPROTO:
			e.What = "EPROTO";
			break;
		case EPROTONOSUPPORT:
			e.What = "EPROTONOSUPPORT";
			break;
		case EPROTOTYPE:
			e.What = "EPROTOTYPE";
			break;
		case ETIME:
			e.What = "ETIME";
			break;
		case ETIMEDOUT:
			e.What = "ETIMEDOUT";
			break;
		case ETXTBSY:
			e.What = "ETXTBSY";
			break;
		case EWOULDBLOCK:
			e.What = "EWOULDBLOCK";
			break;
		default:
			e.What = "errno_t";
			break;
		}
		e.Rethrower = &sev::impl::s_ErrNo;
		e.ErrNo = eno;
		e.Static = true;
		sev::impl::s_ErrNo[eno] = e;
		return &sev::impl::s_ErrNo[eno];
	}
}

errno_t SEV_Exception_rethrow(SEV_ExceptionHandle eh)
{
	if (!eh) return SEV_ESUCCESS;
	errno_t eno = eh->ErrNo;
	SEV_Exception_discardEx(eh);
	return eno;
}

void SEV_Exception_discardEx(SEV_ExceptionHandle eh)
{
	if (!eh) return;
	if (eh->Static) return;
	delete eh->What;
	if (eh->Destroy)
		eh->Destroy(eh->Exception);
	else
		SEV_ASSERT(!eh->Exception);
	delete eh;
}

SEV_ExceptionHandle SEV_Exception_captureEx(void *exception, const char *what, void (*destroy)(void *exception), void *rethrower, errno_t eno)
{
	sev::impl::Exception *e = new (nothrow) sev::impl::Exception();
	if (!e) return &sev::impl::s_BadException;
	e->Exception = exception;
	if (what)
	{
		size_t whatLen = strlen(what) + 1;
		char *whatStr = new (nothrow) char[whatLen];
		if (whatStr) memcpy(whatStr, what, whatLen);
		e->What = whatStr;
	}
	else
	{
		e->What = null;
	}
	e->Destroy = destroy;
	e->Rethrower = rethrower;
	e->ErrNo = eno;
	e->Static = false;
	return e;
}

void SEV_Exception_extractEx(SEV_ExceptionHandle eh, void **exception, const char **what, void (**destroy)(void *exception), void **rethrower, errno_t *eno)
{
	if (!eh)
	{
		*exception = null;
		*what = "Everything's okay, everything's alright.";
		*destroy = null;
		*rethrower = null;
		*eno = SEV_ESUCCESS;
	}
	else
	{
		*exception = eh->Exception;
		*what = eh->What;
		*destroy = eh->Destroy;
		*rethrower = eh->Rethrower;
		*eno = eh->ErrNo;
	}
}

/* end of file */
