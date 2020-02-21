///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// To do: Deal with possible int64_t rollover in various parts of code below.
///////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/EACallback.h>
#include <EAStdC/EARandomDistribution.h>
#include <string.h>
#include <EAAssert/eaassert.h>



///////////////////////////////////////////////////////////////////////////
// Some stuff used to support the callbacks
//
#if EASTDC_THREADING_SUPPORTED
	#define EA_CALLBACK_PROCESSOR_MUTEX_LOCK()   mMutex.Lock()
	#define EA_CALLBACK_PROCESSOR_MUTEX_UNLOCK() mMutex.Unlock()
#else
	#define EA_CALLBACK_PROCESSOR_MUTEX_LOCK() 
	#define EA_CALLBACK_PROCESSOR_MUTEX_UNLOCK() 
#endif


// We define the following macros from CoreAllocator here to avoid a dependency on the MemoryMan package.
// #ifndef EA_CB_CA_NEW
//     #define EA_CB_CA_NEW(Class, pAllocator, pName) new ((pAllocator)->Alloc(sizeof(Class), pName, 0, EA_ALIGN_OF(Class), 0)) Class
// #endif
// 
// #ifndef EA_CB_CA_DELETE
//     #define EA_CB_CA_DELETE(pObject, pAllocator) EA::StdC::delete_object(pObject, pAllocator)
// #endif



namespace EA
{
namespace StdC
{


///////////////////////////////////////////////////////////////////////////////
// Misc
///////////////////////////////////////////////////////////////////////////////

// The use of standard min/max leads to compile errors sensitive 
// to the state of EASTDC_THREADING_SUPPORTED.
template <class T>
const T smin(const T& a, const T& b)
{
	return a < b ? a : b;
}


static void DefaultCallback(Callback* pCallback, void*, uint64_t, uint64_t)
{
	pCallback->Stop();
}


/*
/// delete_object
///
/// Deletes an object created by create_object.
/// See create_object for specifications and examples.
///
template <typename T>
inline void delete_object(T* pObject, Allocator::ICoreAllocator* pAllocator)
{
	if(pObject) // As per the C++ standard, deletion of NULL results in a no-op.
	{
		pObject->~T();
		pAllocator->Free(pObject);
	}
}
*/



///////////////////////////////////////////////////////////////////////////////
// Callback
///////////////////////////////////////////////////////////////////////////////

Callback::Callback()
	: mPeriod(UINT64_C(1000000000))
	, mPrecision(500000)
	, mpCallbackManager(NULL)
	, mpFunction(NULL)
	, mpFunctionArg(NULL)
	, mType(kTypeTime)
	, mbStarted(0)
	, mbOneShot(false)
	, mbEnableRefCount(false)
	, mNextCallbackEvent(0)
	, mLastCallbackEvent(0)
{
	EA_ASSERT((int64_t)mPeriod > 0);     // Sanity checks.
	EA_ASSERT((int64_t)mPrecision > 0);

	SetFunctionInfo(NULL, NULL, false);
}


Callback::Callback(CallbackFunctionType pCallbackFunc, void* pCallbackFuncArg, uint64_t period, 
				   uint64_t precision, Type type, bool bEnableRefCount)
	: mPeriod(period)
	, mPrecision(precision) 
	, mpCallbackManager(NULL)
	, mpFunction(NULL)
	, mpFunctionArg(NULL)
	, mType(type)
	, mbStarted(0)
	, mbOneShot(false)
	, mbEnableRefCount(false)
	, mNextCallbackEvent(0)
	, mLastCallbackEvent(0)
{
	EA_ASSERT((int64_t)mPeriod > 0);     // Sanity checks.
	EA_ASSERT((int64_t)mPrecision > 0);

	SetFunctionInfo(pCallbackFunc, pCallbackFuncArg, bEnableRefCount);
}


Callback::~Callback()
{
	if((int32_t)mbStarted) // Cast to int32_t because mbStarted might be an atomic int class.
		Stop();
}


// Sets the function which is called when the time/tick count expire. Note that if the 
// in asynch mode, the callback could occur in a different thread from the thread that 
// started the timer.
bool Callback::SetFunctionInfo(Callback::CallbackFunctionType pCallbackFunction, void* pCallbackArgument, bool bEnableRefCount)
{
	if(pCallbackFunction)
	{
		mpFunction    = pCallbackFunction;
		mpFunctionArg = pCallbackArgument;
	}
	else 
	{
		mpFunction    = DefaultCallback;
		mpFunctionArg = this;
	}

	if(bEnableRefCount)
	{
		mbEnableRefCount = true;
		AddRefCallback(); // This will AddRef the pointer if it is non-NULL.
	}

	return true;
}


void Callback::GetFunctionInfo(Callback::CallbackFunctionType& pCallbackFunction, void*& pCallbackArgument) const
{
	pCallbackFunction = mpFunction;
	pCallbackArgument = mpFunctionArg;
}


void Callback::Call(uint64_t absoluteValue, uint64_t deltaValue)
{
	EA_ASSERT(mpFunction);
	if(mpFunction)
		mpFunction(this, mpFunctionArg, absoluteValue, deltaValue);
}


uint64_t Callback::GetPeriod() const
{
	return mPeriod;
}


bool Callback::SetPeriod(uint64_t nPeriod)
{
	EA_ASSERT((int64_t)nPeriod > 0);     // Sanity checks.
	mPeriod = nPeriod;
	return true;
}


uint64_t Callback::GetPrecision() const
{
	return mPrecision;
}


bool Callback::SetPrecision(uint64_t nPrecision)
{
	EA_ASSERT((int64_t)nPrecision >= 0);
	mPrecision = nPrecision;
	return true;
}


bool Callback::Start(ICallbackManager* pCallbackManager, bool bOneShot)
{
	if(!(int32_t)mbStarted) // Cast to int32_t because mbStarted might be an atomic int class.
	{
		if(!pCallbackManager)
			pCallbackManager = GetCallbackManager();

		mpCallbackManager = pCallbackManager;

		if(pCallbackManager)
			mbStarted = (mpCallbackManager->Add(this, bOneShot) ? 1 : 0);
	}

	return ((int32_t)mbStarted != 0);
}


void Callback::Stop()
{
	if((int32_t)mbStarted) // Cast to int32_t because mbStarted might be an atomic int class.
	{
		mpCallbackManager->Remove(this);
		mbStarted = 0;

		// Note that the following may result in the Callback object (this)
		// being deleted, due to a reference count decrement on itself.
		// Thus it is important that this be the last thing done in this function.
		if(mbEnableRefCount)
			ReleaseCallback();
	}
}


bool Callback::IsStarted() const
{
	return ((int32_t)mbStarted != 0); // Cast to int32_t because mbStarted might be an atomic int class.
}


bool Callback::SetType(Type type)
{
	mType = type;
	return true;
}


Callback::Type Callback::GetType() const
{
	return mType;
}


void Callback::AddRefCallback()
{
	Call(kMessageAddRef, 0);
}


void Callback::ReleaseCallback()
{
	Call(kMessageRelease, 0);
}



///////////////////////////////////////////////////////////////////////////////
// CallbackVector
///////////////////////////////////////////////////////////////////////////////

CallbackManager::CallbackVector::CallbackVector()
  : mpBegin(mLocalBuffer),
	mpEnd(mLocalBuffer),
	mpCapacity(mLocalBuffer + EAArrayCount(mLocalBuffer))
{
	#if defined(EA_DEBUG)
		memset(mLocalBuffer, 0, sizeof(mLocalBuffer));
	#endif
}


CallbackManager::CallbackVector::~CallbackVector()
{
	if(mpBegin != mLocalBuffer)
		EASTDC_DELETE[] mpBegin; // It's OK if this is NULL; C++ allows it.
}


CallbackManager::CallbackVector::iterator CallbackManager::CallbackVector::erase(value_type* pIterator)
{
	EA_ASSERT((pIterator >= mpBegin) && (pIterator < mpEnd));
	const size_t moveCount = (size_t)((mpEnd - pIterator) - 1);
	memmove(pIterator, pIterator + 1, moveCount * sizeof(value_type));

	--mpEnd;

	#if defined(EA_DEBUG)
		memset(mpEnd, 0, sizeof(value_type));
	#endif

	return pIterator;
}


CallbackManager::CallbackVector::iterator CallbackManager::CallbackVector::push_back(value_type value)
{
	if((mpEnd + 1) >= mpCapacity) // If there is insufficient existing capacity...
	{
		const size_t oldSize     = (size_t)(mpEnd - mpBegin);
		const size_t oldCapacity = (size_t)(mpCapacity - mpBegin);
		const size_t newCapacity = (oldCapacity >= 2) ? (oldCapacity * 2) : 4;

		value_type* pBegin = EASTDC_NEW("EACallback") value_type[newCapacity];
		EA_ASSERT(pBegin);
		memcpy(pBegin, mpBegin, oldSize * sizeof(value_type));
		if(mpBegin != mLocalBuffer)
			EASTDC_DELETE[] mpBegin;
		mpBegin    = pBegin;
		mpEnd      = pBegin + oldSize;
		mpCapacity = pBegin + newCapacity;
	}

	*mpEnd = value;

	return ++mpEnd;
}



///////////////////////////////////////////////////////////////////////////////
// CallbackManager
///////////////////////////////////////////////////////////////////////////////

static ICallbackManager* gpCallbackManager = NULL;

EASTDC_API ICallbackManager* GetCallbackManager()
{
	return gpCallbackManager;
}

EASTDC_API void SetCallbackManager(ICallbackManager* pCallbackManager)
{
	gpCallbackManager = pCallbackManager;
}



CallbackManager::CallbackManager()
  : mCallbackArray()
  , mStopwatch(EA::StdC::Stopwatch::kUnitsNanoseconds)
  , mTickCounter(0)
  , mUserEventCounter(0)
  , mbInitialized(false)
  , mbRunning(false)
  , mbAsync(false)
  , mRandom()
  #if EASTDC_THREADING_SUPPORTED
  , mNSecPerTick(10000000)
  , mNSecPerTickLastTimeMeasured(INT64_MIN)
  , mNSecPerTickLastTickMeasured(INT64_MIN)
  , mNextCallbackEventTime(0)
  , mNextCallbackEventTick(0)
  , mMutex()
  , mThread()
  , mbThreadStarted(0)
  , mThreadParam()
  #endif
{
	// mCallbackArray.reserve(8); Disabled because it already has a built-in mLocalBuffer.
}

CallbackManager::~CallbackManager()
{
	CallbackManager::Shutdown();
}


bool CallbackManager::Init(bool bAsync, bool bAsyncStart
						#if EASTDC_THREADING_SUPPORTED
                           , EA::Thread::ThreadParameters threadParam	
						#endif
                           )
{
	if(!mbRunning)
	{
		mbAsync   = bAsync;
		mbRunning = true;

        #if EASTDC_THREADING_SUPPORTED
            mThreadParam = threadParam;
        #else
			EA_ASSERT(!mbAsync);
			mbAsync = false; // The best we can do. Should never happen though.
		#endif

		mStopwatch.Restart();

		if(mbAsync && bAsyncStart)
			mbRunning = StartThread(); // If StartThread fails then set mbRunning to false.
	}

	return mbRunning;
}


void CallbackManager::Shutdown()
{
	EA_CALLBACK_PROCESSOR_MUTEX_LOCK();

	if(mbRunning)
	{
		mbRunning = false;  // Set this to false so no further calls to CallbackManager will proceed.

		StopThread();

		mStopwatch.Stop();

		// Stop all running Callbacks. This allows them to do cleanup.
		for(size_t i = 0, iEnd = mCallbackArray.size(); i < iEnd; ++i)
		{
			if(mCallbackArray[i]) // It's possible this could be NULL, because stopped callbacks are merely NULL their in the mCallbackArray.
			{
				Callback* pCallback = mCallbackArray[i]; // Make a temp because we will be unlocking our mutex below.
				mCallbackArray[i] = NULL;                // Leave it as NULL for now. We'll actually erase the entry later during our update cycle. Our code is fine with NULL pointers and it's useful to keep them because their slots can be re-used.

				EA_CALLBACK_PROCESSOR_MUTEX_UNLOCK();
				pCallback->Stop();
				EA_CALLBACK_PROCESSOR_MUTEX_LOCK();
			}
		}

		mCallbackArray.clear();
	}

	EA_CALLBACK_PROCESSOR_MUTEX_UNLOCK();
}


// Returns true if the thread is running upon return of this function.
// Will return true if the thread was already running upon calling this function.
bool CallbackManager::StartThread()
{
	#if EASTDC_THREADING_SUPPORTED
		if(mbAsync)
		{
			if(mbThreadStarted.SetValueConditional(1, 0)) // If the thread was previously 0 and we set it to 1...
			{
				mThreadParam.mpName = "CallbackManager";  // Some platforms have an extremely limited thread name buffer and will clip this.
				EA::Thread::ThreadId threadId = mThread.Begin(RunStatic, static_cast<CallbackManager*>(this), &mThreadParam);

				EA_ASSERT(threadId != EA::Thread::kThreadIdInvalid);
				return (threadId != EA::Thread::kThreadIdInvalid);
			}
			return true; // Else the thread was already running...
		}
	#endif

	return false;
}


void CallbackManager::StopThread()
{
	#if EASTDC_THREADING_SUPPORTED
		if(mbThreadStarted.SetValueConditional(0, 1)) // If the thread was previously 1 and we set it to 0...
		{
			mThread.Wake();            // Should be a semaphore or condition variable signal.
			mThread.WaitForEnd();
		}
	#endif
}

void CallbackManager::Update()
{
	int64_t curTick = 0;
	int64_t curTime = 0;
	int64_t curUserEvent = 0;

	UpdateInternal(curTick, curTime, curUserEvent);

	EA_UNUSED(curTick);
	EA_UNUSED(curTime);
	EA_UNUSED(curUserEvent);
}


struct TempUnitsInfo
{
	int64_t  mUnits;
	int64_t* mpNextEventUnits;
};

void CallbackManager::UpdateInternal(int64_t& curTick, int64_t& curTime, int64_t& curUserEvent)
{
	EA_CALLBACK_PROCESSOR_MUTEX_LOCK();

	EA_ASSERT(mbRunning); // The user must have called CallbackManager::Init before using it.

	curTick      = ++mTickCounter;
	curTime      = (int64_t)mStopwatch.GetElapsedTime();
	curUserEvent = (int64_t)mUserEventCounter;
	
	if(!mCallbackArray.empty())
	{
		// Every time Update is called, we need to call the elapsed Callbacks and then 
		// figure out the next time we'll need to do a callback.

		// Scan our list and call the callbacks as needed
		int64_t nextCallBackUserEvent = 0;

		TempUnitsInfo timeInfo      = { curTime,      &mNextCallbackEventTime };
		TempUnitsInfo tickInfo      = { curTick,      &mNextCallbackEventTick };
		TempUnitsInfo userEventInfo = { curUserEvent, &nextCallBackUserEvent  };

		for(size_t i = 0; i < mCallbackArray.size(); ++i) // Intentionally re-evaluate size every time through, as it could change dynamically below. Intentionally use < instead of !=, as size could decrease by any amount during the execution below.
		{
			Callback*      pCallback = mCallbackArray[i];
			TempUnitsInfo* pTUI      = NULL;

			if(pCallback)
			{
				// Call the callback function if needed 
				switch(pCallback->GetType())
				{
					case Callback::kTypeTime:
						pTUI = &timeInfo;
						break;
						
					case Callback::kTypeTick:
						pTUI = &tickInfo;
						break;

					default:
					case Callback::kTypeUserEvent:
						pTUI = &userEventInfo;
						break;
				}

				EA_ASSERT(pTUI != NULL);
				if(pTUI->mUnits >= pCallback->mNextCallbackEvent) // If it's time to call this callback...
				{
					// We have to beware that this Call might result in the callee manipulating 
					// CallbackManager (us) and change our state, particularly with respect to 
					// starting and stopping callbacks (including this callback).
					// As of this writing, our mutex is locked during the Call. This leaves an 
					// opportunity for threading deadlock. To consider: See if we can unlock the 
					// mutex before calling this. We would need to re-evaluate some of our state
					// upon return if we did this. Maybe have a member variable called mHasChanged
					// to make this more efficient.
					pCallback->Call((uint64_t)pTUI->mUnits, (uint64_t)(pTUI->mUnits - pCallback->mLastCallbackEvent));

					if((i < mCallbackArray.size()) && (mCallbackArray[i] == pCallback)) // If the callback wasn't stopped and removed during the Call to the user above...
					{
						pCallback->mLastCallbackEvent = pTUI->mUnits;

						if(pCallback->mbOneShot)
							pCallback->Stop();
						else 
						{
							const int32_t precision = (int32_t)pCallback->GetPrecision();
							const int64_t period    = (int64_t)pCallback->GetPeriod();

							pCallback->mNextCallbackEvent = (pTUI->mUnits + period);

							if(precision) // To consider; For kTypeTime it might be worth testing for (precision > 100) or similar instead here.
							{
								// An alternative to the use of random below would be a load minimization 
								// strategy with quite a bit more involved implementation.
								const int32_t delta             = RandomInt32UniformRange(mRandom, -precision, precision - 1); // Note by Paul P: I added this -1 so unit tests could pass, but it doesn't seem right.
								const int64_t nextCallbackEvent = pCallback->mNextCallbackEvent + delta;

								if(nextCallbackEvent > pTUI->mUnits) // Ignore precision adjustments that make it so the next event is prior to the current one.
									pCallback->mNextCallbackEvent = nextCallbackEvent;
							}

							EA_ASSERT(pCallback->mNextCallbackEvent >= pTUI->mUnits); // Assert that the next event is not backwards in time.

							if(mbAsync)
							{
								if(*pTUI->mpNextEventUnits > pCallback->mNextCallbackEvent)     // Update mNextCallbackEventTime or mNextCallbackEventTick to reflect what is the
									*pTUI->mpNextEventUnits = pCallback->mNextCallbackEvent;    // minimum time until the next event. We'll use that in the thread Run function to
							}                                                                   // know how long to sleep/wait before it needs to do another callback.
						}
					}
				}
			} // if(pCallback)
			else 
			{
				mCallbackArray.erase(&mCallbackArray[i]);
			}

		} // for(...)

	} // if(!mCallbackArray.empty())

	EA_CALLBACK_PROCESSOR_MUTEX_UNLOCK();
}


// Thread function.
intptr_t CallbackManager::Run()
{
	#if EASTDC_THREADING_SUPPORTED
		EA_ASSERT(mbThreadStarted.GetValue() != 0);

		while(mbRunning)
		{
			int64_t curTick;
			int64_t curTime;  
			int64_t curUserEvent;  

			UpdateInternal(curTick, curTime, curUserEvent);

			// Update msec/tick value if needed.
			// Note by Paul Pedriana: I don't like this nanosecond tick calculation logic. IMO it's too delicate.
			const int64_t kNSecPerTickFrequency = UINT64_C(50000000); // in nsec -- how often to update mNSecPerTick

			if(curTime > (mNSecPerTickLastTimeMeasured + kNSecPerTickFrequency))
			{
				mNSecPerTick                 = ((double)curTime - (double)mNSecPerTickLastTimeMeasured) / ((double)curTick - (double)mNSecPerTickLastTickMeasured);
				mNSecPerTickLastTimeMeasured = curTime;
				mNSecPerTickLastTickMeasured = curTick;
			}

			// Come up with sleeping time and put the thread to sleep 
			// To do: We need to switch this to an alternative synchronization primitive, as sleeping isn't very great.
			#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) // If using a Microsoft OS where Wake is supported and we can sleep for a long time. 
				int64_t timeToNextEventMs = INT_MAX;
			#else
				int64_t timeToNextEventMs = 50;
			#endif

			if(!mCallbackArray.empty()) // If there are any active callbacks...
			{
				if(mNextCallbackEventTime < curTime)
					mNextCallbackEventTime = curTime + 100000000;   // 100 milliseconds worth of nanoseconds. The number is arbitrary. Probably should be a smaller value for faster machines.
				if(mNextCallbackEventTick < curTick)
					mNextCallbackEventTick = curTick + 1000;        // Arbitrary.

			  //const int64_t absoluteTime = (int64_t)mStopwatch.GetElapsedTime();                          // Nanoseconds
				const int64_t timeDelta    = mNextCallbackEventTime - curTime;                              // Nanoseconds
				const int64_t tickDelta    = (int64_t)((mNextCallbackEventTick - curTick) * mNSecPerTick);  // Nanoseconds
				const int64_t minDelta     = smin(timeDelta, tickDelta);                                    // Nanoseconds

				timeToNextEventMs = (minDelta / 1000000) / 2;   // Convert minDelta to milliseconds (what ThreadSleep wants) and half it in order to oversample the callback time (is this necessary?)

				if(timeToNextEventMs < 0)
					timeToNextEventMs = 0; // simply yield.
			} 

			// Question by Paul Pedriana upon examining this code: Why is this implemented using thread sleeping instead of a conventional 
			// thread synchronization primitive such as a Semaphore? At the least this should be a semaphore wait with a timeout set 
			// to be equal to timeToNextEventMs. Then instead of waking a thread with Thread::Wake, we can simply signal the semaphore. This is 
			// expecially so because some platforms don't support waking threads from sleep.

			if(timeToNextEventMs == 0)
				Thread::ThreadSleep(EA::Thread::kTimeoutYield);
			else
				Thread::ThreadSleep(EA::Thread::ThreadTime(timeToNextEventMs));

			//#if defined(EA_DEBUG)
			//    static int64_t lastSleepTimes[200];
			//    static int lastSleepTimeIndex = 0;
			//    if(lastSleepTimeIndex == 200)
			//        lastSleepTimeIndex = 0;
			//    lastSleepTimes[lastSleepTimeIndex++] = timeToNextEventMs;
			//#endif
		}
	#endif

	return 0;
}


bool CallbackManager::Add(Callback* pCallback, bool bOneShot)
{
	bool bReturnValue = false;

	EA_ASSERT(pCallback != NULL);

	EA_CALLBACK_PROCESSOR_MUTEX_LOCK();
	
	if(mbRunning)
	{
		size_t found       = 0xffffffff;
		size_t found_empty = 0xffffffff;

		// See if pCallback is already added and while doing so see if there is an existing empty slot if it's not already added.
		for(size_t i = 0, iEnd = mCallbackArray.size(); i < iEnd; ++i)
		{
			Callback* pCallbackTemp = mCallbackArray[i];

			if(pCallbackTemp == pCallback)
			{
				found = i;
				break;
			}
			else if(!pCallbackTemp && (found_empty == 0xffffffff))
				found_empty = i;
		}              
		
		if(found == 0xffffffff) // If pCallback isn't already present...
		{
			if(found_empty == 0xffffffff) // If no empty slot was found...
				mCallbackArray.push_back(pCallback);
			else 
				mCallbackArray[found_empty] = pCallback;

			int64_t  units           = 0;   // This is the current time, current tick, or current user event number.
			int64_t  nextUnits       = 0;
			int64_t* pNextEventUnits = &nextUnits;
			int32_t  precision       = (int32_t)pCallback->GetPrecision();
			int64_t  period          = (int64_t)pCallback->GetPeriod();

			switch(pCallback->GetType())
			{
				case Callback::kTypeTime: // If the callback triggers after a set amount of time...
					units           = (int64_t)mStopwatch.GetElapsedTime();
					pNextEventUnits = &mNextCallbackEventTime;
					break;
					
				case Callback::kTypeTick: // If the callback triggers after a set amount of ticks...
					units           = (int64_t)mTickCounter;
					pNextEventUnits = &mNextCallbackEventTick;
					break;

				case Callback::kTypeUserEvent: // If the callback triggers after a manually user-generated event...
				default:
					break;
			}

			pCallback->mbOneShot          = bOneShot;
			pCallback->mNextCallbackEvent = units + period;
			pCallback->mLastCallbackEvent = units;

			if(precision)
			{
				const int32_t delta             = RandomInt32UniformRange(mRandom, -precision, precision - 1); // Note by Paul P: I added this -1 so unit tests could pass, but it doesn't seem right.
				const int64_t nextCallbackEvent = pCallback->mNextCallbackEvent + delta;

				if(nextCallbackEvent > pCallback->mNextCallbackEvent) // Ignore precision adjustments that make it so the next event is prior to the current one.
					pCallback->mNextCallbackEvent = nextCallbackEvent;
			}

			EA_ASSERT(pCallback->mNextCallbackEvent >= units); // Assert that the next event is not backwards in time.

			if(mbAsync)
			{
				// Note by Paul P: Is the following really supposed to use a < comparison? I didn't originally write this.
				// It works but I'm not sure it's the best way to do this. It seems to me that the next 
				// event units should by default be a very long time from now and newly added Callback 
				// objects should reduce that time. I think that this code here works because while we set a 
				// mNextCallbackEventTime/mNextCallbackEventTick to be further in the future, the RunInternal function
				// will loop over Callback objects and select the actual soonest one. If we switched the > here to a >
				// then we would need to make sure we initially set mNextCallbackEventTime/mNextCallbackEventTick to be
				// a high value instead of it's initial default of 0, because if it starts as zero it will get stuck 
				// there permanently because it never gets updated (I tried this so I know it happens like so).
				if(*pNextEventUnits < pCallback->mNextCallbackEvent)
					*pNextEventUnits = pCallback->mNextCallbackEvent;
			}
		} 

		bReturnValue = true; // This might turn false below in case of an error.

		#if EASTDC_THREADING_SUPPORTED
			if(mbAsync) // If we run in async (background thread) mode...
			{
				if(mbThreadStarted.GetValue() == 0) // If the thread hasn't been started yet...
					bReturnValue = StartThread();   // Starts it if not already started. Is there something useful we could do with the return value of this?

				if((mNextCallbackEventTime < (int64_t)mStopwatch.GetElapsedTime()) || // If we need to wake the thread now to do a callback...
				   (mNextCallbackEventTick < (int64_t)mTickCounter))
				{
					// Note: Some platforms don't have the capability of waking a sleeping thread.
					// This code should be using a semaphore instead of thread sleep/wake.
					mThread.Wake();
				}
			}
		#endif                    
	}

	EA_CALLBACK_PROCESSOR_MUTEX_UNLOCK();

	return bReturnValue;
} 


bool CallbackManager::Remove(Callback* pCallback)
{
	bool bRemoved = false;

	EA_CALLBACK_PROCESSOR_MUTEX_LOCK();

	if(pCallback)
	{
		if(mbRunning)
		{
			for(size_t i = 0, iEnd = mCallbackArray.size(); i < iEnd; ++i)
			{
				if(mCallbackArray[i] == pCallback)
				{
					mCallbackArray[i] = NULL; // We might re-use this slot later, so we don't take the CPU cycles to free the array right now.
					bRemoved = true;
					break;
				}
			}              
		}
	}

	EA_CALLBACK_PROCESSOR_MUTEX_UNLOCK();

	// It's important to call this outside our mutex lock.
	if(bRemoved)
		pCallback->Stop();

	return bRemoved;
}


#if EASTDC_THREADING_SUPPORTED

EA::Thread::Thread& CallbackManager::GetThread()
{
	return mThread;
}

void CallbackManager::Lock()
{
	mMutex.Lock();
}

void CallbackManager::Unlock()
{
	mMutex.Unlock();
}

#endif



void CallbackManager::OnUserEvent()
{
	// To consider: Call the Update function here if callbacks waiting on user events are due.
	//              The problem with doing this is that it makes OnUserEvent have side effects
	//              which may be beyond what the user wants or expects.

	#if EASTDC_THREADING_SUPPORTED
		// Note: Some platforms don't have the capability to wake a sleeping thread.
		// This code should be using a semaphore instead of thread sleep/wake.
		if(mThread.GetStatus() == EA::Thread::Thread::kStatusRunning)
			mThread.Wake();
	#endif

	++mUserEventCounter;
}


uint64_t CallbackManager::GetTime()
{
	return mStopwatch.GetElapsedTime();
}



} // namespace StdC

} // namespace EA



