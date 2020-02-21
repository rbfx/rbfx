///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "EAAssert/eaassert.h"

#ifndef EA_ASSERT_HAVE_OWN_HEADER

#include <stdio.h>

#if defined(EA_PLATFORM_MICROSOFT)
	#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0400)
		#undef  _WIN32_WINNT
		#define _WIN32_WINNT 0x0400
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#pragma warning(push,0)
	#include <Windows.h>        // ::IsDebuggerPresent
	#pragma warning(pop)
#endif




#if !defined(EA_ASSERT_VSNPRINTF)
	#if defined(EA_PLATFORM_MICROSOFT)
		#define EA_ASSERT_VSNPRINTF _vsnprintf
		#define EA_ASSERT_SNPRINTF   _snprintf
	#else
		#define EA_ASSERT_VSNPRINTF vsnprintf
		#define EA_ASSERT_SNPRINTF   snprintf
	#endif
#endif


namespace EA { 
	namespace Assert { 
	namespace Detail {
	namespace {

		#if defined(EA_ASSERT_ENABLED)
			static void PlatformPrint(const char *str)
			{
				#if defined(EA_PLATFORM_MICROSOFT)
					if (IsDebuggerPresent())
					{
						::OutputDebugStringA(str);
					}
				#endif

				puts(str);
				
				#if defined(EA_PLATFORM_MOBILE)
					fflush(stdout); // Mobile platforms need this because otherwise you can easily lose output if the device crashes.
				#endif
			}
		#endif

		bool DefaultFailureCallback(const char* expr, const char* filename, int line, const char* function, const char* msg, va_list args)
		{
			#if defined(EA_ASSERT_ENABLED)
				const int largeEnough = 2048;
				char output[largeEnough + 1] = {};
				char fmtMsg[largeEnough + 1] = {};

				int len = EA_ASSERT_VSNPRINTF(fmtMsg, largeEnough, msg, args);

				if(len==0)
				{
					len = EA_ASSERT_SNPRINTF(fmtMsg, largeEnough, "none");
				}

				// different platforms return different values for the error, but in both
				// cases it'll be out of bounds, so clamp the return value to largeEnough.
				if (len < 0 || len > largeEnough)
					len = largeEnough;

				fmtMsg[len] = '\0';

				len = EA_ASSERT_SNPRINTF(output, largeEnough,
					"%s(%d) : EA_ASSERT failed: '%s' in function: %s\n, message: %s",
					filename, line, expr, function, fmtMsg);
				if (len < 0 || len > largeEnough)
					len = largeEnough;

				output[len] = '\0';

				PlatformPrint(output);
			#else
				EA_UNUSED(expr);
				EA_UNUSED(filename);
				EA_UNUSED(line);
				EA_UNUSED(function);
				EA_UNUSED(msg);
				EA_UNUSED(args);
			#endif

			return true;
		}
		
		FailureCallback gFailureCallback = &DefaultFailureCallback; 
	}}

	void SetFailureCallback(FailureCallback failureCallback)
	{
		Detail::gFailureCallback = failureCallback;
	}

	FailureCallback GetFailureCallback()
	{
		return Detail::gFailureCallback;
	}

	bool Detail::VCall(const char *expr, const char *filename, int line, const char *function, const char *msg, ...)
	{
		va_list args;
		va_start(args, msg);
		bool ret = (*GetFailureCallback())(expr, filename, line, function, msg, args);
		va_end(args);
		return ret;
	}

	bool Detail::Call(const char *expr, const char *filename, int line, const char *function)
	{
		// Pass an empty string as msg parameter. Some FailureCallback implementation (Frostbite) 
		// will display only msg and discard the rest of the data when msg is non empty.
		return VCall(expr, filename, line, function, "");
	}

	bool Detail::Call(const char *expr, const char *filename, int line, const char *function, const char* msg)
	{
		return VCall(expr, filename, line, function, "%s", msg);

	}

	}}

#endif
