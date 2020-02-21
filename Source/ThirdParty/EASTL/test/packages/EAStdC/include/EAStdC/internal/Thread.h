///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTDC_THREAD_H
#define EASTDC_THREAD_H


#include <EABase/eabase.h>
#include <eathread/eathread_atomic.h>
#include <eathread/eathread_mutex.h>


namespace EA
{
	namespace StdC
	{
		/// Safely sets a new value. Returns the old value.
		uint32_t AtomicSet(uint32_t* pValue, uint32_t newValue);

		/// Safely increments the value. Returns the new value.
		/// This function acts the same as the C++ pre-increment operator.
		uint32_t AtomicIncrement(uint32_t* pValue);

		/// Safely decrements the value. Returns the new value.
		/// This function acts the same as the C++ pre-decrement operator.
		uint32_t AtomicDecrement(uint32_t* pValue);

		/// Safely sets the value to a new value if the original value is equal to
		/// a condition value. Returns true if the condition was met and the
		/// assignment occurred. The comparison and value setting are done as
		/// an atomic operation and thus another thread cannot intervene between
		/// the two as would be the case with simple C code.
		bool AtomicCompareSwap(uint32_t* pValue, uint32_t newValue, uint32_t condition);


		/// Mutex
		///
		/// Implements a very simple portable Mutex class.
		///
		class Mutex
		{
		public:
			void Lock() { mMutex.Lock(); }
			void Unlock() { mMutex.Unlock(); }

		protected:
			EA::Thread::Mutex mMutex;
		};
	}
}




///////////////////////////////////////////////////////////////////////////////
// inline implmentation
///////////////////////////////////////////////////////////////////////////////
namespace EA
{
	namespace StdC
	{
		inline uint32_t AtomicIncrement(uint32_t* pValue) { return EA::Thread::AtomicFetchIncrement(pValue) + 1; }

		inline uint32_t AtomicDecrement(uint32_t* pValue) { return EA::Thread::AtomicFetchDecrement(pValue) - 1; }

		inline uint32_t AtomicSet(uint32_t* pValue, uint32_t newValue)
		{
			return EA::Thread::AtomicSetValue(pValue, newValue);
		}

		inline bool AtomicCompareSwap(uint32_t* pValue, uint32_t newValue, uint32_t condition)
		{
			return EA::Thread::AtomicSetValueConditional(pValue, newValue, condition);
		}

	} // namespace StdC

} // namespace EA




#endif // Header include guard

