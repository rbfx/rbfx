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
#include "Helpers.h"
#include "ParticleGraphNode.h"
#include "ParticleGraphNodeInstance.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

struct TernaryOperatorPermutation
{
    typedef ea::function<void(UpdateContext& context, ParticleGraphPinRef*)> Lambda;

    TernaryOperatorPermutation(VariantType x, VariantType y, VariantType z, VariantType out, Lambda lambda);

    template <class Node, typename X, typename Y, typename Z, typename T> static TernaryOperatorPermutation Make()
    {
        return TernaryOperatorPermutation(GetVariantType<X>(), GetVariantType<Y>(), GetVariantType<Z>(),
            GetVariantType<T>(),
            [](UpdateContext& context, ParticleGraphPinRef* pinRefs) {
                RunUpdate<Node, typename Node::Instance, X, Y, Z, T>(
                    context, nullptr, context.indices_.size(), pinRefs);
            });
    }

    VariantType x_;
    VariantType y_;
    VariantType z_;
    VariantType out_;
    Lambda lambda_;
};

/// Ternary math operator
class URHO3D_API TernaryMathOperator : public ParticleGraphNode
{
public:
    struct Instance : public ParticleGraphNodeInstance
    {
        /// Construct.
        explicit Instance(TernaryMathOperator* op);

        /// Update particles.
        virtual void Update(UpdateContext& context);

        TernaryMathOperator* operator_;
    };

    /// Construct.
    explicit TernaryMathOperator(Context* context, const ea::vector<TernaryOperatorPermutation>& permutations);

    /// Get number of pins.
    unsigned GetNumPins() const override;

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
    const ea::vector<TernaryOperatorPermutation>& permutations_;
    ParticleGraphPin pins_[4];
};

/// Lerp operator.
class URHO3D_API Lerp : public TernaryMathOperator
{
    URHO3D_OBJECT(Lerp, ParticleGraphNode)

public:
    template <typename Tuple>
    static void Evaluate(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        auto& x = ea::get<0>(spans);
        auto& y = ea::get<1>(spans);
        auto& factor = ea::get<2>(spans);
        auto& out = ea::get<3>(spans);
        for (unsigned i = 0; i < numParticles; ++i)
        {
            const auto k = factor[i];
            const auto _k = 1.0f - k;
            out[i] = x[i] * _k + y[i] * k;
        }
    }

public:
    /// Construct.
    explicit Lerp(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
