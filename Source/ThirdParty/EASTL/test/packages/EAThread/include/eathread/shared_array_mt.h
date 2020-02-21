///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This is a multithread-safe version of shared_array_mt.
// For basic documentation, see shared_array_mt.
///////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_SHARED_ARRAY_MT_H
#define EATHREAD_SHARED_ARRAY_MT_H

#ifndef INCLUDED_eabase_H
   #include <EABase/eabase.h>
#endif
#ifndef EATHREAD_EATHREAD_FUTEX_H
   #include <eathread/eathread_futex.h>
#endif
#include <stddef.h> // More properly: #include <cstddef> // Definition of std::ptrdiff_t

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif




/// namespace EA
/// The standard Electronic Arts namespace
namespace EA
{
   namespace Thread
   {
	  /// class shared_array_mt
	  /// A shared_array_mt is the same as shared_ptr but for arrays. 
	  template<class T>
	  class shared_array_mt
	  {
	  private:
		 /// this_type
		 /// This is an alias for shared_array_mt<T>, this class.
		 typedef shared_array_mt<T> this_type;

		 /// reference_count_type
		 /// An internal reference count type. Must be convertable to int
		 /// so that the public use_count function can work.
		 typedef EA::Thread::AtomicInt32 reference_count_type;

		 T*                    mpArray;      /// The owned pointer. Points to an array of T.
		 reference_count_type* mpRefCount;   /// Reference count for owned pointer.
		 mutable Futex         mMutex;       /// Mutex guarding access to this class.

	  public:
		 typedef T element_type;
		 typedef T value_type;

		 /// shared_array_mt
		 /// Takes ownership of the pointer and sets the reference count
		 /// to the pointer to 1. It is OK if the input pointer is null.
		 /// The shared reference count is allocated on the heap via operator new.
		 /// If an exception occurs during the allocation of the shared 
		 /// reference count, the owned pointer is deleted and the exception
		 /// is rethrown. A null pointer is given a reference count of 1.
		 explicit shared_array_mt(T* pArray = 0)
			: mpArray(pArray), mMutex()
		 {
			// We don't lock our mutex in this function, as this is the constructor
			// and we assume that construction is already done in a thread-safe way
			// by the owner of this object.
			#if defined(EA_COMPILER_NO_EXCEPTIONS) || defined(EA_COMPILER_NO_UNWIND)
			   mpRefCount = new reference_count_type(1);
			#else
				EA_DISABLE_VC_WARNING(4571)
				try
				{
					mpRefCount = new reference_count_type(1);
				}
				catch(...)
				{
					delete[] mpArray;
					//mpRefCount = 0; shouldn't be necessary.
					throw;
				}
				EA_RESTORE_VC_WARNING()
			#endif
		 }

		 /// shared_array_mt
		 /// Shares ownership of a pointer with another instance of shared_array_mt.
		 /// This function increments the shared reference count on the pointer.
		 shared_array_mt(shared_array_mt const& sharedArray)
			: mMutex()
		 {
			sharedArray.lock();
			mpArray    = sharedArray.mpArray;
			mpRefCount = sharedArray.mpRefCount;
			mpRefCount->Increment(); // Atomic operation
			sharedArray.unlock();
		 }

		 /// ~shared_array_mt
		 /// Decrements the reference count for the owned pointer. If the 
		 /// reference count goes to zero, the owned pointer is deleted and
		 /// the shared reference count is deleted.
		 ~shared_array_mt()
		 {
			lock();
			const reference_count_type newRefCount(mpRefCount->Decrement()); // Atomic operation
			// EAT_ASSERT(newRefCount >= 0);
			if(newRefCount == 0)
			{
			   delete[] mpArray;
			   delete mpRefCount;
			}
			unlock();
		 }

		 /// operator=
		 /// Copies another shared_array_mt to this object. Note that this object
		 /// may already own a shared pointer with another different pointer
		 /// (but still of the same type) before this call. In that case,
		 /// this function releases the old pointer, decrementing its reference
		 /// count and deleting it if zero, takes shared ownership of the new 
		 /// pointer and increments its reference count.
		 shared_array_mt& operator=(shared_array_mt const& sharedArray)
		 {
			// We don't lock mutexes here because we let the swap function
			// below do the locking and assignment. The if statement below
			// isn't protected within a lock operation because it wouldn't
			// help by being so because if mpValue is changing during the 
			// the execution of this function then the user has an external 
			// race condition that needs to be managed at that level.
			if(mpArray != sharedArray.mpArray)
			{
			   // The easiest thing to do is to create a temporary and 
			   // copy ourselves ourselves into it. This is a standard 
			   // method for switching pointer ownership in systems like this.
			   shared_array_mt(sharedArray).swap(*this);
			}
			return *this;
		 }

		 // operator=
		 // We do not defined this function in order to maintain compatibility 
		 // with the currently proposed (2003) C++ standard addition.  Use reset instead.
		 // shared_array_mt& operator=(T* pValue)
		 // {
		 //     reset(pValue);
		 //     return *this;
		 // }

		 /// lock
		 /// @brief Locks our mutex for thread-safe access.
		 /// It is a const function because const-ness refers to the underlying pointer being
		 /// held and not this class.
		 void lock() const
		 {
			mMutex.Lock(); 
		 }

		 /// unlock
		 /// @brief Unlocks our mutex which was previous locked.
		 /// It is a const function because const-ness refers to the underlying pointer being
		 /// held and not this class.
		 void unlock() const
		 {
			mMutex.Unlock(); 
		 }

		 /// reset
		 /// Releases the owned pointer and takes ownership of the 
		 /// passed in pointer. If the passed in pointer is the same
		 /// as the owned pointer, nothing is done. The passed in pointer
		 /// can be null, in which case the use count is set to 1.
		 void reset(T* pArray = 0)
		 {
			// We don't lock any mutexes here because we let the swap function do that.
			// We don't lock for the 'if' statement below because that wouldn't really buy anything.
			if(pArray != mpArray)
			{
			   // The easiest thing to do is to create a temporary and 
			   // copy ourselves ourselves into it. This is a standard 
			   // method for switching pointer ownership in systems like this.
			   shared_array_mt(pArray).swap(*this);
			}
		 }

		 /// swap
		 /// Exchanges the owned pointer beween two shared_array_mt objects.
		 void swap(shared_array_mt<T>& sharedArray)
		 {
			lock();
			sharedArray.lock();

			// std::swap(mpArray, sharedArray.mpArray); // Not used so that we can reduce a dependency.
			T* const pArray     = sharedArray.mpArray;
			sharedArray.mpArray = mpArray;
			mpArray             = pArray;

			// std::swap(mpRefCount, sharedArray.mpRefCount); // Not used so that we can reduce a dependency.
			reference_count_type* const pRefCount = sharedArray.mpRefCount;
			sharedArray.mpRefCount = mpRefCount;
			mpRefCount             = pRefCount;

			sharedArray.unlock();
			unlock();
		 }

		 /// operator[]
		 /// Returns a reference to the specified item in the owned pointer
		 /// array. 
		 /// Example usage:
		 ///   shared_array_mt<int> ptr = new int[6];
		 ///   int x = ptr[2];
		 T& operator[](ptrdiff_t i) const
		 {
			// We don't lock here because this is essentially a read operation.
			// We don't put a SMP read barrier here because we assume the caller does such things.
			// EAT_ASSERT(mpArray && (i >= 0));
			return mpArray[i];
		 }

		 /// operator*
		 /// Returns the owner pointer dereferenced.
		 /// Example usage:
		 ///   shared_array_mt<int> ptr = new int(3);
		 ///   int x = *ptr;
		 T& operator*() const
		 {
			// We don't lock here because this is essentially a read operation.
			// We don't put a SMP read barrier here because we assume the caller does such things.
			// EAT_ASSERT(mpArray);
			return *mpArray;
		 }

		 /// operator->
		 /// Allows access to the owned pointer via operator->()
		 /// Example usage:
		 ///   struct X{ void DoSomething(); }; 
		 ///   shared_array_mt<int> ptr = new X;
		 ///   ptr->DoSomething();
		 T* operator->() const
		 {
			// We don't lock here because this is essentially a read operation.
			// We don't put a SMP read barrier here because we assume the caller does such things.
			// EAT_ASSERT(mpArray);
			return mpArray;
		 }

		 /// get
		 /// Returns the owned pointer. Note that this class does 
		 /// not provide an operator T() function. This is because such
		 /// a thing (automatic conversion) is deemed unsafe.
		 /// Example usage:
		 ///   struct X{ void DoSomething(); }; 
		 ///   shared_array_mt<int> ptr = new X;
		 ///   X* pX = ptr.get();
		 ///   pX->DoSomething();
		 T* get() const
		 {
			// We don't lock here because this is essentially a read operation.
			// We don't put a SMP read barrier here because we assume the caller does such things.
			return mpArray;
		 }

		 /// use_count
		 /// Returns the reference count on the owned pointer.
		 /// The return value is one if the owned pointer is null.
		 int use_count() const
		 {
			// We don't lock here because this is essentially a read operation.
			// We don't put a SMP read barrier here because we assume the caller does such things.
			// EAT_ASSERT(mpRefCount);
			return (int)*mpRefCount;
		 }

		 /// unique
		 /// Returns true if the reference count on the owned pointer is one.
		 /// The return value is true if the owned pointer is null.
		 bool unique() const
		 {
			// We don't lock here because this is essentially a read operation.
			// We don't put a SMP read barrier here because we assume the caller does such things.
			// EAT_ASSERT(mpRefCount);
			return (*mpRefCount == 1);
		 }

		 /// add_ref
		 /// Manually increments the reference count on the owned pointer.
		 /// This is currently disabled because it isn't in part of the 
		 /// proposed C++ language addition.
		 /// int add_ref()
		 /// {
		 ///    lock();
		 ///    // EAT_ASSERT(mpRefCount);
		 ///    ++*mpRefCount; // Atomic operation
		 ///    unlock();
		 /// }

		 /// release_ref
		 /// Manually increments the reference count on the owned pointer.
		 /// If the reference count becomes zero, then the owned pointer 
		 /// is deleted and reset(0) is called. For any given instance of
		 /// shared_ptr, release_ref can only be called as many times as -- 
		 /// but no more than -- the number of times add_ref was called
		 /// for that same shared_ptr. Otherwise, separate instances of 
		 /// shared_ptr would be left with dangling owned pointer instances.
		 /// This is currently disabled because it isn't in part of the 
		 /// proposed C++ language addition.
		 /// int release_ref()
		 /// {
		 ///    lock();
		 ///    // EAT_ASSERT(mpRefCount);
		 ///    if(*mpRefCount > 1){
		 ///       const int nReturnValue = --*mpRefCount; // Atomic operation
		 ///       unlock();
		 ///       return nReturnValue;
		 ///    }
		 ///    reset(0);
		 ///    unlock();
		 ///    return 0;
		 /// }

		 /// Implicit operator bool
		 /// Allows for using a scoped_ptr as a boolean. 
		 /// Example usage:
		 ///   shared_array_mt<int> ptr = new int(3);
		 ///   if(ptr)
		 ///      ++*ptr;
		 ///    
		 /// Note that below we do not use operator bool(). The reason for this
		 /// is that booleans automatically convert up to short, int, float, etc.
		 /// The result is that this: if(scopedPtr == 1) would yield true (bad).
		 typedef T* (this_type::*bool_)() const;
		 operator bool_() const
		 {
			// We don't lock here because this is essentially a read operation.
			if(mpArray)
			   return &this_type::get;
			return 0;
		 }

		 /// operator!
		 /// This returns the opposite of operator bool; it returns true if 
		 /// the owned pointer is null. Some compilers require this and some don't.
		 ///   shared_array_mt<int> ptr = new int(3);
		 ///   if(!ptr)
		 ///      EAT_ASSERT(false);
		 bool operator!() const
		 {
			// We don't lock here because this is essentially a read operation.
			return (mpArray == 0);
		 }

	  }; // class shared_array_mt


	  /// get_pointer
	  /// returns shared_array_mt::get() via the input shared_array_mt. 
	  template<class T>
	  inline T* get_pointer(const shared_array_mt<T>& sharedArray)
	  {
		 return sharedArray.get();
	  }

	  /// swap
	  /// Exchanges the owned pointer beween two shared_array_mt objects.
	  /// This non-member version is useful for compatibility of shared_array_mt
	  /// objects with the C++ Standard Library and other libraries.
	  template<class T>
	  inline void swap(shared_array_mt<T>& sharedArray1, shared_array_mt<T>& sharedArray2)
	  {
		 sharedArray1.swap(sharedArray2);
	  }


	  /// operator!=
	  /// Compares two shared_array_mt objects for equality. Equality is defined as 
	  /// being true when the pointer shared between two shared_array_mt objects is equal.
	  /// It is debatable what the appropriate definition of equality is between two
	  /// shared_array_mt objects, but we follow the current 2nd generation C++ standard proposal.
	  template<class T, class U>
	  inline bool operator==(const shared_array_mt<T>& sharedArray1, const shared_array_mt<U>& sharedArray2)
	  {
		 // EAT_ASSERT((sharedArray1.get() != sharedArray2.get()) || (sharedArray1.use_count() == sharedArray2.use_count()));
		 return (sharedArray1.get() == sharedArray2.get());
	  }


	  /// operator!=
	  /// Compares two shared_array_mt objects for inequality. Equality is defined as 
	  /// being true when the pointer shared between two shared_array_mt objects is equal.
	  /// It is debatable what the appropriate definition of equality is between two
	  /// shared_array_mt objects, but we follow the current 2nd generation C++ standard proposal.
	  template<class T, class U>
	  inline bool operator!=(const shared_array_mt<T>& sharedArray1, const shared_array_mt<U>& sharedArray2)
	  {
		 // EAT_ASSERT((sharedArray1.get() != sharedArray2.get()) || (sharedArray1.use_count() == sharedArray2.use_count()));
		 return (sharedArray1.get() != sharedArray2.get());
	  }


	  /// operator<
	  /// Returns which shared_array_mt is 'less' than the other. Useful when storing
	  /// sorted containers of scoped_ptr objects.
	  template<class T, class U>
	  inline bool operator<(const shared_array_mt<T>& sharedArray1, const shared_array_mt<U>& sharedArray2)
	  {
		 return (sharedArray1.get() < sharedArray2.get()); // Alternatively use: std::less<T*>(a.get(), b.get());
	  }

   } // namespace Thread

} // namespace EA




#endif // EATHREAD_SHARED_ARRAY_MT_H











