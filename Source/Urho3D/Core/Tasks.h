//
// Copyright (c) 2018 Rokas Kupstys
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


#include "../Container/ArrayPtr.h"
#include "../Core/Object.h"
#include "../Core/Timer.h"
#include "../Core/Thread.h"
#include "../Container/List.h"


#if URHO3D_TASKS
namespace Urho3D
{

class TaskScheduler;
class Tasks;

enum TaskState
{
    /// Task was created, but not executed yet.
    TSTATE_CREATED,
    /// Task was switched to at least once.
    TSTATE_EXECUTING,
    /// Task finished execution and should not be rescheduled.
    TSTATE_FINISHED,
    /// Task termination was requested.
    TSTATE_TERMINATE,
};

/// Default task size.
static const unsigned DEFAULT_TASK_SIZE = 1024 * 64;

/// Object representing a single cooperative t
class URHO3D_API Task : public Object
{
    URHO3D_OBJECT(Task, Object);
private:
    /// Non-copyable.
    Task(const Task& other) = delete;
    /// Non-movable.
    Task(Task& other) = delete;
public:
    /// Construct.
    explicit Task(Context* context) : Object(context) { }
    /// Construct.
    explicit Task(Context* context, const std::function<void()>& taskFunction, unsigned stackSize = DEFAULT_TASK_SIZE)
        : Object(context)
    {
        Initialize(function_, stackSize);
    }

    /// Destruct.
    ~Task() override;
    /// Return true if task is still executing.
    inline bool IsAlive() const { return state_ != TSTATE_FINISHED; };
    /// Return true if task is supposed to terminate shortly.
    inline bool IsTerminating() const { return state_ == TSTATE_TERMINATE; };
    /// Return true if task is ready, false if task is still sleeping.
    inline bool IsReady() { return nextRunTime_ <= Time::GetSystemTime(); }
    /// Explicitly switch execution to specified task. Task must be created on the same thread where this function is
    /// called. Task can be switched to at any time. `data` pointer will be returned by Suspend()/SwitchTo() of next executing task.
    void* SwitchTo(void* data = nullptr);
    /// Request task termination. If exception support is disabled then user must return from the task manually when IsTerminating() returns true.
    /// If exception support is enabled then task will be terminated next time Suspend() method is called. Suspend() will throw an exception that will be caught out-most layer of the task.
    inline void Terminate() { state_ = TSTATE_TERMINATE; }
    /// Set how long task should sleep until next time it yields execution.
    inline void SetSleep(float time) { nextRunTime_ = Time::GetSystemTime() + static_cast<unsigned>(1000.f * time); }

protected:
    /// Construct a task. It has to be manually scheduled by calling Task::SwitchTo(). Caller is responsible for freeing returned object after task finishes execution.
    bool Initialize(const std::function<void()>& taskFunction, unsigned stackSize = DEFAULT_TASK_SIZE);
    /// Handles task execution. Should not be called by user.
    void ExecuteTask();
    /// Starts execution of a task using fiber API.
    static void ExecuteTaskWrapper(void* task);

    /// Fiber context.
    void* context_ = nullptr;
    /// Fiber stack.
    void* stack_ = nullptr;
    /// Fiber stack size.
    size_t stackSize_ = 0;
    /// Valgrind stack id.
    size_t stackId_ = 0;
    /// Time when task should schedule again.
    unsigned nextRunTime_ = 0;
    /// Procedure that executes the task.
    std::function<void()> function_;
    /// Current state of the task.
    TaskState state_ = TSTATE_CREATED;
    /// Thread id on which task was created.
    ThreadID threadID_ = Thread::GetCurrentThreadID();
    /// Object that owns allocated stack memory.
    SharedArrayPtr<unsigned char> stackOwner_;

    friend class TaskScheduler;
    friend class Tasks;
};

/// Task scheduler used for scheduling concurrent tasks.
class URHO3D_API TaskScheduler : public Object
{
    URHO3D_OBJECT(TaskScheduler, Object);
public:
    /// Construct.
    explicit TaskScheduler(Context* context);
    /// Destruct.
    ~TaskScheduler() override;

    /// Create a task and schedule it for execution in specified event.
    SharedPtr<Task> Create(const std::function<void()>& taskFunction, unsigned stackSize = DEFAULT_TASK_SIZE);
    /// Schedule task for execution.
    void Add(Task* task);
    /// Return number of active tasks.
    unsigned GetActiveTaskCount() const;
    /// Schedule tasks created by Create() method. This has to be called periodically, otherwise tasks will not run.
    void ExecuteTasks();
    /// Schedule tasks continuously until all of them exit.
    void ExecuteAllTasks();

private:
    /// List of tasks for every event tasks are executed on.
    Vector<SharedPtr<Task>> tasks_;

    friend class Task;
};

/// Suspend execution of current task. Must be called from within function invoked by callback passed to
/// TaskScheduler::Create() or Tasks::Create().
/// `data` pointer will be returned by Suspend()/SwitchTo() of next executing task.
URHO3D_API void* SuspendTask(float time = 0.f, void* data = nullptr);
/// Switch execution to another task. If task pointer is null then execution will be switched to main task of current thread.
URHO3D_API void* SuspendTask(Task* nextTask, float time = 0.f, void* data = nullptr);

/// Tasks subsystem. Handles execution of tasks on the main thread.
class URHO3D_API Tasks : public Object
{
    URHO3D_OBJECT(Tasks, Object);
public:
    /// Construct.
    explicit Tasks(Context* context);
    /// Create a task and schedule it for execution.
    SharedPtr<Task> Create(const std::function<void()>& taskFunction, unsigned stackSize = DEFAULT_TASK_SIZE);
    /// Create a task and schedule it for execution in specified event.
    SharedPtr<Task> Create(StringHash eventType, const std::function<void()>& taskFunction, unsigned stackSize = DEFAULT_TASK_SIZE);
    /// Scheduled task for execution in specified event.
    void Add(StringHash eventType, Task* task);
    /// Return number of active tasks.
    unsigned GetActiveTaskCount() const;

private:
    /// Schedule tasks created by Create() method.
    void ExecuteTasks(StringHash eventType);

    /// Task schedulers for each scene event.
    HashMap<StringHash, SharedPtr<TaskScheduler> > taskSchedulers_;
};

};
#endif
