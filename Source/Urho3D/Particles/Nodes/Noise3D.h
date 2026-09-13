// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/TemplateNode.h"
#include "Urho3D/Particles/ParticleGraphNode.h"
#include "Urho3D/Particles/ParticleGraphNodeInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
class Noise3DInstance;

class URHO3D_API Noise3D : public TemplateNode<Noise3DInstance, Vector3, float>
{
    URHO3D_OBJECT(Noise3D, ParticleGraphNode)
public:
    /// Construct Noise3D.
    explicit Noise3D(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;

protected:
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
