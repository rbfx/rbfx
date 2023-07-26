//
// Copyright (c) 2017-2020 the rbfx project.
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

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Script/Script.h>

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
