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

#pragma once

#include "Urho3D/Container/MultiVector.h"
#include "Urho3D/Core/Mutex.h"
#include "Urho3D/Core/Object.h"

#include <EASTL/functional.h>
#include <EASTL/list.h>
#include <EASTL/span.h>
#include <EASTL/unique_ptr.h>

#include <atomic>

#ifdef URHO3D_THREADING
namespace enki
{
class ICompletable;
class TaskScheduler;
}
#endif

namespace Urho3D
{

class WorkQueue;

/// Priority of the task.
enum class TaskPriority
{
    /// Special priority. Tasks of Immediate priority are executed and completed on CompleteImmediateForThisThread call.
    /// @note If CompleteImmediateForThisThread is not called, "Immediate" tasks will be executed on Update.
    /// @note Tasks of other priorities should not post tasks of Immediate priority.
    Immediate,
    /// Other priorities.
    /// @{
    Highest,
    High,
    Medium,
    Low,
    /// @}
};

/// Size of small task function that doesn't need to be allocated on the heap.
static constexpr unsigned TaskBufferSize = sizeof(void*) * 8;

/// Task function signature.
/// Using internal EASTL function to get bigger storage.
using TaskFunction = ea::internal::function_detail<TaskBufferSize, void(unsigned threadIndex, WorkQueue* queue)>;
//using TaskFunction = ea::function<void(unsigned threadIndex, WorkQueue* queue)>;

/// Vector-like collection that can be safely filled from different WorkQueue threads simultaneously.
template <class T>
class WorkQueueVector : public MultiVector<T>
{
public:
    /// Clear collection, considering number of threads in WorkQueue.
    void Clear();
#ifndef SWIG
    /// Insert new element. Thread-safe as long as called from WorkQueue threads (or main thread).
    auto Insert(const T& value);
#endif
    /// Emplace element. Thread-safe as long as called from WorkQueue threads (or main thread).
    template <class ... Args>
    T& Emplace(Args&& ... args);
};

/// Deprecated.
using WorkFunction = ea::function<void(unsigned threadIndex)>;

/// Deprecated.
struct WorkItem : public RefCounted
{
    friend class WorkQueue;

public:
    /// Work function. Called with the work item and thread index (0 = main thread) as parameters.
    void (* workFunction_)(const WorkItem*, unsigned){};
    /// Data start pointer.
    void* start_{};
    /// Data end pointer.
    void* end_{};
    /// Auxiliary data pointer.
    void* aux_{};
    /// Priority. Higher value = will be completed first.
    unsigned priority_{};

private:
    /// Work function. Called without any parameters.
    WorkFunction workLambda_;
};

/// Work queue subsystem for multithreading.
/// Tasks can be posted from any thread, but it is the most efficient from main thread or worker threads.
/// Tasks posted from other threads may be posted with up to one frame delay and require heap allocation.
class URHO3D_API WorkQueue : public Object
{
    URHO3D_OBJECT(WorkQueue, Object);

    friend class WorkerThread;

public:
    /// Construct.
    explicit WorkQueue(Context* context);
    /// Destruct.
    ~WorkQueue() override;

    /// Create worker threads. Can only be called once.
    void Initialize(unsigned numThreads);
    /// Do work in main thread. Usually called once per frame.
    void Update();

    /// Post the task for any processing thread.
    void PostTask(TaskFunction&& task, TaskPriority priority = TaskPriority::Medium);
    template <class T> void PostTask(T task, TaskPriority priority = TaskPriority::Medium);
    /// Post the task for specified processing thread.
    void PostTaskForThread(TaskFunction&& task, TaskPriority priority, unsigned threadIndex);
    template <class T> void PostTaskForThread(T task, TaskPriority priority, unsigned threadIndex);
    /// Post the task for main thread.
    void PostTaskForMainThread(TaskFunction&& task, TaskPriority priority = TaskPriority::Medium);
    template <class T> void PostTaskForMainThread(T task, TaskPriority priority = TaskPriority::Medium);
    /// Post delayed task for main thread. It is guaranteed to be invoked between frames.
    void PostDelayedTaskForMainThread(TaskFunction&& task);
    template <class T> void PostDelayedTaskForMainThread(T task);

    /// Complete tasks with Immediate priority, posted from this thread.
    /// Can be called only from main thread or from another task.
    void CompleteImmediateForThisThread();
    /// Wait for completion of all tasks.
    /// Should be called only from main thread.
    void CompleteAll();

    /// Return number of incomplete tasks.
    unsigned GetNumIncomplete() const;
    /// Return whether all work is finished.
    bool IsCompleted() const;

    /// Set how many milliseconds maximum per frame to spend on low-priority work, when there are no worker threads.
    void SetNonThreadedWorkMs(int ms) { maxNonThreadedWorkMs_ = ea::max(ms, 1); }
    /// Return how many milliseconds maximum to spend on non-threaded low-priority work.
    int GetNonThreadedWorkMs() const { return maxNonThreadedWorkMs_; }

    /// Return total number of threads processing tasks, including main thread.
    unsigned GetNumProcessingThreads() const { return numProcessingThreads_; }
    /// Return whether the queue is actually using multithreading.
    bool IsMultithreaded() const { return numProcessingThreads_ > 1; }

    /// Return current thread index.
    static unsigned GetThreadIndex();
    /// Return number of threads used by WorkQueue, including main thread. Current thread index is always lower.
    static unsigned GetThreadIndexCount();
    /// Return whether current thread is one of processing threads.
    static bool IsProcessingThread();

    /// Deprecated API.
    /// @{
    void CallFromMainThread(WorkFunction workFunction);
    void Complete(unsigned priority);

    SharedPtr<WorkItem> GetFreeItem();
    void AddWorkItem(const SharedPtr<WorkItem>& item);
    SharedPtr<WorkItem> AddWorkItem(WorkFunction workFunction, unsigned priority = 0);
    /// @}

private:
    void ProcessPostedTasks();
    void ProcessMainThreadTasks();
    void PurgeProcessedTasksInFallbackQueue();
    void CompleteImmediateForAnotherThread(unsigned threadIndex);

    template <class T> static TaskFunction WrapTask(T&& task);

#ifdef URHO3D_THREADING
    template <class T> void SetupInternalTask(T* internalTask, TaskFunction&& task, TaskPriority priority);

    /// Thread-safe pool for infrequent low-priority tasks.
    template <class T>
    class TaskPool : public NonCopyable
    {
    public:
        TaskPool();
        ~TaskPool();
        T* Allocate();
        void Release(T* item);
        unsigned GetNumUsed() const;

    private:
        ea::vector<ea::unique_ptr<T>> pool_;
        ea::vector<T*> freeItems_;
        mutable Mutex mutex_;
    };

    /// Thread-unsafe stack for frequent high-priority tasks.
    template <class T>
    class TaskStack : public MovableNonCopyable
    {
    public:
        TaskStack();
        ~TaskStack();
        TaskStack(TaskStack&& other);
        TaskStack& operator=(TaskStack&& other);

        T* Allocate();
        void Purge();

    private:
        ea::vector<ea::unique_ptr<T>> pool_;
        unsigned nextFreeIndex_{};
    };

    class InternalTaskInPool;
    class InternalPinnedTaskInPool;
    class InternalTaskInStack;
    class InternalPinnedTaskInStack;
    class TaskCompletionObserver;

    /// Task scheduler.
    ea::unique_ptr<enki::TaskScheduler> taskScheduler_;

    TaskPool<InternalTaskInPool> globalTaskPool_;
    TaskPool<InternalPinnedTaskInPool> globalPinnedTaskPool_;
    ea::vector<TaskStack<InternalTaskInStack>> localTaskStack_;
    ea::vector<TaskStack<InternalPinnedTaskInStack>> localPinnedTaskStack_;

    ea::vector<ea::vector<InternalTaskInStack*>> pendingImmediateTasks_;
#endif

    /// Task queue used for fallback if no threads available.
    /// Used only for PostTask and PostTaskForThread, therefore no need for mutex.
    ea::vector<ea::pair<TaskPriority, TaskFunction>> fallbackTaskQueue_;

    /// Total number of threads, including main thread.
    unsigned numProcessingThreads_{};

    /// Tasks to be invoked from main thread.
    ea::vector<TaskFunction> mainThreadTasks_;
    ea::vector<TaskFunction> mainThreadTasksSwap_;
    Mutex mainThreadTasksMutex_;

    /// Maximum milliseconds per frame to spend on low-priority work, when there are no worker threads.
    int maxNonThreadedWorkMs_{5};
};

/// Process arbitrary array in multiple threads. Callback is copied internally.
/// One copy of callback is always used by at most one thread.
/// One copy of callback is always invoked from smaller to larger indices.
/// Signature of callback: void(unsigned beginIndex, unsigned endIndex)
template <class Callback>
void ForEachParallel(WorkQueue* workQueue, unsigned bucket, unsigned size, Callback callback)
{
    // Just call in main thread
    if (size <= bucket)
    {
        if (size > 0)
            callback(0, size);
        return;
    }

    std::atomic<unsigned> offset = 0;
    const unsigned maxThreads = workQueue->GetNumProcessingThreads();
    for (unsigned i = 0; i < maxThreads; ++i)
    {
        workQueue->PostTask([=, &offset]() mutable
        {
            while (true)
            {
                const unsigned beginIndex = offset.fetch_add(bucket, std::memory_order_relaxed);
                if (beginIndex >= size)
                    break;

                const unsigned endIndex = ea::min(beginIndex + bucket, size);
                callback(beginIndex, endIndex);
            }
        }, TaskPriority::Immediate);
    }
    workQueue->CompleteImmediateForThisThread();
}

/// Process collection in multiple threads.
/// Signature of callback: void(unsigned index, T&& element)
template <class Callback, class Collection>
void ForEachParallel(WorkQueue* workQueue, unsigned bucket, Collection&& collection, const Callback& callback)
{
    using namespace ea;
    const auto collectionSize = static_cast<unsigned>(size(collection));
    ForEachParallel(workQueue, bucket, collectionSize,
        [iter = begin(collection), iterIndex = 0u, &callback](unsigned beginIndex, unsigned endIndex) mutable
    {
        iter += beginIndex - iterIndex;
        for (iterIndex = beginIndex; iterIndex < endIndex; ++iterIndex, ++iter)
            callback(iterIndex, *iter);
    });
}

/// Process collection in multiple threads with default bucket size.
template <class Callback, class Collection>
void ForEachParallel(WorkQueue* workQueue, Collection&& collection, const Callback& callback)
{
    ForEachParallel(workQueue, 1u, collection, callback);
}

/// WorkQueueVector implementation
/// @{
template <class T>
void WorkQueueVector<T>::Clear()
{
    MultiVector<T>::Clear(WorkQueue::GetThreadIndexCount());
}

#ifndef SWIG
template <class T>
auto WorkQueueVector<T>::Insert(const T& value)
{
    const unsigned threadIndex = WorkQueue::GetThreadIndex();
    return this->PushBack(threadIndex, value);
}
#endif

template <class T>
template <class ... Args>
T& WorkQueueVector<T>::Emplace(Args&& ... args)
{
    const unsigned threadIndex = WorkQueue::GetThreadIndex();
    return this->EmplaceBack(threadIndex, std::forward<Args>(args)...);
}
/// @}

/// WorkQueue implementation
/// @{
template <class T> TaskFunction WorkQueue::WrapTask(T&& task)
{
    static constexpr bool hasIndexQueue = ea::is_invocable_r_v<void, T, unsigned, WorkQueue*>;
    static constexpr bool hasIndex = ea::is_invocable_r_v<void, T, unsigned>;
    static constexpr bool hasQueue = ea::is_invocable_r_v<void, T, WorkQueue*>;
    static constexpr bool hasNone = ea::is_invocable_r_v<void, T>;
    static_assert(hasIndexQueue || hasIndex || hasQueue || hasNone, "Invalid task signature");

    if constexpr (hasIndexQueue)
        return TaskFunction{ea::move(task)};
    else if constexpr (hasIndex)
        return [task = ea::move(task)](unsigned threadIndex, WorkQueue*) mutable { task(threadIndex); };
    else if constexpr (hasQueue)
        return [task = ea::move(task)](unsigned, WorkQueue* workQueue) mutable { task(workQueue); };
    else
        return [task = ea::move(task)](unsigned, WorkQueue*) mutable { task(); };
}

template <class T> void WorkQueue::PostTask(T task, TaskPriority priority)
{
    if (!IsProcessingThread())
    {
        auto wrappedTask = [task = ea::move(task), priority](WorkQueue* workQueue) mutable
        { workQueue->PostTask(ea::move(task), priority); };

        PostTaskForMainThread(ea::move(wrappedTask), priority);
        return;
    }

    PostTask(WrapTask(ea::move(task)), priority);
}

template <class T> void WorkQueue::PostTaskForThread(T task, TaskPriority priority, unsigned threadIndex)
{
    if (!IsProcessingThread())
    {
        auto wrappedTask = [task = ea::move(task), priority, threadIndex](WorkQueue* workQueue) mutable
        { workQueue->PostTaskForThread(ea::move(task), priority, threadIndex); };

        PostTaskForMainThread(ea::move(wrappedTask), priority);
        return;
    }

    PostTaskForThread(WrapTask(ea::move(task)), priority, threadIndex);
}

template <class T> void WorkQueue::PostTaskForMainThread(T task, TaskPriority priority)
{
    PostTaskForMainThread(WrapTask(ea::move(task)), priority);
}

template <class T> void WorkQueue::PostDelayedTaskForMainThread(T task)
{
    PostDelayedTaskForMainThread(WrapTask(ea::move(task)));
}

}
