///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// EACallback is a callback timer, also known as an asynchronous timer or alarm. 
// You provide a callback function and your function will be called at a fixed 
// periodic rate or will be called just once ("one-shot"). 
//
// Callback timers are useful for implementing systems that must respond to 
// something periodically or after some amount of time. Examples include:
//    - Streaming buffer periodic refilling
//    - Text editor flashing carets
//    - Alarm clocks
//    - Starting and stopping of background tasks 
//
// Notes:
//    - Time-based callbacks are specified in nanoseconds.
//    - The callback system can work synchronously or asynchronously (user's choice). 
//      In the former case, callbacks are serviced via a manually-called Tick function. 
//      In the latter case, multithreading is used.
//    - In the case of asynchronous callbacks, your callback function will likely be 
//      called from a different thread than the thread used to set up the callback. 
//    - The precision specification of a callback is merely a hint and not a guarantee, 
//      due to the fact that the platforms we are running on are not strictly 
//      real-time systems. This is particularly true with desktop systems like 
//      Windows which implement pre-emptive thread time-slicing. 
//    - The period of a callback will be respected to the extent possible. 
//      However, if another callback is being serviced ahead of your callback, 
//      it might cause delays in the calling of your callback. 
//    - Callbacks may operate in synchronous and asynchronous modes with respect to 
//      the rest of the application. Asynchronous mode is implemented via a separate thread. 
//    - Periodicity may be time or tick based, depending on what you request. 
//    - The callback parameters (e.g. period, function, mode) can be changed at any 
//      time and from any place, including within the callback function itself. 
//    - You can stop a periodic callback at any time and from any place, including 
//      within the callback function itself.
//    - As of this writing, EACallback doesn't guarantee ordering of event notifications.
//      Thus if you have two callbacks, one to call back in 15 ms and another in 20 ms,
//      and the callback system doesn't get polled until 30 ms later, the callback for 
//      20 ms might be notified before the callback for 15 ms. This may be changed in the future.
//
// Example usage:
//     
//     void MyCallback(Callback* pCallback, void* pContext, uint64_t absoluteValue, uint64_t deltaValue)
//     {
//         printf("%llu nanoseconds have passed since the last callback.\n");
//     }
//
//     int main(int, char**)
//     {
//         // Manager setup
//         CallbackManager callbackManager;
//         SetCallbackManager(&callbackManager);
//         callbackManager.Init(false, false);    // Run in synchronous mode.
//
//         // Create and register a couple callbacks.
//         Callback callbackA(MyCallback, NULL, 100000, 0);
//         Callback callbackB(MyCallback, NULL, 300000, 0);
//
//         callbackManager.Add(&callbackA, false);  // periodic callback.
//         callbackManager.Add(&callbackB, true);   // one-shot callback.
//
//         // Run the app.
//         while(!ShouldQuit())
//             callbackManager.Update();  // If we ran in asunchronous (threaded) mode, we wouldn't need to call this.
//
//         return 0;
//     }
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EASTDC_EACALLBACK_H
#define EASTDC_EACALLBACK_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>
#include <EAStdC/EAStopwatch.h>
#include <EAStdC/EARandom.h>
#include <eathread/eathread_atomic.h>   // Used even if EASTDC_THREADING_SUPPORTED is zero, as it's still present. Just no actual threads.
#if EASTDC_THREADING_SUPPORTED
	#include <eathread/eathread_thread.h>
	#include <eathread/eathread_mutex.h>
#endif


#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4371) // Layout of class may have changed from a previous version of the compiler due to better packing of member.
	#pragma warning(disable: 4251) // class (some template) needs to have dll-interface to be used by clients.
	#pragma warning(disable: 4121) // 'EA::StdC::CallbackT<T>' : alignment of a member was sensitive to packing.
#endif


namespace EA
{
	namespace StdC
	{
		class ICallbackManager;
		class CallbackManager;


		///////////////////////////////////////////////////////////////////////////
		/// Callback
		///
		/// This is the user class for a callback.
		///
		class EASTDC_API Callback
		{
		public:
			/// Defines the prototype of a function that can be used with a callback. 
			/// absoluteValue is a value of total of time/ticks passed since the system have been started.
			/// deltaValue is the value of time/ticks passed since last callback.
			/// If the user enables RefCounting then this function can be called with
			/// absoluteValue = kMessageAddRef or kMessageRelease instead of a time value.
			/// Your callback function may safely call back into the CallbackManager.
			typedef void (*CallbackFunctionType)(Callback* pCallback, void* pContext, uint64_t absoluteValue, uint64_t deltaValue);
			
			/// The running mode 
			enum Mode
			{
				kModeAsync,    /// Asynchronous/threaded callbacks.
				kModeSync      /// Synchronous/polled callbacks.
			};
			
			/// The type of event to trigger a callback
			enum Type 
			{
				kTypeTime,      /// For callbacks based on time in nanoseconds.
				kTypeTick,      /// For callbacks based on a number of passed ticks. Every time the CallbackSystem::Update function is called (usually once per application loop), the tick count is incremented.
				kTypeUserEvent  /// For callbacks based on some external user event (e.g. screen vblank (vertical retrace)). CallbackSystem::OnUserEvent needs to be called whenever such an event occurs.
			};

			enum Message
			{
				kMessageAddRef  = 0,    // If your callback is called with timeNS == kMessageAddRef, then this means you are being notified of first usage. See bEnableRefCount.
				kMessageRelease = 1     // If your callback is called with timeNS == kMessageRelease, then this means you are being notified of last usage. See bEnableRefCount.
			};

			// static const bool kPeriodic = false; /// kPeriodic is a synonym for 'false', which is how it's used in the API.
			// static const bool kOneShot  = true;  /// kOneShot is a synonym for 'true', which is how it's used in the API.

		public:
			/// Constructor
			Callback();

			/// Constructor
			/// See SetFunctionInfo for relevant documentation.
			Callback(CallbackFunctionType pCallbackFunction, void* pCallbackArgument, 
					 uint64_t periodNs, uint64_t precisionNs = 500000, 
					 Type type = kTypeTime, bool bEnableRefCount = false);

			virtual ~Callback();

			/// Sets the function which is called when the time/tick/event count expire. 
			/// Note that if the in asynch mode, the callback could occur in a different 
			/// thread from the thread that started resumed it.
			///
			/// If bEnableRefCount is true, kMessageAddRef will be sent immediately
			/// and kMessageRelease will be sent immediately after callback is stopped.
			/// For a one-shot timer, the kMessageRelease is sent immediately after the callback 
			/// completes. For timers that are never stopped, the kMessageRelease is called when 
			/// the associated CallbackManager is shut down or when you call Stop. You should use 
			/// the reference counting system if there is any chance there is an opportunity for 
			/// multithreaded race conditions regarding the lifetime of called code and its data.
			bool SetFunctionInfo(CallbackFunctionType pCallbackFunction, void* pCallbackArgument, bool bEnableRefCount);

			/// Sets new function information.
			/// This can be called at any time, including after the callback is already started.
			void GetFunctionInfo(CallbackFunctionType& pCallbackFunction, void*& pCallbackArgument) const;

			/// Calls the callback function directly.
			/// You don't need to call this function unless you want to manually trigger a callback ad-hoc. 
			void Call(uint64_t absoluteValue, uint64_t deltaValue);
			
			/// Gets the period value in nanoseconds/ticks/events.
			uint64_t GetPeriod() const;

			/// Sets the period of the callback in nanoseconds/ticks/events. For a periodic callback, 
			/// this is the time between repetitive callbacks. For a one-shot callback, this is the time
			/// before its call. Changing the period has effect only after the next callback or if 
			/// the user calls Stop and then Start to restart the callback.
			///
			/// As of this writing, Callback periods times refer to the time from one
			/// callback to the next callback, and not in absolute time. For example,
			/// If you request a callback with a period of 100 ms and the first one comes
			/// at 105 ms, the second one is targeted to arrive 100 ms later than 105 ms
			/// (i.e. 205 ms) and not at 200 ms. Thus the system doesn't try to catch up 
			/// if it gets behind, nor does it slow down if it gets ahead.
			bool SetPeriod(uint64_t nPeriodNs);

			/// Gets the user-specified precision of the callback in nanoseconds/ticks/events.
			/// Lower (i.e. more accurate) precision values may incur increased execution overhead
			/// on some systems, though usually it doesn't.
			uint64_t GetPrecision() const;

			/// The callback will occur every GetPeriod() +/- GetPrecisison() units of nanoseconds/ticks/events.
			/// The default precision is 0, which means to be as accurate as possible. This may or may
			/// not incur any extra overhead, depending on the circumstances, though in most cases it
			/// doesn't. One benefit of setting a non-zero precision is that in the case of there
			/// being multiple callbacks of equal periods setting a non-zero precision results in some
			/// slight randomization of the callback times and avoids all the calls happening at once
			/// and causing an execution pause (lack of smoothness) in the app.
			bool SetPrecision(uint64_t nPrecisionNs);

			/// Activates the callback.
			/// If pCallbackManager is NULL, the default CallbackManager is used.
			/// 'bOneShot' controls whether the callback is a call-once or a periodic kind.
			/// The return value indicates whether or not the callback is started. 
			/// If the function is already Started then calling it a second time will
			/// result in a true return value. Calling Start with bOneShot=true twice in 
			/// a row will result in a true return value but only one callback and not two.
			/// It's possible to start in periodic (not one-shot) mode and Stop the callback at any point.
			bool Start(ICallbackManager* pCallbackManager, bool bOneShot);

			/// Stops callback function being called. In asynch mode it is theoretically
			/// possible to receive a callback very shortly after calling Stop, as a call to 
			/// your callback might already be 'in flight' as you are calling Stop. If you 
			/// need to make sure this doesn't happen, you can check the IsStarted 
			void Stop();

			/// Returns true if the callback has been started (is running).
			/// If the callback is a one-shot, this returns true if the callback has been 
			/// started but its one timeup hasn't occurred yet.
			bool IsStarted() const;
			bool IsRunning() const { return IsStarted(); }

			/// Sets running mode (kModeAsync or kModeSync).
			/// Asynch mode will result in being called back from a different thread.
			/// Changes won't take effect until the next time the callback is Started. 
			//bool SetMode(Mode mode);

			/// Gets the running mode (kModeAsync or kModeSync).
			//Mode GetMode() const;

			/// Sets the callback type (kTypeTime, kTypeTick, or kTypeUserEvent).
			/// kTypeTime means that period/precision functions refer time in nanoseconds, 
			/// while ticks are just a snapshot of the count of times called.
			bool SetType(Type type);

			/// Gets the callback type.
			/// Changes won't take effect until the next time the callback is Started. 
			Type GetType() const;

			/// Manually sends the callback function kMessageAddRef.
			/// You normally don't need to manually call this function but should rather
			/// let the Callback instance call it as needed.
			void AddRefCallback();

			/// Manually sends the callback function kMessageRelease.
			/// You normally don't need to manually call this function but should rather
			/// let the Callback instance call it as needed.
			void ReleaseCallback();

		protected:
			friend class CallbackManager;

			uint64_t                mPeriod;                /// Period in units that match Type (nanoseconds if Type is kTypeTime).
			uint64_t                mPrecision;             /// Precision in units that match Type (nanoseconds if Type is kTypeTime).
			ICallbackManager*       mpCallbackManager;      /// The manager we use for callbacks.
			CallbackFunctionType    mpFunction;             /// The user function we call.
			void*                   mpFunctionArg;          /// The context we pass to the user function.
			Type                   mType;                  /// One of enum Type.
			#if EASTDC_THREADING_SUPPORTED
			EA::Thread::AtomicInt32 mbStarted;              /// True if there is an active callback.
			#else
			int32_t                 mbStarted;
			#endif
			bool                    mbOneShot;              /// True if this is a one-shot event.
			bool                    mbEnableRefCount;       /// If true, the callback is notified before first usage and after last usage. Allows for safe lifetime maintentance of callback in the case that it has a dynamic lifetime.

		private: // Internal data owned by CallbackManager.
			int64_t                 mNextCallbackEvent;     /// Time, tick count, or user event count of next callback we should do.
			int64_t                 mLastCallbackEvent;     /// Time, tick count, or user event count of the previous callback we did.

		}; // class Callback



		///////////////////////////////////////////////////////////////////////////
		/// CallbackT
		///
		/// This is a template class based on Callback. In addition to callbacks 
		/// based on static/global functions, this class allows one to use class 
		/// member functions as callbacks.
		///
		/// Example usage:
		///     // In this example, MyFunction will be called very 1000 nanoseconds.
		/// 
		///     class SomeClass : public EA::CallbackT<SomeClass>
		///     {
		///         SomeClass() 
		///           : EA::StdC::CallbackT<SomeClass>(MyFunction, this, 1000)
		///         {
		///             Start(NULL, false);
		///         }
		///
		///         void MyFunction(uint64_t absoluteValue, uint64_t deltaValue)
		///         { 
		///             // Do something. If mType is kTypeTime, values are in nanoseconds. Else they are event counts.
		///         }
		///     };
		///
		template <typename T>
		class CallbackT : public Callback
		{
		public:
			typedef CallbackT<T> this_type;

			typedef void (T::*CallbackFunctionTypeT)(Callback* pCallback, uint64_t absoluteValue, uint64_t deltaValue);
			
			CallbackT()
				: mpMemberFunctionObject(NULL), mpMemberFunction(NULL) { }

			CallbackT(CallbackFunctionTypeT pMemberFunc, T* pMemberFuncObject, uint64_t periodNs, 
					  uint64_t precisionNs = 500000, Type type = kTypeTime, bool bEnableRefCount = false)
				: Callback(NULL, NULL, periodNs, precisionNs, type)
			{
				SetFunctionInfo(pMemberFunc, pMemberFuncObject, bEnableRefCount);
			}

			bool SetFunctionInfo(CallbackFunctionTypeT function, T* pT, bool bEnableRefCount)
			{
				mpMemberFunction       = function;
				mpMemberFunctionObject = pT;

				return Callback::SetFunctionInfo(generic_callback, this, bEnableRefCount);
			}

			void GetFunctionInfo(CallbackFunctionTypeT& function, T*& pT) const
			{
				function = mpMemberFunction;
				pT       = mpMemberFunctionObject;
			}

		protected:
			static void generic_callback(Callback* pCallback, void* /*arg*/, uint64_t absoluteValue, uint64_t deltaValue)
			{
				this_type* pThis = static_cast<this_type*>(pCallback);

				if(pThis && pThis->mpMemberFunctionObject && pThis->mpMemberFunction)                    
				   (pThis->mpMemberFunctionObject->*pThis->mpMemberFunction)(pCallback, absoluteValue, deltaValue);
			}

		protected:
			T*                    mpMemberFunctionObject;      
			CallbackFunctionTypeT mpMemberFunction;      
		};



		///////////////////////////////////////////////////////////////////////////
		/// ICallbackManager
		///
		class EASTDC_API ICallbackManager
		{
		public:
			/// Virtual destructor.
			virtual ~ICallbackManager() { }

			/// This function needs to be called regularly (every frame) from the main application loop.
			virtual void Update() = 0;

			/// This function needs to be called by the application whenever a callback driving event occur.
			virtual void OnUserEvent() = 0;

			/// Returns time in nanoseconds as the Callback system sees it
			virtual uint64_t GetTime() = 0;

			/// Add a new callback
			virtual bool Add(EA::StdC::Callback* pCallback, bool bOneShot) = 0;

			/// Remove a callback
			virtual bool Remove(EA::StdC::Callback* pCallback) = 0;
		};

		// Global default CallbackManager.
		// This is a global default CallbackManager.
		// Your application doesn't need to call SetCallbackManager unless the you want to 
		// call the Callback::Start function with a NULL (default) CallbackManager.
		EASTDC_API ICallbackManager* GetCallbackManager();
		EASTDC_API void              SetCallbackManager(ICallbackManager* pCallbackManager);



		///////////////////////////////////////////////////////////////////////////
		/// CallbackManager
		///
		/// Maintains a set of Callback instances.
		///
		/// As of this writing, Callback periods times refer to the time from one
		/// callback to the next callback, and not in absolute time. For example,
		/// If you request a callback with a period of 100 ms and the first one comes
		/// at 105 ms, the second one is targeted to arrive 100 ms later than 105 ms
		/// (i.e. 205 ms) and not at 200 ms. Thus the system doesn't try to catch up 
		/// if it gets behind, nor does it slow down if it gets ahead.
		/// To consider: Make a flag that enables the behaviour of going by absolute
		/// time instead of inter-callback time. Note that such a feature would have
		/// to watch out for getting too far behind that callbacks get backlogged.
		/// 
		/// CallbackManager uses 64 bit integers to measure nanoseconds of passed   
		/// time. The CallbackManager can theoretically run for 290 years without 
		/// the integers overflowing.
		///
		class EASTDC_API CallbackManager : public ICallbackManager
		{
		public:
			/// CallbackManager
			/// No siginficant processing is done in this constructor. All initialization
			/// code is done in the Init function.
			CallbackManager();
		   ~CallbackManager();

			/// This needs to be called in order to use CallbackManager.
			/// If bAsync is true then the CallbackManager is run in a separate thread
			/// and callbacks are made to user code from that thread automatically.
			/// If bAsync is false then the user must periodically call Update to 
			/// do processing and callbacks are made to user code from the current thread.
			/// You can do manual callbacks even if Async (threaded) mode is enabled.
            bool Init(bool bAsync, 
                      bool bAsyncStart = false
			#if EASTDC_THREADING_SUPPORTED
                      , EA::Thread::ThreadParameters threadParam = EA::Thread::ThreadParameters()
			#endif
	        );

			/// This needs to be called by application on shutdown to deinitialize 
			/// the CallbackManager. It cancels and unregisters any callbacks and returns 
			/// CallbackManager to the newly constructed state (i.e. before Init).
			void Shutdown();

			/// This must be called by the application on every iteration of the main loop if
			/// async (threaded) mode is not used. However, this function can also be called 
			/// if threaded mode is used.
			/// This function calls user-registered timers which have expired.
			void Update(); 

			/// Returns the number of ticks (calls to Update) that have occurred.
			uint64_t GetTickCounter() { return (uint64_t)mTickCounter; }

			/// This must be called by the application on every user event whatever that might be.
			/// An alternative way would be to use the CallbackManager class below.
			void OnUserEvent();

			// Returns the number of times OnUserEvent has been called.
			uint64_t GetUserEventCounter() { return (uint64_t)mUserEventCounter; }

			/// Returns time in nanoseconds as the Callback system sees it
			uint64_t GetTime();

			/// Add a new callback
			bool Add(EA::StdC::Callback* pCallback, bool bOneShot);

			/// Remove a callback. 
			/// Stops the Callback if it is started.
			bool Remove(EA::StdC::Callback* pCallback);

			/// GetStopwatch
			/// This is a debug function. Don't call mutating functions on it (e.g. Stop).
			/// To do: Remove this function once this class is debugged.
			EA::StdC::Stopwatch& GetStopwatch() { return mStopwatch; }

			#if EASTDC_THREADING_SUPPORTED
				// Manipulate the thread used for async (i.e. threaded) execution.
				EA::Thread::Thread& GetThread(); 
				void Lock();
				void Unlock();
			#endif

		protected:
			//struct CallbackInfo
			//{
			//    Callback* mpCallback;           // The user Callback object associated with this info.
			//    int64_t   mNextCallbackEvent;   // Time, tick count, or user event count of next callback we should do.
			//    int64_t   mLastCallbackEvent;   // Time, tick count, or user event count of the previous callback we did.
			//    bool      mbOneShot;            // If true then we Stop the callback after it's called once.
			//
			//    CallbackInfo() : mpCallback(NULL), mNextCallbackEvent(0), mLastCallbackEvent(0), mbOneShot(false){}
			//};

			// We implement a tiny vector class.
			class EASTDC_API CallbackVector
			{
			public:
				typedef Callback*   value_type;
				typedef value_type* iterator;

				CallbackVector();
			   ~CallbackVector();
			  
				void        reserve(size_t)                 { }
				void        clear()                         { mpEnd = mpBegin; }
				bool        empty() const                   { return mpBegin == mpEnd; }
				iterator    begin()                         { return mpBegin; }
				iterator    end()                           { return mpEnd; }
				value_type& back()                          { return *(mpEnd - 1); }
				size_t      size() const                    { return (size_t)(mpEnd - mpBegin); }
				value_type& operator[](size_t i)            { return mpBegin[i]; }
				iterator    erase(iterator it);
				iterator    push_back(value_type value);

			protected:
				value_type* mpBegin;
				value_type* mpEnd;
				value_type* mpCapacity;
				value_type  mLocalBuffer[8]; // Initial local buffer capable of holding some pointers without us needing to hit the heap.
			};

		protected:
			static intptr_t RunStatic(void* pContext) { return static_cast<CallbackManager*>(pContext)->Run(); }
			intptr_t        Run();
			void            UpdateInternal(int64_t& curTick, int64_t& curTime, int64_t& curUserEvent);
			bool            StartThread();
			void            StopThread();

			CallbackManager(const CallbackManager&); // Declared but not defined.
			void operator=(const CallbackManager&);  // Declared but not defined.

		protected:
			CallbackVector           mCallbackArray;                // Our current callbacks.
			EA::StdC::Stopwatch      mStopwatch;                    // 
			EA::Thread::AtomicInt64  mTickCounter;                  // 
			EA::Thread::AtomicInt64  mUserEventCounter;             // 
			bool                     mbInitialized;                 // 
			volatile bool            mbRunning;                     // 
			bool                     mbAsync;                       // This is whether the processor is in a separate thread.
			EA::StdC::RandomFast     mRandom;                       // 
			double                   mNSecPerTick;                  // Used for guessing for how long to sleep our thread until the next callback time arrives. We need to know this because some events are based on the number of ticks (number of times Update is called), while others are based on time.
			int64_t                  mNSecPerTickLastTimeMeasured;  // Last time when average nsec/tick was measured.
			int64_t                  mNSecPerTickLastTickMeasured;  // Last time when average nsec/tick was measured.
			int64_t                  mNextCallbackEventTime;        // The next earliest time callback event. If we have five registered user callbacks, we want to wake up whenever the soonest one is due.
			int64_t                  mNextCallbackEventTick;        // The next earliest tick callback event.

			#if EASTDC_THREADING_SUPPORTED
				EA::Thread::Mutex       mMutex;
				EA::Thread::Thread      mThread;
				EA::Thread::AtomicInt32 mbThreadStarted;
                EA::Thread::ThreadParameters mThreadParam;
			#endif
		};

	} // namespace StdC

} // namespace EA


#ifdef _MSC_VER
	#pragma warning(pop)
#endif


#endif // Header include guard



