///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements an efficient proper multithread-safe spinlock which supports
// multiple readers but a single writer.
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_RWSPINLOCK_H
#define EATHREAD_EATHREAD_RWSPINLOCK_H

#include <EABase/eabase.h>
#include <eathread/eathread.h>
#include <eathread/eathread_sync.h>
#include <eathread/eathread_atomic.h>
#include <new>


#ifdef _MSC_VER
	 #pragma warning(push)
	 #pragma warning(disable: 4100) // (Compiler claims pRWSpinLock is unreferenced)
#endif

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace EA
{
	namespace Thread
	{
		/// class RWSpinLock
		///
		/// A RWSpinLock is like a SpinLock except you can have multiple
		/// readers but a single exclusive writer. This is very beneficial for 
		/// situations whereby there are many consumers of some data but only 
		/// one producer of the data. Unlock many thread-level read/write lock
		/// implementations, this spin lock, like many others, follows the most
		/// lean approach and does not do arbitration or fairness. The result is
		/// that if you have many readers who are constantly locking the read
		/// lock, write lock attempts may not be able to succeed. So you need to
		/// be careful in how you use this.
		///
		/// We take a page from the Linux kernel here and implement read/write
		/// locks via a mechanism that uses a 'bias' value and limits the number
		/// of total readers to 2^24-1, or 16,777,214. This shouldn't be a problem.
		/// When the spinlock is unlocked, the value is 0x01000000.
		/// Readers decrement the lock by one each, so when the spinlock is 
		/// read-locked, the value is between 1 and 0x00ffffff. Writers decrement
		/// the lock by 0x01000000, so when a spinlock is write-locked, the value
		/// must be zero. It must be zero because there can only be one writer
		/// and because there can be no readers when there is a writer. When a 
		/// reader attempts to get a read-lock, it decrements the lock count and 
		/// examines the new value. If the new value is < 0, then there was a 
		/// write-lock present and so the reader immediately increments the lock
		/// count and tries again later. There are two results that come about due
		/// to this: 
		///     1) In the case of 32 bit integers, if by some wild chance of nature
		///         there are 256 or more reader threads and there is a writer thread
		///         with a write lock and every one of the reader threads executes 
		///         the same decrement and compare to < 0 at the same time, then the
		///         257th thread will mistakenly think that there isn't a write lock.
		///     2) The logic to test if a write-lock is taken is not to compare
		///         against zero but to compare against (> -255 and <= 0). This is
		///         because readers will occasionally be 'mistakenly' decrementing
		///         the lock while trying to obtain read access.
		///
		/// We thus have the following possible values:
		///     0 < value < 0x01000000    ----> read-locked
		///     value == 0x01000000       ----> unlocked
		///     0x01000000 < value <= 0   ----> write-locked
		///
		class RWSpinLock
		{
		public:
			RWSpinLock();

			// This function cannot be called while the current thread  
			// already has a write lock, else this function will hang. 
			// This function can be called if the current thread already 
			// has a read lock, though all read locks must be matched by unlocks. 
			void ReadLock();

			// This function cannot be called while the current thread  
			// already has a write lock, else this function will hang.
			// This function can be called if the current thread already 
			// has a read lock (in which case it will always succeed), 
			// though all read locks must be matched by unlocks. 
			bool ReadTryLock();

			// Returns true if any thread currently has a read lock. 
			// The return value is subject to be out of date by the 
			// time it is read by the current thread, unless the current
			// thread has a read lock. If IsReadLocked is true, then 
			// at that moment IsWriteLocked is necessarily false.
			// If IsReadLocked is false, IsWriteLock may be either true or false.
			bool IsReadLocked() const;

			// Unlocks for reading, as a match to ReadLock or a successful
			// ReadTryLock. All read locks must be matched by ReadUnlock with
			// the same thread that has the read lock.
			void ReadUnlock();

			// This function cannot be called while the current thread  
			// already has a read or write lock, else this function will hang. 
			void WriteLock();

			// If this function is called while the current thread already 
			// has a read or write lock, it will always return false.
			bool WriteTryLock();

			// If this function returns true, then IsReadLocked must at that moment
			// be false.
			bool IsWriteLocked() const;

			// Matches WriteLock or a successful WriteTryLock.
			void WriteUnlock();

			// Returns the address of mValue. This value should be read for 
			// diagnostic purposes only and should not be written.
			void* GetPlatformData();

		public:
			enum Value
			{
				kValueUnlocked = 0x01000000
			};

			AtomicInt32 mValue;
		};



		/// RWSpinLockFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class RWSpinlock.
		/// A primary use of this would be to allow the RWSpinlock implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		///
		class EATHREADLIB_API RWSpinLockFactory
		{
		public:
			static RWSpinLock* CreateRWSpinLock();
			static void        DestroyRWSpinLock(RWSpinLock* pRWSpinLock);
			static size_t      GetRWSpinLockSize();
			static RWSpinLock* ConstructRWSpinLock(void* pMemory);
			static void        DestructRWSpinLock(RWSpinLock* pRWSpinLock);
		};



		/// class AutoRWSpinLock
		///
		/// Example usage:
		///     void Function() {
		///         AutoRWSpinLock autoLock(AutoRWSpinLock::kLockTypeRead);
		///         // Do something
		///     }
		///
		class AutoRWSpinLock
		{
		public:
			enum LockType
			{
				kLockTypeRead, 
				kLockTypeWrite
			};

			AutoRWSpinLock(RWSpinLock& spinLock, LockType lockType) ;
		   ~AutoRWSpinLock();

		protected:
			RWSpinLock& mSpinLock;
			LockType    mLockType;

			// Prevent copying by default, as copying is dangerous.
			AutoRWSpinLock(const AutoRWSpinLock&);
			const AutoRWSpinLock& operator=(const AutoRWSpinLock&);
		};

	} // namespace Thread

} // namespace EA






///////////////////////////////////////////////////////////////////////////////
// inlines
///////////////////////////////////////////////////////////////////////////////

namespace EA
{
	namespace Thread
	{

		///////////////////////////////////////////////////////////////////////
		// RWSpinLock
		///////////////////////////////////////////////////////////////////////

		inline
		RWSpinLock::RWSpinLock()
			: mValue(kValueUnlocked)
		{
		}


		inline
		void RWSpinLock::ReadLock()
		{
			Top: // Due to modern processor branch prediction, the compiler will optimize better for true branches and so we do a manual goto loop here.
			if((unsigned)mValue.Decrement() < kValueUnlocked)
				return;
			mValue.Increment();
			while(mValue.GetValueRaw() <= 0){ // It is better to do this polling loop as a first check than to 
				#ifdef EA_THREAD_COOPERATIVE  // do an atomic decrement repeatedly, as the atomic lock is 
					ThreadSleep();            // potentially not a cheap thing due to potential bus locks on some platforms..
				#else
					EAProcessorPause();       // We don't check for EA_TARGET_SMP here and instead sleep if not defined because you probably shouldn't be using a spinlock on a pre-emptive system unless it is a multi-processing system.     
				#endif
			}
			goto Top;
		}


		inline
		bool RWSpinLock::ReadTryLock()
		{
			const unsigned nNewValue = (unsigned)mValue.Decrement();
			if(nNewValue < kValueUnlocked) // Given that nNewValue is unsigned, we don't need to test for < 0.
				return true;
			mValue.Increment();
			return false;
		}


		inline
		bool RWSpinLock::IsReadLocked() const
		{
			const unsigned nValue = (unsigned)mValue.GetValue();
			return ((nValue - 1) < (kValueUnlocked - 1)); // Given that nNewValue is unsigned, this is faster than comparing ((n > 0) && (n < kValueUnlocked)), due to the presence of only one comparison instead of two.
		}


		inline
		void RWSpinLock::ReadUnlock()
		{
			mValue.Increment();
		}


		inline
		void RWSpinLock::WriteLock()
		{
			Top: 
			if(mValue.Add(-kValueUnlocked) == 0)
				return;
			mValue.Add(kValueUnlocked);
			while(mValue.GetValueRaw() != kValueUnlocked){  // It is better to do this polling loop as a first check than to
				#ifdef EA_THREAD_COOPERATIVE             // do an atomic decrement repeatedly, as the atomic lock is 
					ThreadSleep();                       // potentially not a cheap thing due to potential bus locks on some platforms..
				#else
					EAProcessorPause();                  // We don't check for EA_TARGET_SMP here and instead sleep if not defined because you probably shouldn't be using a spinlock on a pre-emptive system unless it is a multi-processing system.     
				#endif
			}
			goto Top;
		}


		inline
		bool RWSpinLock::WriteTryLock()
		{
			if(mValue.Add(-kValueUnlocked) == 0)
				return true;
			mValue.Add(kValueUnlocked);
			return false;
		}


		inline
		bool RWSpinLock::IsWriteLocked() const
		{
			 return (mValue.GetValue() <= 0); // This fails to work if 127 threads at once are in the middle of a failed write lock attempt.
		}


		inline
		void RWSpinLock::WriteUnlock()
		{
			mValue.Add(kValueUnlocked);
		}


		inline
		void* RWSpinLock::GetPlatformData() 
		{
			return &mValue;
		}



		///////////////////////////////////////////////////////////////////////
		// RWSpinLockFactory
		///////////////////////////////////////////////////////////////////////

		inline
		RWSpinLock* RWSpinLockFactory::CreateRWSpinLock() 
		{
			Allocator* pAllocator = GetAllocator();

			if(pAllocator)
				return new(pAllocator->Alloc(sizeof(RWSpinLock))) RWSpinLock;
			else
				return new RWSpinLock;
		}


		inline
		void RWSpinLockFactory::DestroyRWSpinLock(RWSpinLock* pRWSpinLock)
		{
			Allocator* pAllocator = GetAllocator();

			if(pAllocator)
			{
				pRWSpinLock->~RWSpinLock();
				pAllocator->Free(pRWSpinLock);
			}
			else
				delete pRWSpinLock;
		}


		inline
		size_t RWSpinLockFactory::GetRWSpinLockSize()
		{
			return sizeof(RWSpinLock);
		}


		inline
		RWSpinLock* RWSpinLockFactory::ConstructRWSpinLock(void* pMemory)
		{
			return new(pMemory) RWSpinLock;
		}


		inline
		void RWSpinLockFactory::DestructRWSpinLock(RWSpinLock* pRWSpinLock)
		{
			pRWSpinLock->~RWSpinLock();
		}




		///////////////////////////////////////////////////////////////////////
		// AutoRWSpinLock
		///////////////////////////////////////////////////////////////////////

		inline
		AutoRWSpinLock::AutoRWSpinLock(RWSpinLock& spinLock, LockType lockType) 
			: mSpinLock(spinLock), mLockType(lockType)
		{ 
			if(mLockType == kLockTypeRead)
				mSpinLock.ReadLock();
			else
				mSpinLock.WriteLock();
		}


		inline
		AutoRWSpinLock::~AutoRWSpinLock()
		{ 
			if(mLockType == kLockTypeRead)
				mSpinLock.ReadUnlock();
			else
				mSpinLock.WriteUnlock();
		}


	} // namespace Thread

} // namespace EA



#ifdef _MSC_VER
	#pragma warning(pop)
#endif


#endif // EATHREAD_EATHREAD_RWSPINLOCK_H






