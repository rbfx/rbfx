///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

/// Standalone atomic functions
/// These act the same as the class functions below.
/// The T return values are the previous value, except for the
/// AtomicFetchSwap function which returns the swapped out value.
///
/// T    AtomicGetValue(volatile T*);
/// T    AtomicGetValue(const volatile T*);
/// void AtomicSetValue(volatile T*, T value);
/// T    AtomicFetchIncrement(volatile T*);
/// T    AtomicFetchDecrement(volatile T*);
/// T    AtomicFetchAdd(volatile T*, T value);
/// T    AtomicFetchSub(volatile T*, T value);
/// T    AtomicFetchOr(volatile T*, T value);
/// T    AtomicFetchAnd(volatile T*, T value);
/// T    AtomicFetchXor(volatile T*, T value);
/// T    AtomicFetchSwap(volatile T*, T value);
/// T    AtomicFetchSwapConditional(volatile T*, T value, T condition);
/// bool AtomicSetValueConditional(volatile T*, T value, T condition);

#if defined(EA_COMPILER_MSVC)
	#include <eathread/internal/eathread_atomic_standalone_msvc.h>
#elif defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)
	#include <eathread/internal/eathread_atomic_standalone_gcc.h>
#else
	#error unsupported platform
#endif


