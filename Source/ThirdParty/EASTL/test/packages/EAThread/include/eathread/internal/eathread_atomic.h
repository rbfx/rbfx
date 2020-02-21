///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// eathread_atomic.h
//
// Defines functionality for thread-safe primitive operations.
// 
// EAThread atomics do NOT imply the use of read/write barriers.  This is 
// partly due to historical reasons and partly due to EAThread's internal 
// code being optimized for not using barriers.
//
// In future, we are considering migrating the atomics interface which  
// defaults atomics to use full read/write barriers while allowing users
// to opt-out of full barrier usage.  The new C++11 interface already provides
// similar interfaces.
//
// http://en.cppreference.com/w/cpp/atomic/memory_order
//
// Created by Rob Parolin
/////////////////////////////////////////////////////////////////////////////

#ifndef EATHREAD_INTERNAL_EATHREAD_ATOMIC_H
#define EATHREAD_INTERNAL_EATHREAD_ATOMIC_H

#include <EABase/eabase.h>
#include <eathread/internal/config.h>
#include <eathread/internal/eathread_atomic_standalone.h>
#include <atomic>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

#define EA_THREAD_ATOMIC_IMPLEMENTED

namespace EA
{
	namespace Thread
	{
		/// class AtomicInt
		///
		/// Implements thread-safe access to an integer and primary operations on that integer.
		/// AtomicIntegers are commonly used as lightweight flags and signals between threads
		/// or as the synchronization object for spinlocks. Those familiar with the Win32 API
		/// will find that AtomicInt32 is essentially a platform independent interface to 
		/// the Win32 InterlockedXXX family of functions. Those familiar with Linux may 
		/// find that AtomicInt32 is essentially a platform independent interface to atomic_t 
		/// functionality.
		///
		/// Note that the reference implementation defined here is itself not thread-safe.
		/// A thread-safe version requires platform-specific code.
		///
		/// Example usage
		///     AtomicInt32 i = 0;
		///
		///     ++i;
		///     i--;
		///     i += 7;
		///     i -= 3;
		///     i = 2;
		///     
		///     int x = i.GetValue();
		///     i.Increment();
		///     bool oldValueWas6 = i.SetValueConditional(3, 6);
		///     i.Add(4);
		///
		template <class T>
		class AtomicInt
		{
		public:
			typedef AtomicInt<T> ThisType;
			typedef T ValueType;

			/// AtomicInt
			/// Empty constructor. Intentionally leaves mValue in an unspecified state.
			/// This is done so that an AtomicInt acts like a standard built-in integer.
			AtomicInt()
				{}

			AtomicInt(ValueType n) 
				{ SetValue(n); }

			AtomicInt(const ThisType& x) 
				{ SetValue(x.GetValue()); }

			AtomicInt& operator=(const ThisType& x)
				{ SetValue(x.GetValue()); return *this; }

			ValueType GetValue() const 
				{ return mValue.load(); }

			ValueType GetValueRaw() const
				{ return mValue; }

			ValueType SetValue(ValueType n)
				{ return mValue.exchange(n); }

			bool SetValueConditional(ValueType n, ValueType condition)
				{ return mValue.compare_exchange_strong(condition, n); }

			ValueType Increment()
				{ return mValue.operator++(); }

			ValueType Decrement()
				{ return mValue.operator--(); }

			ValueType Add(ValueType n)
				{ return mValue.fetch_add(n) + n; }

			// operators
			inline            operator const ValueType() const { return GetValue(); }
			inline ValueType  operator =(ValueType n)          { return mValue.operator=(n); }
			inline ValueType  operator+=(ValueType n)          { return mValue.operator+=(n); }
			inline ValueType  operator-=(ValueType n)          { return mValue.operator-=(n); }
			inline ValueType  operator++()                     { return mValue.operator++(); }
			inline ValueType  operator++(int)                  { return mValue.operator++(0); }
			inline ValueType  operator--()                     { return mValue.operator--(); }
			inline ValueType  operator--(int)                  { return mValue.operator--(0); }

		protected:
			std::atomic<ValueType> mValue;
		};

	} // namespace Thread
} // namespace EA


#endif // EATHREAD_INTERNAL_EATHREAD_ATOMIC_H













