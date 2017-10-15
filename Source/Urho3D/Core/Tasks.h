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


namespace Urho3D
{

struct TaskContext;

/// Default task size.
static const unsigned DEFAULT_TASK_SIZE = 1024 * 5;

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
    TaskContext* Create(const std::function<void()>& taskFunction, unsigned stackSize = DEFAULT_TASK_SIZE);
    /// Schedule tasks created by Create() method. This has to be called periodically, otherwise tasks will not run.
    void ExecuteTasks();

private:
    /// List of tasks for every event tasks are executed on.
    LinkedList<TaskContext> tasks_;
};

/// Suspend execution of current task. Must be called from within function invoked by callback passed to TaskScheduler::Create() or Tasks::Create().
URHO3D_API void SuspendTask(float time = 0.f);
/// Explicitly switch execution to specified task. Task must be created on the same thread where this function is called. Task can be switched to at any time.
URHO3D_API void SwitchToTask(TaskContext* task);


/// Tasks subsystem. Handles execution of tasks on the main thread.
class URHO3D_API Tasks : public Object
{
    URHO3D_OBJECT(Tasks, Object);
public:
    /// Construct.
    explicit Tasks(Context* context);
    /// Create a task and schedule it for execution.
    TaskContext* Create(StringHash eventType, const std::function<void()>& taskFunction, unsigned stackSize = DEFAULT_TASK_SIZE);

private:
    /// Schedule tasks created by Create() method.
    void ExecuteTasks(StringHash eventType);

    /// Task schedulers for each scene event.
    HashMap<StringHash, SharedPtr<TaskScheduler> > taskSchedulers_;
};

};
