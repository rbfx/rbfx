///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_INTERNAL_CONFIG_H
#define EATHREAD_INTERNAL_CONFIG_H


#include <EABase/eabase.h>

EA_DISABLE_VC_WARNING(4574)
#include <stddef.h>
EA_RESTORE_VC_WARNING()

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_VERSION
//
// We more or less follow the conventional EA packaging approach to versioning 
// here. A primary distinction here is that minor versions are defined as two
// digit entities (e.g. .03") instead of minimal digit entities ".3"). The logic
// here is that the value is a counter and not a floating point fraction.
// Note that the major version doesn't have leading zeros.
//
// Example version strings:
//      "0.91.00"   // Major version 0, minor version 91, patch version 0. 
//      "1.00.00"   // Major version 1, minor and patch version 0.
//      "3.10.02"   // Major version 3, minor version 10, patch version 02.
//     "12.03.01"   // Major version 12, minor version 03, patch version 
//
// Example usage:
//     printf("EATHREAD_VERSION version: %s", EATHREAD_VERSION);
//     printf("EATHREAD_VERSION version: %d.%d.%d", EATHREAD_VERSION_N / 10000 % 100, EATHREAD_VERSION_N / 100 % 100, EATHREAD_VERSION_N % 100);
//
#ifndef EATHREAD_VERSION
	#define EATHREAD_VERSION   "1.32.09"
	#define EATHREAD_VERSION_N  13209

	// Older style version info
	#define EATHREAD_VERSION_MAJOR (EATHREAD_VERSION_N / 100 / 100 % 100)
	#define EATHREAD_VERSION_MINOR (EATHREAD_VERSION_N       / 100 % 100)
	#define EATHREAD_VERSION_PATCH (EATHREAD_VERSION_N             % 100)
#endif

///////////////////////////////////////////////////////////////////////////////
// _GNU_SOURCE
//
// Defined or not defined.
// If this is defined then GlibC extension functionality is enabled during 
// calls to glibc header files.
//
#if !defined(_GNU_SOURCE)
	#define _GNU_SOURCE
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_TLS_COUNT
//
// Defined as compile-time constant integer > 0.
//
#if !defined(EATHREAD_TLS_COUNT)
	#define EATHREAD_TLS_COUNT 16
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_THREADS_AVAILABLE
//
// Defined as 0 or 1
// Defines if threading is supported on the given platform.
// If 0 then the EAThread implementation is not capable of creating threads,
// but other facilities (e.g. mutex) work in a non-thread-aware way.
//
#ifndef EA_THREADS_AVAILABLE
	#define EA_THREADS_AVAILABLE 1
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_USE_CPP11_CONCURRENCY
//
// Defined as 0 or 1
//
#ifndef EA_USE_CPP11_CONCURRENCY
	#if defined(EA_PLATFORM_WINDOWS) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)  && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_APP) 
		#define EA_USE_CPP11_CONCURRENCY 1
	#else
		#define EA_USE_CPP11_CONCURRENCY 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_USE_COMMON_ATOMICINT_IMPLEMENTATION
//
// Use the common EAThread AtomicInt implementation on all platforms.
//
// Defined as 0 or 1
//
#ifndef EA_USE_COMMON_ATOMICINT_IMPLEMENTATION
	#define EA_USE_COMMON_ATOMICINT_IMPLEMENTATION 1
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_POSIX_THREADS_AVAILABLE
//
// Defined as 0 or 1
//
#ifndef EA_POSIX_THREADS_AVAILABLE
	#if defined(__unix__) || defined(__linux__) || defined(__APPLE__) 
		#define EA_POSIX_THREADS_AVAILABLE 1
	#elif defined(EA_PLATFORM_SONY)
	   #define EA_POSIX_THREADS_AVAILABLE 0  // POSIX threading API is present but use is discouraged by Sony.  They want shipping code to use their scePthreads* API.
	#else
		#define EA_POSIX_THREADS_AVAILABLE 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EAT_ASSERT_ENABLED
//
// Defined as 0 or 1, default is 1 if EA_DEBUG or _DEBUG is defined.
// If defined as 1, then assertion failures are reported via EA::Thread::AssertionFailure(). 
// 
#ifndef EAT_ASSERT_ENABLED
	#if defined(EA_DEBUG) || defined(_DEBUG)
		#define EAT_ASSERT_ENABLED 1
	#else
		#define EAT_ASSERT_ENABLED 0
	#endif
#endif



#if EAT_ASSERT_ENABLED
	#define EAT_ASSERT(expression) \
		EA_DISABLE_VC_WARNING(4127) \
		do { \
			EA_ANALYSIS_ASSUME(expression); \
			if (!(expression) ) \
				EA::Thread::AssertionFailure(__FILE__ "(" EA_STRINGIFY(__LINE__) "): " #expression); \
		} while(0) \
		EA_RESTORE_VC_WARNING()
#else
	#define EAT_ASSERT(expression)
#endif

#if EAT_ASSERT_ENABLED
	#define EAT_ASSERT_MSG(expression, msg) \
		EA_DISABLE_VC_WARNING(4127) \
		do { \
			EA_ANALYSIS_ASSUME(expression); \
			if (!(expression) ) \
				EA::Thread::AssertionFailure(msg); \
		} while(0) \
		EA_RESTORE_VC_WARNING()
#else
	#define EAT_ASSERT_MSG(expression, msg)
#endif

#if EAT_ASSERT_ENABLED
	#define EAT_ASSERT_FORMATTED(expression, pFormat, ...) \
		EA_DISABLE_VC_WARNING(4127) \
		do { \
			EA_ANALYSIS_ASSUME(expression); \
			if (!(expression) ) \
				EA::Thread::AssertionFailureV(pFormat, __VA_ARGS__); \
		} while(0) \
		EA_RESTORE_VC_WARNING()
#else
	#define EAT_ASSERT_FORMATTED(expression, pFormat, ...)
#endif

#if EAT_ASSERT_ENABLED
	#define EAT_FAIL_MSG(msg) (EA::Thread::AssertionFailure(msg))
#else
	#define EAT_FAIL_MSG(msg)
#endif

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// EAT_COMPILETIME_ASSERT   
//
// Compile-time assertion for this module.
// C-like declaration:
//    void EAT_COMPILETIME_ASSERT(bool bExpression);
//
#if !defined(EAT_COMPILETIME_ASSERT)
	#define EAT_COMPILETIME_ASSERT(expression) static_assert(expression, EA_STRINGIFY(expression))
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_TLSALLOC_DTOR_ENABLED
//
// Defined as 0 or 1. Default is 1.
// Defines if the TLSAlloc class destructor frees the TLS thread handle.
// This won't make a difference unless you were using EAThread in a DLL and 
// you were repeatedly loading and unloading DLLs.
// See eathread_pc.cpp for usage of this and more info about the situation.
//
#ifndef EATHREAD_TLSALLOC_DTOR_ENABLED
	#define EATHREAD_TLSALLOC_DTOR_ENABLED 1
#endif
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// EATHREAD_LIKELY / EATHREAD_UNLIKELY
//
// Defined as a macro which gives a hint to the compiler for branch
// prediction. GCC gives you the ability to manually give a hint to 
// the compiler about the result of a comparison, though it's often
// best to compile shipping code with profiling feedback under both
// GCC (-fprofile-arcs) and VC++ (/LTCG:PGO, etc.). However, there 
// are times when you feel very sure that a boolean expression will
// usually evaluate to either true or false and can help the compiler
// by using an explicity directive...
//
// Example usage:
//     if(EATHREAD_LIKELY(a == 0)) // Tell the compiler that a will usually equal 0.
//         { ... }
//
// Example usage:
//     if(EATHREAD_UNLIKELY(a == 0)) // Tell the compiler that a will usually not equal 0.
//         { ... }
//
#ifndef EATHREAD_LIKELY
	#define EATHREAD_LIKELY(x) EA_LIKELY(x)
	#define EATHREAD_UNLIKELY(x) EA_UNLIKELY(x)
#endif
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_NAMING
//
// Defined as 0, 1 (enabled), or 2 (enabled only when debugger is present). 
// 
#define EATHREAD_NAMING_DISABLED 0
#define EATHREAD_NAMING_ENABLED  1
#define EATHREAD_NAMING_OPTIONAL 2

#ifndef EATHREAD_NAMING
	#if defined(EA_SHIP) || defined(EA_FINAL) // These are two de-facto standard EA defines for identifying a shipping build.
		#define EATHREAD_NAMING 0
	#else
		#define EATHREAD_NAMING EATHREAD_NAMING_ENABLED // or EATHREAD_NAMING_OPTIONAL? 
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_NAME_SIZE
//
// Specifies the max size to support for naming threads.
// This value can be changed as desired.
//
#ifndef EATHREAD_NAME_SIZE
	#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_UNIX)
		#define EATHREAD_NAME_SIZE 64
	#else
		#define EATHREAD_NAME_SIZE 32
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EA_XBDM_ENABLED
//
// Defined as 0 or 1, with 1 being the default for debug builds.
// This controls whether xbdm library usage is enabled on XBox 360. This library
// allows for runtime debug functionality. But shipping applications are not
// allowed to use xbdm. 
//
#if !defined(EA_XBDM_ENABLED)
	#if defined(EA_DEBUG)
		#define EA_XBDM_ENABLED 1
	#else
		#define EA_XBDM_ENABLED 0
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EATHREAD_DLL
//
// Defined as 0 or 1. The default is dependent on the definition of EA_DLL.
// If EA_DLL is defined, then EATHREAD_DLL is 1, else EATHREAD_DLL is 0.
// EA_DLL is a define that controls DLL builds within the EAConfig build system. 
// EATHREAD_DLL controls whether EATHREAD_VERSION is built and used as a DLL. 
// Normally you wouldn't do such a thing, but there are use cases for such
// a thing, particularly in the case of embedding C++ into C# applications.
//
#ifndef EATHREAD_DLL
	#if defined(EA_DLL)
		#define EATHREAD_DLL 1
	#else
		#define EATHREAD_DLL 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREADLIB_API
//
// This is used to label functions as DLL exports under Microsoft platforms.
// If EA_DLL is defined, then the user is building EAThread as a DLL and EAThread's
// non-templated functions will be exported. EAThread template functions are not
// labelled as EATHREADLIB_API (and are thus not exported in a DLL build). This is 
// because it's not possible (or at least unsafe) to implement inline templated 
// functions in a DLL.
//
// Example usage of EATHREADLIB_API:
//    EATHREADLIB_API int someVariable = 10;         // Export someVariable in a DLL build.
//
//    struct EATHREADLIB_API SomeClass{              // Export SomeClass and its member functions in a DLL build.
//        EATHREADLIB_LOCAL void PrivateMethod();    // Not exported.
//    };
//
//    EATHREADLIB_API void SomeFunction();           // Export SomeFunction in a DLL build.
//
// For GCC, see http://gcc.gnu.org/wiki/Visibility
//
#ifndef EATHREADLIB_API // If the build file hasn't already defined this to be dllexport...
	#if EATHREAD_DLL
		#if defined(_WIN32)
			#if defined(EATHREAD_EXPORTS)
				#define EATHREADLIB_API      __declspec(dllexport)
			#else
				#define EATHREADLIB_API      __declspec(dllimport)
			#endif
			#define EATHREADLIB_LOCAL
                #elif defined(_MSC_VER)
			#define EATHREADLIB_API      __declspec(dllimport)
			#define EATHREADLIB_LOCAL
		#elif defined(__CYGWIN__)
			#define EATHREADLIB_API      __attribute__((dllimport))
			#define EATHREADLIB_LOCAL
		#elif (defined(__GNUC__) && (__GNUC__ >= 4))
			#define EATHREADLIB_API      __attribute__ ((visibility("default")))
			#define EATHREADLIB_LOCAL    __attribute__ ((visibility("hidden")))
		#else
			#define EATHREADLIB_API
			#define EATHREADLIB_LOCAL
		#endif
	#else
		#define EATHREADLIB_API
		#define EATHREADLIB_LOCAL
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_ALLOC_PREFIX
//
// Defined as a string literal. Defaults to this package's name.
// Can be overridden by the user by predefining it or by editing this file.
// This define is used as the default name used by this package for naming
// memory allocations and memory allocators.
//
// All allocations names follow the same naming pattern:
//     <package>/<module>[/<specific usage>]
// 
// Example usage:
//     void* p = pCoreAllocator->Alloc(37, EATHREAD_ALLOC_PREFIX, 0);
//
// Example usage:
//     gMessageServer.GetMessageQueue().get_allocator().set_name(EATHREAD_ALLOC_PREFIX "MessageSystem/Queue");
//
#ifndef EATHREAD_ALLOC_PREFIX
	#define EATHREAD_ALLOC_PREFIX "EAThread/"
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_USE_STANDARD_NEW
//
// Defines whether we use the basic standard operator new or the named
// extended version of operator new, as per the EASTL package.
//
#ifndef EATHREAD_USE_STANDARD_NEW
	#if EATHREAD_DLL  // A DLL must provide its own implementation of new, so we just use built-in new.
		#define EATHREAD_USE_STANDARD_NEW 1
	#else
		#define EATHREAD_USE_STANDARD_NEW 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_NEW
//
// This is merely a wrapper for operator new which can be overridden and 
// which has debug/release forms.
//
// Example usage:
//    SomeClass* pObject = EATHREAD_NEW("SomeClass") SomeClass(1, 2, 3);
//
#ifndef EATHREAD_NEW
	#if EATHREAD_USE_STANDARD_NEW
			#define EATHREAD_NEW(name)                            new
			#define EATHREAD_NEW_ALIGNED(alignment, offset, name) new
			#define EATHREAD_DELETE                               delete
	#else
		#if defined(EA_DEBUG)
			#define EATHREAD_NEW(name)                            new(name, 0, 0, __FILE__, __LINE__)
			#define EATHREAD_NEW_ALIGNED(alignment, offset, name) new(alignment, offset, name, 0, 0, __FILE__, __LINE__)
			#define EATHREAD_DELETE                               delete
		#else
			#define EATHREAD_NEW(name)                            new(name, 0, 0, 0, 0)
			#define EATHREAD_NEW_ALIGNED(alignment, offset, name) new(alignment, offset, name, 0, 0, 0, 0)
			#define EATHREAD_DELETE                               delete
		#endif
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_HAS_EMULATED_AND_NATIVE_ATOMICS
//
// This symbol is defined if a platform has both native and emulated atomics.
// Currently the only platform that requires this is iOS as earlier versions 
// of the operating system (ie: iOS 3) do not provide OS support for 64-bit
// atomics while later versions (ie: iOS 4/5) do.
#ifndef EATHREAD_HAS_EMULATED_AND_NATIVE_ATOMICS
	#if defined(__APPLE__)
		#define EATHREAD_HAS_EMULATED_AND_NATIVE_ATOMICS 1 
	#else
		#define EATHREAD_HAS_EMULATED_AND_NATIVE_ATOMICS 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_GLIBC_BACKTRACE_AVAILABLE
//
// You generally need to be using GCC, GLIBC, and Linux for backtrace to be available.
// And even then it's available only some of the time.
//
#if !defined(EATHREAD_GLIBC_BACKTRACE_AVAILABLE)
	#if (defined(__clang__) || defined(__GNUC__)) && (defined(EA_PLATFORM_LINUX) || defined(__APPLE__)) && !defined(__CYGWIN__) && !defined(EA_PLATFORM_ANDROID)
		#define EATHREAD_GLIBC_BACKTRACE_AVAILABLE 1
	#else
		#define EATHREAD_GLIBC_BACKTRACE_AVAILABLE 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_GLIBC_VERSION
//
// We provide our own GLIBC numeric version to determine when system library 
// calls are available.
//
#if defined(__GLIBC__)
	#define EATHREAD_GLIBC_VERSION ((__GLIBC__ * 1000) + __GLIBC_MINOR__) 
#endif

///////////////////////////////////////////////////////////////////////////////
// EATHREAD_GETCALLSTACK_SUPPORTED
//
// Defined as 0 or 1.
// Identifies whether runtime callstack unwinding (i.e. GetCallstack()) is 
// supported for the given platform. In some cases it may be that unwinding 
// support code is present but it hasn't been tested for reliability and may
// have bugs preventing it from working properly. In some cases (e.g. x86) 
// it may be that optimized builds make it difficult to read the callstack 
// reliably, despite that we flag the platform as supported.
//
#if !defined(EATHREAD_GETCALLSTACK_SUPPORTED)
	#if EATHREAD_GLIBC_BACKTRACE_AVAILABLE          // Typically this means Linux on x86.
		#define EATHREAD_GETCALLSTACK_SUPPORTED 1
	#elif defined(EA_PLATFORM_IPHONE)
		#define EATHREAD_GETCALLSTACK_SUPPORTED 1
	#elif defined(EA_PLATFORM_ANDROID)
		#define EATHREAD_GETCALLSTACK_SUPPORTED 1
	#elif defined(EA_PLATFORM_IPHONE_SIMULATOR)
		#define EATHREAD_GETCALLSTACK_SUPPORTED 1
	#elif defined(EA_PLATFORM_WINDOWS_PHONE) && defined(EA_PROCESSOR_ARM)       
		#define EATHREAD_GETCALLSTACK_SUPPORTED 0
	#elif defined(EA_PLATFORM_MICROSOFT)
		#define EATHREAD_GETCALLSTACK_SUPPORTED 1
	#elif defined(EA_PLATFORM_LINUX)
		#define EATHREAD_GETCALLSTACK_SUPPORTED 1
	#elif defined(EA_PLATFORM_OSX)
		#define EATHREAD_GETCALLSTACK_SUPPORTED 1
	#elif defined(EA_PLATFORM_SONY)
		#define EATHREAD_GETCALLSTACK_SUPPORTED 1
	#elif defined(EA_PLATFORM_CYGWIN)               // Support hasn't been verified.
		#define EATHREAD_GETCALLSTACK_SUPPORTED 0
	#elif defined(EA_PLATFORM_MINGW)                // Support hasn't been verified.
		#define EATHREAD_GETCALLSTACK_SUPPORTED 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_DEBUG_DETAIL_ENABLED
//
// Defined as 0 or 1. 
// If true then detailed debug info is displayed. Can be enabled in opt builds.
//
#ifndef EATHREAD_DEBUG_DETAIL_ENABLED
	#define EATHREAD_DEBUG_DETAIL_ENABLED 0
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_MIN_ABSOLUTE_TIME
//
// Defined as a time in milliseconds. 
// Locks and waits allow the user to specify an absolute timeout time. In order
// to detect that the user accidentally specified a relative time, we define a
// minimum allowed absolute time which we assert on. This minimum time is one
// that in practice is impossible to be a future absolute time.
//
#ifndef EATHREAD_MIN_ABSOLUTE_TIME
	#define EATHREAD_MIN_ABSOLUTE_TIME  10000
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED
//
// Defined as 0 or 1. 
// If true then the platform supports a user specified thread affinity mask.
//
#ifndef EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED
	#if   defined(EA_PLATFORM_XBOXONE)
		#define EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED 1
	#elif defined(EA_PLATFORM_SONY)
		#define EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED 1
	#elif defined(EA_USE_CPP11_CONCURRENCY) && EA_USE_CPP11_CONCURRENCY
		// CPP11 doesn't not provided a mechanism to set thread affinities.
		#define EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED 0
	#elif defined(EA_PLATFORM_ANDROID) || defined(EA_PLATFORM_APPLE) || defined(EA_PLATFORM_UNIX)
		#define EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED 0
	#else
		#define EATHREAD_THREAD_AFFINITY_MASK_SUPPORTED 1
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_GLOBAL_VARIABLE_DLL_SAFETY
//
// Defined as 0 or 1. 
// 
//
#ifndef EATHREAD_GLOBAL_VARIABLE_DLL_SAFETY
	#define EATHREAD_GLOBAL_VARIABLE_DLL_SAFETY 0
#endif


	
///////////////////////////////////////////////////////////////////////////////
// EATHREAD_SCEDBG_ENABLED 
//
// Defined as 0 or 1. 
// Informs EAThread if Sony Debug libraries are available for us. 
//
#ifndef EATHREAD_SCEDBG_ENABLED 
	#ifndef EA_SCEDBG_ENABLED
		#define EATHREAD_SCEDBG_ENABLED 0
	#else
		#define EATHREAD_SCEDBG_ENABLED  EA_SCEDBG_ENABLED
	#endif 
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_DEBUG_BREAK
//
#ifndef EATHREAD_DEBUG_BREAK
	#ifdef __MSC_VER
		#define EATHREAD_DEBUG_BREAK() __debugbreak()
	#else
		#define EATHREAD_DEBUG_BREAK() *(volatile int*)(0) = 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_C11_ATOMICS_AVAILABLE
//
#ifndef EATHREAD_C11_ATOMICS_AVAILABLE
	#if (defined(EA_ANDROID_SDK_LEVEL) && (EA_ANDROID_SDK_LEVEL >= 21))  
		#define EATHREAD_C11_ATOMICS_AVAILABLE 1
	#else
		#define EATHREAD_C11_ATOMICS_AVAILABLE 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_ALIGNMENT_CHECK
//

namespace EA {
namespace Thread {
namespace detail {
	// Used to assert that memory accesses on x86-64 are atomic when "naturally" aligned to the size of registers.
	template <typename T>
	inline bool IsNaturallyAligned(T* p)
	{
		return ((uintptr_t)p & (sizeof(EA_PLATFORM_WORD_SIZE) - 1)) == 0;
	}
}}}

#ifndef EATHREAD_ALIGNMENT_CHECK
	#define EATHREAD_ALIGNMENT_CHECK(address) EAT_ASSERT_MSG(EA::Thread::detail::IsNaturallyAligned(address), "address is not naturally aligned.")	
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_APPLE_GETMODULEINFO_ENABLED 
//
// This functionality has been migrated to EACallstack.  We provide a preprocessor switch for backwards compatibility
// until the code path is removed completely in a future release.
//
// Defined as 0 or 1. 
//
#ifndef EATHREAD_APPLE_GETMODULEINFO_ENABLED 
	#define EATHREAD_APPLE_GETMODULEINFO_ENABLED 0
#endif



#endif // Header include guard



