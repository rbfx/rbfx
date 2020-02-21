///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif


#ifndef EATHREAD_GCC_EATHREAD_ATOMIC_GCC_H
#define EATHREAD_GCC_EATHREAD_ATOMIC_GCC_H

#include <EABase/eabase.h>
#include <stddef.h>
#include <eathread/internal/eathread_atomic_standalone.h>

#define EA_THREAD_ATOMIC_IMPLEMENTED

namespace EA
{
	namespace Thread
	{
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


		// Recent versions of GCC have atomic primitives built into the compiler and standard library.
		#if defined(EA_COMPILER_CLANG) || defined(__APPLE__) || (defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 403)) || defined(EA_COMPILER_RVCT) // GCC 4.3 or later. Depends on the GCC implementation.

			template <> inline
			AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::GetValue() const
				{ return __sync_add_and_fetch(const_cast<ValueType*>(&mValue), 0); }

			template <> inline
			AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::GetValue() const
				{ return __sync_add_and_fetch(const_cast<ValueType*>(&mValue), 0); }

			template <> inline
			AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::SetValue(ValueType n)
				{ __sync_synchronize(); return __sync_lock_test_and_set(&mValue, n); }

			template <> inline
			AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::SetValue(ValueType n)
				{ __sync_synchronize(); return __sync_lock_test_and_set(&mValue, n); }

			template <> inline
			bool AtomicInt<int32_t>::SetValueConditional(ValueType n, ValueType condition)
				{ return (__sync_val_compare_and_swap(&mValue, condition, n) == condition); }

			template <> inline
			bool AtomicInt<uint32_t>::SetValueConditional(ValueType n, ValueType condition)
				{ return (__sync_val_compare_and_swap(&mValue, condition, n) == condition); }

			template <> inline
			AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Increment()
				{ return __sync_add_and_fetch(&mValue, 1); }

			template <> inline
			AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Increment()
				{ return __sync_add_and_fetch(&mValue, 1); }

			template <> inline
			AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Decrement()
				{ return __sync_sub_and_fetch(&mValue, 1); }

			template <> inline
			AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Decrement()
				{ return __sync_sub_and_fetch(&mValue, 1); }

			template <> inline
			AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Add(ValueType n)
				{ return __sync_add_and_fetch(&mValue, n); }

			template <> inline
			AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Add(ValueType n)
				{ return __sync_add_and_fetch(&mValue, n); }

			template <> inline
			AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::GetValue() const
				{ return __sync_add_and_fetch(const_cast<ValueType*>(&mValue), 0); }

			template <> inline
			AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::GetValue() const
				{ return __sync_add_and_fetch(const_cast<ValueType*>(&mValue), 0); }

			template <> inline
			AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::SetValue(ValueType n)
				{ __sync_synchronize(); return __sync_lock_test_and_set(&mValue, n); }

			template <> inline
			AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::SetValue(ValueType n)
				{ __sync_synchronize(); return __sync_lock_test_and_set(&mValue, n); }

			template <> inline
			bool AtomicInt<int64_t>::SetValueConditional(ValueType n, ValueType condition)
				{ return (__sync_val_compare_and_swap(&mValue, condition, n) == condition); }

			template <> inline
			bool AtomicInt<uint64_t>::SetValueConditional(ValueType n, ValueType condition)
				{ return (__sync_val_compare_and_swap(&mValue, condition, n) == condition); }

			template <> inline
			AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::Increment()
				{ return __sync_add_and_fetch(&mValue, 1); }

			template <> inline
			AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::Increment()
				{ return __sync_add_and_fetch(&mValue, 1); }

			template <> inline
			AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::Decrement()
				{ return __sync_sub_and_fetch(&mValue, 1); }

			template <> inline
			AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::Decrement()
				{ return __sync_sub_and_fetch(&mValue, 1); }

			template <> inline
			AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::Add(ValueType n)
				{ return __sync_add_and_fetch(&mValue, n); }

			template <> inline
			AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::Add(ValueType n)
				{ return __sync_add_and_fetch(&mValue, n); }
		#endif

	} // namespace Thread

} // namespace EA



#endif // EATHREAD_GCC_EATHREAD_ATOMIC_GCC_H









