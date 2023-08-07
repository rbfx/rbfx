//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/WorkQueue.h"

#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Core/ProcessUtils.h"
#include "Urho3D/Core/Profiler.h"
#include "Urho3D/Core/Thread.h"
#include "Urho3D/Core/Timer.h"
#include "Urho3D/IO/Log.h"

#ifdef URHO3D_THREADING
#include <enkiTS/src/TaskScheduler.h>
#endif

namespace Urho3D
{

namespace
{

// Context and WorkQueue are singletons anyway, keep the values here for quick access
unsigned threadIndexCount{};
WorkQueue* workQueue{};

#ifdef URHO3D_THREADING
template <class T>
struct ReleaseInternalTask : enki::ICompletable
{
    enki::Dependency dependency_;

    void OnDependenciesComplete(enki::TaskScheduler* taskScheduler, uint32_t threadNum) override
    {
        enki::ICompletable::OnDependenciesComplete(taskScheduler, threadNum);

        auto task = const_cast<T*>(static_cast<const T*>(dependency_.GetDependencyTask()));
        task->Release();
    }
};
#endif

TaskPriority ConvertLegacyPriority(unsigned priority)
{
    return priority == M_MAX_UNSIGNED
        ? TaskPriority::Immediate
        : static_cast<TaskPriority>(static_cast<unsigned>(TaskPriority::Low) - ea::min(priority, 2u));
}

}

#ifdef URHO3D_THREADING

template <class T> WorkQueue::TaskPool<T>::TaskPool()
{
}

template <class T> WorkQueue::TaskPool<T>::~TaskPool()
{
}

template <class T> T* WorkQueue::TaskPool<T>::Allocate()
{
    MutexLock lock(mutex_);

    if (freeItems_.empty())
    {
        pool_.push_back(ea::make_unique<T>(this));
        return pool_.back().get();
    }
    else
    {
        T* item = freeItems_.back();
        freeItems_.pop_back();
        return item;
    }
}

template <class T> void WorkQueue::TaskPool<T>::Release(T* item)
{
    MutexLock lock(mutex_);
    freeItems_.push_back(item);
}

template <class T> unsigned WorkQueue::TaskPool<T>::GetNumUsed() const
{
    MutexLock lock(mutex_);
    return pool_.size() - freeItems_.size();
}

template <class T> WorkQueue::TaskStack<T>::TaskStack()
{
}

template <class T> WorkQueue::TaskStack<T>::~TaskStack()
{
}

template <class T> WorkQueue::TaskStack<T>::TaskStack(TaskStack<T>&& other)
    : pool_(ea::move(other.pool_))
{
}

template <class T> WorkQueue::TaskStack<T>& WorkQueue::TaskStack<T>::operator=(TaskStack<T>&& other)
{
    pool_ = ea::move(other.pool_);
    return *this;
}

template <class T> T* WorkQueue::TaskStack<T>::Allocate()
{
    if (nextFreeIndex_ == pool_.size())
    {
        ++nextFreeIndex_;
        pool_.push_back(ea::make_unique<T>());
        return pool_.back().get();
    }
    else
    {
        return pool_[nextFreeIndex_++].get();
    }
}

template <class T> void WorkQueue::TaskStack<T>::Purge()
{
    nextFreeIndex_ = 0;
}

class WorkQueue::InternalTaskInPool : public enki::ITaskSet
{
public:
    TaskFunction function_;

    InternalTaskInPool(TaskPool<InternalTaskInPool>* owner)
        : owner_(owner)
    {
        releaser_.SetDependency(releaser_.dependency_, this);
    }

    void Release()
    {
        owner_->Release(this);
    }

    void ExecuteRange(enki::TaskSetPartition range, uint32_t threadNum) override
    {
        URHO3D_ASSERT(function_);
        function_(threadNum, workQueue);
        function_ = nullptr;
    }

private:
    TaskPool<InternalTaskInPool>* owner_{};
    ReleaseInternalTask<InternalTaskInPool> releaser_;
};

class WorkQueue::InternalPinnedTaskInPool : public enki::IPinnedTask
{
public:
    TaskFunction function_;

    InternalPinnedTaskInPool(TaskPool<InternalPinnedTaskInPool>* owner)
        : owner_(owner)
    {
        releaser_.SetDependency(releaser_.dependency_, this);
    }

    void Release()
    {
        owner_->Release(this);
    }

    void Execute() override
    {
        URHO3D_ASSERT(function_);
        function_(threadNum, workQueue);
        function_ = nullptr;
    }

private:
    TaskPool<InternalPinnedTaskInPool>* owner_{};
    ReleaseInternalTask<InternalPinnedTaskInPool> releaser_;
};

class WorkQueue::InternalTaskInStack : public enki::ITaskSet
{
public:
    TaskFunction function_;
    enki::Dependency observerDependency_;

    void ExecuteRange(enki::TaskSetPartition range, uint32_t threadNum) override
    {
        URHO3D_ASSERT(function_);
        function_(threadNum, workQueue);
        function_ = nullptr;
    }
};

class WorkQueue::InternalPinnedTaskInStack : public enki::IPinnedTask
{
public:
    TaskFunction function_;
    enki::Dependency observerDependency_;

    void Execute() override
    {
        URHO3D_ASSERT(function_);
        function_(threadNum, workQueue);
        function_ = nullptr;
    }
};

class WorkQueue::TaskCompletionObserver
    : public enki::ITaskSet
    , public MovableNonCopyable
{
public:
    TaskCompletionObserver() = default;

    void AddDependency(InternalTaskInStack* internalTask)
    {
        SetDependency(internalTask->observerDependency_, internalTask);
    }

    void ExecuteRange(enki::TaskSetPartition range, uint32_t threadNum) override
    {
    }
};

#endif

WorkQueue::WorkQueue(Context* context)
    : Object(context)
{
    if (workQueue == nullptr)
    {
        threadIndexCount = 1;
        workQueue = this;
    }

    SubscribeToEvent(E_BEGINFRAME, &WorkQueue::Update);
}

WorkQueue::~WorkQueue()
{
#ifdef URHO3D_THREADING
    if (taskScheduler_)
    {
        taskScheduler_->ShutdownNow();
        taskScheduler_ = nullptr;
    }
#endif

    if (workQueue == this)
    {
        threadIndexCount = 0;
        workQueue = nullptr;
    }
}

void WorkQueue::Initialize(unsigned numThreads)
{
    // Other subsystems may initialize themselves according to the number of threads.
    // Therefore allow creating the threads only once, after which the amount is fixed
    if (numProcessingThreads_ > 0)
        return;

    numProcessingThreads_ = numThreads + 1;
    threadIndexCount = 1;

#ifdef URHO3D_THREADING
    if (numThreads > 0)
    {
        taskScheduler_ = ea::make_unique<enki::TaskScheduler>();
        taskScheduler_->Initialize(numProcessingThreads_);

        threadIndexCount = taskScheduler_->GetNumTaskThreads();

        localTaskStack_.resize(threadIndexCount);
        localPinnedTaskStack_.resize(threadIndexCount);
        pendingImmediateTasks_.resize(threadIndexCount);

        URHO3D_LOGINFO("Created {} worker thread{}", numThreads, numThreads > 1 ? "s" : "");
    }
#endif
}

void WorkQueue::Update()
{
    ProcessPostedTasks();
    ProcessMainThreadTasks();
}

void WorkQueue::ProcessPostedTasks()
{
#ifdef URHO3D_THREADING
    if (taskScheduler_)
    {
        CompleteImmediateForThisThread();
        taskScheduler_->RunPinnedTasks();
        for (auto& stack : localTaskStack_)
            stack.Purge();
        for (auto& stack : localPinnedTaskStack_)
            stack.Purge();
    }
#endif

    if (!fallbackTaskQueue_.empty())
    {
        HiresTimer timer;
        for (auto& [_, task] : fallbackTaskQueue_)
        {
            if (timer.GetUSec(false) >= maxNonThreadedWorkMs_ * 1000LL)
                break;

            task(0, this);
            task = nullptr;
        }
        PurgeProcessedTasksInFallbackQueue();
    }
}

void WorkQueue::ProcessMainThreadTasks()
{
#ifdef URHO3D_THREADING
    if (taskScheduler_)
        taskScheduler_->RunPinnedTasks();
#endif

    {
        MutexLock lock(mainThreadTasksMutex_);
        ea::swap(mainThreadTasks_, mainThreadTasksSwap_);
    }

    for (const auto& callback : mainThreadTasksSwap_)
        callback(0, this);
    mainThreadTasksSwap_.clear();
}

void WorkQueue::PurgeProcessedTasksInFallbackQueue()
{
    ea::erase_if(fallbackTaskQueue_,
        [](const ea::pair<TaskPriority, TaskFunction>& priorityAndTask) { return !priorityAndTask.second; });
}

#ifdef URHO3D_THREADING
template <class T> void WorkQueue::SetupInternalTask(T* internalTask, TaskFunction&& task, TaskPriority priority)
{
    internalTask->function_ = ea::move(task);
    internalTask->m_Priority = static_cast<enki::TaskPriority>(priority);
}
#endif

void WorkQueue::PostTask(TaskFunction&& task, TaskPriority priority)
{
    if (!IsProcessingThread())
    {
        auto wrappedTask = [task = ea::move(task), priority](unsigned, WorkQueue* queue) mutable
        { queue->PostTask(ea::move(task), priority); };

        PostTaskForMainThread(ea::move(wrappedTask), priority);
        return;
    }

#ifdef URHO3D_THREADING
    if (taskScheduler_)
    {
        if (priority == TaskPriority::Immediate)
        {
            const unsigned threadIndex = GetThreadIndex();
            InternalTaskInStack* internalTask = localTaskStack_[threadIndex].Allocate();
            SetupInternalTask(internalTask, ea::move(task), priority);
            pendingImmediateTasks_[threadIndex].push_back(internalTask);
        }
        else
        {
            InternalTaskInPool* internalTask = globalTaskPool_.Allocate();
            SetupInternalTask(internalTask, ea::move(task), priority);
            taskScheduler_->AddTaskSetToPipe(internalTask);
        }
        return;
    }
#endif

    if (priority == TaskPriority::Immediate)
        task(0, this);
    else
        fallbackTaskQueue_.emplace_back(priority, ea::move(task));
}

void WorkQueue::PostTaskForThread(TaskFunction&& task, TaskPriority priority, unsigned threadIndex)
{
    if (!IsProcessingThread())
    {
        auto wrappedTask = [task = ea::move(task), priority, threadIndex](unsigned, WorkQueue* queue) mutable
        { queue->PostTaskForThread(ea::move(task), priority, threadIndex); };

        PostTaskForMainThread(ea::move(wrappedTask), priority);
        return;
    }

#ifdef URHO3D_THREADING
    if (taskScheduler_)
    {
        if (priority == TaskPriority::Immediate)
        {
            InternalPinnedTaskInStack* internalTask = localPinnedTaskStack_[GetThreadIndex()].Allocate();
            SetupInternalTask(internalTask, ea::move(task), priority);
            internalTask->threadNum = threadIndex;
            taskScheduler_->AddPinnedTask(internalTask);
        }
        else
        {
            InternalPinnedTaskInPool* internalTask = globalPinnedTaskPool_.Allocate();
            SetupInternalTask(internalTask, ea::move(task), priority);
            internalTask->threadNum = threadIndex;
            taskScheduler_->AddPinnedTask(internalTask);
        }
        return;
    }
#endif

    if (priority == TaskPriority::Immediate)
        task(0, this);
    else
        fallbackTaskQueue_.emplace_back(priority, ea::move(task));
}

void WorkQueue::PostTaskForMainThread(TaskFunction&& task, TaskPriority priority)
{
#ifdef URHO3D_THREADING
    if (taskScheduler_ && IsProcessingThread())
    {
        PostTaskForThread(ea::move(task), priority, 0);
        return;
    }
#endif

    if (Thread::IsMainThread())
        task(0, this);
    else
        PostDelayedTaskForMainThread(ea::move(task));
}

void WorkQueue::PostDelayedTaskForMainThread(TaskFunction&& task)
{
    MutexLock lock(mainThreadTasksMutex_);
    mainThreadTasks_.push_back(ea::move(task));
}

void WorkQueue::CompleteImmediateForAnotherThread(unsigned threadIndex)
{
#ifdef URHO3D_THREADING
    static const auto priority = static_cast<enki::TaskPriority>(TaskPriority::Immediate);
    if (taskScheduler_)
    {
        auto& pendingTasks = pendingImmediateTasks_[threadIndex];
        if (!pendingTasks.empty())
        {
            enki::ICompletable observerTask;
            for (InternalTaskInStack* task : pendingTasks)
                task->observerDependency_.SetDependency(task, &observerTask);

            for (InternalTaskInStack* task : pendingTasks)
                taskScheduler_->AddTaskSetToPipe(task);
            taskScheduler_->WaitforTask(&observerTask, priority);

            for (InternalTaskInStack* task : pendingTasks)
                task->observerDependency_.ClearDependency();
            pendingTasks.clear();
        }
    }
#endif
    // Fallback queue never contains immediate tasks
}

void WorkQueue::CompleteImmediateForThisThread()
{
#ifdef URHO3D_THREADING
    if (taskScheduler_)
    {
        const unsigned threadIndex = GetThreadIndex();
        CompleteImmediateForAnotherThread(threadIndex);
        taskScheduler_->RunPinnedTasks();
    }
#endif
}

void WorkQueue::CompleteAll()
{
#ifdef URHO3D_THREADING
    if (taskScheduler_)
        taskScheduler_->WaitforAll();
#endif

    if (!fallbackTaskQueue_.empty())
    {
        for (auto& [taskPriority, task] : fallbackTaskQueue_)
            task(0, this);
        fallbackTaskQueue_.clear();
    }
}

SharedPtr<WorkItem> WorkQueue::GetFreeItem()
{
    // This function is deprecated, so we don't care about performance here.
    return MakeShared<WorkItem>();
}

void WorkQueue::AddWorkItem(const SharedPtr<WorkItem>& item)
{
    if (!item)
    {
        URHO3D_LOGERROR("Null work item submitted to the work queue");
        return;
    }

    PostTask([=] { item->workFunction_(item, GetThreadIndex()); }, ConvertLegacyPriority(item->priority_));
}

SharedPtr<WorkItem> WorkQueue::AddWorkItem(ea::function<void(unsigned threadIndex)> workFunction, unsigned priority)
{
    SharedPtr<WorkItem> item = GetFreeItem();
    item->workLambda_ = ea::move(workFunction);
    item->workFunction_ = [](const WorkItem* item, unsigned threadIndex) { item->workLambda_(threadIndex); };
    item->priority_ = priority;
    AddWorkItem(item);
    return item;
}

unsigned WorkQueue::GetNumIncomplete() const
{
    unsigned result = fallbackTaskQueue_.size();
#ifdef URHO3D_THREADING
    result += globalTaskPool_.GetNumUsed();
    result += globalPinnedTaskPool_.GetNumUsed();
#endif
    return result;
}

bool WorkQueue::IsCompleted() const
{
    return GetNumIncomplete() == 0;
}

unsigned WorkQueue::GetThreadIndex()
{
#ifdef URHO3D_THREADING
    if (workQueue && workQueue->taskScheduler_)
        return workQueue->taskScheduler_->GetThreadNum();
#endif
    return Thread::IsMainThread() ? 0u : M_MAX_UNSIGNED;
}

unsigned WorkQueue::GetThreadIndexCount()
{
    return threadIndexCount;
}

bool WorkQueue::IsProcessingThread()
{
    return GetThreadIndex() < threadIndexCount;
}

void WorkQueue::CallFromMainThread(WorkFunction workFunction)
{
    PostTaskForMainThread([=] { workFunction(0u); }, TaskPriority::Immediate);
}

void WorkQueue::Complete(unsigned priority)
{
    if (priority == M_MAX_UNSIGNED)
        CompleteImmediateForThisThread();
    else
        CompleteAll();
}

}
