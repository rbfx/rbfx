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

#include <fcontext/fcontext.h>

#if defined(HAVE_VALGRIND)
#   include <valgrind/valgrind.h>
#else
#   define VALGRIND_STACK_REGISTER(s, e) 0
#   define VALGRIND_STACK_DEREGISTER(id)
#endif
namespace Urho3D
{

/// Exception which causes task termination. Intentionally does not inherit std::exception to prevent user catching termination request.
class TerminateTaskException { };

#if !URHO3D_TASKS_NO_TLS
thread_local Task* currentTask_ = nullptr;
#endif

void Task::ExecuteTaskWrapper(ContextTransferData transfer)
{
    auto* task = static_cast<Task*>(transfer.data);
    task->SetPreviousTaskContext(transfer.context);
    task->ExecuteTask();
}

Task::Task(TaskScheduler* scheduler, const std::function<void()>& taskFunction, unsigned int stackSize)
    : scheduler_(scheduler)
    , taskProc_(taskFunction)
{
    if (taskFunction)
    {
        fcontext_stack_t stack = create_fcontext_stack(stackSize);
        stack_ = stack.sptr;
        stackSize_ = stack.ssize;
        stackId_ = VALGRIND_STACK_REGISTER((uint8_t*)stack_ + stackSize, (uint8_t*)stack_);
        context_ = make_fcontext(stack_, stackSize_, reinterpret_cast<pfn_fcontext>(&ExecuteTaskWrapper));
    }
}

Task::~Task()
{
    if (stack_)
    {
        VALGRIND_STACK_DEREGISTER(stackId_);
        fcontext_stack_t stack{stack_, stackSize_};
        destroy_fcontext_stack(&stack);
    }
}

void Task::SetPreviousTaskContext(void* context)
{
    if (!scheduler_.Expired())
        scheduler_->previous_->context_ = context;
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
    if (scheduler_.Expired())
    {
        URHO3D_LOGERROR("Manually scheduled tasks can not be suspended.");
        return;
    }

#ifdef URHO3D_TASKS_USE_EXCEPTIONS
    if (state_ == TSTATE_TERMINATE)
        throw TerminateTaskException();
#endif

    nextRunTime_ = Time::GetSystemTime() + static_cast<unsigned>(1000.f * time);
    scheduler_->threadTask_.SwitchTo();
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

    if (!scheduler_.Expired())
    {
        scheduler_->previous_ = scheduler_->current_;
        scheduler_->current_ = this;
#if !URHO3D_TASKS_NO_TLS
        currentTask_ = this;
#endif
    }

    SetPreviousTaskContext(jump_fcontext(context_, this).ctx);
    return true;
}

TaskScheduler::TaskScheduler(Context* context)
    : Object(context)
    , threadTask_(this, nullptr)
{
    threadTask_.state_ = TSTATE_EXECUTING;
    threadTask_.scheduler_ = this;
    current_ = currentTask_ = &threadTask_;
    previous_ = &threadTask_;
}

TaskScheduler::~TaskScheduler() = default;

Task* TaskScheduler::Create(const std::function<void()>& taskFunction, unsigned stackSize)
{
    SharedPtr<Task> task(new Task(this, taskFunction, stackSize));
    tasks_.Push(task);
    return task;
}

void TaskScheduler::ExecuteTasks()
{
    // Tasks with smallest next runtime value end up at the beginning of the list. Null pointers end up at the end of
    // the list.
    Sort(tasks_.Begin(), tasks_.End(), [](SharedPtr<Task>& a, SharedPtr<Task>& b) {
        if (a.Null())
            return false;
        if (b.Null())
            return true;
        return a->nextRunTime_ < b->nextRunTime_;
    });
    // Count null pointers at the end and discard them.
    unsigned newSize = tasks_.Size();
    for (auto it = tasks_.End(); it != tasks_.Begin();)
    {
        if (!(*--it))
            newSize--;
        else
            break;
    }
    tasks_.Resize(newSize);
    // Schedule sorted tasks.
    for (auto it = tasks_.Begin(); it != tasks_.End(); it++)
    {
        Task* task = *it;

        // Any further pointers will be to objects that are not ready therefore early exit is ok.
        if (!task->IsReady())
            break;

        task->SwitchTo();

        if (task->state_ == TSTATE_FINISHED)
            *it = nullptr;
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

#if !URHO3D_TASKS_NO_TLS
void SuspendTask(float time)
{
    currentTask_->Suspend(time);
}
#endif

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
