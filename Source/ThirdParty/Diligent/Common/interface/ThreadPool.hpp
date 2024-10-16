/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include "ThreadPool.h"

#include <atomic>
#include <functional>
#include <thread>

#include "../../Platforms/Basic/interface/DebugUtilities.hpp"

#include "ObjectBase.hpp"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

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

    virtual void DILIGENT_CALL_TYPE Cancel() override
    {
        m_bSafelyCancel.store(true);
    }

    virtual void DILIGENT_CALL_TYPE SetStatus(ASYNC_TASK_STATUS TaskStatus) override final
    {
#ifdef DILIGENT_DEVELOPMENT
        if (TaskStatus != m_TaskStatus)
        {
            switch (TaskStatus)
            {
                case ASYNC_TASK_STATUS_UNKNOWN:
                    DEV_ERROR("UNKNOWN is invalid status.");
                    break;

                case ASYNC_TASK_STATUS_NOT_STARTED:
                    DEV_CHECK_ERR(m_TaskStatus == ASYNC_TASK_STATUS_RUNNING,
                                  "A task should only be moved to NOT_STARTED state from RUNNING state.");
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
        m_TaskStatus.store(TaskStatus);
    }

    virtual ASYNC_TASK_STATUS DILIGENT_CALL_TYPE GetStatus() const override final
    {
        return m_TaskStatus.load();
    }

    virtual void DILIGENT_CALL_TYPE SetPriority(float fPriority) override final
    {
        m_fPriority.store(fPriority);
    }

    virtual float DILIGENT_CALL_TYPE GetPriority() const override final
    {
        return m_fPriority.load();
    }

    virtual bool DILIGENT_CALL_TYPE IsFinished() const override final
    {
        static_assert(ASYNC_TASK_STATUS_COMPLETE > ASYNC_TASK_STATUS_CANCELLED && ASYNC_TASK_STATUS_CANCELLED > ASYNC_TASK_STATUS_RUNNING,
                      "Unexpected enum values");
        return m_TaskStatus.load() >= ASYNC_TASK_STATUS_CANCELLED;
    }

    virtual void DILIGENT_CALL_TYPE WaitForCompletion() const override final
    {
        while (!IsFinished())
            std::this_thread::yield();
    }

    virtual void DILIGENT_CALL_TYPE WaitUntilRunning() const override final
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


/// Enqueues a function to be executed asynchronously by the thread pool.
/// For the list of parameters, see Diligent::IThreadPool::EnqueueTask() method.
/// The handler function must return the task status, see Diligent::IAsyncTask::Run() method.
template <typename HanlderType>
RefCntAutoPtr<IAsyncTask> EnqueueAsyncWork(IThreadPool* pThreadPool,
                                           IAsyncTask** ppPrerequisites,
                                           Uint32       NumPrerequisites,
                                           HanlderType  Handler,
                                           float        fPriority = 0)
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

        virtual ASYNC_TASK_STATUS DILIGENT_CALL_TYPE Run(Uint32 ThreadId) override final
        {
            return !m_bSafelyCancel.load() ?
                m_Handler(ThreadId) :
                ASYNC_TASK_STATUS_CANCELLED;
        }

    private:
        HanlderType m_Handler;
    };

    RefCntAutoPtr<TaskImpl> pTask{MakeNewRCObj<TaskImpl>()(fPriority, std::move(Handler))};
    pThreadPool->EnqueueTask(pTask, ppPrerequisites, NumPrerequisites);

    return pTask;
}

template <typename HanlderType>
RefCntAutoPtr<IAsyncTask> EnqueueAsyncWork(IThreadPool* pThreadPool,
                                           HanlderType  Handler,
                                           float        fPriority = 0)
{
    return EnqueueAsyncWork(pThreadPool, nullptr, 0, std::move(Handler), fPriority);
}

} // namespace Diligent
