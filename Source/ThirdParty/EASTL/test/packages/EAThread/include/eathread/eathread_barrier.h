///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements Posix-style barriers.
// Note that thread synchronization barriers are different from 
// memory synchronization barriers (a.k.a. fences).
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_BARRIER_H
#define EATHREAD_EATHREAD_BARRIER_H


#include <EABase/eabase.h>
#include <eathread/eathread.h>


#if defined(EA_DLL) && defined(_MSC_VER)
	// Suppress warning about class 'AtomicInt32' needs to have a
	// dll-interface to be used by clients of class which have a templated member.
	// 
	// These templates cannot be instantiated outside of the DLL. If you try, a
	// link error will result. This compiler warning is intended to notify users
	// of this.
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



/////////////////////////////////////////////////////////////////////////
/// EABarrierData
///
/// This is used internally by class Barrier.
/// Todo: Consider moving this declaration into a platform-specific 
/// header file.
/// 

#if defined(EA_PLATFORM_SONY)
	#include <kernel.h>
	#include <eathread/internal/timings.h>

	// We implement the barrier manually, as not all Posix thread implementations
	// have barriers and even those that have it lack a timeout wait version.
	struct EABarrierData{
		ScePthreadCond  mCV;            // Wait for barrier.
		ScePthreadMutex mMutex;         // Control access to barrier.
		int             mnHeight;       // Number of threads required.
		int             mnCurrent;      // Current number of threads. As threads wait, this value decreases towards zero.
		unsigned long   mnCycle;        // Cycle count.
		bool            mbValid;        // True if valid.

		EABarrierData();
	};

#elif (defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE) && EA_THREADS_AVAILABLE
	#include <pthread.h>

	// We implement the barrier manually, as not all Posix threads implemetnation 
	// have barrers and even those that have it lack a timeout wait version.
	struct EABarrierData{
		pthread_cond_t  mCV;            // Wait for barrier.
		pthread_mutex_t mMutex;         // Control access to barrier.
		int             mnHeight;       // Number of threads required.
		int             mnCurrent;      // Current number of threads. As threads wait, this value decreases towards zero.
		unsigned long   mnCycle;        // Cycle count.
		bool            mbValid;        // True if valid.

		EABarrierData();
	};

#else // All other platforms
	#include <eathread/eathread_atomic.h>
	#include <eathread/eathread_semaphore.h>

	struct EATHREADLIB_API EABarrierData{
		EA::Thread::AtomicInt32    mnCurrent;       // Current number of threads. As threads wait, this value decreases towards zero.
		int                        mnHeight;        // Number of threads required.
		EA::Thread::AtomicInt32    mnIndex;         // Which semaphore we are using.
		EA::Thread::Semaphore      mSemaphore0;     // First semaphore.     We can't use an array of Semaphores, because that would
		EA::Thread::Semaphore      mSemaphore1;     // Second semaphore.    intefere with our ability to initialize them our way.
		EABarrierData();

	private:
		// Prevent default generation of these functions by declaring but not defining them.
		EABarrierData(const EABarrierData& rhs);               // copy constructor
		EABarrierData& operator=(const EABarrierData& rhs);    // assignment operator
	};

#endif




namespace EA
{
	namespace Thread
	{
		/// BarrierParameters
		/// Specifies barrier settings.
		struct EATHREADLIB_API BarrierParameters
		{
			int  mHeight;        /// Barrier 'height'. Refers to number of threads which must wait before being released.
			bool mbIntraProcess; /// True if the semaphore is intra-process, else inter-process.
			char mName[16];      /// Barrier name, applicable only to platforms that recognize named synchronization objects.

			BarrierParameters(int height = 0, bool bIntraProcess = true, const char* pName = NULL);
		};


		/// Barrier
		/// A Barrier is a synchronization point for a set of threads. A barrier has
		/// a count associated with it and threads call the wait function until the
		/// given count of threads have reached the wait point. Then all threads 
		/// are released. The first thread released is given a special return value
		/// that identifies it uniquely so that one-time work can be done. 
		///
		/// A primary use of barriers is to spread out work between a number of threads 
		/// and wait until the work is complete. For example, if you want to find and
		/// count all objects of a given kind in a large grid, you might have four 
		/// threads each work on a quadrant and wait on the barrier until all are
		/// finished. This particular example is more practical on SMP systems than
		/// uniprocessor systems, but there are also uniprocessor uses. It should be
		/// noted, however, that a Barrier synchronizes the completion of -threads-, 
		/// and not necessarily the completion of -tasks-. There may or may not be 
		/// a direct correspondence between the two.
		///
		class EATHREADLIB_API Barrier
		{
		public:
			enum Result{
				kResultPrimary   =  0,  /// The barrier wait suceeded and this thread is the designated solitary primary thread. Similar to Posix "serial" thread.
				kResultSecondary =  1,  /// The barrier wait suceeded and this thread is one of the secondary threads.
				kResultError     = -1,  /// The wait resulted in error, due to various possible reasons.
				kResultTimeout   = -2   /// The barrier wait timed out.
			};

			/// Barrier
			/// For immediate default initialization, use no args.
			/// For custom immediate initialization, supply a first argument. 
			/// For deferred initialization, use Barrier(NULL, false) then later call Init.
			/// For deferred initialization of an array of objects, create an empty
			/// subclass whose default constructor chains back to Barrier(NULL, false).
			Barrier(const BarrierParameters* pBarrierParameters = NULL, bool bDefaultParameters = true);

			/// Barrier
			/// This is a constructor which initializes the Barrier to a specific height 
			/// and intializes the other Barrier parameters to default values. See the
			/// BarrierParameters struct for info on these default values.
			Barrier(int height);

			/// ~Barrier
			/// Destroys an existing Barrier. The Barrier must not be waited on 
			/// by any thread, otherwise the resulting behaviour is undefined.
			~Barrier();

			/// Init
			/// Initializes the Barrier; used in cases where it cannot be initialized
			/// via the constructor (as in the case with default construction or 
			/// array initialization.
			bool Init(const BarrierParameters* pBarrierParameters);

			/// Wait
			/// Causes the current thread to wait until the designated number of threads have called Wait. 
			///
			/// Returns one of enum Result.
			///
			/// A timeout means that the thread gives up its contribution to the height while 
			/// waiting for the full height to be achieved. A timeout of zero means that a thread 
			/// only succeeds if it is the final thread (the one which puts the height to full); 
			/// otherwise the call returns with a timeout.
			///
			/// Note that the timeout is specified in absolute time and not relative time.
			///
			/// Note also that due to the way thread scheduling works -- particularly in a
			/// time-sliced threading environment -- that the timeout value is a hint and 
			/// the actual amount of time passed before the timeout occurs may be significantly
			/// more or less than the specified timeout time.
			///
			Result Wait(const ThreadTime& timeoutAbsolute = kTimeoutNone);

			/// GetPlatformData
			/// Returns the platform-specific data handle for debugging uses or 
			/// other cases whereby special (and non-portable) uses are required.
			void* GetPlatformData()
				{ return &mBarrierData; }

		protected:
			EABarrierData mBarrierData;

		private:
			// Objects of this class are not copyable.
			Barrier(const Barrier&){}
			Barrier& operator=(const Barrier&){ return *this; }
		};


		/// BarrierFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class Barrier.
		/// A primary use of this would be to allow the Barrier implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		class EATHREADLIB_API BarrierFactory
		{
		public:
			static Barrier*    CreateBarrier();                    // Internally implemented as: return new Barrier;
			static void        DestroyBarrier(Barrier* pBarrier);  // Internally implemented as: delete pBarrier;

			static size_t      GetBarrierSize();                   // Internally implemented as: return sizeof(Barrier);
			static Barrier*    ConstructBarrier(void* pMemory);    // Internally implemented as: return new(pMemory) Barrier;
			static void        DestructBarrier(Barrier* pBarrier); // Internally implemented as: pBarrier->~Barrier();
		};


	} // namespace Thread

} // namespace EA


#if defined(EA_DLL) && defined(_MSC_VER)
   // re-enable warning(s) disabled above.
   #pragma warning(pop)
#endif


#endif // EATHREAD_EATHREAD_BARRIER_H













