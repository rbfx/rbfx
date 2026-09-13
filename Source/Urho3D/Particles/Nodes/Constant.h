// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../ParticleGraphNode.h"
#include "../ParticleGraphNodeInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

/// Operation on attribute
class URHO3D_API Constant : public ParticleGraphNode
{
    URHO3D_OBJECT(Constant, ParticleGraphNode)

public:
    /// Construct.
    explicit Constant(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

protected:
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(Constant* node);
        void Update(UpdateContext& context) override;

    protected:
        Constant* node_;
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

    /// Serialize from/to archive. Return true if successful.
    //bool Serialize(Archive& archive) override;

    const Variant& GetValue() const;

    void SetValue(const Variant&);

protected:

    /// Pins
    ParticleGraphPin pins_[1];

    /// Value
    Variant value_;
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
