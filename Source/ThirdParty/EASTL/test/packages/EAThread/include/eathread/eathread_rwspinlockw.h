///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements an efficient proper multithread-safe spinlock which supports
// multiple simultaneous readers but a single writer, where writers get
// priority over readers.
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_RWSPINLOCKW_H
#define EATHREAD_EATHREAD_RWSPINLOCKW_H

#include <EABase/eabase.h>
#include <eathread/eathread.h>
#include <eathread/eathread_sync.h>
#include <eathread/eathread_atomic.h>
#include <new>


#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4100) // (Compiler claims pRWSpinLockW is unreferenced)
#endif

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace EA
{
	namespace Thread
	{
		/// class RWSpinLockW
		///
		/// This class differs from RWSpinLock in that it gives writers priority.
		/// In exchange for that feature, this version doesn't allow recursive
		/// read locks and it becomes inefficient due to excessive spinning if
		/// there are very many simultaneous readers.
		/// 
		/// A RWSpinLockW is like a SpinLock except you can have multiple
		/// readers but a single exclusive writer. This is very beneficial for 
		/// situations whereby there are many consumers of some data but only 
		/// one producer of the data. Unlock many thread-level read/write lock
		/// implementations, this spin lock, like many others, follows the most
		/// lean approach and does not do arbitration or fairness. The result is
		/// that if you have many readers who are constantly locking the read
		/// lock, write lock attempts may not be able to succeed. So you need to
		/// be careful in how you use this.
		///
		/// Note the usage of GetValueRaw in the source code for this class.
		/// Use of GetValueRaw instead of GetValue is due to a tradeoff that
		/// has been chosen. GetValueRaw does not come with memory read barrier
		/// and thus the read value may be out of date. This is OK because it's 
		/// only used as a rule of thumb to help decide what synchronization 
		/// primitive to use next. This results in significantly faster execution
		/// because only one memory synchronization primitive is typically 
		/// executed instead of two. The problem with GetValueRaw, however, 
		/// is that in cases where there is very high locking activity from 
		/// many threads simultaneously GetValueRaw usage could result in 
		/// a "bad guess" as to what to do next and can also result in a lot
		/// of spinning, even infinite spinning in the most pathological case.
		/// However, in practice on the platforms that target this situation
		/// is unlikely to the point of being virtually impossible in practice.
		/// And if it was possible then we recommend the user use a different
		/// mechanism, such as the regular EAThread RWSpinLockW.
		/// 
		class RWSpinLockW
		{
		public:
			RWSpinLockW();

			// This function cannot be called while the current thread  
			// already has a write lock, else this function will hang. 
			// Nor can this function can be called if the current thread  
			// already has a read lock, as it can result in a hang.
			void ReadLock();

			// This function cannot be called while the current thread  
			// already has a write lock, else this function will hang.
			// Nor can this function can be called if the current thread  
			// already has a read lock, as it can result in a hang. 
			bool ReadTryLock();

			// If this function returns true, then IsReadLocked must at that moment
			// be false.
			bool IsReadLocked() const;

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

			void WriteUnlock();

			/// Returns the platform-specific data handle for debugging uses or 
			/// other cases whereby special (and non-portable) uses are required.
			void* GetPlatformData();

		protected:
			enum Value
			{
				kWriteLockBit       = 0x80000000,
				kWriteWaitingInc    = 0x00010000,
				kReadLockInc        = 0x00000001,
				kWriteWaitingMask   = 0x7FFF0000,
				kReadLockMask       = 0x0000FFFF,
				kLockAllMask        = kWriteLockBit | kReadLockMask,
				kWriteAllMask       = kWriteLockBit | kWriteWaitingMask,
			};

			AtomicInt32 mValue;
		};



		/// RWSpinLockWFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class RWSpinLockW.
		/// A primary use of this would be to allow the RWSpinLockW implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		///
		class EATHREADLIB_API RWSpinLockWFactory
		{
		public:
			static RWSpinLockW* CreateRWSpinLockW();
			static void         DestroyRWSpinLockW(RWSpinLockW* pRWSpinLockW);
			static size_t       GetRWSpinLockWSize();
			static RWSpinLockW* ConstructRWSpinLockW(void* pMemory);
			static void         DestructRWSpinLockW(RWSpinLockW* pRWSpinLockW);
		};



		/// class AutoRWSpinLockW
		///
		/// Example usage:
		///     void Function() {
		///         AutoRWSpinLockW autoLock(AutoRWSpinLockW::kLockTypeRead);
		///         // Do something
		///     }
		///
		class AutoRWSpinLockW
		{
		public:
			enum LockType
			{
				kLockTypeRead, 
				kLockTypeWrite
			};

			AutoRWSpinLockW(RWSpinLockW& SpinLockW, LockType lockType);
		   ~AutoRWSpinLockW();

		protected:
			RWSpinLockW& mSpinLockW;
			LockType     mLockType;

			// Prevent copying by default, as copying is dangerous.
			AutoRWSpinLockW(const AutoRWSpinLockW&);
			const AutoRWSpinLockW& operator=(const AutoRWSpinLockW&);
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
		extern Allocator* gpAllocator;


		///////////////////////////////////////////////////////////////////////
		// RWSpinLockW
		///////////////////////////////////////////////////////////////////////

		inline
		RWSpinLockW::RWSpinLockW()
			: mValue(0)
		{
		}


		inline
		void RWSpinLockW::ReadLock()
		{
			int32_t currVal = mValue.GetValueRaw(); // See not above about GetValueRaw usage.

			// If there is no writer nor waiting writers, attempt a read lock.
			if( (currVal & kWriteAllMask) == 0 )                                             
			{
				if( mValue.SetValueConditional( currVal + kReadLockInc, currVal ) )
					return;
			}

			// Spin until there is no writer, any waiting writers, nor any read lockers.
			// By waiting till there no read or write lockers, we tend to avoid the case
			// whereby readers starve out writers. The downside is that a lot of read
			// activity can cause read parallelism to be reduced and read threads waiting
			// for each other. 
			do
			{
				EA_THREAD_DO_SPIN();
				currVal = mValue.GetValue();    // or EAReadBarrier(); mValue.GetValueRaw();
			}while (currVal & kLockAllMask); // or kWriteAllMask

			// At this point, we ignore waiting writers and take the lock if we 
			// can. Any waiting writers that have shown up right as we execute this 
			// code aren't given any priority over us, unlike above where they are.
			for( ;; )
			{
				// This code has a small problem in that a large number of simultaneous
				// frequently locking/unlocking readers can cause this code to spin
				// a lot (in theory, indefinitely). However, in practice our use cases
				// and target hardware shouldn't cause this to happen.
				if( (currVal & kWriteLockBit) == 0 )                                             
				{
					if( mValue.SetValueConditional( currVal + kReadLockInc, currVal ) )
						return;
				}

				EA_THREAD_DO_SPIN();
				currVal = mValue.GetValue(); // or EAReadBarrier(); mValue.GetValueRaw();
			}
		}


		inline
		bool RWSpinLockW::ReadTryLock()
		{
			int32_t currVal = mValue.GetValueRaw();

			// If there is no writer nor waiting writers, attempt a read lock.
			if( (currVal & kWriteAllMask) == 0 )                                             
			{
				if( mValue.SetValueConditional( currVal + kReadLockInc, currVal ) )
					return true;
			}

			return false;
		}


		inline
		bool RWSpinLockW::IsReadLocked() const
		{
			// This return value has only diagnostic meaning. It cannot be used for thread synchronization purposes.
			return ((mValue.GetValueRaw() & kReadLockMask) != 0);
		}


		inline
		void RWSpinLockW::ReadUnlock()
		{
			EAT_ASSERT(IsReadLocked());  // This can't tell us if the current thread was one of the lockers. But it's better than nothing as a debug test.
			mValue.Add( -kReadLockInc );
		}


		inline
		void RWSpinLockW::WriteLock()
		{
			int32_t currVal = mValue.GetValueRaw();

			// If there is no writer, waiting writers, nor readers, attempt a write lock.
			if( (currVal & kLockAllMask) == 0 )                                             
			{
				if( mValue.SetValueConditional( currVal | kWriteLockBit, currVal ) )
					return;
			}

			// Post a waiting write. This will make new readers spin until all existing
			// readers have released their lock, so that we get an even chance.
			mValue.Add( kWriteWaitingInc );

			// Spin until we get the lock.
			for( ;; )
			{
				if( (currVal & kLockAllMask) == 0 )                                             
				{
					if( mValue.SetValueConditional( (currVal | kWriteLockBit) - kWriteWaitingInc, currVal ) )
						return;
				}

				EA_THREAD_DO_SPIN();
				currVal = mValue.GetValue(); // or EAReadBarrier(); mValue.GetValueRaw();
			}
		}


		inline
		bool RWSpinLockW::WriteTryLock()
		{
			int32_t currVal = mValue.GetValueRaw();

			// If there is no writer, waiting writers, nor readers, attempt a write lock.
			if( (currVal & kLockAllMask) == 0 )                                             
			{
				if( mValue.SetValueConditional( currVal | kWriteLockBit, currVal ) )
					return true;
			}

			return false;
		}


		inline
		bool RWSpinLockW::IsWriteLocked() const
		{
			// This return value has only diagnostic meaning. It cannot be used for thread synchronization purposes.
			return ( (mValue.GetValueRaw() & kWriteLockBit) != 0 );
		}


		inline
		void RWSpinLockW::WriteUnlock()
		{
			EAT_ASSERT(IsWriteLocked());
			mValue.Add( -kWriteLockBit );
		}


		inline
		void* RWSpinLockW::GetPlatformData()
		{
			return &mValue;
		}



		///////////////////////////////////////////////////////////////////////
		// RWSpinLockFactory
		///////////////////////////////////////////////////////////////////////

		inline
		RWSpinLockW* RWSpinLockWFactory::CreateRWSpinLockW() 
		{
			if(gpAllocator)
				return new(gpAllocator->Alloc(sizeof(RWSpinLockW))) RWSpinLockW;
			else
				return new RWSpinLockW;
		}

		inline
		void RWSpinLockWFactory::DestroyRWSpinLockW(RWSpinLockW* pRWSpinLock)
		{
			if(gpAllocator)
			{
				pRWSpinLock->~RWSpinLockW();
				gpAllocator->Free(pRWSpinLock);
			}
			else
				delete pRWSpinLock;
		}

		inline
		size_t RWSpinLockWFactory::GetRWSpinLockWSize()
		{
			return sizeof(RWSpinLockW);
		}

		inline
		RWSpinLockW* RWSpinLockWFactory::ConstructRWSpinLockW(void* pMemory)
		{
			return new(pMemory) RWSpinLockW;
		}

		inline
		void RWSpinLockWFactory::DestructRWSpinLockW(RWSpinLockW* pRWSpinLock)
		{
			pRWSpinLock->~RWSpinLockW();
		}



		///////////////////////////////////////////////////////////////////////
		// AutoRWSpinLock
		///////////////////////////////////////////////////////////////////////

		inline
		AutoRWSpinLockW::AutoRWSpinLockW(RWSpinLockW& spinLock, LockType lockType) 
			: mSpinLockW(spinLock), mLockType(lockType)
		{ 
			if(mLockType == kLockTypeRead)
				mSpinLockW.ReadLock();
			else
				mSpinLockW.WriteLock();
		}


		inline
		AutoRWSpinLockW::~AutoRWSpinLockW()
		{ 
			if(mLockType == kLockTypeRead)
				mSpinLockW.ReadUnlock();
			else
				mSpinLockW.WriteUnlock();
		}


	} // namespace Thread

} // namespace EA



#ifdef _MSC_VER
	#pragma warning(pop)
#endif


#endif // Header include guard













