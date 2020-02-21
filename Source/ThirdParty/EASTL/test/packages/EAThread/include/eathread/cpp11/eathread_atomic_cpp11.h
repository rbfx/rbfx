///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

#ifndef EATHREAD_ATOMIC_CPP11_H
#define EATHREAD_ATOMIC_CPP11_H

EA_DISABLE_VC_WARNING(4265 4365 4836 4571 4625 4626 4628 4193 4127 4548 4574 4731)
#include <atomic>
EA_RESTORE_VC_WARNING()

namespace EA
{
	namespace Thread
	{
		#define EA_THREAD_ATOMIC_IMPLEMENTED

		/// Non-member atomic functions
		/// These act the same as the class functions below.
		/// The T return values are the new value, except for the AtomicSwap function which returns the swapped out value.
		///
		/// todo: Implement me when we have a platform to test this on.  C++11 atomics are disabled on all platforms. 
		///

		template <class T>
		class EATHREADLIB_API AtomicInt
		{
		public:
			typedef AtomicInt<T> ThisType;
			typedef T ValueType;

			/// AtomicInt
			/// Empty constructor. Intentionally leaves mValue in an unspecified state.
			/// This is done so that an AtomicInt acts like a standard built-in integer.
			AtomicInt() {}

			/// AtomicInt
			/// Constructs with an intial value. 
			AtomicInt(ValueType n) : mValue(n) {}

			/// AtomicInt
			/// Copy ctor. Uses GetValue to read the value, and thus is synchronized. 
			AtomicInt(const ThisType& x) : mValue(x.GetValue()) {}

			/// AtomicInt
			/// Assignment operator. Uses GetValue to read the value, and thus is synchronized. 
			AtomicInt& operator=(const ThisType& x)
			{ mValue = x.GetValue(); return *this; }

			/// GetValue
			/// Safely gets the current value. A platform-specific version of 
			/// this might need to do something more than just read the value.
			ValueType GetValue() const volatile { return mValue; }

			/// GetValueRaw
			/// "Unsafely" gets the current value. This is useful for algorithms 
			/// that want to poll the value in a high performance way before 
			/// reading or setting the value in a more costly thread-safe way. 
			/// You should not use this function when attempting to do thread-safe
			/// atomic operations.
			ValueType GetValueRaw() const { return mValue; }

			/// SetValue
			/// Safely sets a new value. Returns the old value. Note that due to 
			/// expected multithreaded accesses, a call to GetValue after SetValue
			/// might return a different value then what was set with SetValue.
			/// This of course depends on your situation.
			ValueType SetValue(ValueType n) { return mValue.exchange(n); }

			/// SetValueConditional
			/// Safely the value to a new value if the original value is equal to 
			/// a condition value. Returns true if the condition was met and the 
			/// assignment occurred. The comparison and value setting are done as
			/// an atomic operation and thus another thread cannot intervene between
			/// the two as would be the case with simple C code.
			bool SetValueConditional(ValueType n, ValueType condition) 
			{ 
				return mValue.compare_exchange_strong(condition, n); 
			}

			/// Increment
			/// Safely increments the value. Returns the new value.
			/// This function acts the same as the C++ pre-increment operator.
			ValueType Increment() { return ++mValue; }


			/// Decrement
			/// Safely decrements the value. Returns the new value.
			/// This function acts the same as the C++ pre-decrement operator.
			ValueType Decrement() { return --mValue; }


			/// Add
			/// Safely adds a value, which can be negative. Returns the new value.
			/// You can implement subtraction with this function by using a negative argument.
			ValueType Add(ValueType n) { return (mValue += n); }


			/// operators
			/// These allow an AtomicInt object to safely act like a built-in type.
			///
			/// Note: The operators for AtomicInt behaves differently than standard
			///         C++ operators in that it will always return a ValueType instead
			///         of a reference.
			///
			/// cast operator
			/// Returns the AtomicInt value as an integral type. This allows the 
			/// AtomicInt to behave like a standard built-in integer type.
			operator const ValueType() const { return mValue; }

			/// operator =
			/// Assigns a new value and returns the value after the operation.
			///
			ValueType operator=(ValueType n) { SetValue(n); return n; }

			/// pre-increment operator+=
			/// Adds a value to the AtomicInt and returns the value after the operation.
			///
			/// This function doesn't obey the C++ standard in that it does not return 
			/// a reference, but rather the value of the AtomicInt after the  
			/// operation is complete. It must be noted that this design is motivated by
			/// the fact that it is unsafe to rely on the returned value being equal to 
			/// the previous value + n, as another thread might have modified the AtomicInt 
			/// immediately after the subtraction operation.  So rather than returning the
			/// reference of AtomicInt, the function returns a copy of the AtomicInt value
			/// used in the function.
			ValueType operator+=(ValueType n)  { mValue += n; return mValue; }

			/// pre-increment operator-=
			/// Subtracts a value to the AtomicInt and returns the value after the operation.
			///
			/// This function doesn't obey the C++ standard in that it does not return 
			//  a reference, but rather the value of the AtomicInt after the  
			/// operation is complete. It must be noted that this design is motivated by
			/// the fact that it is unsafe to rely on the returned value being equal to 
			/// the previous value - n, as another thread might have modified the AtomicInt 
			/// immediately after the subtraction operation.  So rather than returning the
			/// reference of AtomicInt, the function returns a copy of the AtomicInt value
			/// used in the function.
			ValueType operator-=(ValueType n) { mValue -= n; return mValue; }

			/// pre-increment operator++
			/// Increments the AtomicInt. 
			///
			/// This function doesn't obey the C++ standard in that it does not return 
			//  a reference, but rather the value of the AtomicInt after the  
			/// operation is complete. It must be noted that this design is motivated by
			/// the fact that it is unsafe to rely on the returned value being equal to 
			/// the previous value + 1, as another thread might have modified the AtomicInt 
			/// immediately after the subtraction operation.  So rather than returning the
			/// reference of AtomicInt, the function returns a copy of the AtomicInt value
			/// used in the function.
			ValueType operator++() { return ++mValue; }

			/// post-increment operator++
			/// Increments the AtomicInt and returns the value of the AtomicInt before
			/// the increment operation. 
			///
			/// This function doesn't obey the C++ standard in that it does not return 
			//  a reference, but rather the value of the AtomicInt after the  
			/// operation is complete. It must be noted that this design is motivated by
			/// the fact that it is unsafe to rely on the returned value being equal to 
			/// the previous value, as another thread might have modified the AtomicInt 
			/// immediately after the subtraction operation.  So rather than returning the
			/// reference of AtomicInt, the function returns a copy of the AtomicInt value
			/// used in the function.
			ValueType operator++(int) { return mValue++; }

			/// pre-increment operator--
			/// Decrements the AtomicInt.
			///
			/// This function doesn't obey the C++ standard in that it does not return 
			//  a reference, but rather the value of the AtomicInt after the  
			/// operation is complete. It must be noted that this design is motivated by
			/// the fact that it is unsafe to rely on the returned value being equal to 
			/// the previous value - 1, as another thread might have modified the AtomicInt 
			/// immediately after the subtraction operation.  So rather than returning the
			/// reference of AtomicInt, the function returns a copy of the AtomicInt value
			/// used in the function.
			ValueType operator--() { return --mValue; }

			/// post-increment operator--
			/// Increments the AtomicInt and returns the value of the AtomicInt before
			/// the increment operation. 
			///
			/// This function doesn't obey the C++ standard in that it does not return 
			//  a reference, but rather the value of the AtomicInt after the  
			/// operation is complete. It must be noted that this design is motivated by
			/// the fact that it is unsafe to rely on the returned value being equal to 
			/// the previous value, as another thread might have modified the AtomicInt 
			/// immediately after the subtraction operation.  So rather than returning the
			/// reference of AtomicInt, the function returns a copy of the AtomicInt value
			/// used in the function.
			ValueType operator--(int) { return mValue--;}

		private:
			std::atomic<T> mValue;
		};

	}
}


#endif // EATHREAD_ATOMIC_CPP11_H
