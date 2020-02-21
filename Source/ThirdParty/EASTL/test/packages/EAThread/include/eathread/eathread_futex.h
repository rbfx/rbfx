///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Implements a fast, user-space mutex. Also known as a lightweight mutex.
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_FUTEX_H
#define EATHREAD_EATHREAD_FUTEX_H

#include <eathread/eathread.h>
#include <eathread/eathread_atomic.h>
#include <eathread/eathread_sync.h>
#include <eathread/eathread_mutex.h>
#include <stddef.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_MANUAL_FUTEX_ENABLED
//
// Defined as 0 or 1.
// If enabled then Futex is implemented with atomics and semaphores instead of
// via a direct system-supported lightweight mutex implementation.
//
#ifndef EATHREAD_MANUAL_FUTEX_ENABLED
	#if defined(EA_PLATFORM_MICROSOFT)              // VC++ has CriticalSection, which is a futex.
		#define EATHREAD_MANUAL_FUTEX_ENABLED 0     // Currently 0 because that's best. Can be set to 1.
	#elif defined(EA_PLATFORM_SONY)
		#define EATHREAD_MANUAL_FUTEX_ENABLED 0    // Allows us to have a spin count.        
	#else
		#define EATHREAD_MANUAL_FUTEX_ENABLED 1     // Set to 1 until we can resolve any dependencies such as PPMalloc.
	#endif
#endif
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// EATHREAD_FUTEX_SPIN_COUNT
//
#ifndef EATHREAD_FUTEX_SPIN_COUNT
	#define EATHREAD_FUTEX_SPIN_COUNT 256 
#endif
///////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////
/// Futex data
///
/// This is used internally by class Futex.
/// Note that we don't use an EAThread semaphore, as the direct semaphore
/// we use here is more direct and avoid inefficiencies that result from 
/// the possibility of EAThread semaphores being optimized for being 
/// standalone.
/// 
#if !EA_THREADS_AVAILABLE
	#define EA_THREAD_NONTHREADED_FUTEX 1

	#if EATHREAD_MANUAL_FUTEX_ENABLED
		struct EAFutexSemaphore
		{
			int mnCount;
		};
	#endif

#elif EA_USE_CPP11_CONCURRENCY
	EA_DISABLE_VC_WARNING(4265 4365 4836 4571 4625 4626 4628 4193 4127 4548)
	#include <mutex>
	EA_RESTORE_VC_WARNING()

#elif defined(__APPLE__)
	#if EATHREAD_MANUAL_FUTEX_ENABLED
		#include <eathread/eathread_semaphore.h>
		typedef EA::Thread::Semaphore EAFutexSemaphore;
	#endif

#elif defined(EA_PLATFORM_SONY)
	#if EATHREAD_MANUAL_FUTEX_ENABLED
		#include <kernel/semaphore.h>
		#include <eathread/internal/timings.h>

		typedef SceKernelSema EAFutexSemaphore;        
	#endif

#elif defined(EA_PLATFORM_UNIX) || EA_POSIX_THREADS_AVAILABLE
	#if EATHREAD_MANUAL_FUTEX_ENABLED
		#include <semaphore.h>
		typedef sem_t EAFutexSemaphore;
	#endif

#elif defined(EA_PLATFORM_MICROSOFT)

	// We avoid #including heavy system headers, as this file is a common header itself.

		extern "C"
		{
			#if defined(EA_COMPILER_GNUC)
				// Mingw declares these slightly differently.
				struct _RTL_CRITICAL_SECTION;   // Urho3D
				__declspec(dllimport) int           __stdcall InitializeCriticalSectionAndSpinCount(_RTL_CRITICAL_SECTION* pCriticalSection, unsigned long dwSpinCount);
				__declspec(dllimport) void          __stdcall InitializeCriticalSection(_RTL_CRITICAL_SECTION* pCriticalSection);
				__declspec(dllimport) void          __stdcall DeleteCriticalSection(_RTL_CRITICAL_SECTION* pCriticalSection);
				__declspec(dllimport) void          __stdcall EnterCriticalSection(_RTL_CRITICAL_SECTION* pCriticalSection);
				__declspec(dllimport) void          __stdcall LeaveCriticalSection(_RTL_CRITICAL_SECTION* pCriticalSection);
				__declspec(dllimport) int           __stdcall TryEnterCriticalSection(_RTL_CRITICAL_SECTION* pCriticalSection);
			#else
				#if !defined _Must_inspect_result_ 
					#define _Must_inspect_result_
				#endif

				struct _RTL_CRITICAL_SECTION;
				__declspec(dllimport) _Must_inspect_result_ int           __stdcall InitializeCriticalSectionAndSpinCount(_Out_ _RTL_CRITICAL_SECTION* pCriticalSection, _In_ unsigned long dwSpinCount);
				__declspec(dllimport) void          __stdcall InitializeCriticalSection(_Out_ _RTL_CRITICAL_SECTION* pCriticalSection);
				__declspec(dllimport) void          __stdcall DeleteCriticalSection(_Inout_ _RTL_CRITICAL_SECTION* pCriticalSection);
				__declspec(dllimport) void          __stdcall EnterCriticalSection(_Inout_ _RTL_CRITICAL_SECTION* pCriticalSection);
				__declspec(dllimport) void          __stdcall LeaveCriticalSection(_Inout_ _RTL_CRITICAL_SECTION* pCriticalSection);
				__declspec(dllimport) int           __stdcall TryEnterCriticalSection(_Inout_ _RTL_CRITICAL_SECTION* pCriticalSection);
			#endif

			__declspec(dllimport) unsigned long __stdcall GetCurrentThreadId();
		}

	#if EATHREAD_MANUAL_FUTEX_ENABLED
		typedef void* EAFutexSemaphore; // void* instead of HANDLE to avoid #including windows headers.
	#endif

#else
	#define EA_THREAD_NONTHREADED_FUTEX 1

	#if EATHREAD_MANUAL_FUTEX_ENABLED
		struct EAFutexSemaphore
		{
			int mnCount;
		};
	#endif
#endif
/////////////////////////////////////////////////////////////////////////




namespace EA
{
	namespace Thread
	{
		#if defined(_WIN64)
			static const int FUTEX_PLATFORM_DATA_SIZE = 40; // CRITICAL_SECTION is 40 bytes on Win64.
		#elif defined(_WIN32)
			static const int FUTEX_PLATFORM_DATA_SIZE = 32; // CRITICAL_SECTION is 24 bytes on Win32 and 28 bytes on XBox 360.
		#endif


		/// class Futex
		///
		/// A Futex is a fast user-space mutex. It works by attempting to use
		/// atomic integer updates for the common case whereby the mutex is
		/// not already locked. If the mutex is already locked then the futex
		/// drops down to waiting on a system-level semaphore. The result is 
		/// that uncontested locking operations can be significantly faster 
		/// than contested locks. Contested locks are slightly slower than in 
		/// the case of a formal mutex, but usually not by much.
		///
		/// The Futex acts the same as a conventional mutex with respect to  
		/// memory synchronization. Specifically: 
		///     - A Lock or successful TryLock implies a read barrier (i.e. acquire).
		///     - A second lock by the same thread implies no barrier.
		///     - A failed TryLock implies no barrier.
		///     - A final unlock by a thread implies a write barrier (i.e. release).
		///     - A non-final unlock by a thread implies no barrier.
		///
		/// Futex limitations relative to Mutexes:
		///     - Futexes cannot be inter-process.
		///     - Futexes cannot be named.
		///     - Futexes cannot participate in condition variables. A special 
		///       condition variable could be made that works with them, though.
		///     - Futex locks don't have timeouts. This could probably be
		///       added with some work, though users generally shouldn't need timeouts. 
		///
		class EATHREADLIB_API Futex
		{
		public:
			enum Result
			{
				kResultTimeout = -2
			};

			/// Futex
			///
			/// Creates a Futex. There are no creation options.
			///
			Futex();

			/// ~Futex
			///
			/// Destroys an existing futex. The futex must not be locked by any thread
			/// upon this call, otherwise the resulting behaviour is undefined.
			///
			~Futex();

			/// TryLock
			///
			/// Tries to lock the futex; returns true if possible.
			/// This function always returns immediately. It will return false if 
			/// the futex is locked by another thread, and it will return true 
			/// if the futex is not locked or is already locked by the current thread.
			///
			bool TryLock();

			/// Lock
			///
			/// Locks the futex; returns the new lock count.
			/// If the futex is locked by another thread, this call will block until
			/// the futex is unlocked. If the futex is not locked or is locked by the
			/// current thread, this call will return immediately.
			///
			void Lock();

			/// Lock
			///
			/// Tries to lock the futex until the given time.
			/// If the futex is locked by another thread, this call will block until
			/// the futex is unlocked or the given time has passed. If the futex is not locked
			/// or is locked by the current thread, this call will return immediately.
			///
			/// Return value:
			///     kResultTimeout Timeout
			///     > 0            The new lock count.
			int Lock(const ThreadTime& timeoutAbsolute);

			/// Unlock
			///
			/// Unlocks the futex. The futex must already be locked at least once by 
			/// the calling thread. Otherwise the behaviour is not defined.
			///
			void Unlock();

			/// GetLockCount
			///
			/// Returns the number of locks on the futex. The return value from this 
			/// function is only reliable if the calling thread already has one lock on 
			/// the futex. Otherwise the returned value may not represent actual value
			/// at any point in time, as other threads lock or unlock the futex soon after the call.
			///
			int GetLockCount() const;

			/// HasLock
			/// Returns true if the current thread has the futex locked. 
			bool HasLock() const;

			/// SetSpinCount
			/// Specifies how many times we spin while waiting to acquire the lock.
			void SetSpinCount(Uint spinCount);

		protected:
			#if EATHREAD_MANUAL_FUTEX_ENABLED
				void CreateFSemaphore();
				void DestroyFSemaphore();
				void SignalFSemaphore();
				void WaitFSemaphore();
				bool WaitFSemaphore(const ThreadTime& timeoutAbsolute);
				void OnLockAcquired(ThreadUniqueId threadUniqueId);
			#endif

		private:
			// Objects of this class are not copyable.
			Futex(const Futex&){}
			Futex& operator=(const Futex&){ return *this; }

		protected:
			#if EATHREAD_MANUAL_FUTEX_ENABLED
				AtomicUWord      mUseCount;         /// Not the same thing as lock count, as waiters increment this value.
				uint16_t         mRecursionCount;   /// The number of times the lock-owning thread has the mutex. This is currently uint16_t for backward compatibility with PPMalloc.
				uint16_t         mSpinCount;        /// The number of times we spin while waiting for the lock.   To do: Change these to be uint32_t once PPMalloc is no longer dependent on this.
				ThreadUniqueId   mThreadUniqueId;   /// Unique id for owning thread; not necessarily same as type ThreadId.
				EAFutexSemaphore mSemaphore;        /// OS-level semaphore that waiters wait on when lock attempts failed.
			#else

				#if EA_USE_CPP11_CONCURRENCY
					std::recursive_timed_mutex mMutex;
					int mnLockCount;
					std::thread::id mLockingThread;
				#elif defined(EA_COMPILER_MSVC) && defined(EA_PLATFORM_MICROSOFT) // In the case of Microsoft platforms, we just use CRITICAL_SECTION, as it is essentially a futex.
					// We use raw structure math because otherwise we'd expose the user to system headers, 
					// which breaks code and bloats builds. We validate our math in eathread_futex.cpp.
					#if defined(_WIN64)
						uint64_t mCRITICAL_SECTION[FUTEX_PLATFORM_DATA_SIZE / sizeof(uint64_t)];
					#else
						uint64_t mCRITICAL_SECTION[FUTEX_PLATFORM_DATA_SIZE / sizeof(uint64_t)];
					#endif
				#elif defined(EA_PLATFORM_SONY)
					EA::Thread::Mutex mMutex;
					Uint mSpinCount;
				#else
					#define EAT_FUTEX_USE_MUTEX 1
					EA::Thread::Mutex mMutex;
				#endif
			#endif
		};



		/// FutexFactory
		/// 
		/// Implements a factory-based creation and destruction mechanism for class Futex.
		/// A primary use of this would be to allow the Futex implementation to reside in
		/// a private library while users of the class interact only with the interface
		/// header and the factory. The factory provides conventional create/destroy 
		/// semantics which use global operator new, but also provides manual construction/
		/// destruction semantics so that the user can provide for memory allocation 
		/// and deallocation.
		///
		class EATHREADLIB_API FutexFactory
		{
		public:
			static Futex*  CreateFutex();                    // Internally implemented as: return new Futex;
			static void    DestroyFutex(Futex* pFutex);      // Internally implemented as: delete pFutex;

			static size_t  GetFutexSize();                   // Internally implemented as: return sizeof(Futex);
			static Futex*  ConstructFutex(void* pMemory);    // Internally implemented as: return new(pMemory) Futex;
			static void    DestructFutex(Futex* pFutex);     // Internally implemented as: pFutex->~Futex();
		};



		/// class AutoFutex
		/// An AutoFutex locks the Futex in its constructor and 
		/// unlocks the Futex in its destructor (when it goes out of scope).
		class EATHREADLIB_API AutoFutex
		{
		public:
			AutoFutex(Futex& futex);
		   ~AutoFutex();

		protected:
			Futex& mFutex;

			// Prevent copying by default, as copying is dangerous.
			AutoFutex(const AutoFutex&);
			const AutoFutex& operator=(const AutoFutex&);
		};


	} // namespace Thread

} // namespace EA






///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// EAFutexReadBarrier
//
// For futexes, which are intended to be used only in user-space and without 
// talking to IO devices, DMA memory, or uncached memory, we directly use
// memory barriers.
	#define EAFutexReadBarrier      EAReadBarrier
	#define EAFutexWriteBarrier     EAWriteBarrier
	#define EAFutexReadWriteBarrier EAReadWriteBarrier
///////////////////////////////////////////////////////////////////////////////



namespace EA
{
	namespace Thread
	{
		#if EATHREAD_MANUAL_FUTEX_ENABLED

			inline Futex::Futex()
			  : mUseCount(0), 
				mRecursionCount(0),
				mSpinCount(EATHREAD_FUTEX_SPIN_COUNT),
				mThreadUniqueId(kThreadUniqueIdInvalid),
				mSemaphore()
			{
				CreateFSemaphore();
			}


			inline Futex::~Futex()
			{
				EAT_ASSERT(mUseCount == 0);

				DestroyFSemaphore();
			}
		
		
			inline void Futex::OnLockAcquired(ThreadUniqueId threadUniqueId)
			{
				EAFutexReadBarrier();
				mThreadUniqueId = threadUniqueId;
				mRecursionCount = 1;
			}


			inline bool Futex::TryLock()
			{
				ThreadUniqueId threadUniqueId;
				EAThreadGetUniqueId(threadUniqueId);

				if(mUseCount.SetValueConditional(1, 0)) // If we could acquire the lock... (set it to 1 if it's 0)
				{
					OnLockAcquired(threadUniqueId);
					return true;
				}

				// This only happens in the case of recursion on the same thread
				// This is threadsafe because the only case where this equality passes
				// is when this value was set on this thread anyway.
				if(EATHREAD_LIKELY(mThreadUniqueId == threadUniqueId)) // If it turns out that we already have the lock...
				{
					++mUseCount;
					++mRecursionCount;
					return true;
				}

				return false;
			}


			inline void Futex::Lock()
			{
				ThreadUniqueId threadUniqueId;
				EAThreadGetUniqueId(threadUniqueId);

				if(mSpinCount) // If we have spinning enabled (usually true)...
				{
					if(mUseCount.SetValueConditional(1, 0)) // If we could acquire the lock... (set it to 1 if it's 0)
					{
						OnLockAcquired(threadUniqueId);
						return;
					}

					if(mThreadUniqueId != threadUniqueId) // Don't spin if we already have the lock.
					{
						for(Uint count = mSpinCount; count > 0; count--) // Implement a spin lock for a number of tries.
						{
							// We use GetValueRaw calls below instead of atomics because we don't want atomic behavior.
							if(mUseCount.GetValueRaw() > 1) // If there are multiple waiters, don't bother spinning any more, as they are already spinning themselves.
								break;

							if(mUseCount.GetValueRaw() == 0) // If it looks like the lock is now free, try to acquire it.
							{
								if(mUseCount.SetValueConditional(1, 0)) // If we could acquire the lock... (set it to 1 if it's 0)
								{
									OnLockAcquired(threadUniqueId);
									return;
								}
							}

							EAProcessorPause();
						}
					}
				}

				if(++mUseCount > 1) // If we could not get the lock (previous value of mUseCount was >= 1 and not 0) or we already had the lock...
				{
					if(mThreadUniqueId == threadUniqueId) // If we already have the lock...
					{
						mRecursionCount++;
						return;
					}
					WaitFSemaphore(); 
				}
				// Else the increment was from 0 to 1, and we own the lock.
				OnLockAcquired(threadUniqueId);
			}




			inline int Futex::Lock(const ThreadTime& timeoutAbsolute)
			{
				if(timeoutAbsolute == kTimeoutNone)
				{
					Lock();
					return (int)mRecursionCount;
				}
				else if(timeoutAbsolute == kTimeoutImmediate)
				{
					if(TryLock())
						return (int)mRecursionCount;
					else
						return kResultTimeout;
				}
				else
				{
					ThreadUniqueId threadUniqueId;
					EAThreadGetUniqueId(threadUniqueId);

					if(++mUseCount > 1) // If we could not get the lock (previous value of mUseCount was >= 1 and not 0) or we already had the lock...
					{
						if(mThreadUniqueId == threadUniqueId) // If we already have the lock...
							return (int)++mRecursionCount;

						if(!WaitFSemaphore(timeoutAbsolute))
						{
							--mUseCount;
							return kResultTimeout;
						}
					}
					// Else the increment was from 0 to 1, and we own the lock.
					OnLockAcquired(threadUniqueId);
					return 1;  // Return mRecursionCount.
				}
			}


			inline void Futex::Unlock()
			{
				#if EAT_ASSERT_ENABLED
					ThreadUniqueId threadUniqueId;
					EAThreadGetUniqueId(threadUniqueId);
					EAT_ASSERT(mThreadUniqueId == threadUniqueId);
					EAT_ASSERT((mRecursionCount > 0) && (mUseCount > 0));
				#endif

				if(EATHREAD_LIKELY(--mRecursionCount == 0))
				{
					mThreadUniqueId = kThreadUniqueIdInvalid;

					// after the decrement below we will no longer own the lock
					EAFutexWriteBarrier();
					if(EATHREAD_UNLIKELY(--mUseCount > 0))
						SignalFSemaphore();
				}
				else
				{
					// this thread still owns the lock, was recursive
					--mUseCount;
				}
			}


			inline int Futex::GetLockCount() const
			{
				// No atomic operation or memory barrier required, as this function only
				// has validity if it is being called from the lock-owning thread. However,
				// we don't at this time choose to assert that mThreadUniqueId == GetThreadId().
				return (int)mRecursionCount;
			}


			inline bool Futex::HasLock() const
			{
				ThreadUniqueId threadUniqueId;
				EAThreadGetUniqueId(threadUniqueId);

				return (mThreadUniqueId == threadUniqueId);
			}


			inline void Futex::SetSpinCount(Uint spinCount)
			{
				mSpinCount = spinCount;
			}

		#else // #if EATHREAD_MANUAL_FUTEX_ENABLED

			#if EA_USE_CPP11_CONCURRENCY

				inline Futex::Futex() : mnLockCount(0) {}

				inline Futex::~Futex() { EAT_ASSERT(!GetLockCount()); }

				inline bool Futex::TryLock() 
				{ 
					if (mMutex.try_lock())
					{
						EAT_ASSERT(mnLockCount >= 0);
						EAT_ASSERT(mnLockCount == 0 || mLockingThread == std::this_thread::get_id());
						++mnLockCount;
						mLockingThread = std::this_thread::get_id();
						return true;
					}

					return false;
				}

				inline void Futex::Lock() { mMutex.lock(); mLockingThread = std::this_thread::get_id(); ++mnLockCount; }

				inline int Futex::Lock(const ThreadTime& timeoutAbsolute)
				{
					if (timeoutAbsolute == kTimeoutNone)
					{
						if (!mMutex.try_lock())
						{
							return kResultTimeout;
						}
					}
					else
					{
						std::chrono::milliseconds timeoutAbsoluteMs(timeoutAbsolute);
						std::chrono::time_point<std::chrono::system_clock> timeout_time(timeoutAbsoluteMs);
						if (!mMutex.try_lock_until(timeout_time))
						{
							return kResultTimeout;
						}
					}

					EAT_ASSERT(mnLockCount >= 0);
					EAT_ASSERT(mnLockCount == 0 || mLockingThread == std::this_thread::get_id());
					mLockingThread = std::this_thread::get_id();
					return ++mnLockCount; // This is safe to do because we have the lock.
				}

				inline void Futex::Unlock()
				{
					EAT_ASSERT(HasLock());
					--mnLockCount;
					if (mnLockCount == 0)
						mLockingThread = std::thread::id();
					mMutex.unlock();
				}

				inline int Futex::GetLockCount() const { return mnLockCount; }

				inline bool Futex::HasLock() const 
				{ 
					if ((mnLockCount > 0) && (std::this_thread::get_id() == mLockingThread))
						return true;
					return false;
				}  

				inline void Futex::SetSpinCount(Uint)
				{
					// Not supported
				}

			#elif defined(EA_COMPILER_MSVC) && defined(EA_PLATFORM_MICROSOFT) // Win32, Win64, etc.

				inline Futex::Futex()
				{
					// We use InitializeCriticalSectionAndSpinCount, as that has resulted in improved performance in practice on multiprocessors systems.
					int rv = InitializeCriticalSectionAndSpinCount((_RTL_CRITICAL_SECTION*)mCRITICAL_SECTION, EATHREAD_FUTEX_SPIN_COUNT);
					EAT_ASSERT(rv != 0);
					EA_UNUSED(rv);
				}

				inline Futex::~Futex()
				{
					EAT_ASSERT(!GetLockCount());
					DeleteCriticalSection((_RTL_CRITICAL_SECTION*)mCRITICAL_SECTION);
				}

				inline bool Futex::TryLock()
				{
					return TryEnterCriticalSection((_RTL_CRITICAL_SECTION*)mCRITICAL_SECTION) != 0;
				}

				inline void Futex::Lock()
				{
					EnterCriticalSection((_RTL_CRITICAL_SECTION*)mCRITICAL_SECTION);
				}

				inline int Futex::Lock(const ThreadTime& timeoutAbsolute)
				{
					if(timeoutAbsolute == kTimeoutNone)
					{
						Lock();
						return GetLockCount();
					}
					else if(timeoutAbsolute == kTimeoutImmediate)
					{
						if(TryLock())
							return GetLockCount();
						else
							return kResultTimeout;
					}
					else
					{
						while(!TryLock())
						{
							if(GetThreadTime() >= timeoutAbsolute)
								return kResultTimeout;
							ThreadSleep(1);
						}
						return GetLockCount();
					}
				}

				inline void Futex::Unlock()
				{
					EAT_ASSERT(HasLock());
					LeaveCriticalSection((_RTL_CRITICAL_SECTION*)mCRITICAL_SECTION);
				} 

				inline int Futex::GetLockCount() const
				{
					// Return the RecursionCount member of RTL_CRITICAL_SECTION.

					// We use raw structure math because otherwise we'd expose the user to system headers, 
					// which breaks code and bloats builds. We validate our math in eathread_futex.cpp.
					#if defined(_WIN64)
						return *((int*)mCRITICAL_SECTION + 3); 
					#else
						return *((int*)mCRITICAL_SECTION + 2);
					#endif
				}

				inline bool Futex::HasLock() const
				{
					// Check the OwningThread member of RTL_CRITICAL_SECTION.

					// We use raw structure math because otherwise we'd expose the user to system headers, 
					// which breaks code and bloats builds. We validate our math in eathread_futex.cpp.
					#if defined(_WIN64)
						return (*((uint32_t*)mCRITICAL_SECTION + 4) == (uintptr_t)GetCurrentThreadId());
					#else
						return (*((uint32_t*)mCRITICAL_SECTION + 3) == (uintptr_t)GetCurrentThreadId());
					#endif
				}

				inline void Futex::SetSpinCount(Uint)
				{
					// Not supported
				}

			#elif defined(EAT_FUTEX_USE_MUTEX)

				inline Futex::Futex()
				  { }

				inline Futex::~Futex()
				  { }

				inline bool Futex::TryLock()
				  { return mMutex.Lock(EA::Thread::kTimeoutImmediate) > 0; }

				inline void Futex::Lock()
				  { mMutex.Lock(); }

				inline int Futex::Lock(const ThreadTime& timeoutAbsolute)
				  { return mMutex.Lock(timeoutAbsolute); }

				inline void Futex::Unlock()
				  { mMutex.Unlock(); }

				inline int Futex::GetLockCount() const
				  { return mMutex.GetLockCount(); }

				inline bool Futex::HasLock() const
				  { return mMutex.HasLock(); }

				inline void Futex::SetSpinCount(Uint)
				  { }

			#endif // _MSC_VER

		#endif // EATHREAD_MANUAL_FUTEX_ENABLED



		inline AutoFutex::AutoFutex(Futex& futex) 
		  : mFutex(futex)
		{
			mFutex.Lock();
		}

		inline AutoFutex::~AutoFutex()
		{
			mFutex.Unlock();
		}

	} // namespace Thread

} // namespace EA



#endif // EATHREAD_EATHREAD_FUTEX_H













