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
		case ENOENT:
			e.What = "ENOENT";
		case ESRCH:
			e.What = "ESRCH";
		case EINTR:
			e.What = "EINTR";
		case EIO:
			e.What = "EIO";
		case ENXIO:
			e.What = "ENXIO";
		case E2BIG:
			e.What = "E2BIG";
		case ENOEXEC:
			e.What = "ENOEXEC";
		case EBADF:
			e.What = "EBADF";
		case ECHILD:
			e.What = "ECHILD";
		case EAGAIN:
			e.What = "EAGAIN";
		case ENOMEM:
			e.What = "ENOMEM";
		case EACCES:
			e.What = "EACCES";
		case EFAULT:
			e.What = "EFAULT";
		case EBUSY:
			e.What = "EBUSY";
		case EEXIST:
			e.What = "EEXIST";
		case EXDEV:
			e.What = "EXDEV";
		case ENODEV:
			e.What = "ENODEV";
		case ENOTDIR:
			e.What = "ENOTDIR";
		case EISDIR:
			e.What = "EISDIR";
		case ENFILE:
			e.What = "ENFILE";
		case EMFILE:
			e.What = "EMFILE";
		case ENOTTY:
			e.What = "ENOTTY";
		case EFBIG:
			e.What = "EFBIG";
		case ENOSPC:
			e.What = "ENOSPC";
		case ESPIPE:
			e.What = "ESPIPE";
		case EROFS:
			e.What = "EROFS";
		case EMLINK:
			e.What = "EMLINK";
		case EPIPE:
			e.What = "EPIPE";
		case EDOM:
			e.What = "EDOM";
		case EDEADLK:
			e.What = "EDEADLK";
		case ENAMETOOLONG:
			e.What = "ENAMETOOLONG";
		case ENOLCK:
			e.What = "ENOLCK";
		case ENOSYS:
			e.What = "ENOSYS";
		case ENOTEMPTY:
			e.What = "ENOTEMPTY";
		case EINVAL:
			e.What = "EINVAL";
		case ERANGE:
			e.What = "ERANGE";
		case EILSEQ:
			e.What = "EILSEQ";
		case STRUNCATE:
			e.What = "STRUNCATE";
		case EADDRINUSE:
			e.What = "EADDRINUSE";
		case EADDRNOTAVAIL:
			e.What = "EADDRNOTAVAIL";
		case EAFNOSUPPORT:
			e.What = "EAFNOSUPPORT";
		case EALREADY:
			e.What = "EALREADY";
		case EBADMSG:
			e.What = "EBADMSG";
		case ECANCELED:
			e.What = "ECANCELED";
		case ECONNABORTED:
			e.What = "ECONNABORTED";
		case ECONNREFUSED:
			e.What = "ECONNREFUSED";
		case ECONNRESET:
			e.What = "ECONNRESET";
		case EDESTADDRREQ:
			e.What = "EDESTADDRREQ";
		case EHOSTUNREACH:
			e.What = "EHOSTUNREACH";
		case EIDRM:
			e.What = "EIDRM";
		case EINPROGRESS:
			e.What = "EINPROGRESS";
		case EISCONN:
			e.What = "EISCONN";
		case ELOOP:
			e.What = "ELOOP";
		case EMSGSIZE:
			e.What = "EMSGSIZE";
		case ENETDOWN:
			e.What = "ENETDOWN";
		case ENETRESET:
			e.What = "ENETRESET";
		case ENETUNREACH:
			e.What = "ENETUNREACH";
		case ENOBUFS:
			e.What = "ENOBUFS";
		case ENODATA:
			e.What = "ENODATA";
		case ENOLINK:
			e.What = "ENOLINK";
		case ENOMSG:
			e.What = "ENOMSG";
		case ENOPROTOOPT:
			e.What = "ENOPROTOOPT";
		case ENOSR:
			e.What = "ENOSR";
		case ENOSTR:
			e.What = "ENOSTR";
		case ENOTCONN:
			e.What = "ENOTCONN";
		case ENOTRECOVERABLE:
			e.What = "ENOTRECOVERABLE";
		case ENOTSOCK:
			e.What = "ENOTSOCK";
		case ENOTSUP:
			e.What = "ENOTSUP";
		case EOPNOTSUPP:
			e.What = "EOPNOTSUPP";
		case EOTHER:
			e.What = "EOTHER";
		case EOVERFLOW:
			e.What = "EOVERFLOW";
		case EOWNERDEAD:
			e.What = "EOWNERDEAD";
		case EPROTO:
			e.What = "EPROTO";
		case EPROTONOSUPPORT:
			e.What = "EPROTONOSUPPORT";
		case EPROTOTYPE:
			e.What = "EPROTOTYPE";
		case ETIME:
			e.What = "ETIME";
		case ETIMEDOUT:
			e.What = "ETIMEDOUT";
		case ETXTBSY:
			e.What = "ETXTBSY";
		case EWOULDBLOCK:
			e.What = "EWOULDBLOCK";
		}
		e.Rethrower = &sev::impl::s_ErrNo;
		e.ErrNo = eno;
		e.Static = true;
		sev::impl::s_ErrNo[eno] = e;
		return &e;
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
	size_t whatLen = strlen(what) + 1;
	char *whatStr = new (nothrow) char[whatLen];
	if (whatStr) memcpy(whatStr, what, whatLen);
	e->What = whatStr;
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
