///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif


#ifndef EATHREAD_GCC_EATHREAD_ATOMIC_ANDROID_H
#define EATHREAD_GCC_EATHREAD_ATOMIC_ANDROID_H


#include <EABase/eabase.h>
#include <stddef.h>
#include <sys/atomics.h>
#include <eathread/internal/eathread_atomic_standalone.h>

#define EA_THREAD_ATOMIC_IMPLEMENTED

namespace EA
{
	namespace Thread
	{
		/// android_fake_atomics_*
		///
		int64_t android_fake_atomic_swap_64(int64_t value, volatile int64_t* addr);
		int android_fake_atomic_cmpxchg_64(int64_t oldvalue, int64_t newvalue, volatile int64_t* addr);
		int64_t android_fake_atomic_read_64(volatile int64_t* addr);

		/// class AtomicInt
		/// Actual implementation may vary per platform. May require certain alignments, sizes, 
		/// and declaration specifications per platform.
		template <class T>
		class AtomicInt
		{
		public:
			typedef AtomicInt<T> ThisType;
			typedef T            ValueType;

			/// AtomicInt
			/// Empty constructor. Intentionally leaves mValue in an unspecified state.
			/// This is done so that an AtomicInt acts like a standard built-in integer.
			AtomicInt()
				{}

			AtomicInt(ValueType n) 
				{ SetValue(n); }

			AtomicInt(const ThisType& x)
				: mValue(x.GetValue()) {}

			AtomicInt& operator=(const ThisType& x)
				{ mValue = x.GetValue(); return *this; }

			ValueType GetValue() const
				{ return mValue; }

			ValueType GetValueRaw() const
				{ return mValue; }

			ValueType SetValue(ValueType n);
			bool      SetValueConditional(ValueType n, ValueType condition);
			ValueType Increment();
			ValueType Decrement();
			ValueType Add(ValueType n);

			// operators
			inline            operator const ValueType() const { return GetValue(); }
			inline ValueType  operator =(ValueType n)          {        SetValue(n); return n; }
			inline ValueType  operator+=(ValueType n)          { return Add(n);}
			inline ValueType  operator-=(ValueType n)          { return Add(-n);}
			inline ValueType  operator++()                     { return Increment();}
			inline ValueType  operator++(int)                  { return Increment() - 1;}
			inline ValueType  operator--()                     { return Decrement(); }
			inline ValueType  operator--(int)                  { return Decrement() + 1;}

		protected:
			volatile ValueType mValue;
		};


		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::SetValue(ValueType n)
			{ return __atomic_swap(n, &mValue); }

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::SetValue(ValueType n)
			{ return __atomic_swap(n, (volatile int*)&mValue); }

		template <> inline
		bool AtomicInt<int32_t>::SetValueConditional(ValueType n, ValueType condition)
			{ return (__atomic_cmpxchg(condition, n, &mValue) == 0); }

		template <> inline
		bool AtomicInt<uint32_t>::SetValueConditional(ValueType n, ValueType condition)
			{ return (__atomic_cmpxchg(condition, n, (volatile int*)&mValue) == 0); }

		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Increment()
			{ return __atomic_inc(&mValue) + 1; }

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Increment()
			{ return __atomic_inc((volatile int*)&mValue) + 1; }

		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Decrement()
			{ return __atomic_dec(&mValue) - 1; }

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Decrement()
			{ return __atomic_dec((volatile int*)&mValue) - 1; }

		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Add(ValueType n)
			{
				// http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Atomic-Builtins.html
				return __sync_add_and_fetch(&mValue, n); 
			}

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Add(ValueType n)
			{ 
				// http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Atomic-Builtins.html
				return __sync_add_and_fetch(&mValue, n); 
			}


		///////////////////////////////////////////////////////////
		/// 64 bit, simulated
		///
		template <> inline
		AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::GetValue() const
			{ return android_fake_atomic_read_64((volatile int64_t*)&mValue); }

		template <> inline
		AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::GetValue() const
			{ return android_fake_atomic_read_64((volatile int64_t*)&mValue); }

		template <> inline
		AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::SetValue(ValueType n)
		{
			const ValueType nOldValue(mValue);
			android_fake_atomic_swap_64((int64_t)n, (volatile int64_t*)&mValue);
			return nOldValue;
		}

		template <> inline
		AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::SetValue(ValueType n)
		{
			const ValueType nOldValue(mValue);
			android_fake_atomic_swap_64((int64_t)n, (volatile int64_t*)&mValue);
			return nOldValue;
		}

		template <> inline
		bool AtomicInt<int64_t>::SetValueConditional(ValueType n, ValueType condition)
		{
			return android_fake_atomic_cmpxchg_64(condition, n, (volatile int64_t*)&mValue) == 0;
		}

		template <> inline
		bool AtomicInt<uint64_t>::SetValueConditional(ValueType n, ValueType condition)
		{
			return android_fake_atomic_cmpxchg_64(condition, n, (volatile int64_t*)&mValue) == 0;
		}

		template <> inline
		AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::Add(ValueType n)
		{
			int64_t old;

			do {
				old = mValue;
			}
			while (android_fake_atomic_cmpxchg_64((int64_t)old, (int64_t)old+n, (volatile int64_t*)&mValue) != 0);

			return mValue;
		}

		template <> inline
		AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::Add(ValueType n)
		{
			uint64_t old;

			do {
				old = mValue;
			}
			while (android_fake_atomic_cmpxchg_64((int64_t)old, (int64_t)old+n, (volatile int64_t*)&mValue) != 0);

			return mValue;
		}

		template <> inline
		AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::Increment()
			{ return Add(1); }

		template <> inline
		AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::Increment()
			{ return Add(1); }

		template <> inline
		AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::Decrement()
			{ return Add(-1); }

		template <> inline
		AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::Decrement()
			{ return Add(-1); }

	} // namespace Thread

} // namespace EA



#endif // EATHREAD_GCC_EATHREAD_ATOMIC_ANDROID_H









