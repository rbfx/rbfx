// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../ParticleGraphNode.h"
#include "../ParticleGraphNodeInstance.h"
#include "../ParticleGraphPin.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

/// Operation on attribute
class URHO3D_API Random : public ParticleGraphNode
{
    URHO3D_OBJECT(Random, ParticleGraphNode)
public:
    /// Construct.
    explicit Random(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

protected:
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(Random* node);
        void Update(UpdateContext& context) override;

    protected:
        Random* node_;
    };

public:
    /// Get number of pins.
    unsigned GetNumPins() const override { return static_cast<unsigned>(ea::size(pins_)); }

    /// Get pin by index.
    ParticleGraphPin& GetPin(unsigned index) override { return pins_[index]; }

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override { return sizeof(Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override
    {
        return new (ptr) Instance(this);
    }

    const Variant& GetMin() const { return min_; }
    void SetMin(const Variant& val) { min_ = val; }
    const Variant& GetMax() const { return max_; }
    void SetMax(const Variant& val) { max_ = val; }

protected:
    /// Pins
    ParticleGraphPin pins_[1];

    /// Min value.
    Variant min_{0.0f};
    /// Max value.
    Variant max_{1.0f};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
