
#ifndef SEV_CONFIG_H
#define SEV_CONFIG_H

///////////////////////////////////////////////////////////////////////
// Standard libraries
///////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#define SEV_DEPEND_MSVC_CONCURRENCY
#endif

#ifdef WIN32
#define SEV_DEPEND_WIN32
#define SEV_DEPEND_WIN32_FIBER
#define SEV_DEPEND_WIN32_OPTIONAL
#endif

#ifdef SEV_DEPEND_WIN32_OPTIONAL
#define SEV_DEPEND_WIN32_SYNCHRONIZATION_EVENT
#endif

///////////////////////////////////////////////////////////////////////
// Module selection
///////////////////////////////////////////////////////////////////////

#define SEV_MODULE_EVENTLOOP

#if defined(SEV_MODULE_EVENTLOOP) && defined(SEV_DEPEND_WIN32_FIBER)
#define SEV_MODULE_EVENTLOOP_FIBER
#endif

#ifdef SEV_MODULE_EVENTLOOP
#define SEV_MODULE_STREAM
#ifdef SEV_MODULE_EVENTLOOP_FIBER
#define SEV_MODULE_STREAM_READER_WRITER
#endif
#endif

///////////////////////////////////////////////////////////////////////
// Library export decl
///////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#define SEV_DECL_EXPORT __declspec(dllexport)
#define SEV_DECL_IMPORT __declspec(dllimport)
#else
#define SEV_DECL_EXPORT
#define SEV_DECL_IMPORT
#endif

#if defined(SEV_LIB_EXPORT)
#  define SEV_LIB SEV_DECL_EXPORT
#elif defined(SEV_LIB_STATIC)
#  define SEV_LIB
#else
#  define SEV_LIB SEV_DECL_IMPORT
#endif

#endif /* SEV_CONFIG_H */

/* end of file */
