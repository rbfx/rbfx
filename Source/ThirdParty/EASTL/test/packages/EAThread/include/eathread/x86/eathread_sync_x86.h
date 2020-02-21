///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

/////////////////////////////////////////////////////////////////////////////
// Functionality related to memory and code generation synchronization.
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_X86_EATHREAD_SYNC_X86_H
#define EATHREAD_X86_EATHREAD_SYNC_X86_H


#ifndef INCLUDED_eabase_H
	#include <EABase/eabase.h>
#endif


#if defined(EA_PROCESSOR_X86)
	#define EA_THREAD_SYNC_IMPLEMENTED

	// By default, we define EA_TARGET_SMP to be true. The reason for this is that most 
	// applications that users of this code are likely to write are going to be executables
	// which run properly on any system, be it multiprocessing or not.
	#ifndef EA_TARGET_SMP
		#define EA_TARGET_SMP 1
	#endif

	// EAProcessorPause
	// Intel has defined a 'pause' instruction for x86 processors starting with the P4, though this simply
	// maps to the otherwise undocumented 'rep nop' instruction. This pause instruction is important for 
	// high performance spinning, as otherwise a high performance penalty incurs. 

	#if defined(EA_COMPILER_MSVC) || defined(EA_COMPILER_INTEL) || defined(EA_COMPILER_BORLAND)
		// Year 2003+ versions of the Microsoft SDK define 'rep nop' as YieldProcessor and/or __yield or _mm_pause. 
		#define EAProcessorPause() __asm { rep nop } 
	#elif defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)
		#define EAProcessorPause() __asm__ __volatile__ ("rep ; nop")
	#else
		// In this case we use an Intel-style asm statement. If this doesn't work for your compiler then 
		// there most likely is some way to make the `rep nop` inline asm statement. 
		#define EAProcessorPause() __asm { rep nop } // Alternatively: { __asm { _emit 0xf3 }; __asm { _emit 0x90 } }
	#endif


	// EAReadBarrier / EAWriteBarrier / EAReadWriteBarrier
	// The x86 processor memory architecture ensures read and write consistency on both single and
	// multi processing systems. This makes programming simpler but limits maximimum system performance.
	// We define EAReadBarrier here to be the same as EACompilerMemory barrier in order to limit the 
	// compiler from making any assumptions at its level about memory usage. Year 2003+ versions of the 
	// Microsoft SDK define a 'MemoryBarrier' statement which has the same effect as EAReadWriteBarrier.
	#if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 401) // GCC 4.1 or later
		#define EAReadBarrier      __sync_synchronize
		#define EAWriteBarrier     __sync_synchronize
		#define EAReadWriteBarrier __sync_synchronize
	#else
		#define EAReadBarrier      EACompilerMemoryBarrier
		#define EAWriteBarrier     EACompilerMemoryBarrier
		#define EAReadWriteBarrier EACompilerMemoryBarrier
	#endif

	// EACompilerMemoryBarrier
	#if (defined(EA_COMPILER_MSVC) && (EA_COMPILER_VERSION >= 1300) && defined(EA_PLATFORM_MICROSOFT)) || (defined(EA_COMPILER_INTEL) && (EA_COMPILER_VERSION >= 9999999)) // VC7+ or Intel (unknown version at this time)
		extern "C" void _ReadWriteBarrier();
		#pragma intrinsic(_ReadWriteBarrier)
		#define EACompilerMemoryBarrier() _ReadWriteBarrier()
	#elif defined(EA_COMPILER_GNUC)
		#define EACompilerMemoryBarrier() __asm__ __volatile__ ("":::"memory")
	#else
		#define EACompilerMemoryBarrier() // Possibly `EAT_ASSERT(false)` here?
	#endif


#endif // EA_PROCESSOR_X86


#endif // EATHREAD_X86_EATHREAD_SYNC_X86_H








