// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../TemplateNode.h"
#include "../ParticleGraphNode.h"
#include "../ParticleGraphNodeInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
class ApplyForceInstance;

class URHO3D_API ApplyForce : public TemplateNode<ApplyForceInstance, Vector3, Vector3, Vector3>
{
    URHO3D_OBJECT(ApplyForce, ParticleGraphNode)
public:
    /// Construct ApplyForce.
    explicit ApplyForce(Context* context);
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
