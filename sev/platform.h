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

Polyverse OÜ base include file for C++.

*/

#pragma once
#ifndef SEV_PLATFORM_H
#define SEV_PLATFORM_H

// Use C math defines for M_PI
#define _USE_MATH_DEFINES

#ifdef _WIN32
// Include Win32.
// Ensure malloc is included before anything else,
// There are some odd macros that may conflict otherwise.
// Include STL algorithm to ensure std::min and std::max
// are used everywhere, instead of min and max macros.
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0600
#ifdef __cplusplus
#define NOMINMAX
#endif /* __cplusplus */
#include <malloc.h>
#ifdef __cplusplus
#include <algorithm>
using std::max;
using std::min;
#endif /* __cplusplus */
#include <Windows.h>
#endif /* _WIN32 */

// C++
#ifdef __cplusplus

// Require C++17
#if defined(_MSC_VER) && (!defined(_HAS_CXX17) || !_HAS_CXX17)
static_assert(false, "C++17 is required");
#endif

// Define null, with color highlight
constexpr decltype(nullptr) null = nullptr;
#define null null

// Include STL string and allow string literals.
// Always use sv suffix when declaring string literals.
// Ideally, assign them as `constexpr std::string_view`.
#include <string>
#include <string_view>
using namespace std::string_literals;
using namespace std::string_view_literals;

// A couple of types for the global namespace, for everyone's sanity.
#if defined(_HAS_CXX20) && _HAS_CXX20
#include <compare>
using std::strong_ordering;
#endif
using std::move;
using std::nothrow;
using std::forward;

// Include GSL
// auto _ = gsl::finally([&] { delete xyz; });
#include "gsl/gsl_util"

// The usual
#include <functional>

// Force inline
#ifdef _MSC_VER
#	define SEV_FORCE_INLINE __forceinline
#else
#	define SEV_FORCE_INLINE inline __attribute__((always_inline))
#endif

// Include debug_break
#include <debugbreak.h>
#define SEV_DEBUG_BREAK() debug_break()

#if defined(_DEBUG) && !defined(NDEBUG)
#define SEV_DEBUG
#else
#define SEV_RELEASE
#endif

// Assert
#ifdef SEV_DEBUG
#	define SEV_ASSERT(cond) do { if (!(cond)) SEV_DEBUG_BREAK(); } while (false)
#else
#	define SEV_ASSERT(cond) do { } while (false)
#endif

// Library export decl
#ifdef _MSC_VER
#	define SEV_DECL_EXPORT __declspec(dllexport)
#	define SEV_DECL_IMPORT __declspec(dllimport)
#else
#	define SEV_DECL_EXPORT
#	define SEV_DECL_IMPORT
#endif

#if defined(SEV_LIB_EXPORT)
#ifdef _MSC_VER
// #pragma warning(disable: 4577)
// #pragma warning(disable: 4251)
#endif
#  define SEV_LIB SEV_DECL_EXPORT
#elif defined(SEV_LIB_STATIC)
#  define SEV_LIB
#else
#  define SEV_LIB SEV_DECL_IMPORT
#endif

#endif /* __cplusplus */

#endif /* SEV_PLATFORM_H */

/* end of file */
