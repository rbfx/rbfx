//
// Copyright (c) 2021 the rbfx project.
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

#include "ParticleGraphNode.h"
#include "ParticleGraphNodeInstance.h"

namespace Urho3D
{
    
namespace ParticleGraphNodes
{
/// Operation on attribute
class URHO3D_API Attribute : public ParticleGraphNode
{
    URHO3D_OBJECT(Attribute, ParticleGraphNode);

protected:
    /// Construct.
    explicit Attribute(Context* context, const ParticleGraphNodePin& pin);

    class Instance : public ParticleGraphNodeInstance
    {
    public:
        void Update(UpdateContext& context) override {}
    };

public:
    /// Get number of pins.
    unsigned NumPins() const override { return 1; }

    /// Get pin by index.
    ParticleGraphNodePin& GetPin(unsigned index) override { return pins_[index]; }

    /// Evaluate size required to place new node instance.
    unsigned EvalueInstanceSize() override { return sizeof(Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override
    {
        return new (ptr) Instance();
    }

    /// Set attribute name
    /// @property
    void SetAttributeName(const ea::string& name) { SetPinName(0, name); }

    /// Get attribute name
    /// @property
    const ea::string& GetAttributeName() const { return  pins_[0].GetName(); }

    /// Set attribute type
    /// @property
    void SetAttributeType(VariantType valueType) { SetPinValueType(0, valueType); }

    /// Get attribute type
    /// @property
    VariantType GetAttributeType() const { return pins_[0].GetRequestedType(); }

protected:
    /// Pins
    ParticleGraphNodePin pins_[1];
};

/// Get particle attribute value.
class URHO3D_API GetAttribute : public Attribute
{
    URHO3D_OBJECT(GetAttribute, Attribute);

public:
    /// Construct.
    explicit GetAttribute(Context* context);
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

    /// Evaluate size required to place new node instance.
    unsigned EvalueInstanceSize() override { return sizeof(Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override
    {
        return new (ptr) Instance(this);
    }
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
