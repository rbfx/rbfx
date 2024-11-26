/*
 *  Copyright 2024 Diligent Graphics LLC
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
#include <memory>
#include <vector>
#include <algorithm>

#include "SpinLock.hpp"
#include "ThreadPool.hpp"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

class AsyncInitializer
{
public:
    static_assert((ASYNC_TASK_STATUS_UNKNOWN < ASYNC_TASK_STATUS_NOT_STARTED &&
                   ASYNC_TASK_STATUS_NOT_STARTED < ASYNC_TASK_STATUS_RUNNING &&
                   ASYNC_TASK_STATUS_RUNNING < ASYNC_TASK_STATUS_CANCELLED &&
                   ASYNC_TASK_STATUS_CANCELLED < ASYNC_TASK_STATUS_COMPLETE),
                  "ASYNC_TASK_STATUS enum values are not ordered correctly");

    ASYNC_TASK_STATUS Update(bool WaitForCompletion)
    {
        ASYNC_TASK_STATUS CurrStatus = m_Status.load();
        VERIFY_EXPR(CurrStatus != ASYNC_TASK_STATUS_UNKNOWN);
        if (CurrStatus <= ASYNC_TASK_STATUS_RUNNING)
        {
            RefCntAutoPtr<IAsyncTask> pTask;
            {
                Threading::SpinLockGuard Guard{m_TaskLock};
                pTask = m_wpTask.Lock();
            }

            ASYNC_TASK_STATUS NewStatus = ASYNC_TASK_STATUS_UNKNOWN;
            if (pTask)
            {
                if (WaitForCompletion)
                {
                    pTask->WaitForCompletion();
                }
                NewStatus = pTask->GetStatus();
            }

            if (NewStatus == ASYNC_TASK_STATUS_CANCELLED || NewStatus == ASYNC_TASK_STATUS_COMPLETE)
            {
                Threading::SpinLockGuard Guard{m_TaskLock};
                m_wpTask.Release();
            }

            if (NewStatus > CurrStatus)
            {
                while (!m_Status.compare_exchange_weak(CurrStatus, (std::max)(CurrStatus, NewStatus)))
                {
                    // If exchange fails, CurrStatus will hold the actual value of m_Status.
                }
            }
        }

        return m_Status.load();
    }

    ASYNC_TASK_STATUS GetStatus() const
    {
        return m_Status.load();
    }

    RefCntAutoPtr<IAsyncTask> GetCompileTask() const
    {
        Threading::SpinLockGuard Guard{m_TaskLock};
        return m_wpTask.Lock();
    }

    template <typename HanlderType>
    static std::unique_ptr<AsyncInitializer> Start(IThreadPool*  pThreadPool,
                                                   IAsyncTask**  ppPrerequisites,
                                                   Uint32        NumPrerequisites,
                                                   HanlderType&& Handler)
    {
        VERIFY_EXPR(pThreadPool != nullptr);
        return std::unique_ptr<AsyncInitializer>{
            new AsyncInitializer{
                EnqueueAsyncWork(pThreadPool, ppPrerequisites, NumPrerequisites,
                                 [Handler = std::forward<HanlderType>(Handler)](Uint32 ThreadId) mutable {
                                     Handler(ThreadId);
                                     return ASYNC_TASK_STATUS_COMPLETE;
                                 }),
            },
        };
    }

    template <typename HanlderType>
    static std::unique_ptr<AsyncInitializer> Start(IThreadPool*                                  pThreadPool,
                                                   const std::vector<RefCntAutoPtr<IAsyncTask>>& Prerequisites,
                                                   HanlderType&&                                 Handler)
    {
        std::vector<IAsyncTask*> PrerequisitePtrs{Prerequisites.begin(), Prerequisites.end()};
        return Start(pThreadPool, PrerequisitePtrs.data(), static_cast<Uint32>(PrerequisitePtrs.size()), std::forward<HanlderType>(Handler));
    }

    template <typename HanlderType>
    static std::unique_ptr<AsyncInitializer> Start(IThreadPool*  pThreadPool,
                                                   HanlderType&& Handler)
    {
        return Start(pThreadPool, nullptr, 0, std::forward<HanlderType>(Handler));
    }

    static ASYNC_TASK_STATUS Update(std::unique_ptr<AsyncInitializer>& Initializer, bool WaitForCompletion)
    {
        return Initializer ? Initializer->Update(WaitForCompletion) : ASYNC_TASK_STATUS_UNKNOWN;
    }

    static RefCntAutoPtr<IAsyncTask> GetAsyncTask(const std::unique_ptr<AsyncInitializer>& Initializer)
    {
        return Initializer ? Initializer->GetCompileTask() : RefCntAutoPtr<IAsyncTask>{};
    }

private:
    AsyncInitializer(RefCntAutoPtr<IAsyncTask> pTask) :
        m_wpTask{pTask}
    {
        // Do not get the actual status from the task since it needs to be done in Update().
        // The task may happen to be complete by the time the initializer is created.
    }

private:
    // It is important to set the task status to a non-unknown value before the task is started.
    std::atomic<ASYNC_TASK_STATUS> m_Status{ASYNC_TASK_STATUS_NOT_STARTED};

    // Note that while RefCntAutoPtr/RefCntWeakPtr allow safely accessing the same object
    // from multiple threads using different pointers, they are not thread-safe by themselves
    // (e.g. it is not safe to call Lock() or Release() on the same pointer from multiple threads).
    // Therefore, we need to use a lock to protect access to m_wpTask.
    mutable Threading::SpinLock       m_TaskLock;
    mutable RefCntWeakPtr<IAsyncTask> m_wpTask;
};

} // namespace Diligent
