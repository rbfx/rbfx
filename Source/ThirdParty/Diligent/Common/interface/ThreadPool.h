/*
 *  Copyright 2024 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

#include "../../Primitives/interface/Object.h"
#include "../../Primitives/interface/BasicTypes.h"

// clang-format off

DILIGENT_BEGIN_NAMESPACE(Diligent)

/// Asynchronous task status
DILIGENT_TYPED_ENUM(ASYNC_TASK_STATUS, Uint32)
{
    /// The asynchronous task status is unknown.
    ASYNC_TASK_STATUS_UNKNOWN,

    /// The asynchronous task has not been started yet.
    ASYNC_TASK_STATUS_NOT_STARTED,

    /// The asynchronous task is running.
    ASYNC_TASK_STATUS_RUNNING,

    /// The asynchronous task was cancelled.
    ASYNC_TASK_STATUS_CANCELLED,

    /// The asynchronous task is complete.
    ASYNC_TASK_STATUS_COMPLETE
};


// {B06D1DDA-AEA0-4CFD-969A-C8E2011DC294}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_AsyncTask =
    {0xb06d1dda, 0xaea0, 0x4cfd, {0x96, 0x9a, 0xc8, 0xe2, 0x1, 0x1d, 0xc2, 0x94}};


#define DILIGENT_INTERFACE_NAME IAsyncTask
#include "../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IAsyncTaskInclusiveMethods \
    IObjectInclusiveMethods;       \
    IAsyncTaskMethods AsyncTask

/// Asynchronous task interface
DILIGENT_BEGIN_INTERFACE(IAsyncTask, IObject)
{
    /// Run the asynchronous task.

    /// \param [in] ThreadId - Id of the thread that is running this task.
    ///
    /// \remarks    Before starting the task, the thread pool sets its
    ///             status to ASYNC_TASK_STATUS_RUNNING.
    ///
    ///             The method must return one of the following values:
    ///               - ASYNC_TASK_STATUS_CANCELLED to indicate that the task was cancelled.
    ///               - ASYNC_TASK_STATUS_COMPLETE to indicate that the task is finished successfully.
    ///               - ASYNC_TASK_STATUS_NOT_STARTED to request the task to be rescheduled.
    ///
    ///             The thread pool will set the task status to the returned value after
    ///             the Run() method completes. This way if the GetStatus() method returns
    ///             any value other than ASYNC_TASK_STATUS_RUNNING, it is guaranteed that the task
    ///             is not executed by any thread.
    VIRTUAL ASYNC_TASK_STATUS METHOD(Run)(THIS_
                                     Uint32 ThreadId) PURE;

    /// Cancel the task, if possible.
    ///
    /// \remarks    If the task is running, the task implementation should
    ///             abort the task execution, if possible.
    VIRTUAL void METHOD(Cancel)(THIS) PURE;

    /// Sets the task status, see Diligent::ASYNC_TASK_STATUS.
    VIRTUAL void METHOD(SetStatus)(THIS_
                                   ASYNC_TASK_STATUS TaskStatus) PURE;

    /// Gets the task status, see Diligent::ASYNC_TASK_STATUS.
    VIRTUAL ASYNC_TASK_STATUS METHOD(GetStatus)(THIS) CONST PURE;

    /// Sets the task priorirty.
    VIRTUAL void METHOD(SetPriority)(THIS_
                                    float fPriority) PURE;

    /// Returns the task priorirty.
    VIRTUAL float METHOD(GetPriority)(THIS) CONST PURE;

    /// Checks if the task is finished (i.e. cancelled or complete).
    VIRTUAL bool METHOD(IsFinished)(THIS) CONST PURE;

    /// Waits until the task is complete.
    ///
    /// \note   This method must not be called from the same thread that is
    ///         running the task or a deadlock will occur.
    VIRTUAL void METHOD(WaitForCompletion)(THIS) CONST PURE;

    /// Waits until the tasks is running.
    ///
    /// \warning  An application is responsible to make sure that
    ///           tasks currently in the queue will eventually finish
    ///           allowing the task to start.
    ///
    ///           This method must not be called from the worker thread.
    VIRTUAL void METHOD(WaitUntilRunning)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IAsyncTask_Run(This, ...)          CALL_IFACE_METHOD(AsyncTask, Run, This, __VA_ARGS__)
#    define IAsyncTask_Cancel(This)            CALL_IFACE_METHOD(AsyncTask, Cancel, This)
#    define IAsyncTask_SetStatus(This, ...)    CALL_IFACE_METHOD(AsyncTask, SetStatus, This, __VA_ARGS__)
#    define IAsyncTask_GetStatus(This)         CALL_IFACE_METHOD(AsyncTask, GetStatus, This)
#    define IAsyncTask_SetPriority(This, ...)  CALL_IFACE_METHOD(AsyncTask, SetPriority, This, __VA_ARGS__)
#    define IAsyncTask_GetPriority(This)       CALL_IFACE_METHOD(AsyncTask, GetPriority, This)
#    define IAsyncTask_IsFinished(This)        CALL_IFACE_METHOD(AsyncTask, IsFinished, This)
#    define IAsyncTask_WaitForCompletion(This) CALL_IFACE_METHOD(AsyncTask, WaitForCompletion, This)
#    define IAsyncTask_WaitUntilRunning(This)  CALL_IFACE_METHOD(AsyncTask, WaitUntilRunning, This)

#endif


// {8BB92B5E-3EAB-4CC3-9DA2-5470DBBA7120}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_ThreadPool =
    {0x8bb92b5e, 0x3eab, 0x4cc3, {0x9d, 0xa2, 0x54, 0x70, 0xdb, 0xba, 0x71, 0x20}};

#define DILIGENT_INTERFACE_NAME IThreadPool
#include "../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IThreadPoolInclusiveMethods \
    IObjectInclusiveMethods;        \
    IThreadPoolMethods ThreadPool

/// Thread pool interface
DILIGENT_BEGIN_INTERFACE(IThreadPool, IObject)
{
    /// Enqueues asynchronous task for execution.

    /// \param[in] pTask            - Task to run.
    /// \param[in] ppPrerequisites  - Array of task prerequisites, e.g. the tasks
    ///                               that must be completed before this task can start.
    /// \param[in] NumPrerequisites - Number of prerequisites.
    ///
    /// \remarks    Thread pool will keep a strong reference to the task,
    ///             so an application is free to release it after enqueuing.
    /// 
    /// \note       An application must ensure that the task prerequisites are not circular
    ///             to avoid deadlocks.
    VIRTUAL void METHOD(EnqueueTask)(THIS_
                                     IAsyncTask*  pTask,
                                     IAsyncTask** ppPrerequisites  DEFAULT_VALUE(nullptr),
                                     Uint32       NumPrerequisites DEFAULT_VALUE(0)) PURE;


    /// Reprioritizes the task in the queue.

    /// \param[in] pTask - Task to reprioritize.
    ///
    /// \return     true if the task was found in the queue and was
    ///             successfully reprioritized, and false otherwise.
    ///
    /// \remarks    When the tasks is enqueued, its priority is used to
    ///             place it in the priority queue. When an application changes
    ///             the task priority, it should call this method to update the task
    ///             position in the queue.
    VIRTUAL bool METHOD(ReprioritizeTask)(THIS_
                                          IAsyncTask* pTask) PURE;


    /// Reprioritizes all tasks in the queue.

    /// \remarks    This method should be called if task priorities have changed
    ///             to update the positions of all tasks in the queue.
    VIRTUAL void METHOD(ReprioritizeAllTasks)(THIS) PURE;


    /// Removes the task from the queue, if possible.

    /// \param[in] pTask - Task to remove from the queue.
    ///
    /// \return    true if the task was successfully removed from the queue,
    ///            and false otherwise.
    VIRTUAL bool METHOD(RemoveTask)(THIS_
                                    IAsyncTask* pTask) PURE;


    /// Waits until all tasks in the queue are finished.

    /// \remarks    The method blocks the calling thread until all
    ///             tasks in the quque are finished and the queue is empty.
    ///             An application is responsible to make sure that all tasks
    ///             will finish eventually.
    VIRTUAL void METHOD(WaitForAllTasks)(THIS) PURE;


    /// Returns the current queue size.
    VIRTUAL Uint32 METHOD(GetQueueSize)(THIS) PURE;

    /// Returns the number of currently running tasks
    VIRTUAL Uint32 METHOD(GetRunningTaskCount)(THIS) CONST PURE;


    /// Stops all worker threads.

    /// \note   This method makes all worker threads to exit.
    ///         If an application enqueues tasks after calling this methods,
    ///         this tasks will never run.
    VIRTUAL void METHOD(StopThreads)(THIS) PURE;


    /// Manually processes the next task from the queue.

    /// \param[in] ThreadId    - Id of the thread that is running this task.
    /// \param[in] WaitForTask - whether the function should wait for the next task:
    ///                          - if true, the function will block the thread until the next task
    ///                            is retrieved from the queue and processed.
    ///                          - if false, the function will return immediately if there are no
    ///                            tasks in the queue.
    ///
    /// \return     Whether there are more tasks to process. The calling thread must keep
    ///             calling the function until it returns false.
    ///
    /// \remarks    This method allows an application to implement its own threading strategy.
    ///             A thread pool may be created with zero threads, and the application may call
    ///             ProcessTask() method from its own threads.
    ///
    ///             An application must keep calling the method until it returns false.
    ///             If there are unhandled tasks in the queue and the application stops processing
    ///             them, the thread pool will hang up.
    ///
    ///             An example of handling the tasks is shown below:
    ///
    ///                 // Initialization
    ///                 auto pThreadPool = CreateThreadPool(ThreadPoolCreateInfo{0});
    ///
    ///                 std::vector<std::thread> WorkerThreads(4);
    ///                 for (Uint32 i PURE; i < WorkerThreads.size(); ++i)
    ///                 {
    ///                     WorkerThreads[i] = std::thread{
    ///                         [&ThreadPool = *pThreadPool, i] //
    ///                         {
    ///                             while (ThreadPool.ProcessTask(i, true))
    ///                             {
    ///                             }
    ///                         }};
    ///                 }
    ///
    ///                 // Enqueue async tasks
    ///
    ///                 pThreadPool->WaitForAllTasks();
    ///
    ///                 // Stop all threads in the pool
    ///                 pThreadPool->StopThreads();
    ///
    ///                 // Cleanup (must be done after all threads are stopped)
    ///                 for (auto& Thread : WorkerThreads)
    ///                 {
    ///                     Thread.join();
    ///                 }
    ///
    VIRTUAL bool METHOD(ProcessTask)(THIS_
                                     Uint32 ThreadId,
                                     bool   WaitForTask) PURE;
};
DILIGENT_END_INTERFACE

#include "../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define IThreadPool_EnqueueTask(This, ...)      CALL_IFACE_METHOD(ThreadPool, EnqueueTask, This, __VA_ARGS__)
#    define IThreadPool_ReprioritizeTask(This, ...) CALL_IFACE_METHOD(ThreadPool, ReprioritizeTask, This, __VA_ARGS__)
#    define IThreadPool_ReprioritizeAllTasks(This)  CALL_IFACE_METHOD(ThreadPool, ReprioritizeAllTasks, This)
#    define IThreadPool_RemoveTask(This, ...)       CALL_IFACE_METHOD(ThreadPool, RemoveTask, This, __VA_ARGS__)
#    define IThreadPool_WaitForAllTasks(This)       CALL_IFACE_METHOD(ThreadPool, WaitForAllTasks, This)
#    define IThreadPool_GetQueueSize(This)          CALL_IFACE_METHOD(ThreadPool, GetQueueSize, This)
#    define IThreadPool_GetRunningTaskCount(This)   CALL_IFACE_METHOD(ThreadPool, GetRunningTaskCount, This)
#    define IThreadPool_StopThreads(This)           CALL_IFACE_METHOD(ThreadPool, StopThreads, This)
#    define IThreadPool_ProcessTask(This, ...)      CALL_IFACE_METHOD(ThreadPool, ProcessTask, This, __VA_ARGS__)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
