///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>
#include <eathread/eathread_thread.h>
#include <eathread/eathread.h>
#include <eathread/eathread_callstack.h>
#include <eathread/eathread_sync.h>
#include "eathread/internal/eathread_global.h"


#if defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
    #include <new>
    #include <pthread.h>
    #include <time.h>
    #include <stdio.h>
    #include <string.h>
    #include <errno.h>
    #if defined(EA_PLATFORM_WINDOWS)
		EA_DISABLE_ALL_VC_WARNINGS()
        #include <Windows.h> // Presumably we are using pthreads-win32.
		EA_RESTORE_ALL_VC_WARNINGS()
    #elif defined(EA_PLATFORM_LINUX)
        #include <sched.h>
        #include <sys/prctl.h>
        #include <sys/syscall.h>
        #include <unistd.h>
    #elif defined(EA_PLATFORM_APPLE) || defined(__APPLE__)
		#include <unistd.h>
        #include <dlfcn.h>
    #elif defined(EA_PLATFORM_BSD) || defined(EA_PLATFORM_CONSOLE_BSD) || defined(__FreeBSD__)
        #include <pthread_np.h>
    #endif

    #if defined(EA_PLATFORM_LINUX) 
        #define EA_ALLOW_POSIX_THREADS_PRIORITIES 0
    #else
        #define EA_ALLOW_POSIX_THREADS_PRIORITIES 1
    #endif

    #ifdef EA_PLATFORM_ANDROID
        #include <jni.h>
        #include "../android/com_ea_EAThread_EAThread.h"
    #endif

namespace
{
#ifdef EA_PLATFORM_ANDROID
    void SetCurrentThreadNameJava(JNIEnv* env, const char* name);
#endif

    // We convert a an EAThread priority (higher value implies higher priority) to a native priority 
    // value, as some implementations of pthreads use lower values to indicate higher priority.
    void ConvertToNativePriority(int eathreadPriority, sched_param& param, int& policy)
    {
        using namespace EA::Thread;

        #if defined(EA_PLATFORM_WINDOWS)
            param.sched_priority = THREAD_PRIORITY_NORMAL + (nPriority - kThreadPriorityDefault);

        #elif defined(EA_PLATFORM_LINUX) && !defined(__CYGWIN__)
            // We are assuming Kernel 2.6 and later behaviour, but perhaps we should dynamically detect.
            // Linux supports three scheduling policies SCHED_OTHER, SCHED_RR, and SCHED_FIFO. 
            // The process needs to be run with superuser privileges to use SCHED_RR or SCHED_FIFO.
            // Thread priorities for SCHED_OTHER do not exist; there is only one allowed thread priority: 0.
            // Thread priorities for SCHED_RR and SCHED_FIFO are limited to the range of [1, 99] (verified with Linux 2.6.17), 
            // despite documentation on the Internet that refers to ranges of 0-99, 1-100, 1-140, etc.
            // Higher values in this range mean higher priority.
            // All of the SCHED_RR and SCHED_FIFO privileges are higher than anything running at SCHED_OTHER,
            // as they are considered to be real-time scheduling. A result of this is that there is no
            // such thing as having a thread of lower priority than normal; there are only higher real-time priorities.
            policy = 0;

            #if EA_ALLOW_POSIX_THREADS_PRIORITIES
                if(eathreadPriority <= kThreadPriorityDefault)
            #endif
                {
                    #if defined(SCHED_OTHER)
                        policy = SCHED_OTHER;
                    #endif
                    param.sched_priority = 0;
                }
            #if EA_ALLOW_POSIX_THREADS_PRIORITIES
                else
                {
                    #if defined(SCHED_RR)
                        policy = SCHED_RR;
                    #endif
                    param.sched_priority = (eathreadPriority - kThreadPriorityDefault);
                }
            #else
                EA_UNUSED(eathreadPriority);
            #endif

        #else
            #if defined(__CYGWIN__)
                policy = SCHED_OTHER;
            #else
                policy = SCHED_FIFO;
            #endif

            int nMin      = sched_get_priority_min(policy);
            int nMax      = sched_get_priority_max(policy);
            int adjustDir = 1;

            // Some implementations of Pthreads associate higher priorities with smaller
            // integer values. We hide this. To the user, a higher value must always
            // indicate higher priority.
            if(nMin > nMax)
            {
                adjustDir = nMax;
                nMax      = nMin;
                nMin      = adjustDir;
                adjustDir = -1; // Translate user's desire for higher priority to lower value
            }

            // native_priority = EAThread_native_priority_default +/- EAThread_user_priority
            // This calculation sets the default to be in the middle of low and high, which might not be so for all platforms in practice.
            param.sched_priority = ((nMin + nMax) / 2) + (adjustDir * eathreadPriority);

            if(param.sched_priority < nMin)
                param.sched_priority = nMin;
            else if(param.sched_priority > nMax)
                param.sched_priority = nMax;
        #endif
    }


    // We convert a native priority value to an EAThread priority (higher value implies higher 
    // priority), as some implementations of pthreads use lower values to indicate higher priority.
    int ConvertFromNativePriority(const sched_param& param, int policy)
    {
        using namespace EA::Thread;

        #if defined(EA_PLATFORM_LINUX) && !defined(__CYGWIN__)
            EA_UNUSED(policy);
            return kThreadPriorityDefault + param.sched_priority; // This works for both SCHED_OTHER, SCHED_RR, and SCHED_FIFO.
        #else
            #if defined(EA_PLATFORM_WINDOWS) // In the case of windows, we know for sure that normal priority is defined by 'THREAD_PRIORITY_NORMAL'.
                if(param.sched_priority == THREAD_PRIORITY_NORMAL)
                    return kThreadPriorityDefault;
            #elif !(defined(__CYGWIN__) || defined(CS_UNDEFINED_STRING))
                if(policy == SCHED_OTHER)
                    return 0;   // 0 is the only priority permitted with the SCHED_OTHER scheduling scheme.
            #endif

            // Some implementations of Pthreads associate higher priorities with smaller
            // integer values. We hide this. To the user, a higher value must always
            // indicate higher priority.

            // EAThread_user_priority = +/-(native_priority - EAThread_native_priority_default)
            const int nMin               = sched_get_priority_min(policy);
            const int nMax               = sched_get_priority_max(policy);
            const int nativeBasePriority = (nMin + nMax) / 2;
            const int adjustDir          = (nMin < nMax) ? 1 : -1;

            return adjustDir * (param.sched_priority - nativeBasePriority);
        #endif
    }


    // Setup stack and/or priority of a new thread
    void SetupThreadAttributes(pthread_attr_t& creationAttribs, const EA::Thread::ThreadParameters* pTP)
    {
        int result = 0;
        EA_UNUSED( result ); //only used for assertions

        // We create the thread as attached, and we'll call either pthread_join or pthread_detach, 
        // depending on whether WaitForEnd (pthread_join) is called or not (pthread_detach).

        if(pTP)
        {
            // Set thread stack address and/or size
            if(pTP->mpStack)
            {
                EAT_ASSERT(pTP->mnStackSize != 0);

                #if !defined(__CYGWIN__)  // Some implementations of pthreads does not support pthread_attr_setstack.
					#if defined(PTHREAD_STACK_MIN)
						EAT_ASSERT((pTP->mnStackSize >= PTHREAD_STACK_MIN));
					#endif
                    result = pthread_attr_setstack(&creationAttribs, (void*)pTP->mpStack, pTP->mnStackSize);
                    EAT_ASSERT(result == 0);
                #endif
            }
            else if(pTP->mnStackSize)
            {
			#if defined(PTHREAD_STACK_MIN)
				EAT_ASSERT((pTP->mnStackSize >= PTHREAD_STACK_MIN));
			#endif
                result = pthread_attr_setstacksize(&creationAttribs, pTP->mnStackSize);
                EAT_ASSERT(result == 0);
            }

            // Set initial non-zero priority
            // Even if pTP->mnPriority == kThreadPriorityDefault, we need to run this on some platforms, as the thread priority for new threads on them isn't the same as the thread priority for the main thread.
            int         policy = SCHED_OTHER;
            sched_param param;

			// initialize all fields of param before we start tinkering with them
			result = pthread_attr_getschedparam(&creationAttribs, &param);
			EAT_ASSERT(result == 0);
            ConvertToNativePriority(pTP->mnPriority, param, policy);
            result = pthread_attr_setschedpolicy(&creationAttribs, policy);
            EAT_ASSERT(result == 0);
            result = pthread_attr_setschedparam(&creationAttribs, &param);
            EAT_ASSERT(result == 0);

            // Unix doesn't let you specify thread CPU affinity via pthread attributes.
            // Instead you need to call sched_setaffinity or pthread_setaffinity_np.
        }
    }

    static void SetPlatformThreadAffinity(EAThreadDynamicData* pTDD)
    {
        if(pTDD->mThreadId != EA::Thread::kThreadIdInvalid) // If the thread has been created...
        {
            #if defined(EA_PLATFORM_LINUX) || defined(CS_UNDEFINED_STRING)
                EAT_ASSERT(pTDD && (pTDD->mThreadId != EA::Thread::kThreadIdInvalid));

                #if defined(EA_PLATFORM_ANDROID) // Android doesn't provide pthread_setaffinity_np.
                    if(pTDD->mThreadPid != 0) // If the running thread has assigned its pid yet (it does so right after starting)...
                    {
                        int processor = (1 << pTDD->mStartupProcessor);
                        syscall(__NR_sched_setaffinity, pTDD->mThreadPid, sizeof(processor), &processor);
                    }
                    // Else wait till the thread has started and let it set its own affinity.
                #else
                    cpu_set_t cpus;
                    CPU_ZERO(&cpus);
                    CPU_SET(pTDD->mStartupProcessor, &cpus);

                    pthread_setaffinity_np(pTDD->mThreadId, sizeof(cpus), &cpus);
                    // We don't assert on the pthread_setaffinity_np return value, as that could be very noisy for some users.   
                #endif
            #endif
        }
        // Else the thread hasn't started yet, or has already exited. Let the thread set its own 
        // affinity when it starts.
    }

#ifdef EA_PLATFORM_ANDROID
    static JavaVM* gJavaVM = NULL;
    static jclass  gEAThreadClass = NULL;
    static jmethodID gSetNameMethodId = NULL;

    // This function needs to be called by Java code on app startup, before 
    // the C main entrypoint is called, or very shortly thereafter. It's called
    // via Java with EAThread.Init(); If your Java code doesn't call EAThread.Init
    // then any C++ calls to Java code will fail (though those C++ calls could 
    // manually set the JNIEnv per call).
    extern "C" __attribute__ ((visibility("default"))) JNIEXPORT void JNICALL Java_com_ea_EAThread_EAThread_Init(JNIEnv* env, jclass eaThreadClass)
    {
        gEAThreadClass = (jclass)env->NewGlobalRef(eaThreadClass);
        int getJavaVMResult = env->GetJavaVM(&gJavaVM);
        EA_UNUSED(getJavaVMResult);
        EAT_ASSERT_MSG(getJavaVMResult == 0, "Unable to get the Java VM from the JNI environment.");
        gSetNameMethodId = env->GetStaticMethodID(gEAThreadClass, "setCurrentThreadName", "(Ljava/lang/String;)V");
        EAT_ASSERT(gSetNameMethodId);
    }

    // This is called just after creation of a C/C++ thread. 
    // The Java JNI requires you to do this, else bad things will happen.
    static JNIEnv* AttachJavaThread()
    {
        if(gJavaVM)
        {
            JNIEnv* env;
            jint resultCode = gJavaVM->AttachCurrentThread(&env, NULL);
            (void)resultCode;
            EAT_ASSERT_MSG(resultCode == 0, "Unable to attach thread");

            return env;
        }

        return NULL;
    }

    // This is called just before destruction of a C/C++ thread.
    // It is the complement of AttachJavaThread.
    static void DetachJavaThread()
    {
        if(gJavaVM)
            gJavaVM->DetachCurrentThread();
    }

    void SetCurrentThreadNameJava(JNIEnv* env, const char* name)
    {
		if(gJavaVM)
        {
	        jstring threadName = env->NewStringUTF(name);
	        env->CallStaticVoidMethod(gEAThreadClass, gSetNameMethodId, threadName);
	        env->DeleteLocalRef(threadName);

	        EAT_ASSERT(env->ExceptionOccurred() == NULL);
	        if (env->ExceptionOccurred() != NULL)
	        {
	            env->ExceptionDescribe();
	            env->ExceptionClear();
	        }
		}
    }
#endif

} // namespace

namespace EA
{ 
    namespace Thread
    {
        extern Allocator* gpAllocator;

        const size_t kMaxThreadDynamicDataCount = 128;

        struct EAThreadGlobalVars
        {
            EA_PREFIX_ALIGN(8)
            char        gThreadDynamicData[kMaxThreadDynamicDataCount][sizeof(EAThreadDynamicData)] EA_POSTFIX_ALIGN(8);
            AtomicInt32 gThreadDynamicDataAllocated[kMaxThreadDynamicDataCount];
            Mutex gThreadDynamicMutex;
        };
        EATHREAD_GLOBALVARS_CREATE_INSTANCE;

        EAThreadDynamicData* AllocateThreadDynamicData()
        {
            for(size_t i(0); i < kMaxThreadDynamicDataCount; i++)
            {
                if(EATHREAD_GLOBALVARS.gThreadDynamicDataAllocated[i].SetValueConditional(1, 0))
                    return (EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];
            }

            // This is a safety fallback mechanism. In practice it won't be used in almost all situations.
            if(gpAllocator)
                return (EAThreadDynamicData*)gpAllocator->Alloc(sizeof(EAThreadDynamicData));
            else
                return (EAThreadDynamicData*)new char[sizeof(EAThreadDynamicData)]; // We assume the returned alignment is sufficient.
        }

        void FreeThreadDynamicData(EAThreadDynamicData* pEAThreadDynamicData)
        {
            if((pEAThreadDynamicData >= (EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData) && (pEAThreadDynamicData < ((EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData + kMaxThreadDynamicDataCount)))
            {
                pEAThreadDynamicData->~EAThreadDynamicData();
                EATHREAD_GLOBALVARS.gThreadDynamicDataAllocated[pEAThreadDynamicData - (EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData].SetValue(0);
            }
            else
            {
                // Assume the data was allocated via the fallback mechanism.
                pEAThreadDynamicData->~EAThreadDynamicData();
                if(gpAllocator)
                    gpAllocator->Free(pEAThreadDynamicData);
                else
                    delete[] (char*)pEAThreadDynamicData;
            }
        }

        // This is a public function.
        EAThreadDynamicData* FindThreadDynamicData(ThreadId threadId)
        {
            for(size_t i(0); i < kMaxThreadDynamicDataCount; i++)
            {
                EAThreadDynamicData* const pTDD = (EAThreadDynamicData*)(void*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];

                if(pTDD->mThreadId == threadId)
                    return pTDD;
            }

            return NULL; // This is no practical way we can find the data unless thread-specific storage was involved.
        }
        
        #if defined(EA_PLATFORM_APPLE) 
        EAThreadDynamicData* FindThreadDynamicData(EA::Thread::SysThreadId sysThreadId)
        {
            for (size_t i(0); i < kMaxThreadDynamicDataCount; ++i)
            {
                EAThreadDynamicData* const pTDD = (EAThreadDynamicData*)EATHREAD_GLOBALVARS.gThreadDynamicData[i];
                if (pTDD->mSysThreadId == sysThreadId)
                return pTDD;
            }
            return NULL; // This is no practical way we can find the data unless thread-specific storage was involved.
        }
        #endif

    }
}


EAThreadDynamicData::EAThreadDynamicData()
  : mThreadId(EA::Thread::kThreadIdInvalid),
	mSysThreadId(0),
    mThreadPid(0),
    mnStatus(EA::Thread::Thread::kStatusNone),
    mnReturnValue(0),
  //mpStartContext[],
    mpBeginThreadUserWrapper(NULL),
    mnRefCount(0),
  //mName[],
    mStartupProcessor(EA::Thread::kProcessorDefault),
    mnThreadAffinityMask(EA::Thread::kThreadAffinityMaskAny),
    mRunMutex(),
    mStartedSemaphore()
{
    memset(mpStartContext, 0, sizeof(mpStartContext));
    memset(mName, 0, sizeof(mName));
}


EAThreadDynamicData::~EAThreadDynamicData()
{
	if (mThreadId != EA::Thread::kThreadIdInvalid)
	{
		pthread_detach(mThreadId);
	}

	mThreadId = EA::Thread::kThreadIdInvalid;
	mThreadPid = 0;
	mSysThreadId = 0;
}


void EAThreadDynamicData::AddRef()
{
    mnRefCount.Increment();    // Note that mnRefCount is an AtomicInt32.
}


void EAThreadDynamicData::Release()
{
    if(mnRefCount.Decrement() == 0)   // Note that mnRefCount is an AtomicInt32.
        EA::Thread::FreeThreadDynamicData(this);
}


EA::Thread::ThreadParameters::ThreadParameters()
  : mpStack(NULL),
    mnStackSize(0),
		mnPriority(kThreadPriorityDefault),  
    mnProcessor(kProcessorDefault),
    mpName(""),
    mnAffinityMask(kThreadAffinityMaskAny),
	mbDisablePriorityBoost(false)
{
    // Empty
}


EA::Thread::RunnableFunctionUserWrapper  EA::Thread::Thread::sGlobalRunnableFunctionUserWrapper = NULL;
EA::Thread::RunnableClassUserWrapper     EA::Thread::Thread::sGlobalRunnableClassUserWrapper    = NULL;
EA::Thread::AtomicInt32                  EA::Thread::Thread::sDefaultProcessor                  = kProcessorAny;
EA::Thread::AtomicUint64                 EA::Thread::Thread::sDefaultProcessorMask              = UINT64_C(0xffffffffffffffff);


EA::Thread::RunnableFunctionUserWrapper EA::Thread::Thread::GetGlobalRunnableFunctionUserWrapper()
{
    return sGlobalRunnableFunctionUserWrapper;
}


void EA::Thread::Thread::SetGlobalRunnableFunctionUserWrapper(EA::Thread::RunnableFunctionUserWrapper pUserWrapper)
{
    if(sGlobalRunnableFunctionUserWrapper)
        EAT_FAIL_MSG("Thread::SetGlobalRunnableFunctionUserWrapper already set."); // Can only be set once for the application. 
    else
        sGlobalRunnableFunctionUserWrapper = pUserWrapper;
}


EA::Thread::RunnableClassUserWrapper EA::Thread::Thread::GetGlobalRunnableClassUserWrapper()
{
    return sGlobalRunnableClassUserWrapper;
}


void EA::Thread::Thread::SetGlobalRunnableClassUserWrapper(EA::Thread::RunnableClassUserWrapper pUserWrapper)
{
    if(sGlobalRunnableClassUserWrapper)
        EAT_FAIL_MSG("EAThread::SetGlobalRunnableClassUserWrapper already set."); // Can only be set once for the application. 
    else
        sGlobalRunnableClassUserWrapper = pUserWrapper;
}


EA::Thread::Thread::Thread()
{
    mThreadData.mpData = NULL;
}


EA::Thread::Thread::Thread(const Thread& t)
  : mThreadData(t.mThreadData)
{
    if(mThreadData.mpData)
        mThreadData.mpData->AddRef();
}


EA::Thread::Thread& EA::Thread::Thread::operator=(const Thread& t)
{
    // We don't synchronize access to mpData; we assume that the user 
    // synchronizes it or this Thread instances is used from a single thread.
    if(t.mThreadData.mpData)
        t.mThreadData.mpData->AddRef();

    if(mThreadData.mpData)
        mThreadData.mpData->Release();

    mThreadData = t.mThreadData;

    return *this;
}


EA::Thread::Thread::~Thread()
{
    // We don't synchronize access to mpData; we assume that the user 
    // synchronizes it or this Thread instances is used from a single thread.
    if(mThreadData.mpData)
        mThreadData.mpData->Release();
}


static void* RunnableFunctionInternal(void* pContext)
{
    // The parent thread is sharing memory with us and we need to 
    // make sure our view of it is synchronized with the parent.
    EAReadWriteBarrier();

    EAThreadDynamicData* const pTDD        = (EAThreadDynamicData*)pContext; 
    EA::Thread::RunnableFunction pFunction = (EA::Thread::RunnableFunction)pTDD->mpStartContext[0];
    void* pCallContext                     = pTDD->mpStartContext[1];

    #if defined(EA_PLATFORM_LINUX) && defined(__NR_gettid)
        // Unfortunately there's no reliable way to translate a pthread_t to a 
        // thread pid value. Thus we can know the thread's pid only via the 
        // thread itself calling gettid().

      //pTDD->mThreadPid = gettid();                    // Some Linux compiler distributions declare gettid(), some don't. 
        pTDD->mThreadPid = (pid_t)syscall(__NR_gettid); // It's safest to just make the syscall directly.

        if(pTDD->mStartupProcessor != EA::Thread::kProcessorDefault && pTDD->mStartupProcessor != EA::Thread::kProcessorAny)
            SetPlatformThreadAffinity(pTDD);
        else if(pTDD->mStartupProcessor == EA::Thread::kProcessorAny)
            EA::Thread::SetThreadAffinityMask(pTDD->mnThreadAffinityMask);
    #elif !defined(EA_PLATFORM_CONSOLE) && !defined(EA_PLATFORM_MOBILE)
        pTDD->mThreadPid = getpid(); // We can't set a thread affinity with a process id. 
    #else
        pTDD->mThreadPid = 0;
    #endif

    // Lock the runtime mutex which is used to allow other threads to wait on this thread with a timeout.
    pTDD->mRunMutex.Lock();         // Important that this be before the semaphore post.
    pTDD->mStartedSemaphore.Post(); // Announce that the thread has started.
    pTDD->mnStatus = EA::Thread::Thread::kStatusRunning;
    pTDD->mpStackBase = EA::Thread::GetStackBase();

#ifdef EA_PLATFORM_ANDROID

    JNIEnv* jni = AttachJavaThread();
    if(pTDD->mName[0])
        SetCurrentThreadNameJava(jni, pTDD->mName);
#elif !EATHREAD_OTHER_THREAD_NAMING_SUPPORTED
    // Under Unix we need to set the thread name from the thread that is being named and not from an outside thread.
    if(pTDD->mName[0])
		EA::Thread::SetThreadName(pTDD->mThreadId, pTDD->mName);
#endif

    if(pTDD->mpBeginThreadUserWrapper)
    {
        // If user wrapper is specified, call user wrapper and pass the pFunction and pContext.
        EA::Thread::RunnableFunctionUserWrapper pWrapperFunction = (EA::Thread::RunnableFunctionUserWrapper)pTDD->mpBeginThreadUserWrapper;
        pTDD->mnReturnValue = pWrapperFunction(pFunction, pCallContext);
    }
    else
        pTDD->mnReturnValue = pFunction(pCallContext);

    #ifdef EA_PLATFORM_ANDROID
        DetachJavaThread();
    #endif

    void* pReturnValue = (void*)pTDD->mnReturnValue;
    pTDD->mnStatus = EA::Thread::Thread::kStatusEnded;
    pTDD->mRunMutex.Unlock();
    pTDD->Release();

    return pReturnValue;
}


static void* RunnableObjectInternal(void* pContext)
{
    EAThreadDynamicData* const pTDD  = (EAThreadDynamicData*)pContext; 
    EA::Thread::IRunnable* pRunnable = (EA::Thread::IRunnable*)pTDD->mpStartContext[0];
    void* pCallContext               = pTDD->mpStartContext[1];

    #if defined(EA_PLATFORM_LINUX) && defined(__NR_gettid)
        // Unfortunately there's no reliable way to translate a pthread_t to a 
        // thread pid value. Thus we can know the thread's pid only via the 
        // thread itself calling gettid().

      //pTDD->mThreadPid = gettid();                    // Some Linux compiler distributions declare gettid(), some don't. 
        pTDD->mThreadPid = (pid_t)syscall(__NR_gettid); // It's safest to just make the syscall directly.

        if(pTDD->mStartupProcessor != EA::Thread::kProcessorDefault && pTDD->mStartupProcessor != EA::Thread::kProcessorAny)
            SetPlatformThreadAffinity(pTDD);
        else if(pTDD->mStartupProcessor == EA::Thread::kProcessorAny)
            EA::Thread::SetThreadAffinityMask(pTDD->mnThreadAffinityMask);

    #elif !defined(EA_PLATFORM_CONSOLE) && !defined(EA_PLATFORM_MOBILE)
        pTDD->mThreadPid = getpid(); // We can't set a thread affinity with a process id. 
    #else
        pTDD->mThreadPid = 0;
    #endif

    pTDD->mRunMutex.Lock();         // Important that this be before the semaphore post.
    pTDD->mStartedSemaphore.Post();

    pTDD->mnStatus = EA::Thread::Thread::kStatusRunning;

#ifdef EA_PLATFORM_ANDROID
    JNIEnv* jni = AttachJavaThread();
    if(pTDD->mName[0])
        SetCurrentThreadNameJava(jni, pTDD->mName);
#elif !EATHREAD_OTHER_THREAD_NAMING_SUPPORTED
    // Under Unix we need to set the thread name from the thread that is being named and not from an outside thread.
    if(pTDD->mName[0])
		EA::Thread::SetThreadName(pTDD->mThreadId, pTDD->mName);
#endif

    if(pTDD->mpBeginThreadUserWrapper)
    {
        // If user wrapper is specified, call user wrapper and pass the pFunction and pContext.
        EA::Thread::RunnableClassUserWrapper pWrapperClass = (EA::Thread::RunnableClassUserWrapper)pTDD->mpBeginThreadUserWrapper;
        pTDD->mnReturnValue = pWrapperClass(pRunnable, pCallContext);
    }
    else
        pTDD->mnReturnValue = pRunnable->Run(pCallContext);

    #ifdef EA_PLATFORM_ANDROID
        DetachJavaThread();
    #endif

    void* const pReturnValue = (void*)pTDD->mnReturnValue;
    pTDD->mnStatus = EA::Thread::Thread::kStatusEnded;
    pTDD->mRunMutex.Unlock();
    pTDD->Release();

    return pReturnValue;
}


/// BeginThreadInternal
/// Extraction of both RunnableFunction and RunnableObject EA::Thread::Begin in order to have thread initialization
/// in one place
static EA::Thread::ThreadId BeginThreadInternal(EAThreadData& mThreadData, void* pRunnableOrFunction, void* pContext, const EA::Thread::ThreadParameters* pTP,
                                                void* pUserWrapper, void* (*InternalThreadFunction)(void*))
{
    using namespace EA::Thread;

    // The parent thread is sharing memory with us and we need to
    // make sure our view of it is synchronized with the parent.
    EAReadWriteBarrier();

    // Check there is an entry for the current thread context in our ThreadDynamicData array.
    EA::Thread::ThreadId thisThreadId = EA::Thread::GetThreadId();
    if(!FindThreadDynamicData(thisThreadId))
    {
        EAThreadDynamicData* pData = new(AllocateThreadDynamicData()) EAThreadDynamicData;
        if(pData)
        {
            pData->AddRef(); // AddRef for ourselves, to be released upon this Thread class being deleted or upon Begin being called again for a new thread.
                             // Do no AddRef for thread execution because this is not an EAThread managed thread.
            pData->AddRef(); // AddRef for this function, to be released upon this function's exit.                
            pData->mThreadId = thisThreadId;
            pData->mSysThreadId = GetSysThreadId();
			pData->mThreadPid = 0;
            strncpy(pData->mName, "external", EATHREAD_NAME_SIZE);
            pData->mName[EATHREAD_NAME_SIZE - 1] = 0;
            pData->mpStackBase = EA::Thread::GetStackBase();
        }
    }
    
    if(mThreadData.mpData)
        mThreadData.mpData->Release(); // Matches the "AddRef for ourselves" below.

    // We use the pData temporary throughout this function because it's possible that mThreadData.mpData could be 
    // modified as we are executing, in particular in the case that mThreadData.mpData is destroyed and changed 
    // during execution.
    EAThreadDynamicData* pData = new(AllocateThreadDynamicData()) EAThreadDynamicData; // Note that we use a special new here which doesn't use the heap.
    EAT_ASSERT(pData);

    if(pData)
    {
        mThreadData.mpData = pData;

        pData->AddRef(); // AddRef for ourselves, to be released upon this Thread class being deleted or upon Begin being called again for a new thread.
        pData->AddRef(); // AddRef for the thread, to be released upon the thread exiting.
        pData->AddRef(); // AddRef for this function, to be released upon this function's exit.
        pData->mThreadId = kThreadIdInvalid;
        pData->mSysThreadId = 0;
        pData->mThreadPid = 0;
        pData->mnStatus = Thread::kStatusNone;
        pData->mpStartContext[0] = pRunnableOrFunction;
        pData->mpStartContext[1] = pContext;
        pData->mpBeginThreadUserWrapper = pUserWrapper;
        pData->mStartupProcessor = pTP ? pTP->mnProcessor % EA::Thread::GetProcessorCount() : kProcessorDefault;
        pData->mnThreadAffinityMask = pTP ? pTP->mnAffinityMask : kThreadAffinityMaskAny;
		if(pTP && pTP->mpName)
			strncpy(pData->mName, pTP->mpName, EATHREAD_NAME_SIZE);
		pData->mName[EATHREAD_NAME_SIZE - 1] = 0;
        
        // Pass NULL attribute pointer if there are no special setup steps
        pthread_attr_t* pCreationAttribs = NULL;
        int result(0);

        pthread_attr_t creationAttribs;

        // SetupThreadAttributes is always called in order to create threads as detached
        pthread_attr_init(&creationAttribs);

		#ifndef EA_PLATFORM_ANDROID
			// Posix has stated that we should call pthread_attr_setinheritsched, otherwise the
			// thread priority set up in pthread_attr_t gets ignored by the newly created thread.
			pthread_attr_setinheritsched(&creationAttribs, PTHREAD_EXPLICIT_SCHED);
		#endif

		SetupThreadAttributes(creationAttribs, pTP);
        pCreationAttribs = &creationAttribs;

        result = pthread_create(&pData->mThreadId, pCreationAttribs, InternalThreadFunction, pData);
        
        if(result == 0) // If success...
        {
            ThreadId threadIdTemp = pData->mThreadId; // Temp value because Release below might delete pData.

            // If additional attributes were used, free initialization data.
            if(pCreationAttribs)
            {
                result = pthread_attr_destroy(pCreationAttribs);
                EAT_ASSERT(result == 0);
            }

			if(pData->mStartupProcessor != kProcessorDefault && pData->mStartupProcessor != EA::Thread::kProcessorAny) 
				SetPlatformThreadAffinity(pData);
			else if(pData->mStartupProcessor == EA::Thread::kProcessorAny)
				EA::Thread::SetThreadAffinityMask(pData->mThreadId, pData->mnThreadAffinityMask);


            pData->Release(); // Matches AddRef for this function.
            return threadIdTemp;
        }

        // If additional attributes were used, free initialization data
        if(pCreationAttribs)
        {
            result = pthread_attr_destroy(pCreationAttribs);
            EAT_ASSERT(result == 0);
        }

        pData->Release(); // Matches AddRef for "cleanup" above.
        pData->Release(); // Matches AddRef for this Thread class above.
        pData->Release(); // Matches AddRef for thread above.
        mThreadData.mpData = NULL; // mThreadData.mpData == pData
    }

    return (ThreadId)kThreadIdInvalid;
}


EA::Thread::ThreadId EA::Thread::Thread::Begin(RunnableFunction pFunction, void* pContext, const ThreadParameters* pTP, RunnableFunctionUserWrapper pUserWrapper)
{
    ThreadId threadId = BeginThreadInternal(mThreadData, reinterpret_cast<void*>((uintptr_t)pFunction), pContext, pTP, reinterpret_cast<void*>((uintptr_t)pUserWrapper), RunnableFunctionInternal);

    if(pTP && pTP->mnProcessor == EA::Thread::kProcessorAny)
        EA::Thread::Thread::SetAffinityMask(pTP->mnAffinityMask);  

    if(pTP && pTP->mpName)
        SetName(pTP->mpName);

    return threadId;
}


EA::Thread::ThreadId EA::Thread::Thread::Begin(IRunnable* pRunnable, void* pContext, const ThreadParameters* pTP, RunnableClassUserWrapper pUserWrapper)
{
    ThreadId threadId = BeginThreadInternal(mThreadData, reinterpret_cast<void*>((uintptr_t)pRunnable), pContext, pTP, reinterpret_cast<void*>((uintptr_t)pUserWrapper), RunnableObjectInternal);

    if(pTP && pTP->mnProcessor == EA::Thread::kProcessorAny)
        EA::Thread::Thread::SetAffinityMask(pTP->mnAffinityMask);  

    if(pTP && pTP->mpName)
        SetName(pTP->mpName);

    return threadId;
}


EA::Thread::Thread::Status EA::Thread::Thread::WaitForEnd(const ThreadTime& timeoutAbsolute, intptr_t* pThreadReturnValue)
{
    // In order to support timeoutAbsolute, we don't just call pthread_join, as that's an infinitely blocking call.
    // Instead we wait on a Mutex (with a timeout) which the running thread locked, and will unlock as it is exiting.
    // Only after the successful Mutex lock do we call pthread_join, as we know that it won't block for an indeterminate
    // amount of time (barring a thread priority inversion problem). If the user never calls WaitForEnd, then we 
    // will eventually call pthread_detach in the EAThreadDynamicData destructor.

    // The mThreadData memory is shared between threads and when 
    // reading it we must be synchronized.
    EAReadWriteBarrier();

    // A mutex lock around mpData is not needed below because mpData is never allowed to go from non-NULL to NULL. 
    // However, there is an argument that can be made for placing a memory read barrier before reading it.

    if(mThreadData.mpData) // If this is non-zero then we must have created the thread.
    {
        EAT_ASSERT(mThreadData.mpData->mThreadId != kThreadIdInvalid); // WaitForEnd can't be called on a thread that hasn't been started

        // We must not call WaitForEnd from the thread we are waiting to end. 
        // That would result in a deadlock, at least if the timeout was infinite.
        EAT_ASSERT(mThreadData.mpData->mThreadId != EA::Thread::GetThreadId());

        Status currentStatus = GetStatus();

        if(currentStatus == kStatusNone) // If the thread hasn't started yet...
        {
            // The thread has not been started yet. Wait on the semaphore (which is posted when the thread actually starts executing).
            Semaphore::Result result = (Semaphore::Result)mThreadData.mpData->mStartedSemaphore.Wait(timeoutAbsolute);
            EAT_ASSERT(result != Semaphore::kResultError);

            if(result >= 0) // If the Wait succeeded, as opposed to timing out...
            {
                // We know for sure that the thread status is running now.
                currentStatus = kStatusRunning;
                mThreadData.mpData->mStartedSemaphore.Post(); // Re-post the semaphore so that any other callers of WaitForEnd don't block on the Wait above.
            }
        } // fall through.

        if(currentStatus == kStatusRunning) // If the thread has started but not yet exited...
        {
            // Lock on the mutex (which is available when the thread is exiting)
            Mutex::Result result = (Mutex::Result)mThreadData.mpData->mRunMutex.Lock(timeoutAbsolute);
            EAT_ASSERT(result != Mutex::kResultError);

            if(result > 0) // If the Lock succeeded, as opposed to timing out... then the thread has exited or is in the process of exiting.
            {
                // Do a pthread join. This is a blocking call, but we know that it will end very soon, 
                // as the mutex unlock the thread did is done right before the thread returns to the OS.
                // The return value of pthread_join has information that isn't currently useful to us.
                pthread_join(mThreadData.mpData->mThreadId, NULL);
                mThreadData.mpData->mThreadId = kThreadIdInvalid;

                // We know for sure that the thread status is ended now.
                currentStatus = kStatusEnded;
                mThreadData.mpData->mRunMutex.Unlock();
            }
            // Else the Lock timed out, which means that the thread didn't exit before we ran out of time.
            // In this case we need to return to the user that the status is kStatusRunning.
        }
        else
        {
            // Else currentStatus == kStatusEnded.
            pthread_join(mThreadData.mpData->mThreadId, NULL);
            mThreadData.mpData->mThreadId = kThreadIdInvalid;
        }

        if(currentStatus == kStatusEnded)
        {
            // Call GetStatus again to get the thread return value.
            currentStatus = GetStatus(pThreadReturnValue);
        }

        return currentStatus;
    }
    else
    {
        // Else the user hasn't started the thread yet, so we wait until the user starts it.
        // Ideally, what we really want to do here is wait for some kind of signal. 
        // Instead for the time being we do a polling loop. 
        while((!mThreadData.mpData || (mThreadData.mpData->mThreadId == kThreadIdInvalid)) && (GetThreadTime() < timeoutAbsolute))
        {
            ThreadSleep(1);
            EAReadWriteBarrier();
            EACompilerMemoryBarrier();
        }

        if(mThreadData.mpData)
            return WaitForEnd(timeoutAbsolute);
    }

    return kStatusNone; 
}


EA::Thread::Thread::Status EA::Thread::Thread::GetStatus(intptr_t* pThreadReturnValue) const
{
    if(mThreadData.mpData)
    {
        EAReadBarrier();
        Status status = (Status)mThreadData.mpData->mnStatus;

        if(pThreadReturnValue && (status == kStatusEnded))
            *pThreadReturnValue = mThreadData.mpData->mnReturnValue;

        return status;
    }

    return kStatusNone;
}


EA::Thread::ThreadId EA::Thread::Thread::GetId() const
{
    // A mutex lock around mpData is not needed below because 
    // mpData is never allowed to go from non-NULL to NULL. 
    if(mThreadData.mpData)
        return mThreadData.mpData->mThreadId;

    return kThreadIdInvalid;
}


int EA::Thread::Thread::GetPriority() const
{
    // A mutex lock around mpData is not needed below because 
    // mpData is never allowed to go from non-NULL to NULL. 
    if(mThreadData.mpData)
    {
        int         policy;
        sched_param param;

        int result = pthread_getschedparam(mThreadData.mpData->mThreadId, &policy, &param);

        if(result == 0)
            return ConvertFromNativePriority(param, policy);

        return kThreadPriorityDefault;
    }

    return kThreadPriorityUnknown;
}


bool EA::Thread::Thread::SetPriority(int nPriority)
{
    // A mutex lock around mpData is not needed below because 
    // mpData is never allowed to go from non-NULL to NULL. 
    EAT_ASSERT(nPriority != kThreadPriorityUnknown);

    if(mThreadData.mpData)
    {
        int         policy;
        sched_param param;

        int result = pthread_getschedparam(mThreadData.mpData->mThreadId, &policy, &param);

        if(result == 0) // If success...
        {
            ConvertToNativePriority(nPriority, param, policy);

            result = pthread_setschedparam(mThreadData.mpData->mThreadId, policy, &param);
        }

        return (result == 0);
    }

    return false;
}


// To consider: Make it so we return a value.
void EA::Thread::Thread::SetProcessor(int nProcessor)
{
    #if defined(EA_PLATFORM_WINDOWS)
        if(mThreadData.mpData)
        {
            static AtomicInt32 nProcessorCount = 0;

            if(nProcessorCount == 0)
            {
                SYSTEM_INFO systemInfo;
                memset(&systemInfo, 0, sizeof(systemInfo));
                GetSystemInfo(&systemInfo);
                nProcessorCount = (int)systemInfo.dwNumberOfProcessors;
            }

            DWORD dwThreadAffinityMask;

            if(nProcessor < 0)
                dwThreadAffinityMask = 0xffffffff;
            else
            {
                if(nProcessor >= nProcessorCount)
                    nProcessor %= nProcessorCount; 

                dwThreadAffinityMask = 1 << nProcessor;
            }

            SetThreadAffinityMask(mThreadData.mpData->mThreadId, dwThreadAffinityMask);
        }
    #elif defined(EA_PLATFORM_LINUX) 
        if(mThreadData.mpData)
        {
            mThreadData.mpData->mStartupProcessor = nProcessor; // Assign this in case the thread hasn't started yet and thus we are leaving it a message to set it when it has started.
            SetPlatformThreadAffinity(mThreadData.mpData);
        }
    #else
        EA_UNUSED(nProcessor);
    #endif
}

void EA::Thread::Thread::SetAffinityMask(EA::Thread::ThreadAffinityMask nAffinityMask)
{
    if(mThreadData.mpData->mThreadId)
    {
        SetThreadAffinityMask(mThreadData.mpData->mThreadId, nAffinityMask);
    }
}

EA::Thread::ThreadAffinityMask EA::Thread::Thread::GetAffinityMask()
{
    if(mThreadData.mpData->mThreadId)
    {
        return mThreadData.mpData->mnThreadAffinityMask;
    }
    
    return kThreadAffinityMaskAny;
}

void EA::Thread::Thread::Wake()
{
    // Todo: implement this. The solution is to use a signal to wake the sleeping thread via an EINTR.
    // Possibly use the SIGCONT signal. Have to look into this to tell what the best approach is.
}

const char* EA::Thread::Thread::GetName() const
{
    if(mThreadData.mpData)
        return mThreadData.mpData->mName;

    return "";
}

void EA::Thread::Thread::SetName(const char* pName)
{
    if(mThreadData.mpData && pName)
		EA::Thread::SetThreadName(mThreadData.mpData->mThreadId, pName);
}

#if defined(EA_PLATFORM_ANDROID)
namespace EA
{
    namespace Thread
    {
        void SetCurrentThreadName(JNIEnv* env, const char* pName)
        {
            SetCurrentThreadNameJava(env, pName);
        }
    }
}
#endif


#endif // EA_PLATFORM_XXX












