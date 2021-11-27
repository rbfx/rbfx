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

#include "ParticleGraphEmitter.h"

namespace Urho3D
{

template <typename T>
struct ScalarSpan
{
    ScalarSpan(void* ptr)
        : data_(reinterpret_cast<T*>(ptr)) {}
    T* data_;
};

template <typename T> struct SparseSpan
{
    SparseSpan(void* ptr, size_t size, const ea::span<unsigned>& indices)
        : data_(static_cast<T*>(ptr))
        , size(size)
        , indices_(indices) {}
    T* data_;
    size_t size;
    ea::span<unsigned> indices_;
};

struct UpdateContext
{
    float timeStep_{};
    ea::span<unsigned> indices_;
    ea::span<uint8_t> attributes_;
    ea::span<uint8_t> tempBuffer_;

    template <typename ValueType> ea::span<ValueType> GetSpan(const ParticleGraphNodePin& pin);
    template <typename ValueType> ScalarSpan<ValueType> GetScalar(const ParticleGraphNodePin& pin);
    template <typename ValueType> SparseSpan<ValueType> GetSparse(const ParticleGraphNodePin& pin, const ea::span<unsigned>& indices);
};

template <typename ValueType> ea::span<ValueType> UpdateContext::GetSpan(const ParticleGraphNodePin& pin)
{
    const auto subspan = tempBuffer_.subspan(pin.offset_, pin.size_);
    return ea::span<ValueType>(reinterpret_cast<ValueType*>(subspan.begin()), reinterpret_cast<ValueType*>(subspan.end()));
}
template <typename ValueType> ScalarSpan<ValueType> UpdateContext::GetScalar(const ParticleGraphNodePin& pin)
{
    const auto subspan = tempBuffer_.subspan(pin.offset_, sizeof(ValueType));
    return ScalarSpan<ValueType>(reinterpret_cast<ValueType*>(subspan.begin()));
}
template <typename ValueType>
SparseSpan<ValueType> UpdateContext::GetSparse(const ParticleGraphNodePin& pin, const ea::span<unsigned>& indices)
{
    const auto subspan = tempBuffer_.subspan(pin.offset_, pin.size_);
    return SparseSpan<ValueType>(reinterpret_cast<ValueType*>(subspan.begin()),
                               reinterpret_cast<ValueType*>(subspan.end()),
                               indices);
}

class URHO3D_API ParticleGraphNodeInstance
{
public:
    ParticleGraphNodeInstance();

    /// Destruct.
    virtual ~ParticleGraphNodeInstance();

    /// Update.
    virtual void Update(UpdateContext& context) = 0;
};

namespace ParticleGraphNodes
{

template <typename Value> class GetValueType
{
};

template <> struct GetValueType<float>
{
    static constexpr ParticleGraphValueType ValueType = ParticleGraphValueType::Float;
};


template <typename Node, typename Value0, typename Value1, typename Value2> class AbstractNode : public ParticleGraphNode
{
protected:
    /// Construct.
    explicit AbstractNode()
        : ParticleGraphNode()
    {
        pins_[0].valueType_ = GetValueType<Value0>::ValueType;
        pins_[1].valueType_ = GetValueType<Value1>::ValueType;
        pins_[2].valueType_ = GetValueType<Value2>::ValueType;
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
                Node::Op(numParticles, context.GetSpan<Value0>(pin0),
                         context.GetSpan<Value1>(pin1),
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
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr) override { return new (ptr) Instance(static_cast<Node*>(this)); }

protected:

    /// Pins
    ParticleGraphNodePin pins_[3];
};

class Add : public AbstractNode<Add,float, float, float>
{
    template <typename Span0, typename Span1, typename Span2>
    static void Op(const unsigned numParticles, Span0 pin0, Span1 pin1, Span2 pin2)
    {
        for (unsigned i = 0; i < numParticles; ++i)
        {
            pin2[i] = pin0[i] + pin1[i];
        }
    }

    friend class AbstractNode<Add, float, float, float>;
};


} // namespace ParticleGraphNodes

} // namespace Urho3D
