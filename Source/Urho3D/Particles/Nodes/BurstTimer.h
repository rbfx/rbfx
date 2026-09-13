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
class BurstTimerInstance;

class URHO3D_API BurstTimer : public TemplateNode<BurstTimerInstance, float, float>
{
    URHO3D_OBJECT(BurstTimer, ParticleGraphNode)
public:
    /// Construct BurstTimer.
    explicit BurstTimer(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;

    /// Set Delay.
    void SetDelay(float value);
    /// Get Delay.
    float GetDelay() const;

    /// Set Interval.
    void SetInterval(float value);
    /// Get Interval.
    float GetInterval() const;

    /// Set Cycles.
    void SetCycles(int value);
    /// Get Cycles.
    int GetCycles() const;

protected:
    float delay_{};
    float interval_{};
    int cycles_{};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
