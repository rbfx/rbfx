///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif


/////////////////////////////////////////////////////////////////////////////
// InterlockedXXX intrinsics
//
#if defined(EA_PLATFORM_MICROSOFT)
	EA_DISABLE_ALL_VC_WARNINGS()
	#include <xatomic.h>
	EA_RESTORE_ALL_VC_WARNINGS()

	extern "C" long           _InterlockedIncrement(long volatile* Addend);
	extern "C" long           _InterlockedDecrement(long volatile* Addend);
	extern "C" long           _InterlockedCompareExchange(long volatile* Dest, long Exchange, long Comp);
	extern "C" long           _InterlockedExchange(long volatile* Target, long Value);
	extern "C" long           _InterlockedExchangeAdd(long volatile* Addend, long Value);
	extern "C" int64_t        _InterlockedCompareExchange64(int64_t volatile* Dest, int64_t Exchange, int64_t Comp);

	#pragma intrinsic (_InterlockedCompareExchange)
	#define InterlockedCompareExchangeImp _InterlockedCompareExchange

	#pragma intrinsic (_InterlockedExchange)
	#define InterlockedExchangeImp        _InterlockedExchange 

	#pragma intrinsic (_InterlockedExchangeAdd)
	#define InterlockedExchangeAddImp     _InterlockedExchangeAdd

	#pragma intrinsic (_InterlockedIncrement)
	#define InterlockedIncrementImp       _InterlockedIncrement

	#pragma intrinsic (_InterlockedDecrement)
	#define InterlockedDecrementImp       _InterlockedDecrement

	#pragma intrinsic (_InterlockedCompareExchange64)
	#define InterlockedCompareExchange64Imp _InterlockedCompareExchange64

	inline bool InterlockedSetIfEqual(volatile int64_t* dest, int64_t newValue, int64_t condition)
	{
		return (InterlockedCompareExchange64Imp(dest, newValue, condition) == condition);
	}

	inline bool InterlockedSetIfEqual(volatile uint64_t* dest, uint64_t newValue, uint64_t condition)
	{
		return (InterlockedCompareExchange64Imp((int64_t volatile*)dest, (int64_t)newValue, (int64_t)condition) == (int64_t)condition);
	}

	#ifndef InterlockedCompareExchangeImp // If the above intrinsics aren't used... 
		extern "C" __declspec(dllimport) long __stdcall InterlockedIncrement(long volatile * pAddend);
		extern "C" __declspec(dllimport) long __stdcall InterlockedDecrement(long volatile * pAddend);
		extern "C" __declspec(dllimport) long __stdcall InterlockedExchange(long volatile * pTarget, long value);
		extern "C" __declspec(dllimport) long __stdcall InterlockedExchangeAdd(long volatile * pAddend, long value);
		extern "C" __declspec(dllimport) long __stdcall InterlockedCompareExchange(long volatile * pDestination, long value, long compare);

		#define InterlockedCompareExchangeImp InterlockedCompareExchange
		#define InterlockedExchangeImp        InterlockedExchange
		#define InterlockedExchangeAddImp     InterlockedExchangeAdd
		#define InterlockedIncrementImp       InterlockedIncrement
		#define InterlockedDecrementImp       InterlockedDecrement
	#endif

	#if defined(EA_PROCESSOR_X86)
		#define _InterlockedExchange64		_InterlockedExchange64_INLINE
		#define _InterlockedExchangeAdd64	_InterlockedExchangeAdd64_INLINE
		#define _InterlockedAnd64			_InterlockedAnd64_INLINE
		#define _InterlockedOr64			_InterlockedOr64_INLINE
		#define _InterlockedXor64			_InterlockedXor64_INLINE
	#endif
#endif // EA_PLATFORM_MICROSOFT




namespace EA
{
namespace Thread
{

namespace detail
{
	template<class T>
	inline T AtomicGetValue(volatile T* ptr);
} // namespace detail

// int
inline int AtomicGetValue(volatile int* ptr) { return detail::AtomicGetValue(ptr); }
inline int AtomicGetValue(const volatile int* ptr) { return AtomicGetValue(const_cast<volatile int*>(ptr)); }
inline int AtomicSetValue(volatile int* ptr, int value) { return _InterlockedExchange((long*)ptr, (long)value); }  
inline int AtomicFetchIncrement(volatile int* ptr) { return static_cast<int>(_InterlockedIncrement((long*)ptr)) - 1; }
inline int AtomicFetchDecrement(volatile int* ptr) { return static_cast<int>(_InterlockedDecrement((long*)ptr)) + 1; }
inline int AtomicFetchAdd(volatile int* ptr, int value) { return static_cast<int>(_InterlockedExchangeAdd((long*)ptr, (long)value)); }
inline int AtomicFetchSub(volatile int* ptr, int value) { return static_cast<int>(_InterlockedExchangeAdd((long*)ptr, -(long)value)); }
inline int AtomicFetchOr(volatile int* ptr, int value) { return static_cast<int>(_InterlockedOr((long*)ptr, (long)value)); }
inline int AtomicFetchAnd(volatile int* ptr, int value) { return static_cast<int>(_InterlockedAnd((long*)ptr, (long)value)); }
inline int AtomicFetchXor(volatile int* ptr, int value) { return static_cast<int>(_InterlockedXor((long*)ptr, (long)value)); }
inline int AtomicFetchSwap(volatile int* ptr, int value) { return static_cast<int>(_InterlockedExchange((long*)ptr, (long)value)); }
inline int AtomicFetchSwapConditional(volatile int* ptr, int value, int condition) { return _InterlockedCompareExchange((long*)ptr, (long)value, (long)condition); }
inline bool AtomicSetValueConditional(volatile int* ptr, int value, int condition) { return _InterlockedCompareExchange((long*)ptr, (long)value, (long)condition) == (long)condition; }

// unsigned int
inline unsigned int AtomicGetValue(volatile unsigned int* ptr) { return detail::AtomicGetValue(ptr); }
inline unsigned int AtomicGetValue(const volatile unsigned int* ptr) { return AtomicGetValue(const_cast<volatile unsigned int*>(ptr)); }
inline unsigned int AtomicSetValue(volatile unsigned int* ptr, unsigned int value) { return static_cast<unsigned int>(_InterlockedExchange((long*)ptr, (long)value)); }  
inline unsigned int AtomicFetchIncrement(volatile unsigned int* ptr) { return static_cast<unsigned int>(_InterlockedExchangeAdd((long*)ptr, (long)1)); }
inline unsigned int AtomicFetchDecrement(volatile unsigned int* ptr) { return static_cast<unsigned int>(_InterlockedExchangeAdd((long*)ptr, (long)-1)); }
inline unsigned int AtomicFetchAdd(volatile unsigned int* ptr, unsigned int value) { return static_cast<unsigned int>(_InterlockedExchangeAdd((long*)ptr, (long)value)); }
inline unsigned int AtomicFetchSub(volatile unsigned int* ptr, unsigned int value) { return static_cast<unsigned int>(_InterlockedExchangeAdd((long*)ptr, -(long)value)); }
inline unsigned int AtomicFetchOr(volatile unsigned int* ptr, unsigned int value) { return static_cast<unsigned int>(_InterlockedOr((long*)ptr, (long)value)); }
inline unsigned int AtomicFetchAnd(volatile unsigned int* ptr, unsigned int value) { return static_cast<unsigned int>(_InterlockedAnd((long*)ptr, (long)value)); }
inline unsigned int AtomicFetchXor(volatile unsigned int* ptr, unsigned int value) { return static_cast<unsigned int>(_InterlockedXor((long*)ptr, (long)value)); }
inline unsigned int AtomicFetchSwap(volatile unsigned int* ptr, unsigned int value) { return static_cast<unsigned int>(_InterlockedExchange((long*)ptr, (long)value)); }
inline unsigned int AtomicFetchSwapConditional(volatile unsigned int* ptr, unsigned int value, unsigned int condition) { return (unsigned int)_InterlockedCompareExchange((long*)ptr, (long)value, (long)condition); }
inline bool AtomicSetValueConditional(volatile unsigned int* ptr, unsigned int value, unsigned int condition) { return _InterlockedCompareExchange((long*)ptr, (long)value, (long)condition) == (long)condition; }

// short
inline short AtomicGetValue(volatile short* ptr) { return detail::AtomicGetValue(ptr); }
inline short AtomicGetValue(const volatile short* ptr) { return AtomicGetValue(const_cast<volatile short*>(ptr)); }
inline short AtomicSetValue(volatile short* ptr, short value) { return static_cast<short>(_InterlockedExchange16((short*)ptr, (short)value)); }
inline short AtomicFetchIncrement(volatile short* ptr) { return static_cast<short>(_InterlockedExchangeAdd16((short*)ptr, (short)1)); }
inline short AtomicFetchDecrement(volatile short* ptr) { return static_cast<short>(_InterlockedExchangeAdd16((short*)ptr, (short)-1)); }
inline short AtomicFetchAdd(volatile short* ptr, short value) { return static_cast<short>(_InterlockedExchangeAdd16((short*)ptr, (short)value)); }
inline short AtomicFetchSub(volatile short* ptr, short value) {  return static_cast<short>(_InterlockedExchangeAdd16((short*)ptr, -value)); } 
inline short AtomicFetchOr(volatile short* ptr, short value) { return static_cast<short>(_InterlockedOr16((short*)ptr, (short)value)); }
inline short AtomicFetchAnd(volatile short* ptr, short value) { return static_cast<short>(_InterlockedAnd16((short*)ptr, (short)value)); }
inline short AtomicFetchXor(volatile short* ptr, short value) { return static_cast<short>(_InterlockedXor16((short*)ptr, (short)value)); }
inline short AtomicFetchSwap(volatile short* ptr, short value) { return static_cast<short>(_InterlockedExchange16((short*)ptr, (short)value)); }
inline short AtomicFetchSwapConditional(volatile short* ptr, short value, short condition) { return _InterlockedCompareExchange16(ptr, value, condition); }
inline bool AtomicSetValueConditional(volatile short* ptr, short value, short condition) { return _InterlockedCompareExchange16(ptr, value, condition) == condition; }

// unsigned short
inline unsigned short AtomicGetValue(volatile unsigned short* ptr) { return detail::AtomicGetValue(ptr); }
inline unsigned short AtomicGetValue(const volatile unsigned short* ptr) { return AtomicGetValue(const_cast<volatile unsigned short*>(ptr)); }
inline unsigned short AtomicSetValue(volatile unsigned short* ptr, unsigned short value) { return static_cast<unsigned short>(_InterlockedExchange16((short*)ptr, (short)value)); }
inline unsigned short AtomicFetchIncrement(volatile unsigned short* ptr) { return static_cast<unsigned short>(_InterlockedExchangeAdd16((short*)ptr, (short)1)); }
inline unsigned short AtomicFetchDecrement(volatile unsigned short* ptr) { return static_cast<unsigned short>(_InterlockedExchangeAdd16((short*)ptr, (short)-1)); }
inline unsigned short AtomicFetchAdd(volatile unsigned short* ptr, unsigned short value) { return static_cast<unsigned short>(_InterlockedExchangeAdd16((short*)ptr, (short)value)); }
inline unsigned short AtomicFetchSub(volatile unsigned short* ptr, unsigned short value) { return static_cast<unsigned short>(_InterlockedExchangeAdd16((short*)ptr, -(short)value)); }
inline unsigned short AtomicFetchOr(volatile unsigned short* ptr, unsigned short value) { return static_cast<unsigned short>(_InterlockedOr16((short*)ptr, (short)value)); }
inline unsigned short AtomicFetchAnd(volatile unsigned short* ptr, unsigned short value) { return static_cast<unsigned short>(_InterlockedAnd16((short*)ptr, (short)value)); }
inline unsigned short AtomicFetchXor(volatile unsigned short* ptr, unsigned short value) { return static_cast<unsigned short>(_InterlockedXor16((short*)ptr, (short)value)); }
inline unsigned short AtomicFetchSwap(volatile unsigned short* ptr, unsigned short value) { return static_cast<unsigned short>(_InterlockedExchange16((short*)ptr, (short)value)); }
inline unsigned short AtomicFetchSwapConditional(volatile unsigned short* ptr, unsigned short value, unsigned short condition) { return (unsigned short)_InterlockedCompareExchange16((short*)ptr, (short)value, (short)condition); }
inline bool AtomicSetValueConditional(volatile unsigned short* ptr, unsigned short value, unsigned short condition) { return _InterlockedCompareExchange16((short*)ptr, (short)value, (short)condition) == (short)condition; }

// long
inline long AtomicGetValue(volatile long* ptr) { return detail::AtomicGetValue(ptr); }
inline long AtomicGetValue(const volatile long* ptr) { return AtomicGetValue(const_cast<volatile long*>(ptr)); }
inline long AtomicSetValue(volatile long* ptr, long value) { return _InterlockedExchange(ptr, value); }
inline long AtomicFetchIncrement(volatile long* ptr) { return _InterlockedIncrement(ptr) - 1; }
inline long AtomicFetchDecrement(volatile long* ptr) { return _InterlockedDecrement(ptr) + 1; }
inline long AtomicFetchAdd(volatile long* ptr, long value)  { return _InterlockedExchangeAdd(ptr, value); }
inline long AtomicFetchSub(volatile long* ptr, long value) { return _InterlockedExchangeAdd(ptr, -value); }
inline long AtomicFetchOr(volatile long* ptr, long value)   { return _InterlockedOr(ptr, value); }
inline long AtomicFetchAnd(volatile long* ptr, long value)  { return _InterlockedAnd(ptr, value); }
inline long AtomicFetchXor(volatile long* ptr, long value)  { return _InterlockedXor(ptr, value); }
inline long AtomicFetchSwap(volatile long* ptr, long value) { return _InterlockedExchange(ptr, value); }
inline long AtomicFetchSwapConditional(volatile long* ptr, long value, long condition) { return _InterlockedCompareExchange(ptr, value, condition); }
inline bool AtomicSetValueConditional(volatile long* ptr, long value, long condition) { return _InterlockedCompareExchange(ptr, value, condition) == condition; }

// unsigned long
inline unsigned long AtomicGetValue(volatile unsigned long* ptr) { return detail::AtomicGetValue(ptr); }
inline unsigned long AtomicGetValue(const volatile unsigned long* ptr) { return AtomicGetValue(const_cast<volatile unsigned long*>(ptr)); }
inline unsigned long AtomicSetValue(volatile unsigned long* ptr, unsigned long value) { return static_cast<unsigned long>(_InterlockedExchange((long*)ptr, (long)value)); }
inline unsigned long AtomicFetchIncrement(volatile unsigned long* ptr) { return static_cast<unsigned long>(_InterlockedIncrement((long*)ptr)) - 1; }
inline unsigned long AtomicFetchDecrement(volatile unsigned long* ptr) { return static_cast<unsigned long>(_InterlockedDecrement((long*)ptr)) + 1; }
inline unsigned long AtomicFetchAdd(volatile unsigned long* ptr, unsigned long value) { return static_cast<unsigned long>(_InterlockedExchangeAdd((long*)ptr, (long)value)); }
inline unsigned long AtomicFetchSub(volatile unsigned long* ptr, unsigned long value) { return static_cast<unsigned long>(_InterlockedExchangeAdd((long*)ptr, -(long)value)); }
inline unsigned long AtomicFetchOr(volatile unsigned long* ptr, unsigned long value) { return static_cast<unsigned long>(_InterlockedOr((long*)ptr, (long)value)); }
inline unsigned long AtomicFetchAnd(volatile unsigned long* ptr, unsigned long value) { return static_cast<unsigned long>(_InterlockedAnd((long*)ptr, (long)value)); }
inline unsigned long AtomicFetchXor(volatile unsigned long* ptr, unsigned long value) { return static_cast<unsigned long>(_InterlockedXor((long*)ptr, (long)value)); }
inline unsigned long AtomicFetchSwap(volatile unsigned long* ptr, unsigned long value) { return static_cast<unsigned long>(_InterlockedExchange((long*)ptr, (long)value)); }
inline unsigned long AtomicFetchSwapConditional(volatile unsigned long* ptr, unsigned long value, unsigned long condition) { return static_cast<unsigned long>(_InterlockedCompareExchange((long*)ptr, (long)value, (long)condition)); }
inline bool AtomicSetValueConditional(volatile unsigned long* ptr, unsigned long value, unsigned long condition) { return static_cast<unsigned long>(_InterlockedCompareExchange((long*)ptr, (long)value, (long)condition)) == condition; }

// char32_t
#if EA_CHAR32_NATIVE
	inline char32_t AtomicGetValue(volatile char32_t* ptr) { return detail::AtomicGetValue(ptr); }
	inline char32_t AtomicGetValue(const volatile char32_t* ptr) { return AtomicGetValue(const_cast<volatile char32_t*>(ptr)); }
    inline char32_t AtomicSetValue(volatile char32_t* ptr, char32_t value) { return static_cast<char32_t>(_InterlockedExchange((long*)ptr, (long)value)); }
	inline char32_t AtomicFetchIncrement(volatile char32_t* ptr) { return static_cast<char32_t>(_InterlockedExchangeAdd((long*)ptr, (long)1)); }
	inline char32_t AtomicFetchDecrement(volatile char32_t* ptr) { return static_cast<char32_t>(_InterlockedExchangeAdd((long*)ptr, (long)-1)); }
	inline char32_t AtomicFetchAdd(volatile char32_t* ptr, char32_t value) { return static_cast<char32_t>(_InterlockedExchangeAdd((long*)ptr, (long)value)); }
	inline char32_t AtomicFetchSub(volatile char32_t* ptr, char32_t value) { return static_cast<char32_t>(_InterlockedExchangeAdd((long*)ptr, -(long)value)); }
	inline char32_t AtomicFetchOr(volatile char32_t* ptr, char32_t value) { return static_cast<char32_t>(_InterlockedOr((long*)ptr, (long)value)); }
	inline char32_t AtomicFetchAnd(volatile char32_t* ptr, char32_t value) { return static_cast<char32_t>(_InterlockedAnd((long*)ptr, (long)value)); }
	inline char32_t AtomicFetchXor(volatile char32_t* ptr, char32_t value) { return static_cast<char32_t>(_InterlockedXor((long*)ptr, (long)value)); }
	inline char32_t AtomicFetchSwap(volatile char32_t* ptr, char32_t value) { return static_cast<char32_t>(_InterlockedExchange((long*)ptr, (long)value)); }
	inline char32_t AtomicFetchSwapConditional(volatile char32_t* ptr, char32_t value, unsigned int condition) { return static_cast<char32_t>(_InterlockedCompareExchange((long*)ptr, (long)value, (long)condition)); }
	inline bool AtomicSetValueConditional(volatile char32_t* ptr, char32_t value, unsigned int condition) { return _InterlockedCompareExchange((long*)ptr, (long)value, (long)condition) == (long)condition; }
#endif


// long long
inline long long AtomicGetValue(volatile long long* ptr) { return detail::AtomicGetValue(ptr); }
inline long long AtomicGetValue(const volatile long long* ptr) { return AtomicGetValue(const_cast<volatile long long*>(ptr)); }
inline long long AtomicSetValue(volatile long long* ptr, long long value) { return static_cast<long long>(_InterlockedExchange64(ptr, value)); }  
inline long long AtomicFetchIncrement(volatile long long* ptr) { return static_cast<long long>(_InterlockedExchangeAdd64(ptr, (long long)1)); }
inline long long AtomicFetchDecrement(volatile long long* ptr) { return static_cast<long long>(_InterlockedExchangeAdd64(ptr, (long long)-1)); }
inline long long AtomicFetchAdd(volatile long long* ptr, long long value) { return static_cast<long long>(_InterlockedExchangeAdd64(ptr, value)); }
inline long long AtomicFetchSub(volatile long long* ptr, long long value) { return static_cast<long long>(_InterlockedExchangeAdd64(ptr, -(long long)value)); }
inline long long AtomicFetchOr(volatile long long* ptr, long long value) { return static_cast<long long>(_InterlockedOr64(ptr, value)); }
inline long long AtomicFetchAnd(volatile long long* ptr, long long value) { return static_cast<long long>(_InterlockedAnd64(ptr, value)); }
inline long long AtomicFetchXor(volatile long long* ptr, long long value) { return static_cast<long long>(_InterlockedXor64(ptr, value)); }
inline long long AtomicFetchSwap(volatile long long* ptr, long long value) { return static_cast<long long>(_InterlockedExchange64(ptr, value)); }
inline long long AtomicFetchSwapConditional(volatile long long* ptr, long long value, long long condition) { return _InterlockedCompareExchange64(ptr, value, condition); }
inline bool AtomicSetValueConditional(volatile long long* ptr, long long value, long long condition) { return _InterlockedCompareExchange64(ptr, value, condition) == condition; }

// unsigned long long 
inline unsigned long long AtomicGetValue(volatile unsigned long long* ptr) { return detail::AtomicGetValue(ptr); }
inline unsigned long long AtomicGetValue(const volatile unsigned long long* ptr) { return AtomicGetValue(const_cast<volatile unsigned long long*>(ptr)); }
inline unsigned long long AtomicSetValue(volatile unsigned long long* ptr, unsigned long long value) { return static_cast<unsigned long long>(_InterlockedExchange64(reinterpret_cast<volatile long long*>(ptr), (long long)value)); }  
inline unsigned long long AtomicFetchIncrement(volatile unsigned long long* ptr) { return static_cast<unsigned long long>(_InterlockedExchangeAdd64(reinterpret_cast<volatile long long*>(ptr), (long long)1)); }
inline unsigned long long AtomicFetchDecrement(volatile unsigned long long* ptr) { return static_cast<unsigned long long>(_InterlockedExchangeAdd64(reinterpret_cast<volatile long long*>(ptr), (long long)-1)); }
inline unsigned long long AtomicFetchAdd(volatile unsigned long long* ptr, unsigned long long value) { return static_cast<unsigned long long>(_InterlockedExchangeAdd64(reinterpret_cast<volatile long long*>(ptr), (long long)value)); }
inline unsigned long long AtomicFetchSub(volatile unsigned long long* ptr, unsigned long long value) { return static_cast<unsigned long long>(_InterlockedExchangeAdd64(reinterpret_cast<volatile long long*>(ptr), -(long long)value)); }
inline unsigned long long AtomicFetchOr(volatile unsigned long long* ptr, unsigned long long value) { return static_cast<unsigned long long>(_InterlockedOr64(reinterpret_cast<volatile long long*>(ptr), (long long)value)); }
inline unsigned long long AtomicFetchAnd(volatile unsigned long long* ptr, unsigned long long value) { return static_cast<unsigned long long>(_InterlockedAnd64(reinterpret_cast<volatile long long*>(ptr),(long long) value)); }
inline unsigned long long AtomicFetchXor(volatile unsigned long long* ptr, unsigned long long value) { return static_cast<unsigned long long>(_InterlockedXor64(reinterpret_cast<volatile long long*>(ptr),(long long) value)); }
inline unsigned long long AtomicFetchSwap(volatile unsigned long long* ptr, unsigned long long value) { return static_cast<unsigned long long>(_InterlockedExchange64(reinterpret_cast<volatile long long*>(ptr),(long long) value)); }
inline unsigned long long AtomicFetchSwapConditional(volatile unsigned long long* ptr, unsigned long long value, unsigned long long condition) { return static_cast<unsigned long long>(_InterlockedCompareExchange64(reinterpret_cast<volatile long long*>(ptr), (long long)value, (long long)condition)); }
inline bool AtomicSetValueConditional(volatile unsigned long long* ptr, unsigned long long value, unsigned long long condition) { return static_cast<unsigned long long>(_InterlockedCompareExchange64(reinterpret_cast<volatile long long*>(ptr), (long long)value, (long long)condition)) == condition; }


namespace detail
{
	template<class T>
	inline T AtomicGetValue(volatile T* ptr)
	{
	#if EA_PLATFORM_WORD_SIZE >= 8 && defined(EA_PROCESSOR_X86_64)
		EATHREAD_ALIGNMENT_CHECK(ptr);
		EACompilerMemoryBarrier();
		T value = *ptr;
		EACompilerMemoryBarrier();
		return value;
	#else
		return AtomicFetchAdd(ptr, T(0));
	#endif
	}
} // namespace detail


} // namespace Thread
} // namespace EA

