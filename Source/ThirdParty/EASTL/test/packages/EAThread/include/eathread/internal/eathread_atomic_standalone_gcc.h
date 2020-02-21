///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

namespace EA
{
namespace Thread
{

// TODO(rparolin): Consider use of clang builtin __sync_swap.
// https://clang.llvm.org/docs/LanguageExtensions.html#sync-swap

// TODO(rparolin):  Consider use of C11 atomics
// https://clang.llvm.org/docs/LanguageExtensions.html#c11-atomic-builtins

namespace detail
{
	template<class T>
	inline T AtomicGetValue(volatile T* ptr);
} // namespace detail

// int
inline int AtomicGetValue(volatile int* ptr) { return detail::AtomicGetValue(ptr); }
inline int AtomicGetValue(const volatile int* ptr) { return AtomicGetValue(const_cast<volatile int*>(ptr)); }
inline int AtomicSetValue(volatile int* dest, int value) { return __sync_lock_test_and_set(dest, value); }
inline int AtomicFetchIncrement(volatile int* dest) { return __sync_fetch_and_add(dest, int(1)); }
inline int AtomicFetchDecrement(volatile int* dest) { return __sync_fetch_and_add(dest, int(-1)); }
inline int AtomicFetchAdd(volatile int* dest, int value) { return __sync_fetch_and_add(dest, value); }
inline int AtomicFetchSub(volatile int* dest, int value) { return __sync_fetch_and_sub(dest, value); }
inline int AtomicFetchOr(volatile int* dest, int value) { return __sync_fetch_and_or(dest, value); }
inline int AtomicFetchAnd(volatile int* dest, int value) { return __sync_fetch_and_and(dest, value); }
inline int AtomicFetchXor(volatile int* dest, int value) { return __sync_fetch_and_xor(dest, value); }
inline int AtomicFetchSwap(volatile int* dest, int value) { return __sync_lock_test_and_set(dest, value); }
inline int AtomicFetchSwapConditional(volatile int* dest, int value, int condition) { return __sync_val_compare_and_swap(dest, condition, value); }
inline bool AtomicSetValueConditional(volatile int* dest, int value, int condition) { return __sync_bool_compare_and_swap(dest, condition, value); }

// unsigned int
inline unsigned int AtomicGetValue(volatile unsigned int* ptr) { return detail::AtomicGetValue(ptr); }
inline unsigned int AtomicGetValue(const volatile unsigned int* ptr) { return AtomicGetValue(const_cast<volatile unsigned int*>(ptr)); }
inline unsigned int AtomicSetValue(volatile unsigned int* dest, unsigned int value) { return __sync_lock_test_and_set(dest, value); }
inline unsigned int AtomicFetchIncrement(volatile unsigned int* dest) { return __sync_fetch_and_add(dest, (unsigned int)(1)); }
inline unsigned int AtomicFetchDecrement(volatile unsigned int* dest) { return __sync_fetch_and_add(dest, (unsigned int)(-1)); }
inline unsigned int AtomicFetchAdd(volatile unsigned int* dest, unsigned int value) { return __sync_fetch_and_add(dest, value); } 
inline unsigned int AtomicFetchSub(volatile unsigned int* dest, unsigned int value) { return __sync_fetch_and_sub(dest, value); }
inline unsigned int AtomicFetchOr(volatile unsigned int* dest, unsigned int value) { return __sync_fetch_and_or(dest, value); }
inline unsigned int AtomicFetchAnd(volatile unsigned int* dest, unsigned int value) { return __sync_fetch_and_and(dest, value); }
inline unsigned int AtomicFetchXor(volatile unsigned int* dest, unsigned int value) { return __sync_fetch_and_xor(dest, value); }
inline unsigned int AtomicFetchSwap(volatile unsigned int* dest, unsigned int value) { return __sync_lock_test_and_set(dest, value); }
inline unsigned int AtomicFetchSwapConditional(volatile unsigned int* dest, unsigned int value, unsigned int condition) { return __sync_val_compare_and_swap(dest, condition, value); }
inline bool AtomicSetValueConditional(volatile unsigned int* dest, unsigned int value, unsigned int condition) { return __sync_bool_compare_and_swap(dest, condition, value); }

// short
inline short AtomicGetValue(volatile short* ptr) { return detail::AtomicGetValue(ptr); }
inline short AtomicGetValue(const volatile short* ptr) { return AtomicGetValue(const_cast<volatile short*>(ptr)); }
inline short AtomicSetValue(volatile short* dest, short value) { return __sync_lock_test_and_set(dest, value); }
inline short AtomicFetchIncrement(volatile short* dest) { return __sync_fetch_and_add(dest, short(1)); }
inline short AtomicFetchDecrement(volatile short* dest) { return __sync_fetch_and_add(dest, short(-1)); }
inline short AtomicFetchAdd(volatile short* dest, short value) { return __sync_fetch_and_add(dest, value); }
inline short AtomicFetchSub(volatile short* dest, short value) { return __sync_fetch_and_sub(dest, value); }
inline short AtomicFetchOr(volatile short* dest, short value) { return __sync_fetch_and_or(dest, value); }
inline short AtomicFetchAnd(volatile short* dest, short value) { return __sync_fetch_and_and(dest, value); }
inline short AtomicFetchXor(volatile short* dest, short value) { return __sync_fetch_and_xor(dest, value); }
inline short AtomicFetchSwap(volatile short* dest, short value) { return __sync_lock_test_and_set(dest, value); }
inline short AtomicFetchSwapConditional(volatile short* dest, short value, short condition) { return __sync_val_compare_and_swap(reinterpret_cast<volatile unsigned short*>(dest), static_cast<unsigned short>(condition), static_cast<unsigned short>(value)); }
inline bool AtomicSetValueConditional(volatile short* dest, short value, short condition) { return __sync_bool_compare_and_swap(reinterpret_cast<volatile unsigned short*>(dest), static_cast<unsigned short>(condition), static_cast<unsigned short>(value)); }

// unsigned short
inline unsigned short AtomicGetValue(volatile unsigned short* ptr) { return detail::AtomicGetValue(ptr); }
inline unsigned short AtomicGetValue(const volatile unsigned short* ptr) { return AtomicGetValue(const_cast<volatile unsigned short*>(ptr)); }
inline unsigned short AtomicSetValue(volatile unsigned short* dest, unsigned short value) { return __sync_lock_test_and_set(dest, value); }
inline unsigned short AtomicFetchIncrement(volatile unsigned short* dest) { return __sync_fetch_and_add(dest, (unsigned short)(1)); }
inline unsigned short AtomicFetchDecrement(volatile unsigned short* dest) { return __sync_fetch_and_add(dest, (unsigned short)(-1)); }
inline unsigned short AtomicFetchAdd(volatile unsigned short* dest, unsigned short value) { return __sync_fetch_and_add(dest, value); }
inline unsigned short AtomicFetchSub(volatile unsigned short* dest, unsigned short value) { return __sync_fetch_and_sub(dest, value); }
inline unsigned short AtomicFetchOr(volatile unsigned short* dest, unsigned short value) { return __sync_fetch_and_or(dest, value); }
inline unsigned short AtomicFetchAnd(volatile unsigned short* dest, unsigned short value) { return __sync_fetch_and_and(dest, value); }
inline unsigned short AtomicFetchXor(volatile unsigned short* dest, unsigned short value) { return __sync_fetch_and_xor(dest, value); }
inline unsigned short AtomicFetchSwap(volatile unsigned short* dest, unsigned short value) { return __sync_lock_test_and_set(dest, value); }
inline unsigned short AtomicFetchSwapConditional(volatile unsigned short* dest, unsigned short value, unsigned short condition) { return __sync_val_compare_and_swap(dest, condition, value); }
inline bool AtomicSetValueConditional(volatile unsigned short* dest, unsigned short value, unsigned short condition) { return __sync_bool_compare_and_swap(dest, condition, value); }

// long
inline long AtomicGetValue(volatile long* ptr) { return detail::AtomicGetValue(ptr); }
inline long AtomicGetValue(const volatile long* ptr) { return AtomicGetValue(const_cast<volatile long*>(ptr)); }
inline long AtomicSetValue(volatile long* dest, long value) { return __sync_lock_test_and_set(dest, value); }
inline long AtomicFetchIncrement(volatile long* dest) { return __sync_fetch_and_add(dest, long(1)); }
inline long AtomicFetchDecrement(volatile long* dest) { return __sync_fetch_and_add(dest, long(-1)); }
inline long AtomicFetchAdd(volatile long* dest, long value) { return __sync_fetch_and_add(dest, value); }
inline long AtomicFetchSub(volatile long* dest, long value) { return __sync_fetch_and_sub(dest, value); }
inline long AtomicFetchOr(volatile long* dest, long value) { return __sync_fetch_and_or(dest, value); }
inline long AtomicFetchAnd(volatile long* dest, long value) { return __sync_fetch_and_and(dest, value); }
inline long AtomicFetchXor(volatile long* dest, long value) { return __sync_fetch_and_xor(dest, value); }
inline long AtomicFetchSwap(volatile long* dest, long value) { return __sync_lock_test_and_set(dest, value); }
inline long AtomicFetchSwapConditional(volatile long* dest, long value, long condition) { return __sync_val_compare_and_swap(dest, condition, value); }
inline bool AtomicSetValueConditional(volatile long* dest, long value, long condition) { return __sync_bool_compare_and_swap(dest, condition, value); }

// unsigned long
inline unsigned long AtomicGetValue(volatile unsigned long* ptr) { return detail::AtomicGetValue(ptr); }
inline unsigned long AtomicGetValue(const volatile unsigned long* ptr) { return AtomicGetValue(const_cast<volatile unsigned long*>(ptr)); }
inline unsigned long AtomicSetValue(volatile unsigned long* dest, unsigned long value) { return __sync_lock_test_and_set(dest, value); }
inline unsigned long AtomicFetchIncrement(volatile unsigned long* dest) { return __sync_fetch_and_add(dest, (unsigned long)(1)); }
inline unsigned long AtomicFetchDecrement(volatile unsigned long* dest) { return __sync_fetch_and_add(dest, (unsigned long)(-1)); }
inline unsigned long AtomicFetchAdd(volatile unsigned long* dest, unsigned long value) { return __sync_fetch_and_add(dest, value); }
inline unsigned long AtomicFetchSub(volatile unsigned long* dest, unsigned long value) { return __sync_fetch_and_sub(dest, value); }
inline unsigned long AtomicFetchOr(volatile unsigned long* dest, unsigned long value) { return __sync_fetch_and_or(dest, value); }
inline unsigned long AtomicFetchAnd(volatile unsigned long* dest, unsigned long value) { return __sync_fetch_and_and(dest, value); }
inline unsigned long AtomicFetchXor(volatile unsigned long* dest, unsigned long value) { return __sync_fetch_and_xor(dest, value); }
inline unsigned long AtomicFetchSwap(volatile unsigned long* dest, unsigned long value) { return __sync_lock_test_and_set(dest, value); }
inline unsigned long AtomicFetchSwapConditional(volatile unsigned long* dest, unsigned long value, unsigned long condition) { return __sync_val_compare_and_swap(dest, condition, value); }
inline bool AtomicSetValueConditional(volatile unsigned long* dest, unsigned long value, unsigned long condition) { return __sync_bool_compare_and_swap(dest, condition, value); }

// char32_t 
#if EA_CHAR32_NATIVE
	inline char32_t AtomicGetValue(volatile char32_t* ptr) { return detail::AtomicGetValue(ptr); }
	inline char32_t AtomicGetValue(const volatile char32_t* ptr) { return AtomicGetValue(const_cast<volatile char32_t*>(ptr)); }
    inline char32_t AtomicSetValue(volatile char32_t* dest, char32_t value) { return __sync_lock_test_and_set(dest, value); }
	inline char32_t AtomicFetchIncrement(volatile char32_t* dest) { return __sync_fetch_and_add(dest, char32_t(1)); }
	inline char32_t AtomicFetchDecrement(volatile char32_t* dest) { return __sync_fetch_and_add(dest, char32_t(-1)); }
	inline char32_t AtomicFetchAdd(volatile char32_t* dest, char32_t value) { return __sync_fetch_and_add(dest, value); }
	inline char32_t AtomicFetchSub(volatile char32_t* dest, char32_t value) { return __sync_fetch_and_sub(dest, value); }
	inline char32_t AtomicFetchOr(volatile char32_t* dest, char32_t value) { return __sync_fetch_and_or(dest, value); }
	inline char32_t AtomicFetchAnd(volatile char32_t* dest, char32_t value) { return __sync_fetch_and_and(dest, value); }
	inline char32_t AtomicFetchXor(volatile char32_t* dest, char32_t value) { return __sync_fetch_and_xor(dest, value); }
	inline char32_t AtomicFetchSwap(volatile char32_t* dest, char32_t value) { return __sync_lock_test_and_set(dest, value); }
	inline char32_t AtomicFetchSwapConditional(volatile char32_t* dest, char32_t value, char32_t condition) { return __sync_val_compare_and_swap(dest, condition, value); }
	inline bool AtomicSetValueConditional(volatile char32_t* dest, char32_t value, char32_t condition) { return __sync_bool_compare_and_swap(dest, condition, value); }
#endif

// long long
inline long long AtomicGetValue(volatile long long* ptr) { return detail::AtomicGetValue(ptr); }
inline long long AtomicGetValue(const volatile long long* ptr) { return AtomicGetValue(const_cast<volatile long long*>(ptr)); }
inline long long AtomicSetValue(volatile long long* dest, long long value) { return __sync_lock_test_and_set(dest, value); }
inline long long AtomicFetchIncrement(volatile long long* dest) { return __sync_fetch_and_add(dest, (long long)(1)); }
inline long long AtomicFetchDecrement(volatile long long* dest) { return __sync_fetch_and_add(dest, (long long)(-1)); }
inline long long AtomicFetchAdd(volatile long long* dest, long long value) { return __sync_fetch_and_add(dest, value); }
inline long long AtomicFetchSub(volatile long long* dest, long long value) { return __sync_fetch_and_sub(dest, value); }
inline long long AtomicFetchOr(volatile long long* dest, long long value) { return __sync_fetch_and_or(dest, value); }
inline long long AtomicFetchAnd(volatile long long* dest, long long value) { return __sync_fetch_and_and(dest, value); }
inline long long AtomicFetchXor(volatile long long* dest, long long value) { return __sync_fetch_and_xor(dest, value); }
inline long long AtomicFetchSwap(volatile long long* dest, long long value) { return __sync_lock_test_and_set(dest, value); }
inline long long AtomicFetchSwapConditional(volatile long long* dest, long long value, long long condition) { return __sync_val_compare_and_swap(dest, condition, value); }
inline bool AtomicSetValueConditional(volatile long long* dest, long long value, long long condition) { return __sync_bool_compare_and_swap(dest, condition, value); }

// unsigned long long
inline unsigned long long AtomicGetValue(volatile unsigned long long* ptr) { return detail::AtomicGetValue(ptr); }
inline unsigned long long AtomicGetValue(const volatile unsigned long long* ptr) { return AtomicGetValue(const_cast<volatile unsigned long long*>(ptr)); }
inline unsigned long long AtomicSetValue(volatile unsigned long long* dest, unsigned long long value) { return __sync_lock_test_and_set(dest, value); }
inline unsigned long long AtomicFetchIncrement(volatile unsigned long long* dest) { return __sync_fetch_and_add(dest, (unsigned long long)(1)); }
inline unsigned long long AtomicFetchDecrement(volatile unsigned long long* dest) { return __sync_fetch_and_add(dest, (unsigned long long)(-1)); }
inline unsigned long long AtomicFetchAdd(volatile unsigned long long* dest, unsigned long long value) { return __sync_fetch_and_add(dest, value); }
inline unsigned long long AtomicFetchSub(volatile unsigned long long* dest, unsigned long long value) { return __sync_fetch_and_sub(dest, value); }
inline unsigned long long AtomicFetchOr(volatile unsigned long long* dest, unsigned long long value) { return __sync_fetch_and_or(dest, value); }
inline unsigned long long AtomicFetchAnd(volatile unsigned long long* dest, unsigned long long value) { return __sync_fetch_and_and(dest, value); }
inline unsigned long long AtomicFetchXor(volatile unsigned long long* dest, unsigned long long value) { return __sync_fetch_and_xor(dest, value); }
inline unsigned long long AtomicFetchSwap(volatile unsigned long long* dest, unsigned long long value) { return __sync_lock_test_and_set(dest, value); }
inline unsigned long long AtomicFetchSwapConditional(volatile unsigned long long* dest, unsigned long long value, unsigned long long condition) { return __sync_val_compare_and_swap(dest, condition, value); }
inline bool AtomicSetValueConditional(volatile unsigned long long* dest, unsigned long long value, unsigned long long condition) { return __sync_bool_compare_and_swap(dest, condition, value); }

//	
// You can not simply define a template for the above atomics due to the explicit 128bit overloads
// below.  The compiler will prefer those overloads during overload resolution and attempt to convert
// temporaries as they are more specialized than a template.
//
// template<typename T> inline T AtomicGetValue(volatile T* source) { return __sync_fetch_and_add(source, (T)(0)); }
// template<typename T> inline void AtomicSetValue(volatile T* dest, T value) { __sync_lock_test_and_set(dest, value); }
// template<typename T> inline T AtomicFetchIncrement(volatile T* dest) { return __sync_fetch_and_add(dest, (T)(1)); }
// template<typename T> inline T AtomicFetchDecrement(volatile T* dest) { return __sync_fetch_and_add(dest, (T)(-1)); }
// template<typename T> inline T AtomicFetchAdd(volatile T* dest, T value) { return __sync_fetch_and_add(dest, value); }
// template<typename T> inline T AtomicFetchOr(volatile T* dest, T value) { return __sync_fetch_and_or(dest, value); }
// template<typename T> inline T AtomicFetchAnd(volatile T* dest, T value) { return __sync_fetch_and_and(dest, value); }
// template<typename T> inline T AtomicFetchXor(volatile T* dest, T value) { return __sync_fetch_and_xor(dest, value); }
// template<typename T> inline T AtomicFetchSwap(volatile T* dest, T value) { return __sync_lock_test_and_set(dest, value); }
// template<typename T> inline bool AtomicSetValueConditional(volatile T* dest, T value, T condition) { return __sync_bool_compare_and_swap(dest, condition, value); }
//

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

