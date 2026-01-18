// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Jolt/Physics.h"

#include <Jolt/Jolt.h>
// Jolt.h must be the first
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Physics/PhysicsSettings.h>

namespace Urho3D
{

void RegisterJoltLibrary(Context* context)
{
    JPH::RegisterDefaultAllocator();

    auto t = new JPH::JobSystemSingleThreaded(JPH::cMaxPhysicsJobs);
    delete t;
}

} // namespace Urho3D
