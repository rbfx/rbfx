///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements a lightweight mutex with multiple reads but single writer.
// This allows for high performance systems whereby the consumers of data
// are more common than the producers of data.
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_RWMUTEX_H
#define EATHREAD_EATHREAD_RWMUTEX_H

#include <EABase/eabase.h>
#include <eathread/eathread.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



/////////////////////////////////////////////////////////////////////////
/// EARWMutexData
///
/// This is used internally by class RWMutex.
/// Todo: Consider moving this declaration into a platform-specific 
/// header file.
/// 
	#include <eathread/eathread_mutex.h>
	#include <eathread/eathread_condition.h>

	struct EATHREADLIB_API EARWMutexData
	{
		int                   mnReadWaiters;
		int                   mnWriteWaiters;
		int                   mnReaders;
		EA::Thread::ThreadId  mThreadIdWriter;
		EA::Thread::Mutex     mMutex;
		EA::Thread::Condition mReadCondition;
		EA::Thread::Condition mWriteCondition;

		EARWMutexData();

	private:
		// Prevent default generation of these functions by declaring but not defining them.
		EARWMutexData(const EARWMutexData& rhs);               // copy constructor
		EARWMutexData& operator=(const EARWMutexData& rhs);    // assignment operator
	};
/////////////////////////////////////////////////////////////////////////



namespace EA
{
	namespace Thread
	{
		/// RWMutexParameters
		/// Specifies rwlock settings.
		struct EATHREADLIB_API RWMutexParameters
		{
			bool mbIntraProcess; /// True if the mutex is intra-process, else inter-process.
			char mName[16];      /// Mutex name, applicable only to platforms that recognize named synchronization objects.

			RWMutexParameters(bool bIntraProcess = true, const char* pName = NULL);
		};


		/// class RWMutex
		/// Implements a multiple reader / single writer mutex.
		/// This allows for significantly higher performance when data to be protected
		/// is read much more frequently than written. In this case, a waiting writer
		/// gets top priority and all new readers block after a waiter starts waiting.
		class EATHREADLIB_API RWMutex
		{
		public:
			enum Result
			{
				kResultError   = -1,
				kResultTimeout = -2
			};

			enum LockType
			{
				kLockTypeNone  = 0,
				kLockTypeRead  = 1,
				kLockTypeWrite = 2
			};

			/// RWMutex
			/// For immediate default initialization, use no args.
			/// For custom immediate initialization, supply a first argument. 
			/// For deferred initialization, use RWMutex(NULL, false) then later call Init.
			/// For deferred initialization of an array of objects, create an empty
			/// subclass whose default constructor chains back to RWMutex(NULL, false).
			RWMutex(const RWMutexParameters* pRWMutexParameters = NULL, bool bDefaultParameters = true);

			/// ~RWMutex
			/// Destroys an existing mutex. The mutex must not be locked by any thread, 
			/// otherwise the resulting behaviour is undefined.
			~RWMutex();

			/// Init
			/// Initializes the mutex if not done so in the constructor.
			/// This should only be called in the case that this class was constructed 
			/// with RWMutex(NULL, false).
			bool Init(const RWMutexParameters* pRWMutexParameters);

			/// Lock
			/// Returns the new lock count for the given lock type.
			///
			/// Note that the timeout is specified in absolute time and not relative time.
			///
			/// Note also that due to the way thread scheduling works -- particularly in a
			/// time-sliced threading environment -- that the timeout value is a hint and 
			/// the actual amount of time passed before the timeout occurs may be significantly
			/// more or less than the specified timeout time.
			///
			int Lock(LockType lockType, const ThreadTime& timeoutAbsolute = EA::Thread::kTimeoutNone);

			/// Unlock
			/// Unlocks the mutex. The mutex must already be locked by  the 
			/// calling thread. Otherwise the behaviour is not defined.
			/// Return value is the lock count value immediately upon unlock
			/// or is one of enum Result.
			int Unlock();

			/// GetLockCount
			int GetLockCount(LockType lockType);

			/// GetPlatformData
			/// Returns the platform-specific data handle for debugging uses or 
			/// other cases whereby special (and non-portable) uses are required.
			void* GetPlatformData()
				{ return &mRWMutexData; }

		protected:
			EARWMutexData mRWMutexData;

		private:
			// Objects of this class are not copyable.
			RWMutex(const RWMutex&){}
			RWMutex& operator=(const RWMutex&){ return *this; }
		};


		/// RWMutexFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class RWMutex.
		/// A primary use of this would be to allow the RWMutex implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		class EATHREADLIB_API RWMutexFactory
		{
		public:
			static RWMutex*  CreateRWMutex();                       // Internally implemented as: return new RWMutex;
			static void      DestroyRWMutex(RWMutex* pRWMutex);     // Internally implemented as: delete pRWMutex;

			static size_t    GetRWMutexSize();                      // Internally implemented as: return sizeof(RWMutex);
			static RWMutex*  ConstructRWMutex(void* pMemory);       // Internally implemented as: return new(pMemory) RWMutex;
			static void      DestructRWMutex(RWMutex* pRWMutex);    // Internally implemented as: pRWMutex->~RWMutex();
		};


	} // namespace Thread

} // namespace EA




namespace EA
{
	namespace Thread
	{
		/// class AutoRWMutex
		/// An AutoRWMutex locks the RWMutex in its constructor and 
		/// unlocks the AutoRWMutex in its destructor (when it goes out of scope).
		class AutoRWMutex
		{
		public:
			AutoRWMutex(RWMutex& mutex, RWMutex::LockType lockType) 
				: mMutex(mutex)
				{  mMutex.Lock(lockType); }

		  ~AutoRWMutex()
				{  mMutex.Unlock(); }

		protected:
			RWMutex& mMutex;

			// Prevent copying by default, as copying is dangerous.
			AutoRWMutex(const AutoRWMutex&);
			const AutoRWMutex& operator=(const AutoRWMutex&);
		};

	} // namespace Thread

} // namespace EA




#endif // EATHREAD_EATHREAD_RWMUTEX_H













