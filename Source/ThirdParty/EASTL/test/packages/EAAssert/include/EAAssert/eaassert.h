///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

#ifndef EAASSERT_EAASSERT_H
#define EAASSERT_EAASSERT_H

// Users can #define EA_ASSERT_HAVE_OWN_HEADER to their own header file and provide their
// own definition of EA_ASSERT. Make sure to include quotes in your 
// definition, i.e. #define EA_ASSERT_HAVE_OWN_HEADER "my_game/my_game_assert.h"
//
// At minimum, this header needs to provide the following:
//
//      EA_ASSERT(expr)
//      EA_ASSERT_MSG(expr, msg)
//      EA_ASSERT_FORMATTED(expr, fmt)
//      EA_FAIL()
//      EA_FAIL_MSG(msg)
//      EA_FAIL_FORMATTED(fmt)
//      EA_COMPILETIME_ASSERT(expr)     Note: EABase (2.0.21 and later) has static_assert which should now be used in place of this.
//      EA_PANIC(expr)
//      EA_PANIC_MSG(expr, msg)
//      EA_PANIC_FORMATTED(expr, fmt)
//
//  Where:
//  
//      expr    is an expression that evaluates to a boolean
//      msg     is a string (char*)
//      fmt     is a tuple wrapped in parentheses where the first message is 
//              a formatted string (char*) and the rest is printf style parameters
//
#ifdef EA_ASSERT_HAVE_OWN_HEADER
	#include EA_ASSERT_HAVE_OWN_HEADER

#else // The user did not supply their own assert definition, so we'll use ours instead

#include "EABase/eabase.h"
#include <stdarg.h>


// The default assert handling mechanism just breaks into the debugger on an assert failure. If you wish to overwrite this 
// functionality, simply implement the following callback:
//
//      bool EAAssertFailure(
//              const char* expr, 
//              const char* filename,
//              int line,
//              const char* function,
//              const char* msg,
//              va_list args)
//
// And then use:
//
//      EA::Assert::SetCallback(&yourCallback);
//

#if defined(EA_DLL) && defined(_MSC_VER)
	#ifndef EA_ASSERT_API
		#define EA_ASSERT_API __declspec(dllimport)
	#endif
#else
	#define EA_ASSERT_API
#endif

#ifdef __cplusplus
namespace EA { 
namespace Assert {

	typedef bool (*FailureCallback)(const char*, const char*, int, const char*, const char*, va_list);

	EA_ASSERT_API   FailureCallback GetFailureCallback();
	EA_ASSERT_API   void            SetFailureCallback(FailureCallback failureCallback);

namespace Detail {
	EA_ASSERT_API   bool            VCall(const char *expr, const char *filename, int line, const char *function, const char *msg, ...);
	EA_ASSERT_API   bool            Call(const char *expr, const char *filename, int line, const char *function, const char *msg);
	EA_ASSERT_API   bool            Call(const char *expr, const char *filename, int line, const char *function);
}}}
#endif



///////////////////////////////////////////////////////////////////////////////
/// EA_DEBUG_BREAK
///
/// This function causes an app to immediately stop under the debugger.
/// It is implemented as a macro in order to all stopping at the site 
/// of the call.
///
/// Example usage:
///    EA_DEBUG_BREAK();
///
/// The EA_DEBUG_BREAK function must be implemented on a per-platform basis. 
/// On a PC, you would normally define this to function to be the inline
/// assembly: "asm int 3", which tells the debugger to stop here immediately.
/// A very basic platform-independent implementation of EA_DEBUG_BREAK could 
/// be the following:
///   void EA_DEBUG_BREAK()
///   {
///      atoi(""); // Place a breakpoint here if you want to catch breaks.
///   }
/// 
/// The EA_DEBUG_BREAK default behaviour here can be disabled or changed by 
/// globally defining EA_DEBUG_BREAK_DEFINED and implementing an alternative
/// implementation of it. Our implementation here doesn't simply always have
/// it be defined externally because a major convenience of EA_DEBUG_BREAK
/// being inline is that it stops right on the troublesome line of code and
/// not in another function.
///
#if ! defined EA_DEBUG_BREAK && ! defined EA_DEBUG_BREAK_DEFINED
	#if   defined(EA_PLATFORM_SONY) && defined(EA_PROCESSOR_X86_64)
		#define EA_DEBUG_BREAK() do { { __asm volatile ("int $0x41"); } } while(0)
	#elif defined _MSC_VER
		#define EA_DEBUG_BREAK() __debugbreak()
	#elif EA_COMPILER_HAS_BUILTIN(__builtin_debugtrap)
		#define EA_DEBUG_BREAK() __builtin_debugtrap()
	#elif (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64)) && defined(__GNUC__)
		// Using __asm__ here instead of asm is more compatible with clang when compiling in c99 mode.
		#define EA_DEBUG_BREAK() __asm__("int3")
	#elif defined(EA_PROCESSOR_ARM64) && defined(__GNUC__)
		#define EA_DEBUG_BREAK() asm("brk 10")
	#elif defined(EA_PROCESSOR_ARM) && defined(__APPLE__)
		#include <signal.h>
		#include <unistd.h>
		#define EA_DEBUG_BREAK() kill( getpid(), SIGINT )
	#elif defined(EA_PROCESSOR_ARM) && defined(__GNUC__)
		#define EA_DEBUG_BREAK() asm("BKPT 10")     // The 10 is arbitrary. It's just a unique id.
	#elif defined(EA_PROCESSOR_ARM) && defined(__ARMCC_VERSION)
		#define EA_DEBUG_BREAK() __breakpoint(10)
	#else
		#define EA_DEBUG_BREAK() *(int*)(0) = 0
	#endif

	#define EA_DEBUG_BREAK_DEFINED
#endif



///////////////////////////////////////////////////////////////////////////////
/// EA_CRASH
///
/// Executes an invalid memory write, which should result in an exception 
/// on most platforms.
///
#if !defined(EA_CRASH) && !defined(EA_CRASH_DEFINED)
	#define EA_CRASH() *(volatile int*)(0) = 0

	#define EA_CRASH_DEFINED
#endif



///////////////////////////////////////////////////////////////////////////////
/// EA_CURRENT_FUNCTION
///
/// Provides a consistent way to get the current function name as a macro
/// like the __FILE__ and __LINE__ macros work. The C99 standard specifies
/// that __func__ be provided by the compiler, but most compilers don't yet
/// follow that convention. However, many compilers have an alternative.
///
/// We also define EA_CURRENT_FUNCTION_SUPPORTED for when it is not possible
/// to have EA_CURRENT_FUNCTION work as expected.
///
/// Defined inside a function because otherwise the macro might not be 
/// defined and code below might not compile. This happens with some 
/// compilers.
///
#ifndef EA_CURRENT_FUNCTION
	#if defined __GNUC__ || (defined __ICC && __ICC >= 600)
		#define EA_CURRENT_FUNCTION __PRETTY_FUNCTION__
	#elif defined(__FUNCSIG__)
		#define EA_CURRENT_FUNCTION __FUNCSIG__
	#elif (defined __INTEL_COMPILER && __INTEL_COMPILER >= 600) || (defined __IBMCPP__ && __IBMCPP__ >= 500) || (defined CS_UNDEFINED_STRING && CS_UNDEFINED_STRING >= 0x4200)
		#define EA_CURRENT_FUNCTION __FUNCTION__
	#elif defined __BORLANDC__ && __BORLANDC__ >= 0x550
		#define EA_CURRENT_FUNCTION __FUNC__
	#elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901
		#define EA_CURRENT_FUNCTION __func__
	#else
		#define EA_CURRENT_FUNCTION "(unknown function)"
	#endif
#endif





#ifdef __cplusplus
namespace EA { 
namespace Assert { 
namespace Detail 
{
	// The Forwarder class is deprecated on compilers that support variadic macros. Avoiding
	// the temporary instances created by this code can result in significant code size savings.
	// Code outside of the EAAssert library shouldn't be referencing API in the Detail namespace.
	#ifdef EA_COMPILER_NO_VARIADIC_MACROS
		#define EA_ASSERT_PREFIX_DEPRECATED
		#define EA_ASSERT_POSTFIX_DEPRECATED
	#else
		#define EA_ASSERT_PREFIX_DEPRECATED EA_PREFIX_DEPRECATED
		#define EA_ASSERT_POSTFIX_DEPRECATED EA_POSTFIX_DEPRECATED
	#endif

	struct Forwarder
	{ 
		const char* mExpr;
		const char* mFunction;
		const char* mFilename;
		int mLine;
		Forwarder(const char* expr, const char* filename, int line,  const char* function) : mExpr(expr), mFunction(function), mFilename(filename), mLine(line) {}
		EA_ASSERT_PREFIX_DEPRECATED bool Call(const char* msg, ...) const EA_ASSERT_POSTFIX_DEPRECATED
		{
			va_list args;
			va_start(args, msg);
			bool ret = (*GetFailureCallback())(mExpr, mFilename, mLine, mFunction, msg, args);
			va_end(args);
			return ret;
		}
	};

	#ifdef EA_COMPILER_NO_VARIADIC_MACROS
		#define EA_ASSERT_FORMATTED_HANDLER_CALL(EXPR, FILE, LINE, FUNCTION, FMT) EA::Assert::Detail::Forwarder(EXPR, FILE, LINE, FUNCTION).Call FMT
	#else
		#define EA_ASSERT_FORMATTED_HANDLER_CALL(EXPR, FILE, LINE, FUNCTION, FMT) EA::Assert::Detail::VCall(EXPR, FILE, LINE, FUNCTION, EA_ASSERT_EXPAND_VA_ARGS FMT)
		#define EA_ASSERT_EXPAND_VA_ARGS(...) __VA_ARGS__
	#endif

}}}
#endif


///////////////////////////////////////////////////////////////////////////////
/// EA_COMPILETIME_ASSERT
///
/// EA_COMPILETIME_ASSERT is a macro for compile time assertion checks, useful for 
/// validating *constant* expressions. The advantage over using the ASSERT, 
/// VERIFY, etc. macros is that errors are caught at compile time instead 
/// of runtime.
///
/// This EA_COMPILETIME_ASSERT has a weakness in that when used at global scope (outside 
/// functions) there can be two such statements on the same line of a given 
/// file and some compilers might complain about this. 
///
/// Example: 
///    EA_COMPILETIME_ASSERT(sizeof(int) == 4);
///
#ifndef EA_COMPILETIME_ASSERT
	#if defined(EA_COMPILER_EDG) || defined(EA_COMPILER_CLANG)
		// Ideally static_assert would be used wherever possible because it allows for improved error reporting.
		// However, using static_assert with Clang currently breaks code using EAOffsetOf.  We need to fix this
		// macro on clang before enabling the use of static_assert.
		#define EAASSERT_TOKEN_PASTE(a,b) a ## b
		#define EAASSERT_CONCATENATE_HELPER(a,b) EAASSERT_TOKEN_PASTE(a,b)
		#define EA_COMPILETIME_ASSERT(expr) \
			EA_DISABLE_CLANG_WARNING(-Wunknown-pragmas); /* Disable warnings about unknown pragmas in case -Wunused-local-typedefs is not supported*/ \
			EA_DISABLE_CLANG_WARNING(-Wunused-local-typedef); \
			typedef char EAASSERT_CONCATENATE_HELPER(compileTimeAssert,__LINE__)  [((expr) != 0) ? 1 : -1]; \
			EA_RESTORE_CLANG_WARNING() \
			EA_RESTORE_CLANG_WARNING()
	#else
		#define EA_COMPILETIME_ASSERT(expr) static_assert(expr, EA_STRINGIFY(expr))
	#endif
#endif

#ifndef EA_CT_ASSERT
	#define EA_CT_ASSERT EA_COMPILETIME_ASSERT
#endif



#ifdef EA_DEBUG
	#ifndef EA_ASSERT_ENABLED
		#define EA_ASSERT_ENABLED 1
	#endif
	#ifndef EA_PANIC_ENABLED
		#define EA_PANIC_ENABLED 1
	#endif
#endif

#ifdef EA_ASSERT_ENABLED    
	#ifdef __cplusplus
		#ifndef EA_ASSERT
			#define EA_ASSERT(expr) \
				EA_DISABLE_VC_WARNING(4127) \
				do { \
					EA_ANALYSIS_ASSUME(expr); \
					if (!(expr) && EA::Assert::Detail::Call(#expr, __FILE__, __LINE__, EA_CURRENT_FUNCTION)) \
						EA_DEBUG_BREAK(); \
					else \
						((void)0);\
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
		#ifndef EA_ASSERT_MSG
			#define EA_ASSERT_MSG(expr, msg) \
				EA_DISABLE_VC_WARNING(4127) \
				do { \
					EA_ANALYSIS_ASSUME(expr); \
					if (!(expr) && EA::Assert::Detail::Call(#expr, __FILE__, __LINE__, EA_CURRENT_FUNCTION, msg)) \
						EA_DEBUG_BREAK(); \
					else \
						((void)0);\
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
		#ifndef EA_ASSERT_FORMATTED
			#define EA_ASSERT_FORMATTED(expr, fmt) if (!(expr) && EA_ASSERT_FORMATTED_HANDLER_CALL(#expr, __FILE__, __LINE__, EA_CURRENT_FUNCTION, fmt))       EA_DEBUG_BREAK(); else ((void)0)
		#endif
		#ifndef EA_FAIL
			#define EA_FAIL() if ( EA::Assert::Detail::Call("EA_FAIL", __FILE__, __LINE__, EA_CURRENT_FUNCTION)) EA_DEBUG_BREAK(); else ((void)0)
		#endif
		#ifndef EA_FAIL_MSG
			#define EA_FAIL_MSG(msg) if ( EA::Assert::Detail::Call("EA_FAIL", __FILE__, __LINE__, EA_CURRENT_FUNCTION, msg)) EA_DEBUG_BREAK(); else ((void)0)
		#endif
		#ifndef EA_FAIL_FORMATTED
			#define EA_FAIL_FORMATTED(fmt)         if (EA_ASSERT_FORMATTED_HANDLER_CALL("EA_FAIL", __FILE__, __LINE__, EA_CURRENT_FUNCTION, fmt))       EA_DEBUG_BREAK(); else ((void)0)
		#endif
	#else
		#ifndef EA_ASSERT
			#define EA_ASSERT(expr) \
				EA_DISABLE_VC_WARNING(4127)\
				do {\
					EA_ANALYSIS_ASSUME(expr);\
					if(!(expr))\
					{\
						printf("\nEA_ASSERT(%s) failed in %s(%d)\n", #expr, __FILE__, __LINE__); EA_DEBUG_BREAK();\
					}\
					else\
					{\
						((void)0);\
					}\
				} while(0)\
				EA_RESTORE_VC_WARNING()
		#endif
		#ifndef EA_ASSERT_MSG
			#define EA_ASSERT_MSG(expr, msg) \
				EA_DISABLE_VC_WARNING(4127)\
				do\
				{ \
					EA_ANALYSIS_ASSUME(expr); \
					if (!(expr)) \
					{\
						printf("\nEA_ASSERT(%s) failed in %s(%d): %s\n", #expr, __FILE__, __LINE__, msg);EA_DEBUG_BREAK();\
					}\
					else \
					{ \
						((void)0); \
					} \
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
		#ifndef EA_ASSERT_FORMATTED
			#define EA_ASSERT_FORMATTED(expr, fmt) \
				EA_DISABLE_VC_WARNING(4127) \
				do {\
					EA_ANALYSIS_ASSUME(expr); \
					if (!(expr)) \
					{ \
						printf fmt; EA_DEBUG_BREAK(); EA_DEBUG_BREAK(); \
					} \
					else \
					{ \
						((void)0); \
					} \
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
		#ifndef EA_FAIL
			#define EA_FAIL() \
				EA_DISABLE_VC_WARNING(4127) \
				do {\
					printf("\nEA_FAIL: %s(%d)\n", __FILE__, __LINE__); EA_DEBUG_BREAK(); \
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
		#ifndef EA_FAIL_MSG
			#define EA_FAIL_MSG(msg) \
				EA_DISABLE_VC_WARNING(4127) \
				do {\
					printf("\nEA_FAIL: %s(%d): %s\n", __FILE__, __LINE__, msg); EA_DEBUG_BREAK(); \
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
		#ifndef EA_FAIL_FORMATTED
			#define EA_FAIL_FORMATTED(fmt) \
				EA_DISABLE_VC_WARNING(4127) \
				do {\
					printf fmt; EA_DEBUG_BREAK(); \
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
	#endif
#else
	#ifndef EA_ASSERT
		#define EA_ASSERT(expr)                        ((void)0)
	#endif
	#ifndef EA_ASSERT_MSG
		#define EA_ASSERT_MSG(expr, msg)               ((void)0)
	#endif
	#ifndef EA_ASSERT_FORMATTED
		#define EA_ASSERT_FORMATTED(expr, fmt)         ((void)0)
	#endif
	#ifndef EA_FAIL
		#define EA_FAIL()                              ((void)0)
	#endif
	#ifndef EA_FAIL_MSG
		#define EA_FAIL_MSG(msg)                       ((void)0)
	#endif
	#ifndef EA_FAIL_FORMATTED
		#define EA_FAIL_FORMATTED(fmt)                 ((void)0)
	#endif
#endif

///////////////////////////////////////////////////////////////////////////////
/// EA_PANIC
///
// EA_PANIC is a macro for runtime assertion checks in release builds. The difference 
// between using the EA_ASSERT is rather than performing a EA_DEBUG_BREAK, EA_CRASH is 
// used instead.
///
// What this provides is a conditional forced crash on non-final builds since EA_ASSERT
// is only enabled for debug builds which define a handler. The largest motivation
// for using EA_PANIC over EA_ASSERT should be when one wants to explicitly catch 
// critical asserts in release builds.
/// Example: 
///    EA_PANIC(myVar == theirVar);
///
#ifdef EA_PANIC_ENABLED
	#ifdef __cplusplus
		#ifndef EA_PANIC
			#define EA_PANIC(expr) \
				EA_DISABLE_VC_WARNING(4127) \
				do { \
					EA_ANALYSIS_ASSUME(expr); \
					if (!(expr)) { \
						EA::Assert::Detail::Call(#expr, __FILE__, __LINE__, EA_CURRENT_FUNCTION); EA_CRASH(); \
					} else ((void)0); \
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
		#ifndef EA_PANIC_MSG
			#define EA_PANIC_MSG(expr, msg) \
				EA_DISABLE_VC_WARNING(4127) \
				do { \
					EA_ANALYSIS_ASSUME(expr); \
					if (!(expr)) { \
						EA::Assert::Detail::Call(#expr, __FILE__, __LINE__, EA_CURRENT_FUNCTION, msg); EA_CRASH(); \
					} else ((void)0); \
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
		#ifndef EA_PANIC_FORMATTED
			#define EA_PANIC_FORMATTED(expr, fmt) \
				EA_DISABLE_VC_WARNING(4127) \
				do { \
					EA_ANALYSIS_ASSUME(expr); \
					if (!(expr)) { \
						EA_ASSERT_FORMATTED_HANDLER_CALL(#expr, __FILE__, __LINE__, EA_CURRENT_FUNCTION, fmt); EA_CRASH(); \
					} else ((void)0); \
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
	#else
		#ifndef EA_PANIC
			#define EA_PANIC(expr) \
				EA_DISABLE_VC_WARNING(4127) \
				do { \
					EA_ANALYSIS_ASSUME(expr); \
					if (!(expr)) { \
						printf("\nEA_PANIC(%s) failed in %s(%d)\n", #expr, __FILE__, __LINE__); EA_CRASH(); \
					} else ((void)0); \
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
		#ifndef EA_PANIC_MSG
			#define EA_PANIC_MSG(expr, msg) \
				EA_DISABLE_VC_WARNING(4127) \
				do { \
					EA_ANALYSIS_ASSUME(expr); \
					if (!(expr)) { \
						printf("\nEA_PANIC(%s) failed in %s(%d): %s\n", #expr, __FILE__, __LINE__, msg); EA_CRASH(); \
					} else ((void)0); \
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
		#ifndef EA_PANIC_FORMATTED
			#define EA_PANIC_FORMATTED(expr, fmt) \
				EA_DISABLE_VC_WARNING(4127) \
				do { \
					EA_ANALYSIS_ASSUME(expr); \
					if (!(expr)) { \
						printf fmt; EA_CRASH(); \
					} else ((void)0); \
				} while(0) \
				EA_RESTORE_VC_WARNING()
		#endif
	#endif
#else
	#ifndef EA_PANIC
		#define EA_PANIC(expr)                ((void)0)
	#endif
	#ifndef EA_PANIC_MSG
		#define EA_PANIC_MSG(expr, msg)       ((void)0)
	#endif
	#ifndef EA_PANIC_FORMATTED
		#define EA_PANIC_FORMATTED(expr, fmt) ((void)0)
	#endif
#endif

#endif // EA_ASSERT_HAVE_OWN_HEADER

// We provide some synonyms for the message style macros, because we couldn't reach common ground
// and there was too much existing code using different versions. It's not pretty, but relatively harmless
#ifndef EA_ASSERT_M
	#define EA_ASSERT_M EA_ASSERT_MSG
#endif
#ifndef EA_ASSERT_MESSAGE 
	#define EA_ASSERT_MESSAGE EA_ASSERT_MSG
#endif
#ifndef EA_FAIL_M
	#define EA_FAIL_M EA_FAIL_MSG
#endif
#ifndef EA_FAIL_MESSAGE
	#define EA_FAIL_MESSAGE EA_FAIL_MSG
#endif
#ifndef EA_PANIC_M
	#define EA_PANIC_M EA_PANIC_MSG
#endif
#ifndef EA_PANIC_MESSAGE
	#define EA_PANIC_MESSAGE EA_PANIC_MSG
#endif


#endif
