///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements an interprocess mutex with multiple reads but single writer.
// This allows for high performance systems whereby the consumers of mpData
// are more common than the producers of mpData.
/////////////////////////////////////////////////////////////////////////////



#ifndef EATHREAD_EATHREAD_RWMUTEX_IP_H
#define EATHREAD_EATHREAD_RWMUTEX_IP_H


#include <EABase/eabase.h>
#include <eathread/eathread.h>
#include <new>
#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
	#pragma warning(push, 0)
	#include <Windows.h>
	#pragma warning(pop)
#endif

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif


#ifdef _MSC_VER
	#pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
	#pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
	#pragma warning(disable: 6054)  // String 'argument 2' might not be zero-terminated
#endif


namespace EA
{
	namespace Thread
	{
		#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)

			template<typename T>
			class Shared
			{
			public:
				Shared();
				Shared(const char* pName);
			   ~Shared();

				bool Init(const char* pName);
				void Shutdown();
				bool IsNew() const { return mbCreated; }
				T*   operator->()  { return static_cast<T*>(mpData); }

			protected:
				uint32_t& GetRefCount();

				Shared(const Shared&);
				Shared& operator=(const Shared&);

			protected:
				HANDLE mMapping;
				void*  mpData;
				bool   mbCreated;
				char   mName[32];
				T*     mpT;         // For debug purposes only.
			};


			template <typename T>
			inline Shared<T>::Shared()
			  : mMapping(NULL)
			  , mpData(NULL)
			  , mbCreated(false)
			  , mpT(NULL)
			{
			}


			template <typename T>
			inline Shared<T>::Shared(const char* pName)
			  : mMapping(NULL)
			  , mpData(NULL)
			  , mbCreated(false)
			  , mpT(NULL)
			{
				Init(pName);
			}


			template <typename T>
			inline Shared<T>::~Shared()
			{
				Shutdown();
			}


			template <typename T>
			inline bool Shared<T>::Init(const char* pName)
			{
				bool bReturnValue = false;

				if(pName)
					strncpy(mName, pName, sizeof(mName));
				else
					mName[0] = 0;
				mName[sizeof(mName) - 1] = 0;
		 
				char mutexName[sizeof(mName) + 16];
				strcpy(mutexName, mName);
				strcat(mutexName, ".SharedMutex");
				HANDLE hMutex = CreateMutexA(NULL, FALSE, mutexName);
				EAT_ASSERT(hMutex != NULL);
				if(hMutex != NULL)
				{
					WaitForSingleObject(hMutex, INFINITE); // This lock should always be fast, as it belongs to us and we only hold onto it very temporarily.

					const size_t kDataSize = sizeof(T) + 8; // Add bytes so that we can store a ref-count of our own after the mpData. 
					mMapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, kDataSize, mName);

					if(mMapping)
					{
						mbCreated = (GetLastError() != ERROR_ALREADY_EXISTS);
						mpData    = MapViewOfFile(mMapping, FILE_MAP_ALL_ACCESS, 0, 0, kDataSize);

						uint32_t& refCount = GetRefCount(); // The ref count is stored at the end of the mapped data.

						if(mbCreated)           // If we were the first one to create this, then construct it.
						{
							new(mpData) T;
							refCount = 1;
						}
						else
							refCount++;

						mpT = static_cast<T*>(mpData); // For debug purposes only.

						bReturnValue = true;
					}

					ReleaseMutex(hMutex);
					CloseHandle(hMutex);
				}

				return bReturnValue;
			}


			template <typename T>
			inline void Shared<T>::Shutdown()
			{
				char mutexName[sizeof(mName) + 16];
				strcpy(mutexName, mName);
				strcat(mutexName, ".SharedMutex");
				HANDLE hMutex = CreateMutexA(NULL, FALSE, mutexName);
				EAT_ASSERT(hMutex != NULL);
				if(hMutex != NULL)
				{
					WaitForSingleObject(hMutex, INFINITE); // This lock should always be fast, as it belongs to us and we only hold onto it very temporarily.

					if(mMapping)
					{
						if(mpData)
						{
							uint32_t& refCount = GetRefCount(); // The ref count is stored at the end of the mapped data.

							if(refCount == 1)                   // If we are the last to use it, 
								static_cast<T*>(mpData)->~T();
							else
								refCount--;

							UnmapViewOfFile(mpData);
							mpData = NULL;
						}

						CloseHandle(mMapping);
						mMapping = 0;
					}

					ReleaseMutex(hMutex);
					CloseHandle(hMutex);
				} 
			}

			template <typename T>
			inline uint32_t& Shared<T>::GetRefCount()
			{
				// There will be space after T because we allocated it in Init.
				uint32_t* pData32 = (uint32_t*)(((uintptr_t)mpData + sizeof(T) + 3) & ~3); // Round up to next 32 bit boundary.
				return *pData32;
			}

		#else

			template<typename T>
			class Shared
			{
			public:
				Shared()               { }
				Shared(const char*)    { }

				bool Init(const char*) { return true; }
				void Shutdown()        { }
				bool IsNew() const     { return true; }
				T*   operator->()      { return &mT; }

				T mT;
			};

		#endif // #if defined(EA_PLATFORM_WINDOWS)

	} // namespace Thread

} // namespace EA



namespace EA
{
	namespace Thread
	{
		/////////////////////////////////////////////////////////////////////////
		/// EARWMutexIPData
		///
		#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)

			struct EATHREADLIB_API SharedData
			{
				int   mnReadWaiters;
				int   mnWriteWaiters;
				int   mnReaders;
				DWORD mThreadIdWriter;      // Need to use a thread id instead of a thread handle.

				SharedData() : mnReadWaiters(0), mnWriteWaiters(0), mnReaders(0), mThreadIdWriter(EA::Thread::kSysThreadIdInvalid) { }
			};

			struct EATHREADLIB_API EARWMutexIPData
			{
				Shared<SharedData> mSharedData;
				HANDLE             mMutex;
				HANDLE             mReadSemaphore;
				HANDLE             mWriteSemaphore;

				EARWMutexIPData();
			   ~EARWMutexIPData();

				bool Init(const char* pName);
				void Shutdown();

			private:
				EARWMutexIPData(const EARWMutexIPData&);
				EARWMutexIPData& operator=(const EARWMutexIPData&);
			};

		#else

			struct EATHREADLIB_API EARWMutexIPData
			{
				EARWMutexIPData(){}

			private:
				EARWMutexIPData(const EARWMutexIPData&);
				EARWMutexIPData& operator=(const EARWMutexIPData&);
			};

		#endif


		/// RWMutexParameters
		struct EATHREADLIB_API RWMutexIPParameters
		{
			bool mbIntraProcess; /// True if the mutex is intra-process, else inter-process.
			char mName[16];      /// Mutex name, applicable only to platforms that recognize named synchronization objects.

			RWMutexIPParameters(bool bIntraProcess = true, const char* pName = NULL);
		};


		/// class RWMutexIP
		/// Implements an interprocess multiple reader / single writer mutex.
		/// This allows for significantly higher performance when mpData to be protected
		/// is read much more frequently than written. In this case, a waiting writer
		/// gets top priority and all new readers block after a waiter starts waiting.
		class EATHREADLIB_API RWMutexIP
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

			/// RWMutexIP
			/// For immediate default initialization, use no args.
			/// For custom immediate initialization, supply a first argument. 
			/// For deferred initialization, use RWMutexIP(NULL, false) then later call Init.
			/// For deferred initialization of an array of objects, create an empty
			/// subclass whose default constructor chains back to RWMutexIP(NULL, false).
			RWMutexIP(const RWMutexIPParameters* pRWMutexIPParameters = NULL, bool bDefaultParameters = true);

			/// ~RWMutexIP
			/// Destroys an existing mutex. The mutex must not be locked by any thread, 
			/// otherwise the resulting behaviour is undefined.
			~RWMutexIP();

			/// Init
			/// Initializes the mutex if not done so in the constructor.
			/// This should only be called in the case that this class was constructed 
			/// with RWMutexIP(NULL, false).
			bool Init(const RWMutexIPParameters* pRWMutexIPParameters);

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
			/// Returns the platform-specific mpData handle for debugging uses or 
			/// other cases whereby special (and non-portable) uses are required.
			void* GetPlatformData()
				{ return &mRWMutexIPData; }

		protected:
			EARWMutexIPData mRWMutexIPData;

		private:
			// Objects of this class are not copyable.
			RWMutexIP(const RWMutexIP&){}
			RWMutexIP& operator=(const RWMutexIP&){ return *this; }
		};


		/// RWMutexIPFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class RWMutexIP.
		/// A primary use of this would be to allow the RWMutexIP implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		class EATHREADLIB_API RWMutexIPFactory
		{
		public:
			static RWMutexIP*  CreateRWMutexIP();                         // Internally implemented as: return new RWMutexIP;
			static void        DestroyRWMutexIP(RWMutexIP* pRWMutex);     // Internally implemented as: delete pRWMutex;

			static size_t      GetRWMutexIPSize();                        // Internally implemented as: return sizeof(RWMutexIP);
			static RWMutexIP*  ConstructRWMutexIP(void* pMemory);         // Internally implemented as: return new(pMemory) RWMutexIP;
			static void        DestructRWMutexIP(RWMutexIP* pRWMutex);    // Internally implemented as: pRWMutex->~RWMutexIP();
		};


	} // namespace Thread

} // namespace EA




namespace EA
{
	namespace Thread
	{
		/// class AutoRWMutexIP
		/// An AutoRWMutex locks the RWMutexIP in its constructor and 
		/// unlocks the AutoRWMutex in its destructor (when it goes out of scope).
		class AutoRWMutexIP
		{
		public:
			AutoRWMutexIP(RWMutexIP& mutex, RWMutexIP::LockType lockType) 
				: mMutex(mutex)
				{  mMutex.Lock(lockType); }

		  ~AutoRWMutexIP()
				{  mMutex.Unlock(); }

		protected:
			RWMutexIP& mMutex;

			// Prevent copying by default, as copying is dangerous.
			AutoRWMutexIP(const AutoRWMutexIP&);
			const AutoRWMutexIP& operator=(const AutoRWMutexIP&);
		};

	} // namespace Thread

} // namespace EA



#ifdef _MSC_VER
	#pragma warning(pop)
#endif


#endif // EATHREAD_EATHREAD_RWMUTEX_IP_H







