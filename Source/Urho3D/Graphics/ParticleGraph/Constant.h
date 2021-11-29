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

#include <Urho3D/Graphics/ParticleGraphEffect.h>
#include <Urho3D/Graphics/ParticleGraphNodeInstance.h>

namespace Urho3D
{

namespace ParticleGraphNodes
{

/// Operation on attribute
class URHO3D_API Constant : public ParticleGraphNode
{
    URHO3D_OBJECT(Constant, ParticleGraphNode)
public:
    /// Construct.
    explicit Constant(Context* context)
        : ParticleGraphNode(context)
        , pins_{ParticleGraphNodePin(false, "value", VAR_NONE, ParticleGraphContainerType::Scalar)}
    {
    }

protected:
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(Constant* node)
            : node_(node)
        {
        }
        void Update(UpdateContext& context) override
        {
            const auto& pin0 = node_->pins_[0];
            switch (node_->value_.GetType())
            {
            case VAR_FLOAT:
                context.GetScalar<float>(pin0)[0] = node_->value_.GetFloat();
                break;
            case VAR_VECTOR2:
                context.GetScalar<Vector2>(pin0)[0] = node_->value_.GetVector2();
                break;
            case VAR_VECTOR3:
                context.GetScalar<Vector3>(pin0)[0] = node_->value_.GetVector3();
                break;
            case VAR_VECTOR4:
                context.GetScalar<Vector4>(pin0)[0] = node_->value_.GetVector4();
                break;
            default:
                assert(!"Not implemented yet");
                break;
            }
        };

    protected:
        Constant* node_;
    };

public:
    /// Get number of pins.
    unsigned NumPins() const override { return 1; }

    /// Get pin by index.
    ParticleGraphNodePin& GetPin(unsigned index) override { return pins_[index]; }

    /// Evaluate size required to place new node instance.
    unsigned EvalueInstanceSize() override { return sizeof(Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr) override { return new (ptr) Instance(this); }

    const Variant& GetValue();

    void SetValue(const Variant&);

protected:
    /// Save to an XML element. Return true if successful.
    bool Save(XMLElement& dest) const override;

    /// Pins
    ParticleGraphNodePin pins_[1];

    /// Value
    Variant value_;
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
