///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EATHREAD_EATHREAD_THREAD_H
#define EATHREAD_EATHREAD_THREAD_H

#include <eathread/eathread.h>
#include <eathread/eathread_semaphore.h>
#include <eathread/eathread_atomic.h>
EA_DISABLE_ALL_VC_WARNINGS()
#include <stddef.h>
#include <stdlib.h>
#include <type_traits>
EA_RESTORE_ALL_VC_WARNINGS()

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



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


/////////////////////////////////////////////////////////////////////////
/// ThreadData
///
/// This is used internally by class Thread.
/// To consider: Move this declaration into a platform-specific 
/// header file.
/////////////////////////////////////////////////////////////////////////

#if !EA_THREADS_AVAILABLE

	struct EAThreadDynamicData
	{
	};

	struct EAThreadData
	{
		EAThreadDynamicData* mpData;
	};

#elif EA_USE_CPP11_CONCURRENCY
	#include <eathread/eathread_mutex.h>
	#include <eathread/eathread_semaphore.h>

	EA_DISABLE_VC_WARNING(4062 4265 4365 4836 4571 4625 4626 4628 4193 4127 4548 4350)
	#if EA_PLATFORM_WINDOWS
		#include <ctxtcall.h> // workaround for compile errors in winrt.  see http://connect.microsoft.com/VisualStudio/feedback/details/730564/ppl-in-winrt-projects-fail-to-compile
	#endif
	#include <future>
	#include <mutex>

	struct EAThreadDynamicData
	{
		typedef void (*ThreadFunc)(EAThreadDynamicData* tdd, void* userFunc, void* userContext, void* userWrapperFunc);
		EAThreadDynamicData(EA::Thread::ThreadUniqueId uniqueThreadId, const char* pThreadName);
		EAThreadDynamicData(void* userFunc, void* userContext, void* userWrapperFunc, ThreadFunc threadFunc);
		~EAThreadDynamicData();

		void AddRef();
		void Release();

		EA::Thread::AtomicInt32 mnRefCount;
		EA::Thread::AtomicInt32 mStatus;
		intptr_t mReturnValue;
		char mName[EATHREAD_NAME_SIZE];
		void* mpStackBase; 
		EA::Thread::ThreadAffinityMask      mnThreadAffinityMask; 
		
		EA::Thread::ThreadUniqueId mUniqueThreadId;
		struct EAThreadComposite
		{
			EAThreadComposite()
			: mReturnPromise()
			, mReturnFuture(mReturnPromise.get_future())
			, mGetStatusFuture(mReturnFuture)
			{
			}

			std::promise<intptr_t> mReturnPromise;
			std::shared_future<intptr_t> mReturnFuture;
			std::shared_future<intptr_t> mGetStatusFuture;
			std::thread mThread;
		} *mpComp;

	private:
		// Disable copy and assignment
		EAThreadDynamicData(const EAThreadDynamicData&);
		EAThreadDynamicData operator=(const EAThreadDynamicData&);
	};

	struct EAThreadData 
	{
		EAThreadDynamicData* mpData;
	};

	EA_RESTORE_VC_WARNING()

// TODO:  collapse the defines.
#elif defined(EA_PLATFORM_SONY)
	#include <eathread/eathread_mutex.h>
	#include <eathread/eathread_semaphore.h>
	#include <kernel.h>
	#include <scebase.h>

	// Internal queue wrapper which is used to allow for a higher resolution sleep than what is provided by Sony's sleep functions
	// as despite the names, sceKernelSleep, sceKernelUSleep and sceKernelNanosleep are all 1 ms resolution whereas this timer is 100 microseconds
	struct EAThreadTimerQueue
	{
		EAThreadTimerQueue()
		{
			int result = sceKernelCreateEqueue(&mTimerEventQueue, "EAThread Timer Queue");
			mbEnabled = result == SCE_OK;

			// A timer queue will fail to be created when there are too many kernel objects open.  It is a valid
			// use-case for the Event Queue to fail being created as the ThreadSleep function implements a fallback.
			//
			// EAT_ASSERT_FORMATTED(mbEnabled, "Failed to initialize the EAThread Timer Queue (0x%x)", result);
		}

		~EAThreadTimerQueue()
		{
			if(mbEnabled)  // only destroy the queue if it was created.
				sceKernelDeleteEqueue(mTimerEventQueue);
				
			mbEnabled = false;
		}

		SceKernelEqueue mTimerEventQueue;
		EA::Thread::AtomicUint32 mCurrentId = 0;
		bool mbEnabled = false;
	};

	struct EAThreadDynamicData
	{
		EAThreadDynamicData();
	   ~EAThreadDynamicData();

		void  AddRef();
		void  Release();

		EA::Thread::ThreadId			mThreadId;
		EA::Thread::SysThreadId			mSysThreadId;
		pid_t							mThreadPid;                     // For Linux this is the thread ID from gettid(). Otherwise it's the getpid() value.
		volatile int					mnStatus;
		intptr_t						mnReturnValue;
		void*							mpStartContext[2];
		void*							mpBeginThreadUserWrapper;       // User-specified BeginThread function wrapper or class wrapper
		void*							mpStackBase; 
		EA::Thread::AtomicInt32			mnRefCount;
		char							mName[EATHREAD_NAME_SIZE];
		int								mStartupProcessor;              // The thread affinity for the thread to set itself to after it starts. We need to do this because we currently have no way to set the affinity of another thread until after it has started.
		EA::Thread::Mutex				mRunMutex;                      // Locked while the thread is running. The reason for this mutex is that it allows timeouts to be specified in the WaitForEnd function.
		EA::Thread::Semaphore			mStartedSemaphore;              // Signaled when the thread starts. This allows us to know in a thread-safe way when the thread has actually started executing.
		EA::Thread::ThreadAffinityMask  mnThreadAffinityMask; 
		EAThreadTimerQueue				mThreadTimerQueue;				// This queue allows for high resolution timer events to be submitted per thread allowing for better sleep resolution than Sony's provided sleep functions
	};


	struct EAThreadData{
		EAThreadDynamicData* mpData;
	};

#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
	#include <pthread.h>
	#include <eathread/eathread_mutex.h>
	#include <eathread/eathread_semaphore.h>

	struct EAThreadDynamicData
	{
		EAThreadDynamicData();
	   ~EAThreadDynamicData();

		void  AddRef();
		void  Release();

		EA::Thread::ThreadId    mThreadId;
		EA::Thread::SysThreadId mSysThreadId;
		pid_t                   mThreadPid;                     // For Linux this is the thread ID from gettid(). Otherwise it's the getpid() value.
		volatile int            mnStatus;
		intptr_t                mnReturnValue;
		void*                   mpStartContext[2];
		void*                   mpBeginThreadUserWrapper;       // User-specified BeginThread function wrapper or class wrapper
		void*                   mpStackBase; 
		EA::Thread::AtomicInt32 mnRefCount;
		char                    mName[EATHREAD_NAME_SIZE];
		int                     mStartupProcessor;              // DEPRECATED:  The thread affinity for the thread to set itself to after it starts. We need to do this because we currently have no way to set the affinity of another thread until after it has started.
		EA::Thread::ThreadAffinityMask      mnThreadAffinityMask; // mStartupProcessor is deprecated in favor of using the the mnThreadAffinityMask and doesn't suffer from the limitations of only specifying the value at thread startup time.
		EA::Thread::Mutex       mRunMutex;                      // Locked while the thread is running. The reason for this mutex is that it allows timeouts to be specified in the WaitForEnd function.
		EA::Thread::Semaphore   mStartedSemaphore;              // Signaled when the thread starts. This allows us to know in a thread-safe way when the thread has actually started executing.
	};


	struct EAThreadData
	{
		EAThreadDynamicData* mpData;
	};

#elif defined(EA_PLATFORM_MICROSOFT) && !EA_POSIX_THREADS_AVAILABLE

	struct EAThreadDynamicData
	{
		EAThreadDynamicData();
	   ~EAThreadDynamicData();
		void    AddRef();
		void    Release();

		EA::Thread::ThreadId                mhThread;
		unsigned int                        mnThreadId;                     // EA::Thread::SysThreadId
		int                                 mnStatus;
		EA::Thread::ThreadAffinityMask      mnThreadAffinityMask;
		intptr_t                            mnReturnValue;
		void*                               mpStartContext[3];
		void*                               mpBeginThreadUserWrapper;     // User-specified BeginThread function wrapper or class wrapper
		void*                               mpStackBase; 
		EA::Thread::AtomicInt32             mnRefCount;
		char                                mName[EATHREAD_NAME_SIZE];
	};


	struct EAThreadData
	{
		EAThreadDynamicData* mpData;
	};

#endif

namespace EA
{
namespace Thread
{

struct EATHREADLIB_API ThreadEnumData
{
	ThreadEnumData();
	~ThreadEnumData();

	EAThreadDynamicData* mpThreadDynamicData;
	void Release();
};

} 
}
/////////////////////////////////////////////////////////////////////////




namespace EA
{
	namespace Thread
	{
		/// FindThreadDynamicData
		/// Utility functionality, not needed for most uses.
		EATHREADLIB_API EAThreadDynamicData* FindThreadDynamicData(ThreadId threadId);
		EATHREADLIB_API EAThreadDynamicData* FindThreadDynamicData(SysThreadId threadId);
		
		/// EnumerateThreads
		/// Enumerates known threads. For some platforms the returned thread list is limited
		/// to the main thread and threads created by EAThread.
		/// Returns the required count to enumerate all threads.
		/// Fills in thread data up till the supplied capacity.
		///
		/// Example usage:
		///     ThreadEnumData enumData[32];
		///     size_t count = EA::Thread::EnumerateThreads(enumData, EAArrayCount(enumData));
		///
		///     for(size_t i = 0; i < count; i++)
		///     {
		///         printf("Thread id: %s\n", EAThreadIdToString(enumData[i].mpThreadDynamicData->mThreadId));
		///         enumData[i].Release();
		///     }
		size_t EATHREADLIB_API EnumerateThreads(ThreadEnumData* pDataArray, size_t dataArrayCapacity);

		/// RunnableFunction
		/// Defines the prototype of a standalone thread function.
		/// The return value is of type intptr_t, which is a standard integral 
		/// data type that is large enough to hold an int or void*.
		typedef intptr_t (*RunnableFunction)(void* pContext);

		/// IRunnable
		/// Defines a class whose Run function executes in a separate thread.
		/// An implementation of this interface can be run using a Thread class instance.
		struct EATHREADLIB_API IRunnable
		{
			 virtual ~IRunnable() { }

			 /// \brief Task run entry point
			 /// The thread terminates when this method returns. 
			 /// The return value is of type intptr_t, which is a standard integral 
			 /// data type that is large enough to hold an int or void*.
			 virtual intptr_t Run(void* pContext = NULL) = 0;
		};

		/// RunnableFunctionUserWrapper
		/// Defines the prototype of a user callback function when thread function is started.
		/// \param pContext: thread start context void* passed in from thread Thread::Begin() 
		/// \param defaultRunnableFunction: default function Thread::Begin() normally would
		///          call, user must call this function with passed in pContext.
		///
		/// Here's an example:
		/// \code
		/// int ThreadFunction(void*)
		/// {
		///      printf("Throw NULL pointer Exception.\n");
		///      char* pTest = NULL;
		///      *pTest = 1;
		///      return 0;
		/// }
		/// 
		/// intptr_t MyThreadBeginWrapper(RunnableFunction defaultRunnableFunction, void* pContext)
		/// {
		///      // Do pre-start thread function stuff
		///      try {
		///            // must call defaultRunnableFunction to execute thread function, if don't then
		///            // thread function will never gets executed.
		///            intptr_t retValue = defaultRunnableFunction(pContext);
		///      }
		///      catch(...) {
		///            printf("Exception detected.\n");
		///      }
		///     
		///      // do post-start thread function stuff
		///      return retValue;
		/// }
		/// \endcode
		///
		/// In your thread begin() function:
		/// \code
		/// ...
		/// threadIds = threads.Begin(ThreadFunction, NULL, NULL, MyThreadBeginWrapper);
		/// ...
		/// \endcode
		typedef intptr_t (*RunnableFunctionUserWrapper)(RunnableFunction defaultRunnableFunction, void* pContext);


		/// RunnableClassUserWrapper
		/// Defines the prototype of a user callback function when thread function is started.
		/// \param pContext: thread start context void* passed in from thread Thread::Begin() 
		/// \param defaultRunnableFunction: default function Thread::Begin() normally would
		///          call, user must call this function with passed in pContext.
		/// 
		/// Here's an example:
		/// \code
		/// class MyThreadClass
		/// {
		///      virtual intptr_t Run(void* pContext = NULL)
		///      {
		///            printf("Throw NULL pointer Exception.\n");
		///            char* pTest = NULL;
		///            *pTest = 1;
		///            return 0;
		///      }
		/// }
		/// 
		/// intptr_t MyThreadBeginWrapper(IRunnable defaultRunnableFunction, void* pContext)
		/// {
		///      // do pre-start thread function stuff
		///
		///      // a good example is try catch block
		///      try
		///      {
		///            // must call defaultRunnableFunction to execute thread function, if don't then
		///            // thread function will never gets executed.
		///            intptr_t retValue = defaultRunnableFunction->Run(pContext);
		///      }
		///      catch(...)
		///      {
		///            printf("Exception detected.\n");
		///      }
		///     
		///      // do post-start thread function stuff
		///      return retValue;
		/// }
		/// \endcode
		///
		/// In your thread begin() function:
		///
		/// \code 
		/// ...
		/// MyThreadClass myThreadClass = new MyThreadClass();
		/// threadIds = threads.Begin(&myThreadClass, NULL, NULL, MyThreadBeginWrapper);
		/// ...
		/// \endcode
		typedef intptr_t (*RunnableClassUserWrapper)(IRunnable* defaultRunnableClass, void* pContext);

		 
		/// ThreadParameters
		/// Used for specifying thread starting parameters. Note that we do not 
		/// include a 'start paused' parameter. The reason for this is that such 
		/// a thing is not portable and other mechanisms can achieve the same 
		/// effect. Thread pause/resume in general is considered bad practice.
		struct EATHREADLIB_API ThreadParameters
		{
			void*       mpStack;                                       /// Pointer to stack memory. This would be the low address of the memory. A NULL value means to create a default stack. Default is NULL. Note that some platforms (such as Windows) don't support a user-supplied stack.
			size_t      mnStackSize;                                   /// Size of the stack memory. Default is variable, depending on the platform.
			int         mnPriority;                                    /// Value in the range of [kThreadPriorityMin, kThreadPriorityMax]. Default is kThreadPriorityDefault.
			int         mnProcessor;                                   /// 0-based index of which processor to run the thread on. A value of -1 means to use default. Default is -1. See SetThreadProcessor for caveats regarding this value.
			const char* mpName;                                        /// A name to give to the thread. Useful for identifying threads in a descriptive way.
			EA::Thread::ThreadAffinityMask mnAffinityMask;             /// A bitmask representing the cores that the thread is allowed to run on.  NOTE:  This affinity mask is only applied when mnProcessor is set to kProcessorAny.
			bool        mbDisablePriorityBoost;                        /// Whether the system should override the default behavior of boosting the thread priority as they come out of a wait state (currently only supported on Windows).

			ThreadParameters();
		};



		/// Thread
		/// 
		/// Note that we do not provide thread suspend and resume functions.
		/// The reason for this is that such things are inherently unsafe as 
		/// you usually cannot know where the thread is executing when the 
		/// suspension occurs. The safe alternative is to use signal or 
		/// semaphore primitives to achieve the same thing in a safe way.
		///
		/// For performance reasons, the thread creation functions of this 
		/// class are themselves not thread-safe. Thus if you want to call
		/// the Begin functions for an instance of this class from multiple
		/// threads, you will need to synchronize access to the begin 
		/// functions yourself.
		class EATHREADLIB_API Thread
		{
		public:
			enum Status
			{
				kStatusNone,    /// The thread has neither started nor ended.
				kStatusRunning, /// The thread has started but not ended.
				kStatusEnded    /// The thread has both started and ended.
			};

			/// Thread
			/// \brief Thread constructor.
			Thread();

			/// Thread
			/// \brief Thread copy constructor.
			Thread(const Thread& t);

			/// Thread
			/// \brief Thread destructor. The destructor does not take any 
			/// action on the thread associated with it. Any threads created
			/// by this class will continue to run and exit normally after 
			/// this destructor has executed.
		   ~Thread();

			/// operator=
			/// \brief Thread assignment operator.
			Thread& operator=(const Thread& t);

			/// \brief Return global RunnableFunctionUserWrapper set by user.
			/// \return function pointer to RunnableFunctionUserWrapper function user
			/// set, if NULL, nothing is set.
			/// \sa RunnableFunctionUserWrapper
			static RunnableFunctionUserWrapper GetGlobalRunnableFunctionUserWrapper();

			/// \brief Set global RunnableFunctionUserWrapper.  This can only be
			/// set once in the application life time.
			/// \param pUserWrapper user specified wrapper function pointer.
			/// \sa RunnableFunctionUserWrapper
			static void SetGlobalRunnableFunctionUserWrapper(RunnableFunctionUserWrapper pUserWrapper);

			/// \brief Return global RunnableClassUserWrapper set by user.
			/// \return function pointer to RunnableClassUserWrapper function user
			/// set, if NULL, nothing is set.
			/// \sa RunnableClassUserWrapper
			static RunnableClassUserWrapper GetGlobalRunnableClassUserWrapper();

			/// \brief Set global RunnableClassUserWrapper.  This can only be
			/// set once in the application life time.
			/// \sa RunnableClassUserWrapper
			static void SetGlobalRunnableClassUserWrapper(RunnableClassUserWrapper pUserWrapper);

			/// Begin
			/// \brief Starts a thread via a RunnableFunction.
			/// Returns the thread id of the newly running thread.
			/// The pContext argument is passed to the RunnableFunction and serves
			/// to allow the caller to pass information to the thread. 
			/// The pThreadParameters argument allows the caller to specify additional
			/// information about how to start the thread. If this parameter is NULL, 
			/// then default settings will be chosen.
			/// The Begin function itself is not thread-safe. While this Thread class
			/// can be used to Begin multiple threads, the Begin function itself cannot
			/// safely be executed by multiple threads at a time. This is by design and
			/// allows for a simpler more efficient library.
			/// User can have their own RunnableFunction wrapper by specifying one in
			/// pUserWrapper.  When pUserWrapper is used, pUserWrapper will get called
			/// first, then pUserWrapper function can do whatever is desired before the
			/// just-created thread's entry point is called.
			/// \sa RunnableFunctionUserWrapper
			ThreadId Begin(RunnableFunction pFunction, void* pContext = NULL, const ThreadParameters* pThreadParameters = NULL, RunnableFunctionUserWrapper pUserWrapper = GetGlobalRunnableFunctionUserWrapper());

			/// Begin
			/// Starts a thread via an object of the IRunnable interface.
			/// Returns the thread id of the newly running thread.
			/// The pContext argument is passed to the RunnableFunction and serves
			/// to allow the caller to pass information to the thread. 
			/// The pThreadParameters argument allows the caller to specify additional
			/// information about how to start the thread. If this parameter is NULL, 
			/// then default settings will be chosen.
			/// The Begin function itself is not thread-safe. While this Thread class
			/// can be used to Begin multiple threads, the Begin function itself cannot
			/// safely be executed by multiple threads at a time. This is by design and
			/// allows for a simpler more efficient library.
			/// User can have their own RunnableClass wrapper by specifying one pUserWrapper.
			/// When pUserWrapper is used, pUserWrapper will get called first, then
			/// pUserWrapper function can do whatever is desired before the just-created
			/// thread's entry point is called.
			/// \sa RunnableClassUserWrapper
			ThreadId Begin(IRunnable* pRunnable, void* pContext = NULL, const ThreadParameters* pThreadParameters = NULL, RunnableClassUserWrapper pUserWrapper = GetGlobalRunnableClassUserWrapper());

			/// WaitForEnd
			/// Waits for the thread associated with an object of this class
			/// to end. Returns one of enum Status to indicate the status upon
			/// return of this call.
			/// This function is similar to the Posix pthread_join function and
			/// the Windows WaitForSingleObject function.
			/// If input pThreadReturnValue is non-NULL, it will be filled in with
			/// the return value of the thread.
			/// This function must be called only by a single thread at a time.
			/// The resulting behaviour is undefined if multiple threads call this function.
			///
			/// Note that the timeout is specified in absolute time and not relative time.
			///
			/// Note also that due to the way thread scheduling works -- particularly in a
			/// time-sliced threading environment -- that the timeout value is a hint and 
			/// the actual amount of time passed before the timeout occurs may be significantly
			/// more or less than the specified timeout time.
			///
			Status WaitForEnd(const ThreadTime& timeoutAbsolute = kTimeoutNone, intptr_t* pThreadReturnValue = NULL);

			/// GetStatus
			/// Returns one of enum GetStatus. Note that in the most general sense
			/// the running status may change if the thread quit right after 
			/// this call was made. But this function is useful if you know that
			/// a function was running and you want to poll for its status while
			/// waiting for it to exit.
			/// If input pThreadReturnValue is non-NULL, it will be filled in with
			/// the return value of the thread if the Status is kStatusEnded.
			/// If the Status is not kStatusEnded, pThreadReturnValue will be ignored.
			Status GetStatus(intptr_t* pThreadReturnValue = NULL) const;

			/// GetId
			/// Gets the Id of the thread associated with an object of this class.
			/// This Id is unique throughout the system. This function returns a 
			/// value that under Posix threads would be synonymous with pthread_t
			/// and under Windows would be synonymous with a thread HANDLE (and not 
			/// a Windows thread id).
			ThreadId GetId() const;

			/// GetPriority
			/// Gets the priority of the thread. Return kThreadPriorityUnknown if 
			/// the thread associated with this class isn't running. If a thread 
			/// wants to get its own priority, it can use this class member or it 
			/// can simply use the global SetThreadPriority function and not need 
			/// an instance of this class. If you want to manipulate the thread 
			/// priority via the native platform interface, you can use GetId to 
			/// get the platform-specific identifier and use that value with native APIs.
			///
			/// This function can return any int except for kThreadPriorityUnknown, as the 
			/// current thread's priority will always be knowable. A return value of kThreadPriorityDefault
			/// means that this thread is of normal (a.k.a. default) priority.
			/// See the documentation for thread priority constants (e.g. kThreadPriorityDefault) 
			/// for more information about thread priority values and behaviour.
			int GetPriority() const;

			/// SetPriority
			/// Sets the priority of the thread. Returns false if the thread associated
			/// with this class isn't running. If a thread wants to set its own priority,
			/// it can use this class member or it can simply use the global SetThreadPriority
			/// function and not need an instance of this class. If you want to manipulate  
			/// the thread priority via the native platform interface, you can use GetId to 
			/// get the platform-specific identifier and use that value with native APIs.
			///
			/// Accepts any integer priority value except kThreadPriorityUnknown.
			/// On some platforms, this function will automatically convert any invalid 
			/// priority for that particular platform to a valid one.  A normal (a.k.a. default) thread 
			/// priority is identified by kThreadPriorityDefault.
			///
			/// You can set the priority of a Thread object only if it has already begun.
			/// You can also set the priority with the Begin function via the ThreadParameters 
			/// argument to Begin. This design is so in order to simply the implementation, 
			/// but being able to set ThreadParameters before Begin is something that can
			/// be considered in the future.
			bool SetPriority(int priority);

			/// SetProcessor
			/// Sets the processor the given thread should run on. Valid values 
			/// are kThreadProcessorDefault, kThreadProcessorAny, or a processor
			/// index in the range of [0, processor count). If the input value
			/// is >= the processor count, it will be reduced to be a modulo of
			/// the processor count. Any other invalid value will cause the processor
			/// to be set to zero.
			/// 
			/// For some platforms you can set the processor of a Thread object only if it 
			/// has already begun.
			///
			/// You can also set the processor with the Begin function via the ThreadParameters 
			/// argument to Begin. This design is so in order to simply the implementation, 
			/// but being able to set ThreadParameters before Begin is something that can
			/// be considered in the future. This is the most reliable way to set the thread
			/// processor, as it works on all platforms. 
			void SetProcessor(int nProcessor);

			/// Wake
			/// Wakes up a sleeping thread if it is sleeping. This necessarily can only
			/// be called from a thread other than the sleeping thread. You must be careful
			/// to not rely on this function as a synchronization primitive. For example,
			/// in the general case you cannot be sure that after calling Wake that the 
			/// thread will be awake, as it is possible that right after you called Wake
			/// the thread immediately went back to sleep before you could do anything.
			/// Nevertheless, this function is useful in waking up a thread from a 
			/// (potentially long) sleep so that it can examine data, lock a synchronization
			/// primitive, or simply exit. 
			///
			/// Note that this class has no member Sleep function. The reason is that a 
			/// thread can only put itself to sleep and cannot put other threads to sleep.
			/// The thread should use the static Sleep function to put itself to sleep.
			void Wake();

			/// GetName
			/// Returns the name of the thread assigned by the SetName function.
			/// If the thread was not named by the SetName function, then the name is empty ("").
			const char* GetName() const;
			
			/// SetName
			/// Sets a descriptive name or the thread. On some platforms this name is passed
			/// on to the debugging tools so they can see this name. The name length, including
			/// a terminating 0 char, is limited to EATHREAD_NAME_SIZE characters. Any characters
			/// beyond that are ignored.
			/// 
			/// You can set the name of a Thread object only if it has already begun.
			/// You can also set the name with the Begin function via the ThreadParameters 
			/// argument to Begin. This design is so in order to simply the implementation, 
			/// but being able to set ThreadParameters before Begin is something that can
			/// be considered in the future.
			///
			/// Some platforms (e.g. Linux) have the restriction this function works propertly only
			/// when called by the same thread that you want to name. Given this situation,
			/// the most portable way to use this SetName function is to either always call
			/// it from the thread to be named or to use the ThreadParameters to give the 
			/// thread a name before it is started and let the started thread name itself.
			void SetName(const char* pName);

			/// SetAffinityMask
			/// Sets an affinity mask for the thread.  On some platforms, this OS feature is
			/// not supported.  In this situation, you are at the mercy of the OS thread scheduler.
			/// 
			/// Example(s):
			/// "00000100" -> thread is pinned to processor 2
			/// "01010100" -> thread is pinned to processor 2, 4, and 6.
			void SetAffinityMask(ThreadAffinityMask mnAffinityMask);

			/// GetAffinityMask
			/// Returns the affinity mask for this specific thread.
			ThreadAffinityMask GetAffinityMask();

			/// SetDefaultProcessor
			/// Sets the default processor to create threads with. To specify the processor
			/// for a running thread, use SetProcessor() or specify the processor in the
			/// thread creation ThreadParameters.  
			/// 
			/// If nProcessor is set to kProcessorAny, EAThread will automatically determine  
			/// which processor to launch threads to.
			///
			/// Please refer to SetProcessor for valid values for the nProcessor argument.
			static void SetDefaultProcessor(int nProcessor) 
			  { sDefaultProcessor = nProcessor; }


			/// GetDefaultProcessor
			/// Gets the default processor to create threads with.
			static int GetDefaultProcessor()
				{ return sDefaultProcessor; }


			/// SetDefaultProcessorMask
			/// Sets which processors created threads should be explicitly run on. 
			/// The default value is 0xffffffffffffffff.
			/// Each bit refers to the associated processor. A mask of 0xffffffffffffffff
			/// means to allow running on any processor, and on desktop platforms such
			/// as Windows it means that the OS decides what processor to use on its own.
			/// Not all platforms support this functionality, even if multiple processors are present.
			static void SetDefaultProcessorMask(uint64_t mask)
				{ sDefaultProcessorMask.SetValue(mask); }


			/// GetDefaultProcessorMask
			/// Returns the mask set by SetDefaultProcessorMask.
			static uint64_t GetDefaultProcessorMask()
				{ return sDefaultProcessorMask.GetValue(); }


			/// GetPlatformData
			/// Returns platform-specific data for this thread for debugging uses or 
			/// other cases whereby special (and non-portable) uses are required.
			/// The value returned is a struct of type EAThreadData.
			void* GetPlatformData()
				{ return &mThreadData; }

		protected:
			static RunnableFunctionUserWrapper sGlobalRunnableFunctionUserWrapper;
			static RunnableClassUserWrapper    sGlobalRunnableClassUserWrapper;
			static EA::Thread::AtomicInt32     sDefaultProcessor;
			static EA::Thread::AtomicUint64    sDefaultProcessorMask;
			EAThreadData                       mThreadData;
		};


		/// ThreadFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class Thread.
		/// A primary use of this would be to allow the Thread implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		class EATHREADLIB_API ThreadFactory
		{
		public:
			static Thread* CreateThread();                  // Internally implemented as: return new Thread;
			static void    DestroyThread(Thread* pThread);  // Internally implemented as: delete pThread;

			static size_t  GetThreadSize();                 // Internally implemented as: return sizeof(Thread);
			static Thread* ConstructThread(void* pMemory);  // Internally implemented as: return new(pMemory) Thread;
			static void    DestructThread(Thread* pThread); // Internally implemented as: pThread->~Thread();
		};


		/// MakeThread
		///
		/// Simplify creating threads with lambdas
		///
		template <typename F>
		auto MakeThread(F&& f, const EA::Thread::ThreadParameters& params = EA::Thread::ThreadParameters())
		{
			typedef std::decay_t<F> decayed_f_t;

			auto get_memory = [] 
			{
				const auto sz = sizeof(decayed_f_t);
				auto* pAllocator = EA::Thread::GetAllocator();

				if(pAllocator)
					return pAllocator->Alloc(sz);
				else
					return malloc(sz);
			};

			auto thread_enty = [](void* pMemory) -> intptr_t
			{
				auto free_memory = [](void* p)
				{
					auto* pAllocator = EA::Thread::GetAllocator();
					if(pAllocator)
						return pAllocator->Free(p);
					else
						return free(p);
				};

				auto* pF = reinterpret_cast<decayed_f_t*>(pMemory);
				(*pF)();
				pF->~decayed_f_t();
				free_memory(pF);
				return 0;
			};

			EA::Thread::Thread thread;
			thread.Begin(thread_enty, new(get_memory()) decayed_f_t(std::forward<F>(f)), &params);  // deleted in the thread entry function
			return thread;
		}

	} // namespace Thread

} // namespace EA


#if defined(EA_DLL) && defined(_MSC_VER)
	// re-enable warning 4251 (it's a level-1 warning and should not be suppressed globally)
	#pragma warning(pop)
#endif


#endif // EATHREAD_EATHREAD_THREAD_H






