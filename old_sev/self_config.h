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

#ifndef SEV_SELF_CONFIG_H
#define SEV_SELF_CONFIG_H

///////////////////////////////////////////////////////////////////////
// Validate platform
///////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER) && (!defined(_MSVC_LANG) || _MSVC_LANG < 201703L)
static_assert(false, "C++17 is required");
#endif

///////////////////////////////////////////////////////////////////////
// Standard libraries
// Any #define can safely be disabled as needed for testing
///////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#	define SEV_DEPEND_MSVC_CONCURRENT
#endif

#ifdef WIN32
#	define SEV_DEPEND_WIN32
#	define SEV_DEPEND_WIN32_FIBER
#	define SEV_DEPEND_WIN32_OPTIONAL
#endif

#ifdef SEV_DEPEND_WIN32_OPTIONAL
#	define SEV_DEPEND_WIN32_SYNCHRONIZATION_EVENT
#endif

///////////////////////////////////////////////////////////////////////
// Module support selection
// Any #define can safely be disabled as needed for testing
///////////////////////////////////////////////////////////////////////

#define SEV_MODULE_ATOMIC_MUTEX

#define SEV_MODULE_EVENT_LOOP

#define SEV_MODULE_SINGLETON

#define SEV_MODULE_RING_BUFFER

#if defined(SEV_MODULE_EVENT_LOOP) && defined(SEV_DEPEND_WIN32_FIBER)
#	define SEV_MODULE_EVENTLOOP_FIBER
#endif

#ifdef SEV_MODULE_EVENT_LOOP
#	define SEV_MODULE_STREAM
#	ifdef SEV_MODULE_EVENTLOOP_FIBER
#		define SEV_MODULE_STREAM_READER_WRITER
#	endif
#endif

///////////////////////////////////////////////////////////////////////
// Library export decl
///////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#	define SEV_DECL_EXPORT __declspec(dllexport)
#	define SEV_DECL_IMPORT __declspec(dllimport)
#else
#	define SEV_DECL_EXPORT
#	define SEV_DECL_IMPORT
#endif

#if defined(SEV_LIB_EXPORT)
#  define SEV_LIB SEV_DECL_EXPORT
#elif defined(SEV_LIB_STATIC)
#  define SEV_LIB
#else
#  define SEV_LIB SEV_DECL_IMPORT
#endif

///////////////////////////////////////////////////////////////////////
// Magic
///////////////////////////////////////////////////////////////////////

#define null nullptr

#if defined(_DEBUG) && !defined(NDEBUG)
#	define SEV_DEBUG
#else
#	define SEV_RELEASE
#endif

#ifdef _MSC_VER
#	define SEV_FORCE_INLINE __forceinline
#else
#	define SEV_FORCE_INLINE inline __attribute__((always_inline))
#endif

#ifdef _MSC_VER
#define SEV_DEBUG_BREAK() __debugbreak()
#else
#define SEV_DEBUG_BREAK() __builtin_trap()
#endif

#if defined(_DEBUG) && !defined(NDEBUG)
#	define SEV_ASSERT(cond) do { if (!cond) SEV_DEBUG_BREAK(); } while (false)
#else
#	define SEV_ASSERT(cond) do { } while (false)
#endif

///////////////////////////////////////////////////////////////////////

#endif /* #ifndef SEV_SELF_CONFIG_H */

/* end of file */
