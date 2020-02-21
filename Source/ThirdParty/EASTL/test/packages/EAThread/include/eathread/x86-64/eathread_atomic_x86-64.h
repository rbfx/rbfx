///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

/////////////////////////////////////////////////////////////////////////////
// Defines functionality for threadsafe primitive operations.
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_X86_64_EATHREAD_ATOMIC_X86_64_H
#define EATHREAD_X86_64_EATHREAD_ATOMIC_X86_64_H

#include "EABase/eabase.h"
#include <stddef.h>
#include <eathread/internal/eathread_atomic_standalone.h>


#ifdef _MSC_VER
	#pragma warning(push, 0)
	#include <math.h>   // VS2008 has an acknowledged bug that requires math.h (and possibly also string.h) to be #included before intrin.h.
	#include <intrin.h>
	#pragma warning(pop)

	#pragma warning(push)
	#pragma warning(disable: 4146)  // unary minus operator applied to unsigned type, result still unsigned
#endif


#if defined(EA_PROCESSOR_X86_64)

	#define EA_THREAD_ATOMIC_IMPLEMENTED

	namespace EA
	{
		namespace Thread
		{
			///
			/// Non-member 128-bit Atomics implementation 
			///
			#if (_MSC_VER >= 1500) // VS2008+

				#define EATHREAD_ATOMIC_128_SUPPORTED 1

				// Algorithm for implementing an arbitrary atomic modification via AtomicCompareAndSwap:
				//     int128_t oldValue;
				//
				//     do {
				//         oldValue = AtomicGetValue(dest);
				//         newValue = <modification of oldValue>
				//     } while(!AtomicCompareAndSwap(dest, oldValue, newValue));
 
				// The following function is a wrapper for the Microsoft _InterlockedCompareExchange128 function.
				// Early versions of AMD 64-bit hardware do not support 128 bit atomics. To check for hardware support 
				// for the cmpxchg16b instruction, call the __cpuid intrinsic with InfoType=0x00000001 (standard function 1). 
				// Bit 13 of CPUInfo[2] (ECX) is 1 if the instruction is supported.

				inline bool AtomicSetValueConditionall28(volatile int64_t* dest128, const int64_t* value128, const int64_t* condition128)
				{
					__int64 conditionCopy[2] = { condition128[0], condition128[1] };                              // We make a copy because Microsoft modifies the output, which is inconsistent with the rest of our atomic API.
					return _InterlockedCompareExchange128(dest128, value128[1], value128[0], conditionCopy) == 1; // Question: Do we need to reverse the order of value128 if running on big-endian? Microsoft's documentation currently doesn't address this.
				}

				inline bool AtomicSetValueConditionall28(volatile uint64_t* dest128, const uint64_t* value128, const uint64_t* condition128)
				{ 
					__int64 conditionCopy[2] = { (int64_t) condition128[0],  (int64_t)condition128[1] };                                               // We make a copy because Microsoft modifies the output, which is inconsistent with the rest of our atomic API.
					return _InterlockedCompareExchange128((volatile int64_t*)dest128, (int64_t)value128[1], (int64_t)value128[0], conditionCopy) == 1; // Question: Do we need to reverse the order of value128 if running on big-endian? Microsoft's documentation currently doesn't address this.
				}

			#elif defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)

				#if defined(EA_COMPILER_CLANG) || (defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 403)) // GCC 4.3 or later for 128 bit atomics

					#define EATHREAD_ATOMIC_128_SUPPORTED 1

					// GCC on x64 implements all of its __sync functions below via the cmpxchg16b instruction,
					// usually in the form of a loop.
					// Use of 128 bit atomics on GCC requires compiling with the -mcx16 compiler argument. 
					// See http://gcc.gnu.org/onlinedocs/gcc/i386-and-x86_002d64-Options.html.

					inline __int128_t AtomicGetValue(volatile __int128_t* source)
					{
						return __sync_add_and_fetch(source, __int128_t(0)); // Is there a better way to do an atomic read?
					}

					inline void AtomicSetValue(volatile __int128_t* dest, __int128_t value)
					{
						__sync_lock_test_and_set(dest, value);
					}

					inline __int128_t AtomicIncrement(volatile __int128_t* dest)
					{
						return __sync_add_and_fetch(dest, __int128_t(1));
					}

					inline __int128_t AtomicDecrement(volatile __int128_t* dest)
					{
						return __sync_add_and_fetch(dest, __int128_t(-1));
					}

					inline __int128_t AtomicAdd(volatile __int128_t* dest, __int128_t value)
					{
						return __sync_add_and_fetch(dest, value);
					}

					inline __int128_t AtomicOr(volatile __int128_t* dest, __int128_t value)
					{
						return __sync_or_and_fetch(dest, value);
					}

					inline __int128_t AtomicAnd(volatile __int128_t* dest, __int128_t value)
					{
						return __sync_and_and_fetch(dest, value);
					}

					inline __int128_t AtomicXor(volatile __int128_t* dest, __int128_t value)
					{
						return __sync_xor_and_fetch(dest, value);
					}

					inline __int128_t AtomicSwap(volatile __int128_t* dest, __int128_t value)
					{
						return __sync_lock_test_and_set(dest, value);
					}

					inline bool AtomicSetValueConditional(volatile __int128_t* dest, __int128_t value, __int128_t condition)
					{
						return __sync_bool_compare_and_swap(dest, condition, value);
					}

					inline bool AtomicSetValueConditional(volatile __uint128_t* dest, __uint128_t value, __uint128_t condition)
					{
						return __sync_bool_compare_and_swap(dest, condition, value);
					}

					// The following 64-bit-based 128 bit atomic is provided for compatibility with the Microsoft version.
					// GCC supports the native __int128_t data type and thus can support a 128-bit-based 128 bit atomic.

					inline bool AtomicSetValueConditionall28(volatile int64_t* dest128, const int64_t* value128, const int64_t* condition128)
					{
						// Use of this requires compiling with the -mcx16 compiler argument. See http://gcc.gnu.org/onlinedocs/gcc/i386-and-x86_002d64-Options.html.
						return __sync_bool_compare_and_swap((volatile __int128_t*)dest128, *(volatile __int128_t*)condition128, *(volatile __int128_t*)value128);
					}

					inline bool AtomicSetValueConditionall28(volatile uint64_t* dest128, const uint64_t* value128, const uint64_t* condition128)
					{
						// Use of this requires compiling with the -mcx16 compiler argument. See http://gcc.gnu.org/onlinedocs/gcc/i386-and-x86_002d64-Options.html.
						return __sync_bool_compare_and_swap((volatile __uint128_t*)dest128, *(volatile __uint128_t*)condition128, *(volatile __uint128_t*)value128);
					}

				#endif

			#endif



			/// class AtomicInt
			/// Actual implementation may vary per platform. May require certain alignments, sizes, 
			/// and declaration specifications per platform.

			template <class T>
			class  AtomicInt
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

				ValueType GetValueRaw() const
					{ return mValue; }

				ValueType GetValue() const;
				ValueType SetValue(ValueType n);
				bool      SetValueConditional(ValueType n, ValueType condition);
				ValueType Increment();
				ValueType Decrement();
				ValueType Add(ValueType n);

				// operators
				inline            operator const ValueType() const { return GetValue(); }  // Should this be provided? Is it safe enough? Return value of 'const' attempts to make this safe from misuse.
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


			#if defined(EA_COMPILER_MSVC)
				#pragma intrinsic(_InterlockedExchange)
				#pragma intrinsic(_InterlockedExchangeAdd)
				#pragma intrinsic(_InterlockedCompareExchange)
				#pragma intrinsic(_InterlockedIncrement)
				#pragma intrinsic(_InterlockedDecrement)
				#pragma intrinsic(_InterlockedExchange64)
				#pragma intrinsic(_InterlockedExchangeAdd64)
				#pragma intrinsic(_InterlockedCompareExchange64)
				#pragma intrinsic(_InterlockedIncrement64)
				#pragma intrinsic(_InterlockedDecrement64)

				// The following should work under any compiler, including such compilers as GCC under
				// WINE or some other Win32 emulation. Win32 InterlockedXXX functions must exist on
				// any system that supports the Windows API, be it 32 or 64 bit Windows.

				// 32 bit versions
				template<> inline
				AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::GetValue() const
					{ return (ValueType)_InterlockedExchangeAdd((long*)&mValue, 0); } // We shouldn't need to do this, as far as I know, given the x86 architecture.

				template<> inline
				AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::GetValue() const
					{ return (ValueType)_InterlockedExchangeAdd((long*)&mValue, 0); } // We shouldn't need to do this, as far as I know, given the x86 architecture.

				template<> inline
				AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::SetValue(ValueType n)
					{ return (ValueType)_InterlockedExchange((long*)&mValue, (long)n); } // Even though we shouldn't need to use _InterlockedExchange on x86, the intrinsic x86 _InterlockedExchange is at least as fast as C code we would otherwise put here.

				template<> inline
				AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::SetValue(ValueType n)
					{ return (ValueType)_InterlockedExchange((long*)&mValue, (long)n); } // Even though we shouldn't need to use _InterlockedExchange on x86, the intrinsic x86 _InterlockedExchange is at least as fast as C code we would otherwise put here.

				template<> inline
				bool AtomicInt<int32_t>::SetValueConditional(ValueType n, ValueType condition)
					{ return ((ValueType)_InterlockedCompareExchange((long*)&mValue, (long)n, (long)condition) == condition); }

				template<> inline
				bool AtomicInt<uint32_t>::SetValueConditional(ValueType n, ValueType condition)
					{ return ((ValueType)_InterlockedCompareExchange((long*)&mValue, (long)n, (long)condition) == condition); }

				template<> inline
				AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Increment()
					{ return (ValueType)_InterlockedIncrement((long*)&mValue); }

				template<> inline
				AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Increment()
					{ return (ValueType)_InterlockedIncrement((long*)&mValue); }

				template<> inline
				AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Decrement()
					{ return (ValueType)_InterlockedDecrement((long*)&mValue); }

				template<> inline
				AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Decrement()
					{ return (ValueType)_InterlockedDecrement((long*)&mValue); }

				template<> inline
				AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Add(ValueType n)
					{ return ((ValueType)_InterlockedExchangeAdd((long*)&mValue, (long)n) + n); }

				template<> inline
				AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Add(ValueType n)
					{ return ((ValueType)_InterlockedExchangeAdd((long*)&mValue, (long)n) + n); }



				// 64 bit versions
				template<> inline
				AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::GetValue() const
					{ return (ValueType)_InterlockedExchangeAdd64((__int64*)&mValue, 0); } // We shouldn't need to do this, as far as I know, given the x86 architecture.

				template<> inline
				AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::GetValue() const
					{ return (ValueType)_InterlockedExchangeAdd64((__int64*)&mValue, 0); } // We shouldn't need to do this, as far as I know, given the x86 architecture.

				template<> inline
				AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::SetValue(ValueType n)
					{ return (ValueType)_InterlockedExchange64((__int64*)&mValue, (__int64)n); } // Even though we shouldn't need to use _InterlockedExchange on x86, the intrinsic x86 _InterlockedExchange is at least as fast as C code we would otherwise put here.

				template<> inline
				AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::SetValue(ValueType n)
					{ return (ValueType)_InterlockedExchange64((__int64*)&mValue, (__int64)n); } // Even though we shouldn't need to use _InterlockedExchange on x86, the intrinsic x86 _InterlockedExchange is at least as fast as C code we would otherwise put here.

				template<> inline
				bool AtomicInt<int64_t>::SetValueConditional(ValueType n, ValueType condition)
					{ return ((ValueType)_InterlockedCompareExchange64((__int64*)&mValue, (__int64)n, (__int64)condition) == condition); }

				template<> inline
				bool AtomicInt<uint64_t>::SetValueConditional(ValueType n, ValueType condition)
					{ return ((ValueType)_InterlockedCompareExchange64((__int64*)&mValue, (__int64)n, (__int64)condition) == condition); }

				template<> inline
				AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::Increment()
					{ return (ValueType)_InterlockedIncrement64((__int64*)&mValue); }

				template<> inline
				AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::Increment()
					{ return (ValueType)_InterlockedIncrement64((__int64*)&mValue); }

				template<> inline
				AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::Decrement()
					{ return (ValueType)_InterlockedDecrement64((__int64*)&mValue); }

				template<> inline
				AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::Decrement()
					{ return (ValueType)_InterlockedDecrement64((__int64*)&mValue); }

				template<> inline
				AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::Add(ValueType n)
					{ return ((ValueType)_InterlockedExchangeAdd64((__int64*)&mValue, (__int64)n) + n); }

				template<> inline
				AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::Add(ValueType n)
					{ return ((ValueType)_InterlockedExchangeAdd64((__int64*)&mValue, (__int64)n) + n); }


			#elif defined(EA_COMPILER_GNUC) || defined(EA_COMPILER_CLANG)

				// Recent versions of GCC have atomic primitives built into the compiler and standard library.
				#if defined(EA_COMPILER_CLANG) || (defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 401)) // GCC 4.1 or later

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

				#endif // GCC 4.1 or later

			#endif // GCC

		} // namespace Thread


	} // namespace EA


#endif // EA_PROCESSOR_X86_64


#ifdef _MSC_VER
	 #pragma warning(pop)
#endif


#endif // EATHREAD_X86_64_EATHREAD_ATOMIC_X86_64_H













