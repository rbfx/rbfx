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
#include "Helpers.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
struct BinaryOperatorPermutation
{
    typedef ea::function<void(UpdateContext& context, ParticleGraphPinRef*)> Lambda;

    BinaryOperatorPermutation(VariantType x, VariantType y, VariantType out, Lambda lambda);

    template <class Node, typename X, typename Y, typename T> static BinaryOperatorPermutation Make()
    {
        return BinaryOperatorPermutation(GetVariantType<X>(), GetVariantType<Y>(), GetVariantType<T>(),
                                  [](UpdateContext& context, ParticleGraphPinRef* pinRefs)
            { RunUpdate<Node, typename Node::Instance, X, Y, T>(context, nullptr, context.indices_.size(),
                                                pinRefs);
                                  });
    }

    VariantType x_;
    VariantType y_;
    VariantType out_;
    Lambda lambda_;
};

/// Binary math operator
class URHO3D_API BinaryMathOperator : public ParticleGraphNode
{
public:
    struct Instance : public ParticleGraphNodeInstance
    {
        /// Construct.
        explicit Instance(BinaryMathOperator* op);

        /// Update particles.
        virtual void Update(UpdateContext& context);

        BinaryMathOperator* operator_;
    };

    /// Construct.
    explicit BinaryMathOperator(Context* context, ea::vector<BinaryOperatorPermutation>&& permutations);

    /// Get number of pins.
    unsigned NumPins() const override;

    /// Get pin by index.
    ParticleGraphPin& GetPin(unsigned index) override;

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;


protected:
    /// Evaluate runtime output pin type.
    VariantType EvaluateOutputPinType(ParticleGraphPin& pin) override;

    /// Update particles.
    void Update(UpdateContext& context);

protected:
    ea::vector<BinaryOperatorPermutation> permutations_;
    ParticleGraphPin pins_[3];
};

/// Add operator.
class URHO3D_API Add : public BinaryMathOperator
{
    URHO3D_OBJECT(Add, ParticleGraphNode)

public:
    template <typename Tuple>
    static void Op(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        auto& x = ea::get<0>(spans);
        auto& y = ea::get<1>(spans);
        auto& out = ea::get<2>(spans);
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = x[i] + y[i];
        }
    }

public:
    /// Construct.
    explicit Add(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);
};


/// Slerp operator.
class URHO3D_API Slerp : public AbstractNode<Slerp, Quaternion, Quaternion, float, Quaternion>
{
    URHO3D_OBJECT(Slerp, ParticleGraphNode);

public:
    template <typename Tuple>
    static void Op(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        auto& x = ea::get<0>(spans);
        auto& y = ea::get<1>(spans);
        auto& t = ea::get<2>(spans);
        auto& out = ea::get<3>(spans);
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = x[i].Slerp(y[i], t[i]);
        }
    }

public:
    /// Construct.
    explicit Slerp(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);
};

}

} // namespace Urho3D
