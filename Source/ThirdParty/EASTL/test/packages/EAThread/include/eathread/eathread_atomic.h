///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_ATOMIC_H
#define EATHREAD_EATHREAD_ATOMIC_H


#include <EABase/eabase.h>
EA_DISABLE_ALL_VC_WARNINGS()
#include <stddef.h>
EA_RESTORE_ALL_VC_WARNINGS()
#include <eathread/internal/config.h>
#include <eathread/eathread.h>
#include <eathread/eathread_sync.h>

// Urho3D: Atomics are needed regardless of threading availability
// #if !EA_THREADS_AVAILABLE
// 	// Do nothing. Let the default implementation below be used.
// //#elif defined(EA_USE_CPP11_CONCURRENCY) && EA_USE_CPP11_CONCURRENCY
// //    #include <eathread/cpp11/eathread_atomic_cpp11.h> // CPP11 atomics are currently broken and slow.  To be renabled for other platforms when VS2013 released.
// #el
#if defined(EA_USE_COMMON_ATOMICINT_IMPLEMENTATION) && EA_USE_COMMON_ATOMICINT_IMPLEMENTATION
	#include <eathread/internal/eathread_atomic.h>
#elif defined(EA_PLATFORM_APPLE)
	#include <eathread/apple/eathread_atomic_apple.h>
#elif defined(EA_PROCESSOR_X86) || ((defined(EA_PLATFORM_WINRT) || defined(EA_PLATFORM_WINDOWS_PHONE)) && defined(EA_PROCESSOR_ARM))
	#include <eathread/x86/eathread_atomic_x86.h>
#elif defined(EA_PROCESSOR_X86_64)
	#include <eathread/x86-64/eathread_atomic_x86-64.h>
#elif defined(EA_PLATFORM_ANDROID)
	#if EATHREAD_C11_ATOMICS_AVAILABLE
		#include <eathread/android/eathread_atomic_android_c11.h>  // Android API 21+ only support C11 atomics
	#else
		#include <eathread/android/eathread_atomic_android.h>
	#endif
#elif defined(EA_COMPILER_GCC) || defined(CS_UNDEFINED_STRING)
	#include <eathread/gcc/eathread_atomic_gcc.h>
#else
	#error Platform not supported yet.
#endif

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif




// EATHREAD_ATOMIC_128_SUPPORTED
//
// Defined as 0 or 1. Defined as 1 whenever possible for the given compiler/platform combination.
// Defines if 128 bit atomic operations are supported.
// Such operations are only ever supported on 64 bit platforms.
//
#ifndef EATHREAD_ATOMIC_128_SUPPORTED           // If not defined by one of the above headers...
	#define EATHREAD_ATOMIC_128_SUPPORTED 0
#endif

namespace EA
{
	namespace Thread
	{
		enum Atomic64Implementation
		{
			kAtomic64Emulated,
			kAtomic64Native
		};

		/// SetDoubleWordAtomicsImplementation
		/// Some platforms have multiple implementations, some of which support
		/// double word atomics and some that don't. For example, certain ARM
		/// processors will support the ldrexd/strexd atomic instructions but
		/// others will not. 
		EATHREADLIB_API void SetAtomic64Implementation(Atomic64Implementation implementation);
	}
}


#if !defined(EA_THREAD_ATOMIC_IMPLEMENTED) // If there wasn't a processor-specific version already defined...

	// Fail the build if atomics aren't being defined for the given platform/compiler.
	// If we need to add an exception here, we can add an appropriate ifdef.
	static_assert(false, "atomic operations must be defined for this platform.");

	namespace EA
	{
		namespace Thread
		{
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
			class EATHREADLIB_API AtomicInt
			{
			public:
				/// ThisType
				/// A typedef for this class type itself, for usage clarity.
				typedef AtomicInt<T> ThisType;


				/// ValueType
				/// A typedef for the basic object we work with. 
				typedef T ValueType;


				/// AtomicInt
				/// Empty constructor. Intentionally leaves mValue in an unspecified state.
				/// This is done so that an AtomicInt acts like a standard built-in integer.
				AtomicInt()
					{}


				/// AtomicInt
				/// Constructs with an intial value. 
				AtomicInt(ValueType n)
					: mValue(n) {}


				/// AtomicInt
				/// Copy ctor. Uses GetValue to read the value, and thus is synchronized. 
				AtomicInt(const ThisType& x)
					: mValue(x.GetValue()) {}


				/// AtomicInt
				/// Assignment operator. Uses GetValue to read the value, and thus is synchronized. 
				AtomicInt& operator=(const ThisType& x)
					{ mValue = x.GetValue(); return *this; }


				/// GetValue
				/// Safely gets the current value. A platform-specific version of 
				/// this might need to do something more than just read the value.
				ValueType GetValue() const
					{ return mValue; }


				/// GetValueRaw
				/// "Unsafely" gets the current value. This is useful for algorithms 
				/// that want to poll the value in a high performance way before 
				/// reading or setting the value in a more costly thread-safe way. 
				/// You should not use this function when attempting to do thread-safe
				/// atomic operations.
				ValueType GetValueRaw() const
					{ return mValue; }


				/// SetValue
				/// Safely sets a new value. Returns the old value. Note that due to 
				/// expected multithreaded accesses, a call to GetValue after SetValue
				/// might return a different value then what was set with SetValue.
				/// This of course depends on your situation.
				ValueType SetValue(ValueType n)
				{
					const ValueType nOldValue(mValue);
					mValue = n;
					return nOldValue;
				}


				/// SetValueConditional
				/// Safely set the value to a new value if the original value is equal to 
				/// a condition value. Returns true if the condition was met and the 
				/// assignment occurred. The comparison and value setting are done as
				/// an atomic operation and thus another thread cannot intervene between
				/// the two as would be the case with simple C code.
				bool SetValueConditional(ValueType n, ValueType condition)
				{
					if(mValue == condition) 
					{
						mValue = n;
						return true;
					}
					return false;
				}


				/// Increment
				/// Safely increments the value. Returns the new value.
				/// This function acts the same as the C++ pre-increment operator.
				ValueType Increment()
					{ return ++mValue; }


				/// Decrement
				/// Safely decrements the value. Returns the new value.
				/// This function acts the same as the C++ pre-decrement operator.
				ValueType Decrement()
					{ return --mValue; }


				/// Add
				/// Safely adds a value, which can be negative. Returns the new value.
				/// You can implement subtraction with this function by using a negative argument.
				ValueType Add(ValueType n)
					{ return (mValue += n); }


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
				operator const ValueType() const
					 { return mValue; }

				/// operator =
				/// Assigns a new value and returns the value after the operation.
				///
				ValueType operator=(ValueType n)
				{
					 mValue = n;
					 return n;
				}

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
				ValueType operator+=(ValueType n)
				{
					 mValue += n;
					 return mValue;
				}

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
				ValueType operator-=(ValueType n)
				{
					 mValue -= n;
					 return mValue;
				}

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
				ValueType operator++()
					 { return ++mValue; }

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
				ValueType operator++(int)
					 { return mValue++; }

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
				ValueType operator--()
					 { return --mValue; }

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
				ValueType operator--(int)
					 { return mValue--;}

			protected:
				volatile ValueType mValue; /// May not be the same on all platforms.
			};


		} // namespace Thread

	} // namespace EA

#endif // #if EA_THREAD_ATOMIC_IMPLEMENTED





namespace EA
{
	namespace Thread
	{

		// Typedefs
		typedef AtomicInt<int32_t>  AtomicInt32;   /// int32_t  atomic integer.
		typedef AtomicInt<uint32_t> AtomicUint32;  /// uint32_t atomic integer.
		typedef AtomicInt<int64_t>  AtomicInt64;   /// int64_t  atomic integer.
		typedef AtomicInt<uint64_t> AtomicUint64;  /// uint64_t atomic integer.

		#if !defined(EA_PLATFORM_WORD_SIZE) || (EA_PLATFORM_WORD_SIZE == 4)
			typedef AtomicInt32  AtomicIWord;
			typedef AtomicUint32 AtomicUWord;
		#else
			typedef AtomicInt64  AtomicIWord;
			typedef AtomicUint64 AtomicUWord;
		#endif

		#if !defined(EA_PLATFORM_PTR_SIZE) || (EA_PLATFORM_PTR_SIZE == 4)
			typedef AtomicInt32  AtomicIntPtr;
			typedef AtomicUint32 AtomicUintPtr;
		#else
			typedef AtomicInt64  AtomicIntPtr;
			typedef AtomicUint64 AtomicUintPtr;
		#endif


		#ifdef _MSC_VER                  // VC++ yields spurious warnings about void* being cast to an integer type and vice-versa.
			#pragma warning(push)        // These warnings are baseless because we check for platform pointer size above.
			#pragma warning(disable: 4311 4312 4251)
		#endif


		/// class AtomicPointer
		///
		/// For simplicity of the current implementation, we simply have AtomicPointer map
		/// to AtomicInt32. This is reasonably safe because AtomicInt32 uses intptr_t
		/// as its ValueType and there are no foreseeble supported platforms in which 
		/// intptr_t will not exist or be possible as a data type.
		///
		class EATHREADLIB_API AtomicPointer : public AtomicIntPtr
		{
		public:
			typedef void* PointerValueType;

			AtomicPointer(void* p = NULL)
				: AtomicIntPtr(static_cast<ValueType>(reinterpret_cast<uintptr_t>(p))) {}

			AtomicPointer& operator=(void* p) 
				{ AtomicIntPtr::operator=(static_cast<ValueType>(reinterpret_cast<uintptr_t>(p))); return *this; }

			operator const void*() const // It's debateable whether this should be supported.
				{ return (void*)AtomicIntPtr::GetValue(); }

			void* GetValue() const
				{ return (void*)AtomicIntPtr::GetValue(); }

			void* GetValueRaw() const
				{ return (void*)AtomicIntPtr::GetValueRaw(); }

			void* SetValue(void* p)
				{ return (void*)AtomicIntPtr::SetValue(static_cast<ValueType>(reinterpret_cast<uintptr_t>(p))); }

			bool SetValueConditional(void* p, void* pCondition)
				{ return AtomicIntPtr::SetValueConditional(static_cast<ValueType>(reinterpret_cast<uintptr_t>(p)), static_cast<ValueType>(reinterpret_cast<uintptr_t>(pCondition))); }
		};


		#ifdef _MSC_VER
			#pragma warning(pop)
		#endif

	} // namespace Thread

} // namespace EA


#endif // EATHREAD_EATHREAD_ATOMIC_H













