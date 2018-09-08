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

#include "../IO/Log.h"
#include "../Core/CoreEvents.h"
#include "../Core/Tasks.h"
#include "Tasks.h"


#if URHO3D_TASKS
#if defined(_CPPUNWIND) || defined(__cpp_exceptions)
#   define URHO3D_TASKS_USE_EXCEPTIONS
#endif

#include <sc/sc.h>

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

void Task::ExecuteTaskWrapper(void* task)
{
    sc_yield(nullptr);
    ((Task*)task)->ExecuteTask();
}

Task::Task(const std::function<void()>& taskFunction, unsigned int stackSize)
    : function_(taskFunction)
    , stackSize_(stackSize)
{
    if (taskFunction)
    {
        stackOwner_ = new unsigned char[stackSize];
        stack_ = stackOwner_.Get();
        stackId_ = VALGRIND_STACK_REGISTER((uint8_t*)stack_ + stackSize, (uint8_t*)stack_);
        context_ = sc_context_create(stack_, stackSize_, &ExecuteTaskWrapper);
        sc_switch((sc_context_t)context_, (void*)this);
    }
}

Task::~Task()
{
    if (stack_)
        VALGRIND_STACK_DEREGISTER(stackId_);
}

void Task::ExecuteTask()
{
    // This method must not contain objects as local variables, because it's stack will not be unwound and their
    // destructors will not be called.
    state_ = TSTATE_EXECUTING;
    sc_set_data(sc_current_context(), (void*)this);
#ifdef URHO3D_TASKS_USE_EXCEPTIONS
    try
    {
#endif
        function_();
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

    nextRunTime_ = Time::GetSystemTime() + static_cast<unsigned>(1000.f * time);
    sc_yield(nullptr);
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

    sc_switch((sc_context_t)context_, nullptr);
    return true;
}

TaskScheduler::TaskScheduler(Context* context)
    : Object(context)
{
}

TaskScheduler::~TaskScheduler() = default;

Task* TaskScheduler::Create(const std::function<void()>& taskFunction, unsigned stackSize)
{
    SharedPtr<Task> task(new Task(taskFunction, stackSize));
    tasks_.Push(task);
    return task.Get();
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

bool TaskScheduler::SwitchTo() const
{
    sc_switch(sc_main_context(), nullptr);
    return true;
}

void SuspendTask(float time)
{
    ((Task*)sc_get_data(sc_current_context()))->Suspend(time);
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
