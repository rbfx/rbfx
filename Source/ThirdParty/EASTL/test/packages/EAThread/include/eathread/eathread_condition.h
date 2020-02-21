///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements a condition variable in the style of Posix condition variables 
// and Java and C# thread Monitors (Java objects and C# monitors have built-in 
// locks and pthreads condition variables and EAThread::Conditions and Posix
// condition variables do not. A Condition is usually the appropriate thread 
// synchronization mechanism for producer/consumer situations whereby one
// or more threads create data for one or more other threads to work on,
// such as is the case with a message queue.    
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_CONDITION_H
#define EATHREAD_EATHREAD_CONDITION_H


#include <EABase/eabase.h>
#include <eathread/eathread.h>
#include <eathread/eathread_mutex.h>


#if defined(EA_DLL) && defined(_MSC_VER)
	// Suppress warning about class 'EA::Thread::simple_list<T>' needs to have
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
/// EAConditionData
///
/// This is used internally by class Condition.
/// Todo: Consider moving this declaration into a platform-specific 
/// header file.
/// 
#if defined(EA_PLATFORM_SONY)
	// Condition variables are built into Posix/Unix.
	#include <kernel.h>
	#include <eathread/internal/timings.h>

	struct EAConditionData
	{
		ScePthreadCond mCV;
		EAConditionData();
	};

#elif (defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE) && EA_THREADS_AVAILABLE
	// Condition variables are built into Posix/Unix.
	#include <pthread.h>

	struct EAConditionData
	{
		pthread_cond_t mCV;
		EAConditionData();
	};

#else // All other platforms
	#include <eathread/eathread_semaphore.h>
	#include <eathread/eathread_atomic.h>

	struct EATHREADLIB_API EAConditionData
	{
		EA::Thread::AtomicInt32 mnWaitersBlocked;
		int                     mnWaitersToUnblock;
		int                     mnWaitersDone;
		EA::Thread::Semaphore   mSemaphoreBlockQueue;
		EA::Thread::Semaphore   mSemaphoreBlockLock;
		EA::Thread::Mutex       mUnblockLock;

		EAConditionData();

	private:
		// Prevent default generation of these functions by declaring but not defining them.
		EAConditionData(const EAConditionData& rhs);             // copy constructor
		EAConditionData& operator=(const EAConditionData& rhs);  // assignment operator
	};

#endif



namespace EA
{
	namespace Thread
	{
#if defined(EA_PLATFORM_SONY)
		static const int CONDITION_VARIABLE_NAME_LENGTH_MAX = 31;
#else
		static const int CONDITION_VARIABLE_NAME_LENGTH_MAX = 15;
#endif
		/// ConditionParameters
		/// Specifies condition variable settings.
		struct EATHREADLIB_API ConditionParameters
		{
			bool mbIntraProcess;										/// True if the Condition is intra-process, else inter-process.
			char mName[CONDITION_VARIABLE_NAME_LENGTH_MAX + 1];			/// Condition name, applicable only to platforms that recognize named synchronization objects.

			ConditionParameters(bool bIntraProcess = true, const char* pName = NULL);
		};


		/// Condition
		/// Implements a condition variable thread synchronization primitive. A condition variable is usually the 
		/// appropriate thread synchronization mechanism for producer/consumer situations whereby one or more 
		/// threads create data for one or more other threads to work on, such as is the case with a message queue. 
		///
		/// To use a condition variable to wait for resource, you Lock the Mutex for that resource, then (in a loop)
		/// check and Wait on a condition variable that you associate with the mutex. Upon calling Wait, 
		/// the Lock will be released so that other threads can adjust the resource. Upon return from Wait,
		/// the Mutex is re-locked for the caller. To use a Condition to signal a change in something, you simply
		/// call the Signal function. In the case of Signal(false), one blocking waiter will be released,
		/// whereas with Signal(true), all blocking waiters will be released. Upon release of single or multiple
		/// waiting threads, the Lock is contested for by all of them, so in the case or more than one waiter,
		/// only one will immediately come away with ownership of the lock.
		class EATHREADLIB_API Condition
		{
		public:
			enum Result
			{
				kResultOK      =  0,
				kResultError   = -1,
				kResultTimeout = -2
			};

			/// Condition
			/// For immediate default initialization, use no args.
			/// For custom immediate initialization, supply a first argument. 
			/// For deferred initialization, use Condition(NULL, false) then later call Init.
			/// For deferred initialization of an array of objects, create an empty
			/// subclass whose default constructor chains back to Condition(NULL, false).
			Condition(const ConditionParameters* pConditionParameters = NULL, bool bDefaultParameters = true);

			/// ~Condition
			/// Destroys the Condition object. If any threads that are blocking while waiting on 
			/// while the Condition is destroyed, the resulting behaviour is undefined.
			~Condition();

			/// Init
			/// Initializes the Condition.
			bool Init(const ConditionParameters* pConditionParameters);

			/// Wait
			/// Waits for the Condition with timeout. You must have a Mutex 
			/// (that you conceptually associate with the resource) locked before
			/// calling this function or else the resulting behaviour is undefined.
			/// Within a while loop, check the resource state and call Wait if the 
			/// necessary condition is not met.
			///
			/// The call to Wait associates the Condition with your mutex, so it can
			/// then unlock the mutex/resource (allows another thread to fill the resource).
			///
			/// Upon non-error return of Wait, the mutex will be re-locked by the calling 
			/// thread, even if the result is a timeout. Upon returning from wait, before 
			/// doing any processing as a result of a Signal, your loop should always re-check
			/// the resource state. The Posix Wait specification explicitly notes
			/// that uncommon 'spurious wakeups' are possible and so should be tested
			/// for. It impossible to test for a spurious wakeup from within this Wait
			/// function, as this function can't know the resource state that caused the 
			/// Signal to occur.
			///
			/// It should be noted that upon a kResultOK return from Wait, the user should
			/// not assume that what the user was waiting on is still available. The signaling
			/// of a Condition should be considered merely a hint to the waiter that the user
			/// can probably proceed. Also, the user should usually call Wait only if the 
			/// user has nothing to wait for; the user should check for this before calling Wait.
			///
			/// Note that the timeout is specified in absolute time and not relative time.
			///
			/// Note also that due to the way thread scheduling works -- particularly in a
			/// time-sliced threading environment -- that the timeout value is a hint and 
			/// the actual amount of time passed before the timeout occurs may be significantly
			/// more or less than the specified timeout time.
			///
			Result Wait(Mutex* pMutex, const ThreadTime& timeoutAbsolute = kTimeoutNone);

			/// Signal
			/// Releases one or all waiters, depending on the input 'bBroadcast' argument.
			/// The waiters will then contest for the Lock.
			bool Signal(bool bBroadcast = false);

			/// GetPlatformData
			/// Returns the platform-specific data handle for debugging uses or 
			/// other cases whereby special (and non-portable) uses are required.
			void* GetPlatformData()
				{ return &mConditionData; }

		protected:
			EAConditionData mConditionData;

		private:
			// Objects of this class are not copyable.
			Condition(const Condition&){}
			Condition& operator=(const Condition&){ return *this; }
		};


		/// ConditionFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class Condition.
		/// A primary use of this would be to allow the Condition implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		class EATHREADLIB_API ConditionFactory
		{
		public:
			static Condition* CreateCondition();                        // Internally implemented as: return new Condition;
			static void       DestroyCondition(Condition* pCondition);  // Internally implemented as: delete pCondition;

			static size_t     GetConditionSize();                       // Internally implemented as: return sizeof(Condition);
			static Condition* ConstructCondition(void* pMemory);        // Internally implemented as: return new(pMemory) Condition;
			static void       DestructCondition(Condition* pCondition); // Internally implemented as: pCondition->~Condition();
		};


	} // namespace Thread

} // namespace EA



#if defined(EA_DLL) && defined(_MSC_VER)
	// re-enable warning 4251 (it's a level-1 warning and should not be suppressed globally)
	#pragma warning(pop)
#endif



#endif // EATHREAD_EATHREAD_CONDITION_H










