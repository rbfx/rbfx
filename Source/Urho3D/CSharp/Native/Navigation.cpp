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

#include <Urho3D/Navigation/CrowdAgent.h>
#include <Urho3D/Navigation/CrowdManager.h>
#include <Urho3D/Script/Script.h>

#include <EASTL/utility.h>

namespace Urho3D
{

typedef void(SWIGSTDCALL* EventHandlerCallback)(CrowdAgent* agent, float timeStep, Vector3* desiredVelocity, float* desiredSpeed);

extern "C"
{

URHO3D_EXPORT_API void SWIGSTDCALL Urho3D_CrowdManager_SetVelocityShader(CrowdManager* crowdManager, EventHandlerCallback callback, void* callbackHandle)
{
    // GCHandleRef will be moved into std::function() and remain allocated until function is unset. When that happens holder will free gc
    // handle associated with callback enabling .NET runtime to GC callback delegate.
    GCHandleRef callbackHandleHolder(callbackHandle);
    auto shader([callback, callbackHandleHolder{ea::move(callbackHandleHolder)}](CrowdAgent* agent, float timeStep, Vector3& desiredVelocity, float& desiredSpeed) {
        callback(agent, timeStep, &desiredVelocity, &desiredSpeed);
    });
    crowdManager->SetVelocityShader(shader);
}

}   // extern "C"

}   // namespace Urho3D
