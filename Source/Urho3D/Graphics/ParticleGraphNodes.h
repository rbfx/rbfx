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

template <typename T> struct ScalarSpan
{
    ScalarSpan(void* ptr)
        : data_(reinterpret_cast<T*>(ptr))
    {
    }
    inline T& operator[](unsigned index) { return *data_;}
    T* data_;
};

template <typename T> struct SparseSpan
{
    SparseSpan(void* ptr, size_t size, const ea::span<unsigned>& indices)
        : data_(static_cast<T*>(ptr))
        , size(size)
        , indices_(indices)
    {
    }
    inline T& operator[](unsigned index) { return data_[indices_[index]]; }
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
    template <typename ValueType>
    SparseSpan<ValueType> GetSparse(const ParticleGraphNodePin& pin, const ea::span<unsigned>& indices);
};

template <typename ValueType> ea::span<ValueType> UpdateContext::GetSpan(const ParticleGraphNodePin& pin)
{
    const auto subspan = tempBuffer_.subspan(pin.offset_, pin.size_);
    return ea::span<ValueType>(reinterpret_cast<ValueType*>(subspan.begin()),
                               reinterpret_cast<ValueType*>(subspan.end()));
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
                                 reinterpret_cast<ValueType*>(subspan.end()), indices);
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

} // namespace ParticleGraphNodes

#include "ParticleGraphNodes.inl"

namespace Urho3D
{

namespace ParticleGraphNodes
{
URHO3D_PARTICLE_NODE3_BEGIN(AddFloat, "x", float, "y", float, "out", float)
    pin2[index] = pin0[index] + pin1[index];
URHO3D_PARTICLE_NODE_END()

/// Operation on attribute
class URHO3D_API Const : public ParticleGraphNode
{
public:
    /// Construct.
    explicit Const()
        : ParticleGraphNode()
    {
        pins_[0].containerType_ = ParticleGraphContainerType::Scalar;
    }
protected:
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(Const* node)
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
            }
        };

    protected:
        Const* node_;
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
    /// Pins
    ParticleGraphNodePin pins_[1];

    /// Value
    Variant value_;
};

/// Operation on attribute
class URHO3D_API Attribute : public ParticleGraphNode
{
protected:
    /// Construct.
    explicit Attribute()
        : ParticleGraphNode()
    {
        pins_[0].containerType_ = ParticleGraphContainerType::Sparse;
    }

    class Instance : public ParticleGraphNodeInstance
    {
    public:
        void Update(UpdateContext& context) override
        {
        }
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
        return new (ptr) Instance();
    }

protected:
    /// Pins
    ParticleGraphNodePin pins_[1];
};

/// Get particle attribute value.
class URHO3D_API GetAttribute : public Attribute
{
public:
    /// Construct.
    explicit GetAttribute();
};

/// Set particle attribute value.
class URHO3D_API SetAttribute : public Attribute
{
public:
    /// Construct.
    explicit SetAttribute();
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
