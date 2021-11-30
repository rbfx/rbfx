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

#include "../../Core/Context.h"
#include "../ParticleGraphEffect.h"
#include "ParticleGraphNode.h"
#include "ParticleGraphNodeInstance.h"

//#define URHO3D_PARTICLE_NODE1_BEGIN(Name, Pin0Name, Pin0) \
//    class URHO3D_API Name : public AbstractNode1<Name, Pin0> \
//    { \
//    public: \
//        Name(Context* context) \
//            : AbstractNode1(context) \
//        { \
//            pins_[0].name_ = Pin0Name; \
//        } \
//                                                                                                                       \
//    private: \
//        friend class AbstractNode1<Name, Pin0Name, Pin0>; \
//        template <typename Span0> static void Op(const unsigned numParticles, Span0 pin0) \
//        { \
//            for (unsigned index = 0; index < numParticles; ++index) \
//            {
//
//#define URHO3D_PARTICLE_NODE3_BEGIN(Name, Pin0Name, Pin0, Pin1Name, Pin1, Pin2Name, Pin2) \
//    class URHO3D_API Name : public AbstractNode3<Name, Pin0, Pin1, Pin2> \
//    { \
//    public: \
//        Name(Context* context) \
//            : AbstractNode3(context) \
//        { \
//            pins_[0].SetName(Pin0Name); \
//            pins_[1].SetName(Pin1Name); \
//            pins_[2].SetName(Pin2Name); \
//        } \
//                                                                                                                       \
//    private: \
//        friend class AbstractNode3<Name, Pin0, Pin1, Pin2>; \
//        template <typename Span0, typename Span1, typename Span2> \
//        static void Op(const unsigned numParticles, Span0 pin0, Span1 pin1, Span2 pin2) \
//        { \
//            for (unsigned index = 0; index < numParticles; ++index) \
//            {
//
//
//#define URHO3D_PARTICLE_NODE_END() \
//            } \
//        } \
//    };

namespace Urho3D
{

namespace ParticleGraphNodes
{
/// Abstract node with 1 pin
template <typename Node, typename Value0> class AbstractNode1 : public ParticleGraphNode
{
protected:
    /// Construct.
    explicit AbstractNode1(Context* context)
        : ParticleGraphNode(context)
    {
        pins_[0].requestedValueType_ = GetVariantType<Value0>();
    }

    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(Node* node)
            : node_(node)
        {
        }
        void Update(UpdateContext& context) override
        {
            const auto& pin0 = node_->pins_[0];

            const unsigned numParticles = context.indices_.size();
            switch (pin0.valueType_)
            {
            case PGCONTAINER_SPAN:
                Node::Op(numParticles, context.GetSpan<Value0>(pin0));
                break;
            case PGCONTAINER_SCALAR:
                Node::Op(numParticles, context.GetScalar<Value0>(pin0));
                break;
            case PGCONTAINER_SPARSE:
                Node::Op(numParticles, context.GetSparse<Value0>(pin0));
                break;
            default:
                assert(!"Invalid pin container type permutation");
                break;
            }
        }

    protected:
        Node* node_;
    };

public:
    /// Get number of pins.
    unsigned NumPins() const override { return 1; }

    /// Get pin by index.
    ParticleGraphNodePin& GetPin(unsigned index) override { return pins_[index]; }

    /// Evaluate size required to place new node instance.
    unsigned EvalueInstanceSize() override { return sizeof(Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr) override
    {
        return new (ptr) Instance(static_cast<Node*>(this));
    }

protected:
    /// Pins
    ParticleGraphNodePin pins_[1];
};

/// Abstract node with 3 pins
template <typename Node, typename Value0, typename Value1, typename Value2>
class AbstractNode3 : public ParticleGraphNode
{
protected:
    /// Construct.
    explicit AbstractNode3(Context* context)
        : ParticleGraphNode(context)
    {
        pins_[0].requestedValueType_ = GetVariantType<Value0>();
        pins_[1].requestedValueType_ = GetVariantType<Value1>();
        pins_[2].requestedValueType_ = GetVariantType<Value2>();
    }

    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(Node* node)
            : node_(node)
        {
        }
        void Update(UpdateContext& context) override
        {
            const auto& pin0 = node_->pins_[0];
            const auto& pin1 = node_->pins_[1];
            const auto& pin2 = node_->pins_[2];
            const auto permutation = static_cast<unsigned>(pin0.containerType_) +
                                     static_cast<unsigned>(pin1.containerType_) * 3 +
                                     static_cast<unsigned>(pin2.containerType_) * 9;

            const unsigned numParticles = context.indices_.size();
            switch (permutation)
            {
            case 0:
                Node::Op(numParticles, context.GetSpan<Value0>(pin0), context.GetSpan<Value1>(pin1),
                         context.GetSpan<Value2>(pin2));
                break;
            default:
                assert(!"Invalid pin container type permutation");
                break;
            }
        }

    protected:
        Node* node_;
    };

public:
    /// Get number of pins.
    unsigned NumPins() const override { return 3; }

    /// Get pin by index.
    ParticleGraphNodePin& GetPin(unsigned index) override { return pins_[index]; }

    /// Evaluate size required to place new node instance.
    unsigned EvalueInstanceSize() override { return sizeof(Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr) override
    {
        return new (ptr) Instance(static_cast<Node*>(this));
    }

protected:
    /// Pins
    ParticleGraphNodePin pins_[3];
};

template <template <typename> typename T, typename Arg0, typename Arg1>
void SelectByVariantType(VariantType variantType, Arg0 arg0, Arg1 arg1)
{
    switch (variantType)
    {
    case VAR_FLOAT:
    {
        T<float> fn;
        fn(arg0, arg1);
        break;
    }
    case VAR_VECTOR2:
    {
        T<Vector2> fn;
        fn(arg0, arg1);
        break;
    }
    case VAR_VECTOR3:
    {
        T<Vector3> fn;
        fn(arg0, arg1);
        break;
    }
    default:
        assert(!"Not implemented");
    }
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
