///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

/////////////////////////////////////////////////////////////////////////////
// Functionality related to memory and code generation synchronization.
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_X86_64_EATHREAD_SYNC_X86_64_H
#define EATHREAD_X86_64_EATHREAD_SYNC_X86_64_H


#ifndef INCLUDED_eabase_H
	#include "EABase/eabase.h"
#endif


#if defined(EA_PROCESSOR_X86_64)
	#define EA_THREAD_SYNC_IMPLEMENTED

	#ifdef _MSC_VER
		#pragma warning(push, 0)
		#include <math.h>   // VS2008 has an acknowledged bug that requires math.h (and possibly also string.h) to be #included before intrin.h.
		#include <intrin.h>
		#pragma warning(pop)
	#endif

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
		#pragma intrinsic(_mm_pause)
		#define EAProcessorPause() _mm_pause() // The __yield() intrinsic currently doesn't work on x86-64.
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
	#if defined(EA_COMPILER_MSVC)
		#pragma intrinsic(_ReadBarrier)
		#pragma intrinsic(_WriteBarrier)
		#pragma intrinsic(_ReadWriteBarrier)

		#define EAReadBarrier()      _ReadBarrier()
		#define EAWriteBarrier()     _WriteBarrier()
		#define EAReadWriteBarrier() _ReadWriteBarrier()
	#elif defined(EA_PLATFORM_PS4)
		#define EAReadBarrier()      __asm__ __volatile__ ("lfence" ::: "memory");
		#define EAWriteBarrier()     __asm__ __volatile__ ("sfence" ::: "memory");
		#define EAReadWriteBarrier() __asm__ __volatile__ ("mfence" ::: "memory");
	#elif defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 401) // GCC 4.1 or later
		#define EAReadBarrier      __sync_synchronize
		#define EAWriteBarrier     __sync_synchronize
		#define EAReadWriteBarrier __sync_synchronize
	#else
		#define EAReadBarrier      EACompilerMemoryBarrier // Need to implement this for non-VC++
		#define EAWriteBarrier     EACompilerMemoryBarrier // Need to implement this for non-VC++
		#define EAReadWriteBarrier EACompilerMemoryBarrier // Need to implement this for non-VC++
	#endif


	// EACompilerMemoryBarrier
	#if defined(EA_COMPILER_MSVC)
		#define EACompilerMemoryBarrier() _ReadWriteBarrier()
	#elif defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)
		#define EACompilerMemoryBarrier() __asm__ __volatile__ ("":::"memory")
	#else
		#define EACompilerMemoryBarrier() // Possibly `EAT_ASSERT(false)` here?
	#endif


#endif // EA_PROCESSOR_X86


#endif // EATHREAD_X86_64_EATHREAD_SYNC_X86_64_H








