///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements an efficient proper multithread-safe spinlock.
//
// A spin lock is the lightest form of mutex available. The Lock operation is
// simply a loop that waits to set a shared variable. SpinLocks are not 
// recursive (i.e. they can only be locked once by a thread) and are 
// intra-process only. You have to be careful using spin locks because if you 
// have a high priority thread that calls Lock while a lower priority thread
// has the same lock, then on many systems the higher priority thread will 
// use up all the CPU time waiting for the lock and the lower priority thread
// will not get the CPU time needed to free the lock.
//
// From Usenet:
//    A spinlock is a machine-specific "optimized" form of mutex
//    ("MUTual EXclusion" device). However, you should never use
//    a spinlock unless you know that you have multiple threads
//    and that you're running on a multiprocessor. Otherwise, at
//    best you're wasting a lot of time. A spinlock is great for
//    "highly parallel" algorithms like matrix decompositions,
//    where the application (or runtime) "knows" (or at least goes
//    to lengths to ensure) that the threads participating are all
//    running at the same time. Unless you know that, (and, if your
//    code doesn't create threads, you CAN'T know that), don't even
//    think of using a spinlock."
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_SPINLOCK_H
#define EATHREAD_EATHREAD_SPINLOCK_H


#include <EABase/eabase.h>
#include <eathread/eathread.h>
#include <new> // include new for placement new operator

#if defined(EA_PROCESSOR_X86)
	// The reference x86 code works fine, as there is little that assembly
	// code can do to improve it by much, assuming that the code is compiled
	// in an optimized way. With VC7 on the PC platform, compiling with 
	// optimization set to 'minimize size' and most other optimizations 
	// enabled yielded code that was similar to Intel reference asm code.
	// However, when the compiler was set to minimize size and enable inlining,
	// it created an implementation of the Lock function that was less optimal.
	// #include <eathread/x86/eathread_spinlock_x86.h>
#elif defined(EA_PROCESSOR_IA64)
	// The reference code below is probably fine.
	// #include <eathread/ia64/eathread_spinlock_ia64.h>
#endif

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



// The above header files would define EA_THREAD_SPINLOCK_IMPLEMENTED.
#if !defined(EA_THREAD_SPINLOCK_IMPLEMENTED)

	// We provide an implementation that works for all systems but is less optimal.
	#include <eathread/eathread_sync.h>
	#include <eathread/eathread_atomic.h>

	namespace EA
	{
		namespace Thread
		{
			/// class SpinLock
			///
			/// Spinlocks are high-performance locks designed for special circumstances.
			/// As such, they are not 'recursive' -- you cannot lock a spinlock twice.
			/// Spinlocks have no explicit awareness of threading, but they are explicitly
			/// thread-safe. 
			///
			/// You do not want to use spin locks as a *general* replacement for mutexes or
			/// critical sections, even if you know your mutex use won't be recursive.
			/// The reason for this is due to thread scheduling and thread priority issues.
			/// A spinlock is not a kernel- or threading-kernel-level object and thus while
			/// this gives it a certain amount of speed, it also means that if you have a 
			/// low priority thread thread with a spinlock locked and a high priority thread
			/// waiting for the spinlock, the program will hang, possibly indefinitely,
			/// because the thread scheduler is giving all its time to the high priority 
			/// thread which happens to be stuck. 
			/// 
			/// On the other hand, when judiciously used, a spin lock can yield significantly
			/// higher performance than general mutexes, especially on platforms where mutex
			/// locking is particularly expensive or on multiprocessing systems.
			///
			class SpinLock
			{
			protected: // Declared at the top because otherwise some compilers fail to compile inline functions below.
				AtomicInt32 mAI;  /// A value of 0 means unlocked, while 1 means locked.

			public:
				SpinLock();

				void Lock();
				bool TryLock();
				bool IsLocked();
				void Unlock();

				void* GetPlatformData();
			};


			/// SpinLockFactory
			/// 
			/// Implements a factory-based creation and destruction mechanism for class Spinlock.
			/// A primary use of this would be to allow the Spinlock implementation to reside in
			/// a private library while users of the class interact only with the interface
			/// header and the factory. The factory provides conventional create/destroy 
			/// semantics which use global operator new, but also provides manual construction/
			/// destruction semantics so that the user can provide for memory allocation 
			/// and deallocation.
			class EATHREADLIB_API SpinLockFactory
			{
			public:
				static SpinLock* CreateSpinLock();
				static void      DestroySpinLock(SpinLock* pSpinLock);

				static size_t    GetSpinLockSize();
				static SpinLock* ConstructSpinLock(void* pMemory);

				static void DestructSpinLock(SpinLock* pSpinLock);
			};

		} // namespace Thread

	} // namespace EA


#endif // EA_THREAD_SPINLOCK_IMPLEMENTED



namespace EA
{
	namespace Thread
	{
		/// class AutoSpinLock
		/// An AutoSpinLock locks the SpinLock in its constructor and 
		/// unlocks the SpinLock in its destructor (when it goes out of scope).
		class AutoSpinLock
		{
		public:
			AutoSpinLock(SpinLock& spinLock);
		   ~AutoSpinLock();

		protected:
			SpinLock& mSpinLock;

		protected:
			// Prevent copying by default, as copying is dangerous.
			AutoSpinLock(const AutoSpinLock&);
			const AutoSpinLock& operator=(const AutoSpinLock&);
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
		// SpinLock
		///////////////////////////////////////////////////////////////////////

		inline
		SpinLock::SpinLock() 
		  : mAI(0)
		{
		}

		inline
		void SpinLock::Lock()
		{
			Top: // Due to modern processor branch prediction, the compiler will optimize better for true branches and so we do a manual goto loop here.
			if(mAI.SetValueConditional(1, 0))
				return;

			// The loop below is present because the SetValueConditional 
			// call above is likely to be significantly more expensive and 
			// thus we benefit by polling before attempting the real thing.
			// This is a common practice and is recommended by Intel, etc.
			while (mAI.GetValue() != 0)
			{
			#ifdef EA_THREAD_COOPERATIVE
				ThreadSleep();
			#else
				EAProcessorPause();
			#endif
			}
			goto Top;                                          
		}                                                

		inline
		bool SpinLock::TryLock()
		{
			return mAI.SetValueConditional(1, 0);
		}

		inline
		bool SpinLock::IsLocked()
		{
			return mAI.GetValueRaw() != 0;
		}

		inline
		void SpinLock::Unlock()
		{
			EAT_ASSERT(IsLocked());
			mAI.SetValue(0);
		}

		inline
		void* SpinLock::GetPlatformData()
		{
			return &mAI;
		}


		///////////////////////////////////////////////////////////////////////
		// SpinLockFactory
		///////////////////////////////////////////////////////////////////////

		inline
		SpinLock* SpinLockFactory::CreateSpinLock()
		{
			if(gpAllocator)
				return new(gpAllocator->Alloc(sizeof(SpinLock))) SpinLock;
			else
				return new SpinLock;
		}

		inline
		void SpinLockFactory::DestroySpinLock(SpinLock* pSpinLock)
		{
			if(gpAllocator)
			{
				pSpinLock->~SpinLock();
				gpAllocator->Free(pSpinLock);
			}
			else
				delete pSpinLock;
		}

		inline
		size_t SpinLockFactory::GetSpinLockSize()
		{
			return sizeof(SpinLock);
		}

		inline
		SpinLock* SpinLockFactory::ConstructSpinLock(void* pMemory)
		{
			return new(pMemory) SpinLock;
		}

		EA_DISABLE_VC_WARNING(4100) // Compiler mistakenly claims pSpinLock is unreferenced
		inline
		void SpinLockFactory::DestructSpinLock(SpinLock* pSpinLock)
		{
			pSpinLock->~SpinLock();
		}
		EA_RESTORE_VC_WARNING()


		///////////////////////////////////////////////////////////////////////
		// AutoSpinLock
		///////////////////////////////////////////////////////////////////////

		inline
		AutoSpinLock::AutoSpinLock(SpinLock& spinLock) 
		  : mSpinLock(spinLock)
		{
			mSpinLock.Lock();
		}

		inline
		AutoSpinLock::~AutoSpinLock()
		{
			mSpinLock.Unlock();
		}

	} // namespace Thread

} // namespace EA

#endif // EATHREAD_EATHREAD_SPINLOCK_H













