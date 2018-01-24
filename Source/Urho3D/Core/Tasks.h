//
// Copyright (c) 2008-2017 the Urho3D project.
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
class URHO3D_API Task : public RefCounted
{
    /// Construct a task. It has to be manually scheduled by calling Task::SwitchTo(). Caller is responsible for freeing returned object after task finishes execution.
    Task(TaskScheduler* scheduler, const std::function<void()>& taskFunction, unsigned stackSize = DEFAULT_TASK_SIZE);
public:
    /// Destruct.
    ~Task() override;
    /// Return true if task is still executing.
    inline bool IsAlive() const { return state_ != TSTATE_FINISHED; };
    /// Return true if task is supposed to terminate shortly.
    inline bool IsTerminating() const { return state_ == TSTATE_TERMINATE; };
    /// Return true if task is ready, false if task is still sleeping.
    inline bool IsReady() { return nextRunTime_ <= Time::GetSystemTime(); }
    /// Suspend execution of current task. Must be called from within function invoked by callback passed to TaskScheduler::Create() or Tasks::Create().
    void Suspend(float time = 0.f);
    /// Explicitly switch execution to specified task. Task must be created on the same thread where this function is called. Task can be switched to at any time.
    bool SwitchTo();
    /// Request task termination. If exception support is disabled then user must return from the task manually when IsTerminating() returns true.
    /// If exception support is enabled then task will be terminated next time Suspend() method is called. Suspend() will throw an exception that will be caught out-most layer of the task.
    inline void Terminate() { state_ = TSTATE_TERMINATE; }

protected:
    /// Structure which holds context of previous fiber and custom user data pointer.
    struct ContextTransferData
    {
        /// Fiber context data.
        void* context;
        /// Custom user pointer.
        void* data;
    };
    /// Handles task execution. Should not be called by user.
    void ExecuteTask();
    /// Starts execution of a task using fiber API.
    static void ExecuteTaskWrapper(ContextTransferData transfer);
    /// Set context of previous task.
    void SetPreviousTaskContext(void* context);

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
    std::function<void()> taskProc_;
    /// Current state of the task.
    TaskState state_ = TSTATE_CREATED;
    /// Thread id on which task was created.
    ThreadID threadID_ = Thread::GetCurrentThreadID();
    /// Task scheduler which created this task. Null if task is manually scheduled.
    WeakPtr<TaskScheduler> scheduler_;

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

    /// Create a task and schedule it for execution.
    Task* Create(const std::function<void()>& taskFunction, unsigned stackSize = DEFAULT_TASK_SIZE);
    /// Return number of active tasks.
    unsigned GetActiveTaskCount() const;
    /// Schedule tasks created by Create() method. This has to be called periodically, otherwise tasks will not run.
    void ExecuteTasks();
    /// Schedule tasks continuously until all of them exit.
    void ExecuteAllTasks();
    /// Switch to main thread task.
    inline bool SwitchTo() { return threadTask_.SwitchTo(); }
    /// Suspend execution of current task. Must be called from within function invoked by callback passed to TaskScheduler::Create() or Tasks::Create().
    inline void SuspendTask(float time = 0.f) { current_->Suspend(time); }

private:
    /// List of tasks for every event tasks are executed on.
    Vector<SharedPtr<Task>> tasks_;
    /// Thread task which executes scheduler code.
    Task threadTask_;
    /// Current task that is being executed.
    Task* current_ = nullptr;
    /// Previous task that was executed.
    Task* previous_ = nullptr;

    friend class Task;
};

#if !URHO3D_TASKS_NO_TLS
/// Suspend execution of current task. Must be called from within function invoked by callback passed to TaskScheduler::Create() or Tasks::Create().
URHO3D_API void SuspendTask(float time = 0.f);
#endif

/// Tasks subsystem. Handles execution of tasks on the main thread.
class URHO3D_API Tasks : public Object
{
    URHO3D_OBJECT(Tasks, Object);
public:
    /// Construct.
    explicit Tasks(Context* context);
    /// Create a task and schedule it for execution.
    Task* Create(StringHash eventType, const std::function<void()>& taskFunction, unsigned stackSize = DEFAULT_TASK_SIZE);
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
