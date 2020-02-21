///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>

#if defined(EA_PLATFORM_WIN32) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
	#include "pc/eathread_callstack_win32.cpp"
#elif defined(EA_PLATFORM_MICROSOFT) && defined(EA_PROCESSOR_X86_64)
	#include "pc/eathread_callstack_win64.cpp"
#elif defined(EA_PLATFORM_SONY)
	#include "kettle/eathread_callstack_kettle.cpp"
	#include "kettle/eathread_pthread_stack_info.cpp"
#elif defined(EA_PLATFORM_ANDROID) && defined(EA_PROCESSOR_X86)
	#include "x86/eathread_callstack_x86.cpp"
	#include "unix/eathread_pthread_stack_info.cpp"
#elif defined(EA_PLATFORM_ANDROID)
	#include "libunwind/eathread_callstack_libunwind.cpp"
	#include "unix/eathread_pthread_stack_info.cpp"
#elif defined(EA_PLATFORM_APPLE) // OSX, iPhone, iPhone Simulator
	#include "apple/eathread_callstack_apple.cpp"
	#include "unix/eathread_pthread_stack_info.cpp"
#elif defined(EA_PROCESSOR_ARM) 
	#include "arm/eathread_callstack_arm.cpp"
	#if !defined(EA_PLATFORM_MICROSOFT)
		#include "unix/eathread_pthread_stack_info.cpp"
	#endif
#elif (defined(EA_PLATFORM_LINUX) || defined(__CYGWIN__)) && (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64))
	#include "x86/eathread_callstack_x86.cpp"
	#include "unix/eathread_pthread_stack_info.cpp"
#elif defined(__GNUC__) || defined(EA_COMPILER_CLANG)
	#include "unix/eathread_callstack_glibc.cpp"
	#include "unix/eathread_pthread_stack_info.cpp"
#else
	#include "null/eathread_callstack_null.cpp"
#endif
