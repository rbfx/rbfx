// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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

enum class TaskReleaseMode : unsigned char
{
    Automatic,
    Manual
};

#ifdef URHO3D_THREADING
template <class T> struct ReleaseInternalTask : enki::ICompletable
{
    enki::Dependency dependency_;

    void OnDependenciesComplete(enki::TaskScheduler* taskScheduler, uint32_t threadNum) override
    {
        auto task = const_cast<T*>(static_cast<const T*>(dependency_.GetDependencyTask()));
        const auto releaseMode = task->GetReleaseMode();

        enki::ICompletable::OnDependenciesComplete(taskScheduler, threadNum);

        if (releaseMode == TaskReleaseMode::Automatic)
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

/// Base class for pinned and unpinned tasks.
class BaseInternalTaskInPool
{
public:
    void SetReleaseMode(TaskReleaseMode mode) { releaseMode_ = mode; }

    TaskReleaseMode GetReleaseMode() const { return releaseMode_; }

    virtual void Release() = 0;

private:
    TaskReleaseMode releaseMode_{};
};

} // namespace

namespace Detail
{

struct WorkCompletionTokenImpl
{
#ifdef URHO3D_THREADING
    const enki::ICompletable* completionFence_{};
    BaseInternalTaskInPool* poolTask_{};
#endif
};

} // namespace Detail

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

template <class T>
WorkQueue::TaskStack<T>::TaskStack(TaskStack<T>&& other)
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

class WorkQueue::InternalTaskInPool : public enki::ITaskSet, public BaseInternalTaskInPool
{
public:
    TaskFunction function_;

    InternalTaskInPool(TaskPool<InternalTaskInPool>* owner)
        : owner_(owner)
    {
        releaser_.SetDependency(releaser_.dependency_, this);
    }

    const enki::ICompletable* GetCompletionFence() const { return &releaser_; }

    void Release() override { owner_->Release(this); }

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

class WorkQueue::InternalPinnedTaskInPool : public enki::IPinnedTask, public BaseInternalTaskInPool
{
public:
    TaskFunction function_;

    InternalPinnedTaskInPool(TaskPool<InternalPinnedTaskInPool>* owner)
        : owner_(owner)
    {
        releaser_.SetDependency(releaser_.dependency_, this);
    }

    const enki::ICompletable* GetCompletionFence() const { return &releaser_; }

    void Release() override { owner_->Release(this); }

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

    void ExecuteRange(enki::TaskSetPartition range, uint32_t threadNum) override {}
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

        for (unsigned i = 1; i < numProcessingThreads_; ++i)
        {
            const std::string threadName = fmt::format("Worker {}", i);
            PostTaskForThread(
                [=]() { SetProfilerThreadName(threadName.c_str()); }, TaskPriority::Immediate, i, nullptr);
        }
        taskScheduler_->WaitforAll();

        URHO3D_LOGINFO("Created {} worker thread{}", numThreads, numThreads > 1 ? "s" : "");
    }
    else
    {
        URHO3D_LOGINFO("Worker threads are disabled on initialization");
    }
#else
    URHO3D_LOGINFO("Worker threads are disabled in this build");
#endif
}

void WorkQueue::Update()
{
    HiresTimer budgetTimer;
    UpdateOnMainThread(&budgetTimer);
}

WorkCompletionToken WorkQueue::CreateCompletionToken()
{
    return ea::make_shared<Detail::WorkCompletionTokenImpl>();
}

unsigned WorkQueue::ExecuteWithinBudget(
    const ea::vector<TaskFunction>& tasks, HiresTimer* budgetTimer, unsigned budgetMs)
{
    unsigned numExecutedTasks = 0;
    for (auto& callback : tasks)
    {
        if (numExecutedTasks > 0 && budgetTimer && budgetTimer->GetUSec(false) >= budgetMs * 1000LL)
            break;

        callback(0, this);
        ++numExecutedTasks;
    }
    return numExecutedTasks;
}

bool WorkQueue::ProcessPostedTasks(HiresTimer* budgetTimer)
{
    URHO3D_PROFILE("AnyThreadTasks");

#ifdef URHO3D_THREADING
    if (taskScheduler_)
    {
        // TODO: Do we need to call CompleteImmediateForAnotherThread too?
        // Stacks for all threads are purged, so all immediate tasks should be completed too.
        // This code probably works fine with immediate tasks are only posted to and from main thread.
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
        const unsigned numExecutedTasks = ExecuteWithinBudget(fallbackTaskQueue_, budgetTimer, maxNonThreadedWorkMs_);
        fallbackTaskQueue_.erase(fallbackTaskQueue_.begin(), fallbackTaskQueue_.begin() + numExecutedTasks);
        return true;
    }

    return false;
}

bool WorkQueue::ProcessMainThreadTasks(HiresTimer* budgetTimer)
{
    URHO3D_PROFILE("MainThreadTasks");

#ifdef URHO3D_THREADING
    if (taskScheduler_)
        taskScheduler_->RunPinnedTasks();
#endif

    {
        MutexLock lock(mainThreadTasksMutex_);
        ea::swap(mainThreadTasks_, mainThreadTasksSwap_);
    }

    if (!mainThreadTasksSwap_.empty())
    {
        const unsigned numExecutedTasks = ExecuteWithinBudget(mainThreadTasksSwap_, budgetTimer, maxNonThreadedWorkMs_);
        if (numExecutedTasks != mainThreadTasksSwap_.size())
        {
            const auto iterBegin = ea::make_move_iterator(mainThreadTasksSwap_.begin());
            const auto iterEnd = ea::make_move_iterator(mainThreadTasksSwap_.end());

            MutexLock lock(mainThreadTasksMutex_);
            mainThreadTasks_.insert(mainThreadTasks_.begin(), iterBegin + numExecutedTasks, iterEnd);
        }
        mainThreadTasksSwap_.clear();
        return true;
    }

    return false;
}

bool WorkQueue::UpdateOnMainThread(HiresTimer* budgetTimer)
{
    bool result = false;
    result |= ProcessPostedTasks(budgetTimer);
    result |= ProcessMainThreadTasks(budgetTimer);
    return result;
}

#ifdef URHO3D_THREADING
template <class T> void WorkQueue::SetupInternalTask(T* internalTask, TaskFunction&& task, TaskPriority priority)
{
    internalTask->function_ = ea::move(task);
    internalTask->m_Priority = static_cast<enki::TaskPriority>(priority);
}

template <class T> void WorkQueue::SetupCompletionToken(T* internalTask, WorkCompletionToken completionToken)
{
    if (!completionToken)
        internalTask->SetReleaseMode(TaskReleaseMode::Automatic);
    else
    {
        internalTask->SetReleaseMode(TaskReleaseMode::Manual);
        completionToken->completionFence_ = internalTask->GetCompletionFence();
        completionToken->poolTask_ = internalTask;
    }
}
#endif

void WorkQueue::PostTask(TaskFunction&& task, TaskPriority priority, WorkCompletionToken completionToken)
{
    URHO3D_ASSERTLOG(!completionToken || priority != TaskPriority::Immediate);

    if (!IsProcessingThread())
    {
        // clang-format off
        auto wrappedTask = [task = ea::move(task), completionToken = ea::move(completionToken), priority](
            unsigned, WorkQueue* queue) mutable
        {
            queue->PostTask(ea::move(task), priority, completionToken);
        };
        // clang-format on

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
            SetupCompletionToken(internalTask, completionToken);
            taskScheduler_->AddTaskSetToPipe(internalTask);
        }
        return;
    }
#endif

    if (priority == TaskPriority::Immediate || completionToken)
        task(0, this);
    else
        fallbackTaskQueue_.push_back(ea::move(task));
}

void WorkQueue::PostTaskForThread(
    TaskFunction&& task, TaskPriority priority, unsigned threadIndex, WorkCompletionToken completionToken)
{
    URHO3D_ASSERTLOG(!completionToken || priority != TaskPriority::Immediate);

    if (!IsProcessingThread())
    {
        // clang-format off
        auto wrappedTask = [task = ea::move(task), completionToken = ea::move(completionToken), priority, threadIndex](
            unsigned, WorkQueue* queue) mutable
        {
            queue->PostTaskForThread(ea::move(task), priority, threadIndex, completionToken);
        };
        // clang-format on

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
            SetupCompletionToken(internalTask, completionToken);
            internalTask->threadNum = threadIndex;
            taskScheduler_->AddPinnedTask(internalTask);
        }
        return;
    }
#endif

    if (priority == TaskPriority::Immediate || completionToken)
        task(0, this);
    else
        fallbackTaskQueue_.push_back(ea::move(task));
}

void WorkQueue::PostTaskForMainThread(TaskFunction&& task, TaskPriority priority)
{
#ifdef URHO3D_THREADING
    if (taskScheduler_ && IsProcessingThread())
    {
        PostTaskForThread(ea::move(task), priority, 0, nullptr);
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

void WorkQueue::RunTaskOnMainThread(TaskFunction&& task)
{
    if (Thread::IsMainThread())
        task(0, this);
    else
        PostDelayedTaskForMainThread(ea::move(task));
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

void WorkQueue::CompleteTask(WorkCompletionToken completionToken, TaskPriority priority)
{
#ifdef URHO3D_THREADING
    if (taskScheduler_ && completionToken && completionToken->completionFence_ && completionToken->poolTask_)
    {
        taskScheduler_->WaitforTask(completionToken->completionFence_, static_cast<enki::TaskPriority>(priority));

        URHO3D_ASSERT(completionToken->poolTask_->GetReleaseMode() == TaskReleaseMode::Manual);
        completionToken->poolTask_->Release();

        completionToken->completionFence_ = nullptr;
        completionToken->poolTask_ = nullptr;
    }
#endif
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
    URHO3D_PROFILE("CompleteWorkQueueTasks");

#ifdef URHO3D_THREADING
    if (taskScheduler_)
        taskScheduler_->WaitforAll();
#endif

    while (UpdateOnMainThread(nullptr))
    {
        // Run tasks in main thread until there is nothing to run
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

} // namespace Urho3D
