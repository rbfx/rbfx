///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------
// For conditions of distribution and use, see
// https://github.com/preshing/cpp11-on-multicore/blob/master/LICENSE
//---------------------------------------------------------

#ifndef EATHREAD_EATHREAD_RWSEMALOCK_H
#define EATHREAD_EATHREAD_RWSEMALOCK_H

#include "eathread_atomic.h"
#include "eathread_semaphore.h"

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif

namespace EA
{
	namespace Thread
	{
		//---------------------------------------------------------
		// RWSemaLock
		//---------------------------------------------------------
		class RWSemaLock
		{
		public:
			RWSemaLock() : mStatus(0) {}
			RWSemaLock(const RWSemaLock&) = delete;
			RWSemaLock(RWSemaLock&&) = delete;
			RWSemaLock& operator=(const RWSemaLock&) = delete;
			RWSemaLock& operator=(RWSemaLock&&) = delete;

			void ReadLock()
			{
				Status oldStatus, newStatus;
				do
				{
					oldStatus.data = mStatus.GetValue();
					newStatus.data = oldStatus.data;

					if (oldStatus.writers > 0)
					{
						newStatus.waitToRead++;
					}
					else
					{
						newStatus.readers++;
					}
					// CAS until successful. On failure, oldStatus will be updated with the latest value.
				}
				while (!mStatus.SetValueConditional(newStatus.data, oldStatus.data));

				if (oldStatus.writers > 0)
				{
					mReadSema.Wait();
				}
			}

			bool ReadTryLock()
			{
				Status oldStatus, newStatus;
				do
				{
					oldStatus.data = mStatus.GetValue();
					newStatus.data = oldStatus.data;

					if (oldStatus.writers > 0)
					{
						return false;
					}
					else
					{
						newStatus.readers++;
					}
					// CAS until successful. On failure, oldStatus will be updated with the latest value.
				}
				while (!mStatus.SetValueConditional(newStatus.data, oldStatus.data));

				return true;
			}

			void ReadUnlock()
			{
				Status oldStatus;
				oldStatus.data = mStatus.Add(-Status::kIncrementRead) + Status::kIncrementRead;

				EAT_ASSERT(oldStatus.readers > 0);
				if (oldStatus.readers == 1 && oldStatus.writers > 0)
				{
					mWriteSema.Post();
				}
			}

			void WriteLock()
			{
				Status oldStatus;
				oldStatus.data = mStatus.Add(Status::kIncrementWrite) - Status::kIncrementWrite;
				EAT_ASSERT(oldStatus.writers + 1 <= Status::kMaximum);
				if (oldStatus.readers > 0 || oldStatus.writers > 0)
				{
					mWriteSema.Wait();
				}
			}

			bool WriteTryLock()
			{
				Status oldStatus, newStatus;
				do
				{
					oldStatus.data = mStatus.GetValue();
					newStatus.data = oldStatus.data;

					if (oldStatus.writers > 0 || oldStatus.readers > 0)
					{
						return false;
					}
					else
					{
						newStatus.writers++;
					}
					// CAS until successful. On failure, oldStatus will be updated with the latest value.
				}
				while (!mStatus.SetValueConditional(newStatus.data, oldStatus.data));

				return true;
			}

			void WriteUnlock()
			{
				uint32_t waitToRead = 0;
				Status oldStatus, newStatus;
				do
				{
					oldStatus.data = mStatus.GetValue();
					EAT_ASSERT(oldStatus.readers == 0);
					newStatus.data = oldStatus.data;
					newStatus.writers--;
					waitToRead = oldStatus.waitToRead;
					if (waitToRead > 0)
					{
						newStatus.waitToRead = 0;
						newStatus.readers = waitToRead;
					}
					// CAS until successful. On failure, oldStatus will be updated with the latest value.
				}
				while (!mStatus.SetValueConditional(newStatus.data, oldStatus.data));

				if (waitToRead > 0)
				{
					mReadSema.Post(waitToRead);
				}
				else if (oldStatus.writers > 1)
				{
					mWriteSema.Post();
				}
			}

			// NOTE(rparolin): 
			// Since the RWSemaLock uses atomics to update its status flags before blocking on a semaphore, you cannot
			// rely on the answer the IsReadLocked/IsWriteLocked gives you.  It's at a best a guess and you can't rely
			// on it for any kind of validation checks which limits its usefulness.  In addition, the original
			// implementation from Preshing does not include such functionality. 
			//
			// bool IsReadLocked() {...}
			// bool IsWriteLocked() {...}

		protected:
			EA_DISABLE_VC_WARNING(4201) // warning C4201: nonstandard extension used: nameless struct/union
			union Status
			{
				enum
				{
					kIncrementRead			= 1,
					kIncrementWaitToRead	= 1 << 10,
					kIncrementWrite			= 1 << 20,
					kMaximum				= (1 << 10) - 1,
				};

				struct 
				{
					int readers		: 10; // 10-bits = 1024
					int waitToRead	: 10;
					int writers		: 10;
					int pad			: 2;
				};

				int data;
			};
			EA_RESTORE_VC_WARNING()

			AtomicInt32 mStatus;
			Semaphore mReadSema;  // semaphores are non-copyable
			Semaphore mWriteSema; // semaphores are non-copyable
		};


		//---------------------------------------------------------
		// ReadLockGuard
		//---------------------------------------------------------
		class AutoSemaReadLock
		{
		private:
			RWSemaLock& m_lock;

		public:
			AutoSemaReadLock(const AutoSemaReadLock&) = delete;
			AutoSemaReadLock(AutoSemaReadLock&&) = delete;
			AutoSemaReadLock& operator=(const AutoSemaReadLock&) = delete;
			AutoSemaReadLock& operator=(AutoSemaReadLock&&) = delete;

			AutoSemaReadLock(RWSemaLock& lock) : m_lock(lock)
			{
				m_lock.ReadLock();
			}

			~AutoSemaReadLock()
			{
				m_lock.ReadUnlock();
			}
		};


		//---------------------------------------------------------
		// WriteLockGuard
		//---------------------------------------------------------
		class AutoSemaWriteLock
		{
		private:
			RWSemaLock& m_lock;

		public:
			AutoSemaWriteLock(const AutoSemaWriteLock&) = delete;
			AutoSemaWriteLock(AutoSemaWriteLock&&) = delete;
			AutoSemaWriteLock& operator=(const AutoSemaWriteLock&) = delete;
			AutoSemaWriteLock& operator=(AutoSemaWriteLock&&) = delete;

			AutoSemaWriteLock(RWSemaLock& lock) : m_lock(lock)
			{
				m_lock.WriteLock();
			}

			~AutoSemaWriteLock()
			{
				m_lock.WriteUnlock();
			}
		};
	}
}

#endif // EATHREAD_EATHREAD_RWSEMALOCK_H
