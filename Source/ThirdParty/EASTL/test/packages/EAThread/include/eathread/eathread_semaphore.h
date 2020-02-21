///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements a semaphore thread synchronization class.
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_SEMAPHORE_H
#define EATHREAD_EATHREAD_SEMAPHORE_H


#include <eathread/internal/config.h>
#include <eathread/eathread.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



///////////////////////////////////////////////////////////////////////////////
// EATHREAD_USE_SYNTHESIZED_SEMAPHORE
//
// Defined as 0 or 1. Defined as 1 if the OS provides no native semaphore support.
//
#ifndef EATHREAD_USE_SYNTHESIZED_SEMAPHORE
	#define EATHREAD_USE_SYNTHESIZED_SEMAPHORE 0
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_FAST_MS_SEMAPHORE_ENABLED
//
// Defined as 0 or 1.
// Enables the usage of a faster intra-process semaphore on Microsoft platforms.
// By faster we mean that it is typically 10x or more faster.
// Has the downside that it is not interchangeable with the SEMAPHORE built-in
// type and it's behaviour won't be strictly identical.
// Even if this option is enabled, you can still get the built-in behaviour
// of Microsoft semaphores by specifying the semaphore as inter-process.
//
#ifndef EATHREAD_FAST_MS_SEMAPHORE_ENABLED
	#define EATHREAD_FAST_MS_SEMAPHORE_ENABLED 1
#endif


/////////////////////////////////////////////////////////////////////////
/// EASemaphoreData
///
/// This is used internally by class Semaphore.
/// Todo: Consider moving this declaration into a platform-specific 
/// header file.
/// 
#if !EA_THREADS_AVAILABLE
	struct EASemaphoreData
	{
		volatile int mnCount;
		int mnMaxCount;

		EASemaphoreData();
	};

#elif EATHREAD_USE_SYNTHESIZED_SEMAPHORE
	#include <eathread/eathread_condition.h>
	#include <eathread/eathread_mutex.h>
	#include <eathread/eathread_atomic.h>

	struct EASemaphoreData
	{
		EA::Thread::Condition   mCV;
		EA::Thread::Mutex       mMutex;
		EA::Thread::AtomicInt32 mnCount;
		int                     mnMaxCount;
		bool                    mbValid;

		EASemaphoreData();
	};

#elif defined(__APPLE__)

	#include <mach/semaphore.h>
	#include <eathread/eathread_atomic.h>

	struct EASemaphoreData
	{
		semaphore_t mSemaphore;
		EA::Thread::AtomicInt32 mnCount;
		int  mnMaxCount;
		bool mbIntraProcess;
		
		EASemaphoreData();
	};

#elif defined(EA_PLATFORM_SONY)
	#include <kernel/semaphore.h>
	#include <eathread/eathread_atomic.h>
	#include <eathread/internal/timings.h>
	struct EASemaphoreData
	{
		SceKernelSema mSemaphore;

		int  mnMaxCount;
		EA::Thread::AtomicInt32 mnCount;

		EASemaphoreData();
	};

#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
	#include <semaphore.h>
	#include <eathread/eathread_atomic.h>

	#if defined(EA_PLATFORM_WINDOWS)
		#ifdef CreateSemaphore
			#undef CreateSemaphore // Windows #defines CreateSemaphore to CreateSemaphoreA or CreateSemaphoreW.
		#endif
	#endif

	struct EASemaphoreData
	{
		sem_t mSemaphore;
		EA::Thread::AtomicInt32 mnCount;
		int  mnMaxCount;
		bool mbIntraProcess;

		EASemaphoreData();
	};

#elif defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE
	#ifdef CreateSemaphore
		#undef CreateSemaphore // Windows #defines CreateSemaphore to CreateSemaphoreA or CreateSemaphoreW.
	#endif

	struct EATHREADLIB_API EASemaphoreData
	{
		void*   mhSemaphore;    // We use void* instead of HANDLE in order to avoid #including windows.h. HANDLE is typedef'd to (void*) on all Windows-like platforms.
		int32_t mnCount;        // Number of available posts. Under the fast semaphore pathway, a negative value means there are waiters.
		int32_t mnCancelCount;  // Used by fast semaphore logic. Is the deferred cancel count.
		int32_t mnMaxCount;     // 
		bool    mbIntraProcess; // Used under Windows, which can have multiple processes. Always true for XBox.

		EASemaphoreData();
		void UpdateCancelCount(int32_t n);
	};

#endif
/////////////////////////////////////////////////////////////////////////





namespace EA
{
	namespace Thread
	{
		/// SemaphoreParameters
		/// Specifies semaphore settings.
		struct EATHREADLIB_API SemaphoreParameters
		{
			int  mInitialCount;  /// Initial available count
			int  mMaxCount;      /// Max possible count. Defaults to INT_MAX.
			bool mbIntraProcess; /// True if the semaphore is intra-process, else inter-process.
			char mName[16];      /// Semaphore name, applicable only to platforms that recognize named synchronization objects.

			SemaphoreParameters(int initialCount = 0, bool bIntraProcess = true, const char* pName = NULL);
		};


		/// class Semaphore
		/// A semaphore is an object which has an associated count which is >= 0 and
		/// a value > 0 means that a thread can 'grab' the semaphore and decrement its
		/// value by one. A value of 0 means that threads must wait until another thread
		/// 'un-grabs' the semaphore. Thus a semaphore is like a car rental agency which
		/// has a limited number of cars for rent and if they are out of cars, you have 
		/// to wait until one of the renters returns their car.
		class EATHREADLIB_API Semaphore
		{
		public:
			enum Result{
				kResultError   = -1,
				kResultTimeout = -2
			};

			/// Semaphore
			/// For immediate default initialization, use no args.
			/// For custom immediate initialization, supply a first argument. 
			/// For deferred initialization, use Semaphore(NULL, false) then later call Init.
			/// For deferred initialization of an array of objects, create an empty
			/// subclass whose default constructor chains back to Semaphore(NULL, false).
			Semaphore(const SemaphoreParameters* pSemaphoreParameters = NULL, bool bDefaultParameters = true);

			/// Semaphore
			/// This is a constructor which initializes the Semaphore to a specific count 
			/// and intializes the other Semaphore parameters to default values. See the
			/// SemaphoreParameters struct for info on these default values.
			Semaphore(int initialCount);

			/// ~Semaphore
			/// Destroys an existing semaphore. The semaphore must not be locked 
			/// by any thread, otherwise the resulting behaviour is undefined.
			~Semaphore();

			/// Init
			/// Initializes the semaphore with given parameters.
			bool Init(const SemaphoreParameters* pSemaphoreParameters);

			/// Wait
			/// Locks the semaphore (reducing its count by one) or gives up trying to 
			/// lock it after a given timeout has expired. If the semaphore count is > 0
			/// then the count will be reduced by one. If the semaphore count is 0, the
			/// call will block until another thread unlocks it or the timeout expires.
			///
			/// Note that the timeout is specified in absolute time and not relative time.
			///
			/// Note also that due to the way thread scheduling works -- particularly in a
			/// time-sliced threading environment -- that the timeout value is a hint and 
			/// the actual amount of time passed before the timeout occurs may be significantly
			/// more or less than the specified timeout time.
			///
			/// Return value:
			///     kResultError      The semaphore could not be obtained due to error.
			///     kResultTimeout    The semaphore could not be obtained due to timeout.
			///     >= 0              The new count for the semaphore.
			///
			/// It's possible that two threads waiting on the same semaphore will return 
			/// with a result of zero. Thus you cannot rely on the semaphore's return value
			/// to ascertain which was the last thread to return from the Wait. 
			int Wait(const ThreadTime& timeoutAbsolute = kTimeoutNone);

			/// Post
			/// Increments the signalled value of the semaphore by the count. 
			/// Returns the available count after the operation has completed. 
			/// Returns kResultError upon error. A Wait is often eventually 
			/// followed by a corresponding Post.
			/// For the case of count being greater than 1, not all platforms
			/// act the same. If count results in exceeding the max count then
			/// kResultError is returned. Some platforms return kResultError 
			/// before any of account is applied, while others return 
			/// kResultError after some of count has been applied.
			int Post(int count = 1);

			/// GetCount
			/// Returns current number of available locks associated with the semaphore.
			/// This is useful for debugging and for quick polling checks of the 
			/// status of the semaphore. This value changes over time as multiple
			/// threads wait and post to the semaphore. This value cannot be trusted
			/// to exactly represent the count upon its return if multiple threads are 
			/// using this Semaphore at the time.
			int GetCount() const;

			/// GetPlatformData
			/// Returns the platform-specific data handle for debugging uses or 
			/// other cases whereby special (and non-portable) uses are required.
			void* GetPlatformData()
				{ return &mSemaphoreData; }

		protected:
			EASemaphoreData mSemaphoreData;

		private:
			// Objects of this class are not copyable.
			Semaphore(const Semaphore&){}
			Semaphore& operator=(const Semaphore&){ return *this; }
		};


		/// SemaphoreFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class Semaphore.
		/// A primary use of this would be to allow the Semaphore implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		class EATHREADLIB_API SemaphoreFactory
		{
		public:
			static Semaphore* CreateSemaphore();                        // Internally implemented as: return new Semaphore;
			static void       DestroySemaphore(Semaphore* pSemaphore);  // Internally implemented as: delete pSemaphore;

			static size_t     GetSemaphoreSize();                       // Internally implemented as: return sizeof(Semaphore);
			static Semaphore* ConstructSemaphore(void* pMemory);        // Internally implemented as: return new(pMemory) Semaphore;
			static void       DestructSemaphore(Semaphore* pSemaphore); // Internally implemented as: pSemaphore->~Semaphore();
		};


	} // namespace Thread

} // namespace EA




namespace EA
{
	namespace Thread
	{
		/// class AutoSemaphore
		/// An AutoSemaphore grabs the Semaphore in its constructor and posts 
		/// the Semaphore once in its destructor (when it goes out of scope).
		class EATHREADLIB_API AutoSemaphore
		{
		public:
			AutoSemaphore(Semaphore& semaphore) 
				: mSemaphore(semaphore)
				{ mSemaphore.Wait(); }

			~AutoSemaphore()
				{ mSemaphore.Post(1); }

		protected:
			Semaphore& mSemaphore;

			// Prevent copying by default, as copying is dangerous.
			AutoSemaphore(const AutoSemaphore&);
			const AutoSemaphore& operator=(const AutoSemaphore&);
		};

	} // namespace Thread

} // namespace EA

#endif // EATHREAD_EATHREAD_SEMAPHORE_H













