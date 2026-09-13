// Copyright (c) 2021-2026 the rbfx project.
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
class BounceInstance;

class URHO3D_API Bounce : public TemplateNode<BounceInstance, Vector3, Vector3, Vector3, Vector3>
{
    URHO3D_OBJECT(Bounce, ParticleGraphNode)
public:
    /// Construct Bounce.
    explicit Bounce(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;

    /// Set Dampen.
    void SetDampen(float value);
    /// Get Dampen.
    float GetDampen() const;

    /// Set BounceFactor.
    void SetBounceFactor(float value);
    /// Get BounceFactor.
    float GetBounceFactor() const;

protected:
    float dampen_{};
    float bounceFactor_{};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
