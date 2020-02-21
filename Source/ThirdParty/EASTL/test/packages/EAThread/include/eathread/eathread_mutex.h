///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements a lightweight mutex.
/////////////////////////////////////////////////////////////////////////////

// TODO(rparolin):  Consider adding support for static thread safety analysis.
// https://clang.llvm.org/docs/ThreadSafetyAnalysis.html


#ifndef EATHREAD_EATHREAD_MUTEX_H
#define EATHREAD_EATHREAD_MUTEX_H

#if defined(_MSC_VER)
#include <math.h>   // #include math.h because VC++ has a header file but that requires math.h to be #included before some other headers, lest you get a warning.
#endif
#include <stddef.h>
#include <eathread/internal/config.h>
#include <eathread/eathread.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



/////////////////////////////////////////////////////////////////////////
/// EAMutexData
///
/// This is used internally by class Mutex.
/// Todo: Consider moving this declaration into a platform-specific 
/// header file.
/// 
#if !EA_THREADS_AVAILABLE
	#define EA_THREAD_NONTHREADED_MUTEX 1

	struct EAMutexData
	{
		int mnLockCount;

		EAMutexData();
	};

#elif EA_USE_CPP11_CONCURRENCY
	EA_DISABLE_ALL_VC_WARNINGS()
	#include <mutex>
	EA_RESTORE_ALL_VC_WARNINGS()

	#if defined EA_PLATFORM_MICROSOFT
		#ifdef CreateMutex
			#undef CreateMutex // Windows #defines CreateMutex to CreateMutexA or CreateMutexW.
		#endif
	#endif

	struct EAMutexData
	{
		std::recursive_timed_mutex mMutex;
		int mnLockCount;
		#if EAT_ASSERT_ENABLED
			EA::Thread::ThreadId mThreadId; // This value is only valid in debug builds.
		#endif

		EAMutexData();

	private:
		EAMutexData(const EAMutexData&);
		EAMutexData& operator=(const EAMutexData&);
	};

#elif defined(EA_PLATFORM_SONY)
	#include <kernel.h>
	#include <eathread/internal/timings.h>

	struct EAMutexData
	{
		ScePthreadMutex mMutex;
		int mnLockCount;
		#if EAT_ASSERT_ENABLED
			EA::Thread::ThreadId mThreadId;    // This value is only valid in debug builds.
		#endif

		EAMutexData();
		void SimulateLock(bool bLock);
	};

#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
	#include <pthread.h>

	#if defined(EA_PLATFORM_WINDOWS)
		#ifdef CreateMutex
			#undef CreateMutex // Windows #defines CreateMutex to CreateMutexA or CreateMutexW.
		#endif
	#endif

	struct EAMutexData
	{
		pthread_mutex_t mMutex;
		int mnLockCount;
		#if EAT_ASSERT_ENABLED
			EA::Thread::ThreadId mThreadId;    // This value is only valid in debug builds.
		#endif

		EAMutexData();
		void SimulateLock(bool bLock);
	};

#elif defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE

	#ifdef EA_PROCESSOR_X86_64
		static const int MUTEX_PLATFORM_DATA_SIZE = 40; // CRITICAL_SECTION is 40 bytes on Win64.
	#else
		static const int MUTEX_PLATFORM_DATA_SIZE = 32; // CRITICAL_SECTION is 24 bytes on Win32, 28 bytes on XBox 360.
	#endif

	#ifdef CreateMutex
		#undef CreateMutex // Windows #defines CreateMutex to CreateMutexA or CreateMutexW.
	#endif

	struct EATHREADLIB_API EAMutexData
	{
		uint64_t mData[MUTEX_PLATFORM_DATA_SIZE / sizeof(uint64_t)]; // Holds either CRITICAL_SECTION or HANDLE if mbIntraProcess is true or false, respectively.
		int      mnLockCount;
		bool     mbIntraProcess;
		#if EAT_ASSERT_ENABLED
			EA::Thread::ThreadId mThreadId;    // This value is only valid in debug builds.
			EA::Thread::SysThreadId mSysThreadId; // This value is only valid in debug builds.
		#endif

		EAMutexData();
	};

#else
	#define EA_THREAD_NONTHREADED_MUTEX 1

	struct EAMutexData
	{
		int mnLockCount;

		EAMutexData();
	};



#endif
/////////////////////////////////////////////////////////////////////////




namespace EA
{
	namespace Thread
	{
		/// MutexParameters
		/// Specifies mutex settings.
		struct EATHREADLIB_API MutexParameters
		{
			bool mbIntraProcess; /// True if the mutex is intra-process, else inter-process.
			char mName[128];      /// Mutex name, applicable only to platforms that recognize named synchronization objects.

			MutexParameters(bool bIntraProcess = true, const char* pName = NULL);
		};


		/// class Mutex
		///
		/// Mutex are assumed to always be 'recursive', meaning that a given thread 
		/// can lock the mutex more than once. If you want a specifically non-recursive 
		/// mutex, you can use a semaphore with a lock count of 1.
		class EATHREADLIB_API Mutex
		{
		public:
			enum Result
			{
				kResultError   = -1,
				kResultTimeout = -2
			};

			/// Mutex
			/// For immediate default initialization, use no args.
			/// For custom immediate initialization, supply a first argument. 
			/// For deferred initialization, use Mutex(NULL, false) then later call Init.
			/// For deferred initialization of an array of objects, create an empty
			/// subclass whose default constructor chains back to Mutex(NULL, false).
			Mutex(const MutexParameters* pMutexParameters = NULL, bool bDefaultParameters = true);

			/// ~Mutex
			/// Destroys an existing mutex. The mutex must not be locked by any thread, 
			/// otherwise the resulting behaviour is undefined.
			~Mutex();

			/// Init
			/// Initializes the mutex if not done so in the constructor.
			/// This should only be called in the case that this class was constructed 
			/// with RWMutex(NULL, false).
			bool Init(const MutexParameters* pMutexParameters);

			/// Lock
			/// Locks the mutex, with a timeout specified. This function will
			/// return immediately if the mutex is not locked or if the calling
			/// thread already has it locked at least once. If the mutex is 
			/// locked by another thread, this function will block until the mutex
			/// is unlocked by the owning thread or until the timeout time has
			/// passed. This function may return before the specified timeout has passed
			/// and so should not be implicitly used as a timer. Some platforms may 
			/// return immediately if the timeout is specified as anything but kTimeoutNone.
			///
			/// Note that the timeout is specified in absolute time and not relative time.
			///
			/// Note also that due to the way thread scheduling works -- particularly in a
			/// time-sliced threading environment -- that the timeout value is a hint and 
			/// the actual amount of time passed before the timeout occurs may be significantly
			/// more or less than the specified timeout time.
			///
			/// Return value:
			///     kResultError   Error
			///     kResultTimeout Timeout
			///     > 0            The new lock count.
			int Lock(const ThreadTime& timeoutAbsolute = EA::Thread::kTimeoutNone);

			/// Unlock
			/// Unlocks the mutex. The mutex must already be locked at least once by 
			/// the calling thread. Otherwise the behaviour is not defined.
			/// Return value is the lock count value immediately upon unlock.
			int Unlock();

			/// GetLockCount
			/// Returns the number of locks on the mutex. The return value from this 
			/// function is only reliable if the calling thread already has one lock on 
			/// the critical section. Otherwise the value could be changing as other 
			/// threads lock or unlock the mutex soon after the call.
			/// This function is useful in debugging and asserting and useful for backing
			/// out of recursive locks under the case of exceptions and other abortive 
			/// situations. This function will not necessarily call memory synchronization 
			/// primitives (e.g. ReadBarrier) itself on systems that require SMP synchronization.
			int GetLockCount() const;


			/// HasLock
			/// Returns true if the current thread has the mutex locked. 
			/// This function is reliable only in a debug build whereby 
			/// EAT_ASSERT_ENABLED is defined to 1. This function can thus
			/// only be used in debugging situations whereby you want to 
			/// assert that you have a mutex locked or not. To make this 
			/// function work in a non-debug environment would necessitate
			/// adding an undesirable amount of code and data.
			bool HasLock() const;

			/// GetPlatformData
			/// Returns the platform-specific data handle for debugging uses or 
			/// other cases whereby special (and non-portable) uses are required.
			void* GetPlatformData()
				{ return &mMutexData; }

		protected:
			EAMutexData mMutexData;

		private:
			// Objects of this class are not copyable.
			Mutex(const Mutex&){}
			Mutex& operator=(const Mutex&){ return *this; }
		};



		/// MutexFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class Mutex.
		/// A primary use of this would be to allow the Mutex implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		class EATHREADLIB_API MutexFactory
		{
		public:
			static Mutex*  CreateMutex();                    // Internally implemented as: return new Mutex;
			static void    DestroyMutex(Mutex* pMutex);      // Internally implemented as: delete pMutex;

			static size_t  GetMutexSize();                   // Internally implemented as: return sizeof(Mutex);
			static Mutex*  ConstructMutex(void* pMemory);    // Internally implemented as: return new(pMemory) Mutex;
			static void    DestructMutex(Mutex* pMutex);     // Internally implemented as: pMutex->~Mutex();
		};


	} // namespace Thread

} // namespace EA





namespace EA
{
	namespace Thread
	{
		/// class AutoMutex
		/// An AutoMutex locks the Mutex in its constructor and 
		/// unlocks the Mutex in its destructor (when it goes out of scope).
		class EATHREADLIB_API AutoMutex
		{
		public:
			inline AutoMutex(Mutex& mutex) 
				: mMutex(mutex)
				{ mMutex.Lock(); }

			inline ~AutoMutex()
				{ mMutex.Unlock(); }

		protected:
			Mutex& mMutex;

			// Prevent copying by default, as copying is dangerous.
			AutoMutex(const AutoMutex&);
			const AutoMutex& operator=(const AutoMutex&);
		};

	} // namespace Thread

} // namespace EA



#endif // EATHREAD_EATHREAD_MUTEX_H













