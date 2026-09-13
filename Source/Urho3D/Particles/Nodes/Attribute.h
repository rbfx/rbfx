// Copyright (c) 2021-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Helpers.h"
#include "../ParticleGraphNode.h"
#include "../ParticleGraphNodeInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
/// Operation on attribute
class URHO3D_API Attribute : public ParticleGraphNode
{
    URHO3D_OBJECT(Attribute, ParticleGraphNode)

protected:
    /// Construct.
    explicit Attribute(Context* context);

public:
    /// Set attribute name
    void SetAttributeName(const ea::string& name) { SetPinName(0, name); }

    /// Get attribute name
    const ea::string& GetAttributeName() const { return GetPinName(0); }

    /// Set attribute type
    virtual void SetAttributeType(VariantType valueType);

    /// Get attribute type
    VariantType GetAttributeType() const { return GetPinValueType(0); }

};

/// Get particle attribute value.
class URHO3D_API GetAttribute : public Attribute
{
    URHO3D_OBJECT(GetAttribute, Attribute)

protected:
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        void Update(UpdateContext& context) override {}

        template <typename... Spans>
        void operator()(const UpdateContext& context, unsigned numParticles, Spans... spans)
        {
        }
    };

public:
    /// Construct.
    explicit GetAttribute(Context* context);
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
        return new (ptr) Instance();
    }

protected:
    ParticleGraphPin* LoadOutputPin(ParticleGraphReader& reader, GraphOutPin& pin) override;

    /// Pins
    ParticleGraphPin pins_[1];
};

/// Set particle attribute value.
class URHO3D_API SetAttribute : public Attribute
{
    URHO3D_OBJECT(SetAttribute, Attribute);
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(SetAttribute* node);
        void Update(UpdateContext& context) override;
    protected:
        SetAttribute* node_;
    };

public:
    /// Construct.
    explicit SetAttribute(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

    /// Get number of pins.
    unsigned GetNumPins() const override { return static_cast<unsigned>(ea::size(pins_)); }

    /// Get pin by index.
    ParticleGraphPin& GetPin(unsigned index) override { return pins_[index]; }

    /// Set attribute type
    void SetAttributeType(VariantType valueType) override;

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override { return sizeof(Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override
    {
        return new (ptr) Instance(this);
    }

protected:
    ParticleGraphPin* LoadOutputPin(ParticleGraphReader& reader, GraphOutPin& pin) override;

    /// Pins
    ParticleGraphPin pins_[2];
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
