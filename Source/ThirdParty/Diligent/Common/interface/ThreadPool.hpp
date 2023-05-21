/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

#include <atomic>
#include <functional>
#include <thread>

#include "../../Primitives/interface/Object.h"
#include "../../Platforms/Basic/interface/DebugUtilities.hpp"

#include "ObjectBase.hpp"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

/// Asynchronous task status
enum ASYNC_TASK_STATUS
{
    /// The asynchronous task status is unknown.
    ASYNC_TASK_STATUS_UNKNOWN = 0,

    /// The asynchronous task has not been started yet.
    ASYNC_TASK_STATUS_NOT_STARTED,

    /// The asynchronous task is running.
    ASYNC_TASK_STATUS_RUNNING,

    /// The asynchronous task was cancelled.
    ASYNC_TASK_STATUS_CANCELLED,

    /// The asynchronous task is complete.
    ASYNC_TASK_STATUS_COMPLETE
};


// {B06D1DDA-AEA0-4CFD-969A-C8E2011DC294}
static const INTERFACE_ID IID_AsyncTask =
    {0xb06d1dda, 0xaea0, 0x4cfd, {0x96, 0x9a, 0xc8, 0xe2, 0x1, 0x1d, 0xc2, 0x94}};

/// Asynchronous task interface
class IAsyncTask : public IObject
{
public:
    /// Run the asynchronous task.

    /// \param [in] ThreadId - Id of the thread that is running this task.
    ///
    /// \remarks    This method is only called once by the thread pool.
    ///             Before starting the task, the thread pool sets its
    ///             status to ASYNC_TASK_STATUS_RUNNING.
    ///
    ///             Before returning from the function, the task implementation must
    ///             set the task status to either ASYNC_TASK_STATUS_CANCELLED or
    ///             ASYNC_TASK_STATUS_COMPLETE.
    virtual void Run(Uint32 ThreadId) = 0;

    /// Cancel the task, if possible.
    ///
    /// \remarks    If the task is running, the task implementation should
    ///             abort the task execution, if possible.
    virtual void Cancel() = 0;

    /// Sets the task status, see Diligent::ASYNC_TASK_STATUS.
    virtual void SetStatus(ASYNC_TASK_STATUS Status) = 0;

    /// Gets the task status, see Diligent::ASYNC_TASK_STATUS.
    virtual ASYNC_TASK_STATUS GetStatus() const = 0;

    /// Sets the task priorirty.
    virtual void SetPriority(float fPriority) = 0;

    /// Returns the task priorirty.
    virtual float GetPriority() const = 0;

    /// Checks if the task is finished (i.e. cancelled or complete).
    virtual bool IsFinished() const = 0;

    /// Waits until the task is complete.
    ///
    /// \note   This method must not be called from the same thread that is
    ///         running the task or a deadlock will occur.
    virtual void WaitForCompletion() const = 0;

    /// Waits until the tasks is running.
    ///
    /// \warning  An application is responsible to make sure that
    ///           tasks currently in the queue will eventually finish
    ///           allowing the task to start.
    ///
    ///           This method must not be called from the worker thread.
    virtual void WaitUntilRunning() const = 0;
};


// {8BB92B5E-3EAB-4CC3-9DA2-5470DBBA7120}
static const INTERFACE_ID IID_ThreadPool =
    {0x8bb92b5e, 0x3eab, 0x4cc3, {0x9d, 0xa2, 0x54, 0x70, 0xdb, 0xba, 0x71, 0x20}};

/// Thread pool interface
class IThreadPool : public IObject
{
public:
    /// Enqueues asynchronous task for execution.

    /// \param[in] pTask - Task to run.
    ///
    /// \remarks   Thread pool will keep a strong reference to the task,
    ///            so an application is free to release it after enqueuing.
    virtual void EnqueueTask(IAsyncTask* pTask) = 0;


    /// Reprioritizes the task in the queue.

    /// \param[in] pTask - Task to reprioritize.
    ///
    /// \return     true if the task was found in the queue and was
    ///             successfully reprioritized, and false otherwise.
    ///
    /// \remarks    When the tasks is enqueued, its priority is used to
    ///             place it in the priority queue. When an application changes
    ///             the task priority, it should call this method to update the task
    ///             position in the queue.
    virtual bool ReprioritizeTask(IAsyncTask* pTask) = 0;


    /// Reprioritizes all tasks in the queue.

    /// \remarks    This method should be called if task priorities have changed
    ///             to update the positions of all tasks in the queue.
    virtual void ReprioritizeAllTasks() = 0;


    /// Removes the task from the queue, if possible.

    /// \param[in] pTask - Task to remove from the queue.
    ///
    /// \return    true if the task was successfully removed from the queue,
    ///            and false otherwise.
    virtual bool RemoveTask(IAsyncTask* pTask) = 0;


    /// Waits until all tasks in the queue are finished.

    /// \remarks    The method blocks the calling thread until all
    ///             tasks in the quque are finished and the queue is empty.
    ///             An application is responsible to make sure that all tasks
    ///             will finish eventually.
    virtual void WaitForAllTasks() = 0;


    /// Returns the current queue size.
    virtual Uint32 GetQueueSize() = 0;

    /// Returns the number of currently running tasks
    virtual Uint32 GetRunningTaskCount() const = 0;


    /// Stops all worker threads.

    /// \note   This method makes all worker threads to exit.
    ///         If an application enqueues tasks after calling this methods,
    ///         this tasks will never run.
    virtual void StopThreads() = 0;


    /// Manually processes the next task from the queue.

    /// \param[in] ThreadId    - Id of the thread that is running this task.
    /// \param[in] WaitForTask - whether the function should wait for the next task:
    ///                          - if true, the function will block the thread until the next task
    ///                            is retrieved from the queue and processed.
    ///                          - if false, the function will return immediately if there are no
    ///                            tasks in the queue.
    ///
    /// \return     Whether there are more tasks to process. The calling thread must keep
    ///             calling the function until it returns false.
    ///
    /// \remarks    This method allows an application to implement its own threading strategy.
    ///             A thread pool may be created with zero threads, and the application may call
    ///             ProcessTask() method from its own threads.
    ///
    ///             An application must keep calling the method until it returns false.
    ///             If there are unhandled tasks in the queue and the application stops processing
    ///             them, the thread pool will hang up.
    ///
    ///             An example of handling the tasks is shown below:
    ///
    ///                 // Initialization
    ///                 auto pThreadPool = CreateThreadPool(ThreadPoolCreateInfo{0});
    ///
    ///                 std::vector<std::thread> WorkerThreads(4);
    ///                 for (Uint32 i = 0; i < WorkerThreads.size(); ++i)
    ///                 {
    ///                     WorkerThreads[i] = std::thread{
    ///                         [&ThreadPool = *pThreadPool, i] //
    ///                         {
    ///                             while (ThreadPool.ProcessTask(i, true))
    ///                             {
    ///                             }
    ///                         }};
    ///                 }
    ///
    ///                 // Enqueue async tasks
    ///
    ///                 pThreadPool->WaitForAllTasks();
    ///
    ///                 // Stop all threads in the pool
    ///                 pThreadPool->StopThreads();
    ///
    ///                 // Cleanup (must be done after all threads are stopped)
    ///                 for (auto& Thread : WorkerThreads)
    ///                 {
    ///                     Thread.join();
    ///                 }
    ///
    virtual bool ProcessTask(Uint32 ThreadId, bool WaitForTask) = 0;
};


/// Thread pool create information
struct ThreadPoolCreateInfo
{
    /// The number of worker threads to start.

    /// \remarks    An application may create a thread pool with
    ///             zero threads, in which case it will be responsible
    ///             for manually calling the IThreadPool::ProcessTask() method.
    size_t NumThreads = 0;

    /// An optional function that will be called by the thread pool from
    /// the worker thread after the thread has just started, but before
    /// the first task is processed.
    std::function<void(Uint32)> OnThreadStarted = nullptr;

    /// An optional function that will be called by the thread pool from
    /// the worker thread before the worker thread exits.
    std::function<void(Uint32)> OnThreadExiting = nullptr;
};

RefCntAutoPtr<IThreadPool> CreateThreadPool(const ThreadPoolCreateInfo& ThreadPoolCI);


/// Base implementation of the IAsyncTask interface.
class AsyncTaskBase : public ObjectBase<IAsyncTask>
{
public:
    using TBase = ObjectBase<IAsyncTask>;
    explicit AsyncTaskBase(IReferenceCounters* pRefCounters,
                           float               fPriority = 0) noexcept :
        TBase{pRefCounters},
        m_fPriority{fPriority}
    {
    }
    virtual ~AsyncTaskBase() = 0;

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_AsyncTask, TBase)

    virtual void Cancel() override
    {
        m_bSafelyCancel.store(true);
    }

    virtual void SetStatus(ASYNC_TASK_STATUS Status) override final
    {
#ifdef DILIGENT_DEVELOPMENT
        if (Status != m_TaskStatus)
        {
            switch (Status)
            {
                case ASYNC_TASK_STATUS_UNKNOWN:
                    DEV_ERROR("UNKNOWN is invalid status.");
                    break;

                case ASYNC_TASK_STATUS_NOT_STARTED:
                    DEV_ERROR("NOT_STARTED is only allowed as initial task status.");
                    break;

                case ASYNC_TASK_STATUS_RUNNING:
                    DEV_CHECK_ERR(m_TaskStatus == ASYNC_TASK_STATUS_NOT_STARTED,
                                  "A task should be moved to RUNNING state from NOT_STARTED state.");
                    break;

                case ASYNC_TASK_STATUS_CANCELLED:
                    DEV_CHECK_ERR((m_TaskStatus == ASYNC_TASK_STATUS_NOT_STARTED ||
                                   m_TaskStatus == ASYNC_TASK_STATUS_RUNNING),
                                  "A task should be moved to CANCELLED state from either NOT_STARTED or RUNNING states.");
                    break;

                case ASYNC_TASK_STATUS_COMPLETE:
                    DEV_CHECK_ERR(m_TaskStatus == ASYNC_TASK_STATUS_RUNNING,
                                  "A task should be moved to COMPLETE state from RUNNING state.");
                    break;

                default:
                    UNEXPECTED("Unexpected task status");
            }
        }
#endif
        m_TaskStatus.store(Status);
    }

    ASYNC_TASK_STATUS GetStatus() const override final
    {
        return m_TaskStatus.load();
    }

    virtual void SetPriority(float fPriority) override final
    {
        m_fPriority.store(fPriority);
    }

    virtual float GetPriority() const override final
    {
        return m_fPriority.load();
    }

    virtual bool IsFinished() const override final
    {
        static_assert(ASYNC_TASK_STATUS_COMPLETE > ASYNC_TASK_STATUS_CANCELLED && ASYNC_TASK_STATUS_CANCELLED > ASYNC_TASK_STATUS_RUNNING,
                      "Unexpected enum values");
        return m_TaskStatus.load() >= ASYNC_TASK_STATUS_CANCELLED;
    }

    virtual void WaitForCompletion() const override final
    {
        while (!IsFinished())
            std::this_thread::yield();
    }

    virtual void WaitUntilRunning() const override final
    {
        while (GetStatus() == ASYNC_TASK_STATUS_NOT_STARTED)
            std::this_thread::yield();
    }

protected:
    std::atomic<bool> m_bSafelyCancel{false};

private:
    std::atomic<float>             m_fPriority{0};
    std::atomic<ASYNC_TASK_STATUS> m_TaskStatus{ASYNC_TASK_STATUS_NOT_STARTED};
};


template <typename HanlderType>
RefCntAutoPtr<IAsyncTask> EnqueueAsyncWork(IThreadPool* pThreadPool, HanlderType Handler, float fPriority = 0)
{
    class TaskImpl final : public AsyncTaskBase
    {
    public:
        TaskImpl(IReferenceCounters* pRefCounters,
                 float               fPriority,
                 HanlderType&&       Handler) :
            AsyncTaskBase{pRefCounters, fPriority},
            m_Handler{std::move(Handler)}
        {}

        virtual void Run(Uint32 ThreadId) override final
        {
            m_Handler(ThreadId);
            SetStatus(ASYNC_TASK_STATUS_COMPLETE);
        }

    private:
        HanlderType m_Handler;
    };

    RefCntAutoPtr<TaskImpl> pTask{MakeNewRCObj<TaskImpl>()(fPriority, std::move(Handler))};
    pThreadPool->EnqueueTask(pTask);

    return pTask;
}

} // namespace Diligent
