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

#include "ThreadPool.hpp"

#include <mutex>
#include <thread>
#include <map>
#include <vector>
#include <condition_variable>

namespace Diligent
{

AsyncTaskBase::~AsyncTaskBase()
{
}

class ThreadPoolImpl final : public ObjectBase<IThreadPool>
{
public:
    using TBase = ObjectBase<IThreadPool>;

    ThreadPoolImpl(IReferenceCounters*         pRefCounters,
                   const ThreadPoolCreateInfo& PoolCI) :
        TBase{pRefCounters}
    {
        m_WorkerThreads.reserve(PoolCI.NumThreads);
        for (Uint32 i = 0; i < PoolCI.NumThreads; ++i)
        {
            m_WorkerThreads.emplace_back(
                [this, PoolCI, i] //
                {
                    if (PoolCI.OnThreadStarted)
                        PoolCI.OnThreadStarted(i);

                    while (ProcessTask(i, /*WaitForTask =*/true))
                    {
                    }

                    if (PoolCI.OnThreadExiting)
                        PoolCI.OnThreadExiting(i);
                });
        }
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_ThreadPool, TBase)

    virtual bool ProcessTask(Uint32 ThreadId, bool WaitForTask) override final
    {
        RefCntAutoPtr<IAsyncTask> pTask;
        {
            std::unique_lock<std::mutex> lock{m_TasksQueueMtx};
            if (WaitForTask)
            {
                // The effects of notify_one()/notify_all() and each of the three atomic parts of
                // wait()/wait_for()/wait_until() (unlock+wait, wakeup, and lock) take place in a
                // single total order that can be viewed as modification order of an atomic variable:
                // the order is specific to this individual condition variable. This makes it impossible
                // for notify_one() to, for example, be delayed and unblock a thread that started waiting
                // just after the call to notify_one() was made.
                m_NextTaskCond.wait(lock,
                                    [this] //
                                    {
                                        return m_Stop.load() || !m_TasksQueue.empty();
                                    } //
                );
            }

            // m_Stop must be accessed under the mutex
            if (m_Stop.load() && m_TasksQueue.empty())
                return false;

            if (!m_TasksQueue.empty())
            {
                auto front = m_TasksQueue.begin();
                pTask      = std::move(front->second);
                // NB: we must increment the running task counter while holding the lock and
                //     before removing the task from the queue, otherwise WaitForAllTasks() may
                //     miss the task.
                m_NumRunningTasks.fetch_add(1);
                m_TasksQueue.erase(front);
            }
        }

        if (pTask)
        {
            pTask->SetStatus(ASYNC_TASK_STATUS_RUNNING);
            pTask->Run(ThreadId);
            DEV_CHECK_ERR((pTask->GetStatus() == ASYNC_TASK_STATUS_COMPLETE ||
                           pTask->GetStatus() == ASYNC_TASK_STATUS_CANCELLED),
                          "Finished tasks must be in COMPLETE or CANCELLED state");

            {
                std::unique_lock<std::mutex> lock{m_TasksQueueMtx};

                const auto NumRunningTasks = m_NumRunningTasks.fetch_add(-1) - 1;
                if (m_TasksQueue.empty() && NumRunningTasks == 0)
                {
                    m_TasksFinishedCond.notify_one();
                }
            }
        }

        return true;
    }

    virtual void EnqueueTask(IAsyncTask* pTask) override final
    {
        VERIFY_EXPR(pTask != nullptr);
        if (pTask == nullptr)
            return;

        {
            std::unique_lock<std::mutex> lock{m_TasksQueueMtx};
            DEV_CHECK_ERR(!m_Stop, "Enqueue on a stopped ThreadPool");

            m_TasksQueue.emplace(pTask->GetPriority(), pTask);
        }
        m_NextTaskCond.notify_one();
    }

    virtual void WaitForAllTasks() override final
    {
        std::unique_lock<std::mutex> lock{m_TasksQueueMtx};
        if (!m_TasksQueue.empty() || m_NumRunningTasks.load() > 0)
        {
            m_TasksFinishedCond.wait(lock,
                                     [this] //
                                     {
                                         return m_TasksQueue.empty() && m_NumRunningTasks.load() == 0;
                                     } //
            );
        }
    }

    virtual void StopThreads() override final
    {
        {
            std::unique_lock<std::mutex> lock{m_TasksQueueMtx};
            // NB: even if the shared variable is atomic, it must be modified under the mutex
            //     in order to correctly publish the modification to the waiting thread.
            m_Stop.store(true);
        }
        // Note that if there are outstanding tasks in the queue, the threads may be woken up
        // by the corresponding notify_one() as notify*() and wait*() take place in a single
        // total order.
        m_NextTaskCond.notify_all();
        for (std::thread& worker : m_WorkerThreads)
            worker.join();

        m_WorkerThreads.clear();
    }

    virtual bool RemoveTask(IAsyncTask* pTask) override final
    {
        std::unique_lock<std::mutex> lock{m_TasksQueueMtx};

        auto it = m_TasksQueue.begin();
        while (it != m_TasksQueue.end() && it->second != pTask)
            ++it;
        if (it != m_TasksQueue.end())
        {
            m_TasksQueue.erase(it);
            return true;
        }

        return false;
    }

    virtual bool ReprioritizeTask(IAsyncTask* pTask) override final
    {
        const auto Priority = pTask->GetPriority();

        std::unique_lock<std::mutex> lock{m_TasksQueueMtx};

        auto it = m_TasksQueue.begin();
        while (it != m_TasksQueue.end() && it->second != pTask)
            ++it;
        if (it != m_TasksQueue.end())
        {
            if (it->first != Priority)
            {
                auto pExistingTask = std::move(it->second);
                m_TasksQueue.erase(it);
                m_TasksQueue.emplace(Priority, std::move(pExistingTask));
            }

            return true;
        }
        return false;
    }

    virtual void ReprioritizeAllTasks() override final
    {
        std::unique_lock<std::mutex> lock{m_TasksQueueMtx};

        m_ReprioritizationList.clear();
        auto it = m_TasksQueue.begin();
        while (it != m_TasksQueue.end())
        {
            auto pTask    = it->second;
            auto Priority = pTask->GetPriority();
            if (it->first != Priority)
            {
                it = m_TasksQueue.erase(it);
                m_ReprioritizationList.emplace_back(Priority, std::move(pTask));
            }
            else
            {
                ++it;
            }
        }

        if (!m_ReprioritizationList.empty())
            m_TasksQueue.insert(m_ReprioritizationList.begin(), m_ReprioritizationList.end());

        m_ReprioritizationList.clear();
    }

    Uint32 GetQueueSize() override final
    {
        std::unique_lock<std::mutex> lock{m_TasksQueueMtx};
        return StaticCast<Uint32>(m_TasksQueue.size());
    }

    virtual Uint32 GetRunningTaskCount() const override final
    {
        return m_NumRunningTasks.load();
    }

    ~ThreadPoolImpl()
    {
        StopThreads();
        VERIFY_EXPR(m_TasksQueue.empty());
        VERIFY_EXPR(m_NumRunningTasks.load() == 0);
    }

private:
    std::vector<std::thread> m_WorkerThreads;

    // Priority queue
    std::mutex                                                           m_TasksQueueMtx;
    std::multimap<float, RefCntAutoPtr<IAsyncTask>, std::greater<float>> m_TasksQueue;

    std::vector<std::pair<float, RefCntAutoPtr<IAsyncTask>>> m_ReprioritizationList;

    std::condition_variable m_NextTaskCond{};
    std::condition_variable m_TasksFinishedCond{};
    std::atomic<bool>       m_Stop{false};

    std::atomic<int> m_NumRunningTasks{0};
};

RefCntAutoPtr<IThreadPool> CreateThreadPool(const ThreadPoolCreateInfo& ThreadPoolCI)
{
    return RefCntAutoPtr<ThreadPoolImpl>{MakeNewRCObj<ThreadPoolImpl>()(ThreadPoolCI)};
}

} // namespace Diligent
