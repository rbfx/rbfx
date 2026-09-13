// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "All.h"

#include "ParticleGraphSystem.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{

void RegisterGraphNodes(ParticleGraphSystem* system)
{
    Add::RegisterObject(system);
    Bounce::RegisterObject(system);
    BurstTimer::RegisterObject(system);
    Cone::RegisterObject(system);
    Constant::RegisterObject(system);
    Curve::RegisterObject(system);
    Destroy::RegisterObject(system);
    Divide::RegisterObject(system);
    Emit::RegisterObject(system);
    Expire::RegisterObject(system);
    GetAttribute::RegisterObject(system);
    GetUniform::RegisterObject(system);
    Lerp::RegisterObject(system);
    Make::RegisterObject(system);
    Move::RegisterObject(system);
    Multiply::RegisterObject(system);
    Negate::RegisterObject(system);
    Print::RegisterObject(system);
    Random::RegisterObject(system);
    RenderBillboard::RegisterObject(system);
    SetAttribute::RegisterObject(system);
    SetUniform::RegisterObject(system);
    Slerp::RegisterObject(system);
    Subtract::RegisterObject(system);
    TimeStep::RegisterObject(system);
    TimeStepScale::RegisterObject(system);
    EffectTime::RegisterObject(system);
    NormalizedEffectTime::RegisterObject(system);
    RenderMesh::RegisterObject(system);
    Sphere::RegisterObject(system);
    Hemisphere::RegisterObject(system);
    LimitVelocity::RegisterObject(system);
    ApplyForce::RegisterObject(system);
    Normalized::RegisterObject(system);
    Length::RegisterObject(system);
    Break::RegisterObject(system);
    Circle::RegisterObject(system);
    Box::RegisterObject(system);
    Cast::RegisterObject(system);
    Noise3D::RegisterObject(system);
    CurlNoise3D::RegisterObject(system);
}

}

} // namespace Urho3D
