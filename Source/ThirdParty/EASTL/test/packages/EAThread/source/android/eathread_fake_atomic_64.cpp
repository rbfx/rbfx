///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/// Pseudo implementation of 64 bit primitives modelled after Android's internals.
/// Return values and semantics are intended to be the same as 32 bit versions.
///
/// Basically just does a mutex lock around the operation. Rather than just
/// one global lock, uses a fixed set of mutexes to lock based on incoming
/// address to reduce contention.
///
/// Abuses the fact that the initializer for a pthread_mutex_t in Android is
/// simply "{0}" on a volatile int to avoid requiring global initialization of
/// these mutexes.


#include <EABase/eabase.h>

#if defined(EA_PLATFORM_ANDROID)

#include <pthread.h>

namespace EA
{
namespace Thread
{

#define EAT_FAKE_ATOMIC_SWAP_LOCK_COUNT 32U
static pthread_mutex_t sFakeAtomic64SwapLocks[EAT_FAKE_ATOMIC_SWAP_LOCK_COUNT];
// Urho3D: unsigned -> int64_t to fix build error on 64bit android
#define EAT_SWAP_LOCK(addr) &sFakeAtomic64SwapLocks[((int64_t)(void*)(addr) >> 3U) % EAT_FAKE_ATOMIC_SWAP_LOCK_COUNT]


int64_t android_fake_atomic_swap_64(int64_t value, volatile int64_t* addr)
{
	int64_t oldValue;
	pthread_mutex_t* lock = EAT_SWAP_LOCK(addr);

	pthread_mutex_lock(lock);

	oldValue = *addr;
	*addr = value;

	pthread_mutex_unlock(lock);
	return oldValue;
}


int android_fake_atomic_cmpxchg_64(int64_t oldvalue, int64_t newvalue, volatile int64_t* addr)
{
	int ret;
	pthread_mutex_t* lock = EAT_SWAP_LOCK(addr);

	pthread_mutex_lock(lock);

	if (*addr == oldvalue)
	{
		*addr = newvalue;
		ret = 0;
	}
	else
	{
		ret = 1;
	}
	pthread_mutex_unlock(lock);
	return ret;
}


int64_t android_fake_atomic_read_64(volatile int64_t* addr)
{
	int64_t ret;
	pthread_mutex_t* lock = EAT_SWAP_LOCK(addr);

	pthread_mutex_lock(lock);
	ret = *addr;
	pthread_mutex_unlock(lock);
	return ret;
}

} // namespace Thread
} // namespace EA

#endif // #if defined(EA_PLATFORM_ANDROID)
