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

#include "../IO/Log.h"
#include "../Core/CoreEvents.h"
#include "../Core/Thread.h"
#include "../Core/Tasks.h"


#ifdef _WIN32
#   include <windows.h>
#else
#   include <ucontext.h>
#   include <csetjmp>

// Windows fiber API implementation for unix operating systems.

// Dummy value for compatibility with windows API
#define FIBER_FLAG_FLOAT_SWITCH 0
#define WINAPI

/// Internal fiber context data.
struct FiberContext
{
    /// Destruct.
    ~FiberContext()
    {
        delete[] stack_;
    }

    /// Current fiber context, used for switching in mid of execution.
    jmp_buf currentContext_{};
    /// Fiber stack.
    uint8_t* stack_ = nullptr;
};

/// Fiber context data required only for first context switch.
struct FiberStartContext
{
    FiberContext* fiber_;
    /// Initial fiber context, used for switching to a fiber very first time. It is responsible for setting newly allocated stack in a platform-independent way.
    ucontext_t fiberContext_;
    /// Context for resuming from fiber execution first time.
    ucontext_t resumeContext_;
    /// Fiber task function which is executed on newly allocated stack.
    void (*taskFunction_)(void*);
    /// Parameter passed to taskFunction_.
    void* taskParameter_;
};

/// Thread fiber context. Keeps track of main thread fiber context and currently executing fiber.
struct ThreadFiberContext
{
    /// Fiber that is currently executing.
    FiberContext* currentFiber_ = nullptr;
    /// Main thread fiber.
    FiberContext* mainThreadFiber_ = nullptr;
};

thread_local ThreadFiberContext threadFiberContext_;

void _FiberSetup(void* p)
{
    // Copy context to local stack because it will be destroyed after setcontext() call below.
    FiberStartContext context = *(FiberStartContext*)p;
    // Set up a task context so it can be switched by using setjmp/longjmp.
    if (_setjmp(context.fiber_->currentContext_) == 0)
    {
        // Resume back to CreateFiber() and not execute task just yet.
        setcontext(&context.resumeContext_);
    }
    // SwitchToFiber() was just called, execute actual fiber.
    context.taskFunction_(context.taskParameter_);
    assert(false);  // On windows returning from a fiber causes a thread to exit, therefore this should never be reached.
}

void* ConvertThreadToFiberEx(void* parameter, unsigned flags)
{
    if (threadFiberContext_.mainThreadFiber_ == nullptr)
        threadFiberContext_.mainThreadFiber_ = threadFiberContext_.currentFiber_ = new FiberContext();
    return threadFiberContext_.mainThreadFiber_;
}

void* CreateFiberEx(unsigned stackSize, unsigned _, unsigned flags, void(*startAddress)(void*), void* parameter)
{
    FiberStartContext startContext{};
    startContext.fiber_ = new FiberContext();
    startContext.fiber_->stack_ = new uint8_t[stackSize];
    startContext.taskFunction_ = startAddress;
    startContext.taskParameter_ = parameter;
    // Create a fiber context ucontext. This is a portable way to switch stack pointer. Be aware - costs 2 syscalls.
    getcontext(&startContext.fiberContext_);
    startContext.fiberContext_.uc_stack.ss_size = stackSize;
    startContext.fiberContext_.uc_stack.ss_sp = startContext.fiber_->stack_;
    startContext.fiberContext_.uc_link = nullptr;
    makecontext(&startContext.fiberContext_, reinterpret_cast<void (*)()>(_FiberSetup), 1, &startContext);
    // Immediately switch to a fiber. Fiber will not execute just yet, but it will set up state for SwitchToFiber() to work.
    swapcontext(&startContext.resumeContext_, &startContext.fiberContext_);
    return startContext.fiber_;
}

void SwitchToFiber(void* fiber)
{
    // _setjmp/_longjmp are used instead. They are much faster because they do not perform any syscalls.
    FiberContext* task = static_cast<FiberContext*>(fiber);
    if (_setjmp(threadFiberContext_.currentFiber_->currentContext_) == 0)
    {
        threadFiberContext_.currentFiber_ = task;
        _longjmp(task->currentContext_, 1);
    }
}

void DeleteFiber(void* fiber)
{
    FiberContext* context = static_cast<FiberContext*>(fiber);
    if (context == threadFiberContext_.mainThreadFiber_)
        threadFiberContext_.mainThreadFiber_ = threadFiberContext_.currentFiber_ = nullptr;
    else
        assert(context != threadFiberContext_.currentFiber_);
    delete context;
}
#endif

namespace Urho3D
{

enum TaskState
{
    /// Task was created, but not executed yet.
    TSTATE_CREATED,
    /// Task was switched to at least once.
    TSTATE_EXECUTING,
    /// Task finished execution and should not be resheduled.
    TSTATE_FINISHED,
};

struct TaskContext
{
    /// Destruct.
    ~TaskContext();
    /// Return true if task is ready, false if task is still sleeping.
    bool IsReady();

    /// Next task in a linked list.
    TaskContext* next_ = nullptr;
    /// Fiber context.
    void* fiber_ = nullptr;
    /// Timer which keeps track of how long task should sleep.
    Timer sleepTimer_{};
    /// Number of milliseconds task should sleep for.
    unsigned sleepMSec_ = 0;
    /// Procedure that executes the task.
    std::function<void()> taskProc_;
    /// Current state of the task.
    TaskState state_ = TSTATE_CREATED;
    /// Thread id on which task was created.
    ThreadID threadID_ = Thread::GetCurrentThreadID();
};

thread_local struct TaskData
{
    /// Destruct.
    ~TaskData()
    {
        if (threadFiber_ != nullptr)
            DeleteFiber(threadFiber_);
    }

    /// Thread fiber, used to store context of thread so fibers can resume execution to it.
    void* threadFiber_ = nullptr;
    /// Task that is executing at the moment.
    TaskContext* currentTask_ = nullptr;
} taskData_;

TaskContext::~TaskContext()
{
    DeleteFiber(fiber_);
}

bool TaskContext::IsReady()
{
    if (sleepTimer_.GetMSec(false) < sleepMSec_)
        return false;

    sleepTimer_.Reset();
    sleepMSec_ = 0;
    return true;
}

TaskScheduler::TaskScheduler(Context* context) : Object(context)
{
    if (taskData_.threadFiber_ == nullptr)
        taskData_.threadFiber_ = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
}

TaskScheduler::~TaskScheduler()
{
}

void WINAPI StartTaskExecution(void* userdata)
{
    TaskContext& task = *static_cast<TaskContext*>(userdata);
    task.state_ = TSTATE_EXECUTING;
    task.taskProc_();
    task.state_ = TSTATE_FINISHED;
    // Suspend one last time. This call will not return and task will be destroyed. Returning would cause a crash.
    SuspendTask();
    assert(false);
}

TaskContext* TaskScheduler::Create(const std::function<void()>& taskFunction, unsigned stackSize)
{
    auto task = new TaskContext();
    task->taskProc_ = taskFunction;
    task->fiber_ = CreateFiberEx(stackSize, stackSize, FIBER_FLAG_FLOAT_SWITCH, StartTaskExecution, task);
    tasks_.Insert(task);
    return task;
}

void TaskScheduler::ExecuteTasks()
{
    for (auto task = tasks_.First(); task != nullptr;)
    {
        if (!task->IsReady())
        {
            task = tasks_.Next(task);
            continue;
        }

        taskData_.currentTask_ = task;
        SwitchToFiber(task->fiber_);
        taskData_.currentTask_ = nullptr;

        if (task->state_ == TSTATE_FINISHED)
        {
            auto nextTask = tasks_.Next(task);
            tasks_.Erase(task);
            task = nextTask;
        }
        else
            task = tasks_.Next(task);
    }
}

void SuspendTask(float time)
{
    taskData_.currentTask_->sleepMSec_ = static_cast<unsigned>(1000.f * time);
    SwitchToFiber(taskData_.threadFiber_);
}

void SwitchToTask(TaskContext* task)
{
    if (task->threadID_ != Thread::GetCurrentThreadID())
        return;

    SwitchToFiber(task->fiber_);
}

Tasks::Tasks(Context* context) : Object(context)
{
}

TaskContext* Tasks::Create(StringHash eventType, const std::function<void()>& taskFunction, unsigned stackSize)
{
    auto it = taskSchedulers_.Find(eventType);
    TaskScheduler* scheduler = nullptr;
    if (it == taskSchedulers_.End())
    {
        taskSchedulers_[eventType] = scheduler = new TaskScheduler(context_);
        SubscribeToEvent(eventType, [&](StringHash eventType_, VariantMap&) { ExecuteTasks(eventType_); });
    }
    else
        scheduler = it->second_;

    return scheduler->Create(taskFunction, stackSize);
}

void Tasks::ExecuteTasks(StringHash eventType)
{
    auto it = taskSchedulers_.Find(eventType);
    if (it == taskSchedulers_.End())
    {
        URHO3D_LOGWARNING("Tasks subsystem received event it was not supposed to handle.");
        return;
    }
    it->second_->ExecuteTasks();
}

}
