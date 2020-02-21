///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EATHREAD_INTERNAL_ATOMIC_H
#define EATHREAD_INTERNAL_ATOMIC_H

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

namespace EA
{
	namespace Thread
	{
		typedef int64_t(*AtomicAdd64Function)(volatile int64_t *ptr, int64_t value);
		typedef int64_t(*AtomicGetValue64Function)(volatile int64_t *ptr);
		typedef int64_t(*AtomicSetValue64Function)(volatile int64_t *ptr, int64_t value);
		typedef bool(*AtomicSetValueConditional64Function)(volatile int64_t *ptr, int64_t value, int64_t condition);


		extern AtomicAdd64Function AtomicAdd64;
		extern AtomicGetValue64Function AtomicGetValue64;
		extern AtomicSetValue64Function AtomicSetValue64;
		extern AtomicSetValueConditional64Function AtomicSetValueConditional64;
	}
}

#endif
