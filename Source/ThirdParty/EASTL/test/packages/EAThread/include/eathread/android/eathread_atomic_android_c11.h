///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif


#ifndef EATHREAD_ATOMIC_ANDROID_C11_H
#define EATHREAD_ATOMIC_ANDROID_C11_H


#include <EABase/eabase.h>
#include <stddef.h>
#include <stdatomic.h>
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
			typedef T ValueType;
			typedef _Atomic(T) AtomicValueType;

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
				{ return atomic_load_explicit(const_cast<AtomicValueType*>(&mValue), memory_order_relaxed); }

			ValueType GetValueRaw() const
				{ return atomic_load_explicit(const_cast<AtomicValueType*>(&mValue), memory_order_relaxed); }

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
			AtomicValueType mValue;
		};


		///////////////////////////////////////////////////////////
		/// 32 bit
		///
		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::SetValue(ValueType n)
			{ return atomic_exchange_explicit(&mValue, n, memory_order_relaxed); }

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::SetValue(ValueType n)
			{ return atomic_exchange_explicit(&mValue, n, memory_order_relaxed); }

		template <> inline
		bool AtomicInt<int32_t>::SetValueConditional(ValueType n, ValueType condition)
			{ return atomic_compare_exchange_strong_explicit(&mValue, &condition, n, memory_order_relaxed, memory_order_relaxed); }

		template <> inline
		bool AtomicInt<uint32_t>::SetValueConditional(ValueType n, ValueType condition)
			{ return atomic_compare_exchange_strong_explicit(&mValue, &condition, n, memory_order_relaxed, memory_order_relaxed); }

		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Increment()
			{ return atomic_fetch_add_explicit(&mValue, 1, memory_order_relaxed) + 1; }  

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Increment()
			{ return atomic_fetch_add_explicit(&mValue, 1u, memory_order_relaxed) + 1u; }

		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Decrement()
			{ return atomic_fetch_sub_explicit(&mValue, 1, memory_order_relaxed) - 1; }

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Decrement()
			{ return atomic_fetch_sub_explicit(&mValue, 1u, memory_order_relaxed) - 1u; }  

		template <> inline
		AtomicInt<int32_t>::ValueType AtomicInt<int32_t>::Add(ValueType n)
			{ return atomic_fetch_add_explicit(&mValue, n, memory_order_relaxed) + n; } 

		template <> inline
		AtomicInt<uint32_t>::ValueType AtomicInt<uint32_t>::Add(ValueType n)
			{ return atomic_fetch_add_explicit(&mValue, n, memory_order_relaxed) + n; }


		///////////////////////////////////////////////////////////
		/// 64 bit
		///
		template <> inline
		AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::GetValue() const
			{ return atomic_load_explicit(const_cast<AtomicValueType*>(&mValue), memory_order_relaxed); }

		template <> inline
		AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::GetValue() const
			{ return atomic_load_explicit(const_cast<AtomicValueType*>(&mValue), memory_order_relaxed); }

		template <> inline
		AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::SetValue(ValueType n)
			{ return atomic_exchange_explicit(&mValue, n, memory_order_relaxed); }

		template <> inline
		AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::SetValue(ValueType n)
			{ return atomic_exchange_explicit(&mValue, n, memory_order_relaxed); }

		template <> inline
		bool AtomicInt<int64_t>::SetValueConditional(ValueType n, ValueType condition)
			{ return atomic_compare_exchange_strong_explicit(&mValue, &condition, n, memory_order_relaxed, memory_order_relaxed); }

		template <> inline
		bool AtomicInt<uint64_t>::SetValueConditional(ValueType n, ValueType condition)
			{ return atomic_compare_exchange_strong_explicit(&mValue, &condition, n, memory_order_relaxed, memory_order_relaxed); }

		template <> inline
		AtomicInt<int64_t>::ValueType AtomicInt<int64_t>::Add(ValueType n)
			{ return atomic_fetch_add_explicit(&mValue, n, memory_order_relaxed) + n; }  

		template <> inline
		AtomicInt<uint64_t>::ValueType AtomicInt<uint64_t>::Add(ValueType n)
			{ return atomic_fetch_add_explicit(&mValue, n, memory_order_relaxed) + n; } 

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


////////////////////////////////////////////////////////////////////////////////
// Use of the C11 atomics API on Android is problematic because the platform 
// implements the atomics API via macro wrappers around their platform specific
// functions.  Unfortunately, macros affect header files outside of its
// scoped namespace and will be applied to areas of the code in undesirable
// ways.  One instance of this is the C11 atomics colliding with the atomic
// functions of C++11 std::shared_ptr.
// 
// We attempt to prevent external impact of the stdatomics.h by undefining the
// relevant functions.
// 
// Note:  If you #include <stdatomic.h> above an eathread header it will undefined macros.
//
// http://en.cppreference.com/w/cpp/memory/shared_ptr
//
// std::atomic_compare_exchange_strong(std::shared_ptr)
// std::atomic_compare_exchange_strong_explicit(std::shared_ptr)
// std::atomic_compare_exchange_weak(std::shared_ptr)
// std::atomic_compare_exchange_weak_explicit(std::shared_ptr)
// std::atomic_exchange(std::shared_ptr)
// std::atomic_exchange_explicit(std::shared_ptr)
// std::atomic_is_lock_free(std::shared_ptr)
// std::atomic_load(std::shared_ptr)
// std::atomic_load_explicit(std::shared_ptr)
// std::atomic_store(std::shared_ptr)
// std::atomic_store_explicit(std::shared_ptr)
//

#undef atomic_compare_exchange_strong
#undef atomic_compare_exchange_strong_explicit
#undef atomic_compare_exchange_weak
#undef atomic_compare_exchange_weak_explicit
#undef atomic_exchange
#undef atomic_exchange_explicit
#undef atomic_is_lock_free
#undef atomic_load
#undef atomic_load_explicit
#undef atomic_store
#undef atomic_store_explicit

#endif // EATHREAD_ATOMIC_ANDROID_C11_H



