///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

/////////////////////////////////////////////////////////////////////////////
// Defines functionality for threadsafe primitive operations.
/////////////////////////////////////////////////////////////////////////////

#ifndef EATHREAD_APPLE_EATHREAD_ATOMIC_APPLE_H
#define EATHREAD_APPLE_EATHREAD_ATOMIC_APPLE_H

#include <EABase/eabase.h>
#include <stddef.h>
#include <libkern/OSAtomic.h>
#include "eathread/internal/atomic.h"
#include "eathread/internal/eathread_atomic_standalone.h"

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
				{ }

			AtomicInt(ValueType n) 
				{ SetValue(n); }

			AtomicInt(const ThisType& x)
				: mValue(x.GetValue()) { }

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
		
		template <>
		class AtomicInt<uint64_t>
		{
		public:
			typedef AtomicInt<uint64_t> ThisType;
			typedef uint64_t          ValueType;
			
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
			{ return (uint64_t)AtomicGetValue64((volatile int64_t *)&mValue); }
			
			ValueType GetValueRaw() const
			{ return mValue; }
			
			ValueType SetValue(ValueType n)
			{ return (uint64_t)AtomicSetValue64((volatile int64_t *)&mValue, n); }
			
			bool      SetValueConditional(ValueType n, ValueType condition)
			{ return AtomicSetValueConditional64((volatile int64_t *)&mValue, n, condition); }
			
			ValueType Increment()
			{ return (uint64_t)AtomicAdd64((volatile int64_t *)&mValue, 1); }
			
			ValueType Decrement()
			{ return (uint64_t)AtomicAdd64((volatile int64_t *)&mValue, -1); }
			
			ValueType Add(ValueType n)
			{ return (uint64_t)AtomicAdd64((volatile int64_t *)&mValue, n); }
			
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
		}__attribute__((aligned(8)));
		
		template <>
		class AtomicInt<int64_t>
		{
		public:
			typedef AtomicInt<int64_t> ThisType;
			typedef int64_t          ValueType;
			
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
			{ return AtomicGetValue64((volatile int64_t *)&mValue); }
			
			ValueType GetValueRaw() const
			{ return mValue; }
			
			ValueType SetValue(ValueType n)
			{ return AtomicSetValue64((volatile int64_t *)&mValue, n); }
			
			bool      SetValueConditional(ValueType n, ValueType condition)
			{ return AtomicSetValueConditional64((volatile int64_t *)&mValue, n, condition); }
			
			ValueType Increment()
			{ return AtomicAdd64((volatile int64_t *)&mValue, 1); }
			
			ValueType Decrement()
			{ return AtomicAdd64((volatile int64_t *)&mValue, -1); }
			
			ValueType Add(ValueType n)
			{ return AtomicAdd64((volatile int64_t *)&mValue, n); }
			
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
		}__attribute__((aligned(8)));
		

		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::GetValue() const
			{ return OSAtomicAdd32(0, reinterpret_cast<volatile int32_t*>(const_cast<ValueType*>(&mValue))); }

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::GetValue() const
			{ return OSAtomicAdd32(0, reinterpret_cast<volatile int32_t*>(const_cast<ValueType*>(&mValue))); }

		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::SetValue(ValueType n)
		{ 
			int32_t old;
			do
			{
				old = mValue; 
			}
			while ( ! OSAtomicCompareAndSwap32(old, n, reinterpret_cast<volatile int32_t*>(&mValue)));
			return old; 
		}

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::SetValue(ValueType n)
		{
			uint32_t old;
			do
			{
				old = mValue;
			} while ( ! OSAtomicCompareAndSwap32(old, n, reinterpret_cast<volatile int32_t*>(&mValue)));
			return old;
		}

		template <> inline
		bool AtomicInt<int32_t>::SetValueConditional(ValueType n, ValueType condition)
			{ return OSAtomicCompareAndSwap32(condition, n, reinterpret_cast<volatile int32_t*>(&mValue)); }

		template <> inline
		bool AtomicInt<uint32_t>::SetValueConditional(ValueType n, ValueType condition)
			{ return OSAtomicCompareAndSwap32(condition, n, reinterpret_cast<volatile int32_t*>(&mValue)); }

		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Increment()
			{ return OSAtomicIncrement32(reinterpret_cast<volatile int32_t*>(&mValue)); }

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Increment()
			{ return OSAtomicIncrement32(reinterpret_cast<volatile int32_t*>(&mValue)); }

		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Decrement()
			{ return OSAtomicDecrement32(reinterpret_cast<volatile int32_t*>(&mValue)); }

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Decrement()
			{ return OSAtomicDecrement32(reinterpret_cast<volatile int32_t*>(&mValue)); }

		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Add(ValueType n)
			{ return OSAtomicAdd32(n, reinterpret_cast<volatile int32_t*>(&mValue)); }

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Add(ValueType n)
			{ return OSAtomicAdd32(n, reinterpret_cast<volatile int32_t*>(&mValue)); }
	}
}

#endif
