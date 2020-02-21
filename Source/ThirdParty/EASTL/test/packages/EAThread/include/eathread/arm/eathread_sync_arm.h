///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

/////////////////////////////////////////////////////////////////////////////
// Functionality related to memory and code generation synchronization.
//
// Created by Rob Parolin 
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_ARM_EATHREAD_SYNC_ARM_H
#define EATHREAD_ARM_EATHREAD_SYNC_ARM_H

#include <EABase/eabase.h>


#if defined(EA_COMPILER_CLANG)
	#define EA_THREAD_SYNC_IMPLEMENTED

	#define EAProcessorPause()

	#define EAReadBarrier      __sync_synchronize
	#define EAWriteBarrier     __sync_synchronize
	#define EAReadWriteBarrier __sync_synchronize

	#define EACompilerMemoryBarrier() __asm__ __volatile__ ("" : : : "memory")


#elif defined(EA_COMPILER_GNUC)
	#define EA_THREAD_SYNC_IMPLEMENTED

	#define EAProcessorPause()

	#if (((__GNUC__ * 100) + __GNUC_MINOR__) >= 401) // GCC 4.1 or later
		#define EAReadBarrier      __sync_synchronize
		#define EAWriteBarrier     __sync_synchronize
		#define EAReadWriteBarrier __sync_synchronize
	#else
		#define EAReadBarrier      EACompilerMemoryBarrier
		#define EAWriteBarrier     EACompilerMemoryBarrier
		#define EAReadWriteBarrier EACompilerMemoryBarrier
	#endif

	#define EACompilerMemoryBarrier() __asm__ __volatile__ ("" : : : "memory")

#endif

#endif // EATHREAD_ARM_EATHREAD_SYNC_ARM_H

