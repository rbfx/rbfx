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
#include <EASTL/tuple.h>

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
/// Abstract update runner.
template <typename Node, typename Instance, typename Tuple>
void RunUpdate(UpdateContext& context, Instance* instance, unsigned numParticles,
               ParticleGraphPinRef* pinRefs, Tuple tuple)
{
    Node::Op(context, static_cast<typename Node::Instance*>(instance), numParticles, std::move(tuple));
};

/// Abstract update runner.
template <typename Node, typename Instance, typename Tuple, typename Value0, typename... Values>
void RunUpdate(UpdateContext& context, Instance* instance, unsigned numParticles,
               ParticleGraphPinRef* pinRefs, Tuple tuple)
{
    switch (pinRefs[0].type_)
    {
    case PGCONTAINER_SPAN:
    {
        auto nextTuple = ea::tuple_cat(std::move(tuple), ea::make_tuple(context.GetSpan<Value0>(*pinRefs)));
        RunUpdate<Node, Instance, decltype(nextTuple), Values...>(context, instance, numParticles, pinRefs + 1,
                                                                  nextTuple);
        return;
    }
    case PGCONTAINER_SPARSE:
    {
        auto nextTuple = ea::tuple_cat(std::move(tuple), ea::make_tuple(context.GetSparse<Value0>(*pinRefs)));
        RunUpdate<Node, Instance, decltype(nextTuple), Values...>(context, instance, numParticles, pinRefs + 1,
                                                                  nextTuple);
        return;
    }
    case PGCONTAINER_SCALAR:
    {
        auto nextTuple = ea::tuple_cat(std::move(tuple), ea::make_tuple(context.GetScalar<Value0>(*pinRefs)));
        RunUpdate<Node, Instance, decltype(nextTuple), Values...>(context, instance, numParticles, pinRefs + 1,
                                                                  nextTuple);
        return;
    }
    default:
        assert(!"Invalid pin container type permutation");
        break;
    }
};

/// Abstract update runner.
template <typename Node, typename Instance, typename Value0, typename... Values>
void RunUpdate(UpdateContext& context, Instance * instance, unsigned numParticles, ParticleGraphPinRef* pinRefs)
{
    switch (pinRefs[0].type_)
    {
    case PGCONTAINER_SPAN:
    {
        ea::tuple<ea::span<Value0>> nextTuple = ea::make_tuple(context.GetSpan<Value0>(*pinRefs));
        RunUpdate<Node, Instance, ea::tuple<ea::span<Value0>>, Values...>(context, instance, numParticles, pinRefs + 1,
                                                                         std::move(nextTuple));
        return;
    }
    case PGCONTAINER_SPARSE:
    {
        ea::tuple<SparseSpan<Value0>> nextTuple = ea::make_tuple(context.GetSparse<Value0>(*pinRefs));
        RunUpdate<Node, Instance, ea::tuple<SparseSpan<Value0>>, Values...>(context, instance, numParticles,
                                                                            pinRefs + 1,
                                                                  std::move(nextTuple));
        return;
    }
    case PGCONTAINER_SCALAR:
    {
        ea::tuple<ScalarSpan<Value0>> nextTuple = ea::make_tuple(context.GetScalar<Value0>(*pinRefs));
        RunUpdate<Node, Instance, ea::tuple<ScalarSpan<Value0>>, Values...>(context, instance, numParticles,
                                                                            pinRefs + 1,
                                                                  std::move(nextTuple));
        return;
    }
    default:
        assert(!"Invalid pin container type permutation");
        break;
    }
};
/// Abstract node
template <typename Node, typename... Values> class AbstractNode : public ParticleGraphNode
{
protected:
    typedef AbstractNode<Node, Values...> AbstractNodeType;
    static constexpr unsigned NumberOfPins = sizeof...(Values);
    typedef ea::array<ParticleGraphPin, NumberOfPins> PinArray;

protected:
    /// Helper methods to assign pin types based on template argument types
    template <typename LastValue> void SetPinTypes(ParticleGraphPin* pins, const ParticleGraphPin* src)
    {
        *pins = (*src).WithType(GetVariantType<LastValue>());
    }
    template <typename First, typename Second, typename... PinTypes>
    void SetPinTypes(ParticleGraphPin* pins, const ParticleGraphPin* src)
    {
        *pins = (*src).WithType(GetVariantType<First>());
        SetPinTypes<Second, PinTypes...>(pins + 1, src + 1);
    }

    /// Construct.
    explicit AbstractNode(Context* context, const PinArray& pins)
        : ParticleGraphNode(context)
    {
        SetPinTypes<Values...>(&pins_[0], &pins[0]);
    }

public:
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(Node* node, ParticleGraphLayerInstance* layer)
            : node_(node)
        {
        }

        void Update(UpdateContext& context) override
        {
            ParticleGraphPinRef pinRefs[NumberOfPins];
            for (unsigned i = 0; i < NumberOfPins; ++i)
            {
                pinRefs[i] = node_->pins_[i].GetMemoryReference();
            }
            RunUpdate<Node, Instance, Values...>(context, this, context.indices_.size(), pinRefs);
        }

    protected:
        Node* node_;
    };

public:
    /// Get number of pins.
    unsigned NumPins() const override { return NumberOfPins; }

    /// Get pin by index.
    ParticleGraphPin& GetPin(unsigned index) override { return pins_[index]; }

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() override { return sizeof(typename Node::Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override
    {
        return new (ptr) typename Node::Instance(static_cast<Node*>(this), layer);
    }

protected:
    /// Pins
    PinArray pins_;
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
