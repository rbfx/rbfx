//
// Copyright (c) 2021-2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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
