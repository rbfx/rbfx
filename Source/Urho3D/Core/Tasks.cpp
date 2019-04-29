//
// Copyright (c) 2017-2019 Rokas Kupstys.
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

#include <EASTL/sort.h>

#include "../IO/Log.h"
#include "../Core/Context.h"
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

bool Task::Initialize(const std::function<void()>& taskFunction, unsigned int stackSize)
{
    if (!taskFunction)
    {
        URHO3D_LOGERROR("Task function must be non-null.");
        state_ = TSTATE_TERMINATE;
        return false;
    }

    function_ = taskFunction;
    stackSize_ = stackSize;
    stackOwner_.reset(new unsigned char[stackSize]);
    stack_ = stackOwner_.get();
    stackId_ = VALGRIND_STACK_REGISTER((uint8_t*)stack_ + stackSize, (uint8_t*)stack_);
    context_ = sc_context_create(stack_, stackSize_, &ExecuteTaskWrapper);
    sc_switch((sc_context_t)context_, (void*)this);
    return true;
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
    assert(state_ == TSTATE_CREATED);
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
    SuspendTask();
    assert(false);
}

void* Task::SwitchTo(void* data)
{
    if (threadID_ != Thread::GetCurrentThreadID())
    {
        URHO3D_LOGERROR("Task must be scheduled on the same thread where it was created.");
        return nullptr;
    }

    if (state_ == TSTATE_FINISHED)
    {
        URHO3D_LOGERROR("Finished task may not be scheduled again.");
        return nullptr;
    }

    return sc_switch((sc_context_t)context_, data);
}

TaskScheduler::TaskScheduler(Context* context)
    : Object(context)
{
}

TaskScheduler::~TaskScheduler() = default;

ea::shared_ptr<Task> TaskScheduler::Create(const std::function<void()>& taskFunction, unsigned stackSize)
{
    if (ea::shared_ptr<Task> task = GetTasks()->Create(taskFunction, stackSize))
    {
        Add(task.get());
        return task;
    }
    return nullptr;
}

void TaskScheduler::Add(Task* task)
{
    tasks_.push_back(ea::shared_ptr<Task>(task));   // TODO: verify!!!
}

void TaskScheduler::ExecuteTasks()
{
    // Tasks with smallest next runtime value end up at the beginning of the list. Null pointers end up at the end of
    // the list.
    ea::quick_sort(tasks_.begin(), tasks_.end(), [](const ea::shared_ptr<Task>& a, const ea::shared_ptr<Task>& b) {
        if (!a)
            return false;
        if (!b)
            return true;
        return a->nextRunTime_ < b->nextRunTime_;
    });
    // Count null pointers at the end and discard them.
    unsigned newSize = tasks_.size();
    for (auto it = tasks_.end(); it != tasks_.begin();)
    {
        if (!(*--it))
            newSize--;
        else
            break;
    }
    tasks_.resize(newSize);
    // Schedule sorted tasks.
    for (auto it = tasks_.begin(); it != tasks_.end(); it++)
    {
        Task* task = it->get();

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
    return tasks_.size();
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

void* SuspendTask(float time, void* data)
{
    auto context = sc_current_context();
    if (context == nullptr)
    {
        URHO3D_LOGERROR("Main task of current thread can not be suspended.");
        return nullptr;
    }
    auto* currentTask = ((Task*)sc_get_data(context));

#ifdef URHO3D_TASKS_USE_EXCEPTIONS
    if (currentTask->IsTerminating())
        throw TerminateTaskException();
#endif

    currentTask->SetSleep(time);
    return sc_yield(data);
}

void* SuspendTask(Task* nextTask, float time, void* data)
{
    auto context = sc_current_context();
    if (context == nullptr)
    {
        URHO3D_LOGERROR("Main task of current thread can not be suspended.");
        return nullptr;
    }
    auto* currentTask = ((Task*)sc_get_data(context));

#ifdef URHO3D_TASKS_USE_EXCEPTIONS
    if (currentTask->IsTerminating())
        throw TerminateTaskException();
#endif

    currentTask->SetSleep(time);
    if (nextTask == nullptr)
    {
        context = sc_main_context();
        return sc_switch(context, data);
    }
    else
        return nextTask->SwitchTo(data);
}

Tasks::Tasks(Context* context) : Object(context)
{
    RegisterTasksLibrary(context);
}

ea::shared_ptr<Task> Tasks::Create(const std::function<void()>& taskFunction, unsigned stackSize)
{
    ea::shared_ptr<Task> task = context_->CreateObject<Task>();
    if (task->Initialize(taskFunction, stackSize))
        return task;
    return nullptr;
}

ea::shared_ptr<Task> Tasks::Create(StringHash eventType, const std::function<void()>& taskFunction, unsigned stackSize)
{
    if (ea::shared_ptr<Task> task = Create(taskFunction, stackSize))
    {
        Add(eventType, task.get());
        return task;
    }
    return nullptr;
}

void Tasks::Add(StringHash eventType, Task* task)
{
    if (task == nullptr)
    {
        URHO3D_LOGERROR("Task must not be null.");
        return;
    }

    auto it = taskSchedulers_.find(eventType);
    ea::shared_ptr<TaskScheduler> scheduler;
    if (it == taskSchedulers_.end())
    {
        taskSchedulers_[eventType] = scheduler = context_->CreateObject<TaskScheduler>();
        SubscribeToEvent(eventType, [&](StringHash eventType_, VariantMap&) { ExecuteTasks(eventType_); });
    }
    else
        scheduler = it->second;
    scheduler->Add(task);
}

void Tasks::ExecuteTasks(StringHash eventType)
{
    auto it = taskSchedulers_.find(eventType);
    if (it == taskSchedulers_.end())
    {
        URHO3D_LOGWARNING("Tasks subsystem received event it was not supposed to handle.");
        return;
    }
    it->second->ExecuteTasks();
}

unsigned Tasks::GetActiveTaskCount() const
{
    unsigned activeTasks = 0;
    for (const auto& scheduler: taskSchedulers_)
        activeTasks += scheduler.second->GetActiveTaskCount();
    return activeTasks;
}

void RegisterTasksLibrary(Context* context)
{
    context->RegisterFactory<Task>();
    context->RegisterFactory<TaskScheduler>();
}

}
#endif
