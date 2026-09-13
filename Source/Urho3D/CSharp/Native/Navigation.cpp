// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Navigation/CrowdAgent.h"
#include "Urho3D/Navigation/CrowdManager.h"
#include "Urho3D/Script/Script.h"

#include <EASTL/utility.h>

namespace Urho3D
{

typedef void(SWIGSTDCALL* EventHandlerCallback)(CrowdAgent* agent, float timeStep, Vector3* desiredVelocity, float* desiredSpeed);

extern "C"
{

URHO3D_EXPORT_API void SWIGSTDCALL Urho3D_CrowdManager_SetVelocityCallback(CrowdManager* crowdManager, EventHandlerCallback callback, void* callbackHandle)
{
    // GCHandleRef will be moved into ea::function() and remain allocated until function is unset. When that happens holder will free gc
    // handle associated with callback enabling .NET runtime to GC callback delegate.
    GCHandleRef callbackHandleHolder(callbackHandle);
    auto shader([callback, callbackHandleHolder{ea::move(callbackHandleHolder)}](CrowdAgent* agent, float timeStep, Vector3& desiredVelocity, float& desiredSpeed) {
        callback(agent, timeStep, &desiredVelocity, &desiredSpeed);
    });
    crowdManager->SetVelocityCallback(shader);
}

}   // extern "C"

}   // namespace Urho3D
