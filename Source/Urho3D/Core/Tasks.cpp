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
#include "../Core/Tasks.h"


#if URHO3D_TASKS
#if defined(_CPPUNWIND) || defined(__cpp_exceptions)
#   define URHO3D_TASKS_USE_EXCEPTIONS
#endif

#ifdef _WIN32
#   include <windows.h>
#else
#   include <ucontext.h>
#   include <csetjmp>

// Windows fiber API implementation for unix operating systems.

// Dummy value for compatibility with windows API
#define FIBER_FLAG_FLOAT_SWITCH 0
#define WINAPI

typedef void(*LPFIBER_START_ROUTINE)(void*);

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
    (void)(parameter);
    (void)(flags);
    if (threadFiberContext_.mainThreadFiber_ == nullptr)
        threadFiberContext_.mainThreadFiber_ = threadFiberContext_.currentFiber_ = new FiberContext();
    return threadFiberContext_.mainThreadFiber_;
}

void* CreateFiberEx(unsigned stackSize, unsigned _, unsigned flags, void(*startAddress)(void*), void* parameter)
{
    (void)(_);
    (void)(flags);
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

thread_local struct
{
    /// Thread fiber, used to store context of thread so fibers can resume execution to it.
    Task threadTask_{};
    /// Task that is executing at the moment.
    Task* currentTask_ = nullptr;
} taskData_;

/// Exception which causes task termination. Intentionally does not inherit std::exception to prevent user catching termination request.
class TerminateTaskException { };

Task::~Task()
{
    if (fiber_ != nullptr)
        DeleteFiber(fiber_);
}

bool Task::IsReady()
{
    if (sleepTimer_.GetMSec(false) < sleepMSec_)
        return false;

    sleepTimer_.Reset();
    sleepMSec_ = 0;
    return true;
}

void Task::ExecuteTaskWrapper(void* parameter)
{
    static_cast<Task*>(parameter)->ExecuteTask();
}

void Task::ExecuteTask()
{
    // This method must not contain objects as local variables, because it's stack will not be unwound and their
    // destructors will not be called.
    state_ = TSTATE_EXECUTING;
#ifdef URHO3D_TASKS_USE_EXCEPTIONS
    try
    {
#endif
        taskProc_();
#ifdef URHO3D_TASKS_USE_EXCEPTIONS
    }
    catch (TerminateTaskException&)
    {
        // Task was terminated by user request.
    }
#endif
    state_ = TSTATE_FINISHED;
    // Suspend one last time. This call will not return and task will be destroyed. Returning would cause a crash.
    Suspend();
    assert(false);
}

void Task::Suspend(float time)
{
#ifdef URHO3D_TASKS_USE_EXCEPTIONS
    if (state_ == TSTATE_TERMINATE)
        throw TerminateTaskException();
#endif

    // Save sleep time of current task
    taskData_.currentTask_->sleepMSec_ = static_cast<unsigned>(1000.f * time);
    // Switch to main thread task
    taskData_.currentTask_ = &taskData_.threadTask_;
    taskData_.currentTask_->SwitchTo();
}

bool Task::SwitchTo()
{
    if (threadID_ != Thread::GetCurrentThreadID())
    {
        URHO3D_LOGERROR("Task must be scheduled on the same thread where it was created.");
        return false;
    }

    if (state_ == TSTATE_FINISHED)
    {
        URHO3D_LOGERROR("Finished task may not be scheduled again.");
        return false;
    }

    taskData_.currentTask_ = this;
    SwitchToFiber(fiber_);
    return true;
}

Task* Task::GetThreadTask()
{
    Task& threadTask = taskData_.threadTask_;
    if (threadTask.fiber_ == nullptr)
    {
        threadTask.fiber_ = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
        threadTask.state_ = TSTATE_EXECUTING;
    }
    return &threadTask;
}

Task::Task(const std::function<void()>& taskFunction, unsigned int stackSize)
{
    taskProc_ = taskFunction;
    // On x86 Windows ExecuteTaskWrapper should be stdcall, but is cdecl instead. Invalid calling convention is not a
    // problem because ExecuteTaskWrapper never returns. This saves us exposing platform quirks in the header.
    fiber_ = CreateFiberEx(stackSize, stackSize, FIBER_FLAG_FLOAT_SWITCH, (LPFIBER_START_ROUTINE)&ExecuteTaskWrapper, this);
}

TaskScheduler::TaskScheduler(Context* context) : Object(context)
{
    // Initialize task for current thread.
    Task::GetThreadTask();
}

TaskScheduler::~TaskScheduler() = default;

Task* TaskScheduler::Create(const std::function<void()>& taskFunction, unsigned stackSize)
{
    SharedPtr<Task> task(new Task(taskFunction, stackSize));
    tasks_.Push(task);
    return task;
}

void TaskScheduler::ExecuteTasks()
{
    for (auto it = tasks_.Begin(); it != tasks_.End();)
    {
        Task* task = *it;

        if (!task->IsReady())
        {
            ++it;
            continue;
        }

        task->SwitchTo();

        if (task->state_ == TSTATE_FINISHED)
            it = tasks_.Erase(it);
        else
            ++it;
    }
}

unsigned TaskScheduler::GetActiveTaskCount() const
{
    return tasks_.Size();
}

void TaskScheduler::ExecuteAllTasks()
{
    while (GetActiveTaskCount() > 0)
    {
        ExecuteTasks();
        // Do not starve other threads
        Time::Sleep(0);
    }
}

void SuspendTask(float time)
{
    taskData_.currentTask_->Suspend(time);
}

Tasks::Tasks(Context* context) : Object(context)
{
}

Task* Tasks::Create(StringHash eventType, const std::function<void()>& taskFunction, unsigned stackSize)
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

unsigned Tasks::GetActiveTaskCount() const
{
    unsigned activeTasks = 0;
    for (const auto& scheduler: taskSchedulers_)
        activeTasks += scheduler.second_->GetActiveTaskCount();
    return activeTasks;
}

}
#endif
