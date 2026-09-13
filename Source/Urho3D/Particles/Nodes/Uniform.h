// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphNode.h"
#include "Urho3D/Particles/ParticleGraphNodeInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
/// Operation on attribute
class URHO3D_API Uniform : public ParticleGraphNode
{
    URHO3D_OBJECT(Uniform, ParticleGraphNode)

protected:
    /// Construct.
    explicit Uniform(Context* context);

public:
    /// Set attribute name
    void SetUniformName(const ea::string& name) { SetPinName(0, name); }

    /// Get attribute name
    const ea::string& GetUniformName() const { return GetPinName(0); }

    /// Set attribute type
    virtual void SetUniformType(VariantType valueType);

    /// Get attribute type
    VariantType GetUniformType() const { return GetPinValueType(0); }
};

/// Get particle attribute value.
class URHO3D_API GetUniform : public Uniform
{
    URHO3D_OBJECT(GetUniform, Uniform)

protected:
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(GetUniform* node, unsigned uniformIndex);
        void Update(UpdateContext& context) override;

    protected:
        GetUniform* node_;
        unsigned uniformIndex_{};
    };

public:
    /// Construct.
    explicit GetUniform(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

    /// Get number of pins.
    unsigned GetNumPins() const override { return static_cast<unsigned>(ea::size(pins_)); }

    /// Get pin by index.
    ParticleGraphPin& GetPin(unsigned index) override { return pins_[index]; }

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override { return sizeof(Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override
    {
        return new (ptr) Instance(this, layer->GetUniformIndex(GetUniformName(), GetUniformType()));
    }

protected:
    ParticleGraphPin* LoadOutputPin(ParticleGraphReader& reader, GraphOutPin& pin) override;

    /// Pins
    ParticleGraphPin pins_[1];
};

/// Set particle attribute value.
class URHO3D_API SetUniform : public Uniform
{
    URHO3D_OBJECT(SetUniform, Uniform);
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(SetUniform* node, unsigned uniformIndex);
        void Update(UpdateContext& context) override;

    protected:
        SetUniform* node_;
        unsigned uniformIndex_{};
    };

public:
    /// Construct.
    explicit SetUniform(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

    /// Get number of pins.
    unsigned GetNumPins() const override { return static_cast<unsigned>(ea::size(pins_)); }

    /// Get pin by index.
    ParticleGraphPin& GetPin(unsigned index) override { return pins_[index]; }

    /// Set attribute type
    void SetUniformType(VariantType valueType) override;

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override { return sizeof(Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override
    {
        return new (ptr) Instance(this, layer->GetUniformIndex(GetUniformName(), GetUniformType()));
    }

protected:
    ParticleGraphPin* LoadOutputPin(ParticleGraphReader& reader, GraphOutPin& pin) override;

    /// Pins
    ParticleGraphPin pins_[2];
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
