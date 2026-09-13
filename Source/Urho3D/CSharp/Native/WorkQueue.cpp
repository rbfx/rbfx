// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/Script/Script.h"

#include <EASTL/shared_ptr.h>

namespace Urho3D
{

typedef void(SWIGSTDCALL* TaskFunctionCallback)(void*, unsigned threadIndex, WorkQueue* queue);

TaskFunction WrapCSharpHandler(TaskFunctionCallback callback, void* callbackHandle)
{
    const ea::shared_ptr<void> callbackHandlePtr(callbackHandle,
        [](void* handle)
        {
        if (handle)
            Script::GetRuntimeApi()->FreeGCHandle(handle);
    });

    return [=](unsigned threadIndex, WorkQueue* queue) { callback(callbackHandlePtr.get(), threadIndex, queue); };
}


extern "C"
{
    URHO3D_EXPORT_API void SWIGSTDCALL Urho3D_WorkQueue_PostTaskForMainThread(
        WorkQueue* queue, TaskFunctionCallback callback, void* callbackHandle, TaskPriority taskPriority)
    {
        const auto taskFuncHandler = WrapCSharpHandler(callback, callbackHandle);
        queue->PostTaskForMainThread(taskFuncHandler, taskPriority);
    }

    URHO3D_EXPORT_API void SWIGSTDCALL Urho3D_WorkQueue_PostDelayedTaskForMainThread(
        WorkQueue* queue, TaskFunctionCallback callback, void* callbackHandle)
    {
        const auto taskFuncHandler = WrapCSharpHandler(callback, callbackHandle);
        queue->PostDelayedTaskForMainThread(taskFuncHandler);
    }
}

} // namespace Urho3D
