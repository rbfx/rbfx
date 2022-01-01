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

struct BinaryOperatorPermutation
{
    typedef ea::function<void(UpdateContext& context, ParticleGraphNodeInstance* instance, ParticleGraphPinRef*)>
        Lambda;

    BinaryOperatorPermutation(VariantType x, VariantType y, VariantType out, Lambda lambda);

    template <class Node, typename X, typename Y, typename T> static BinaryOperatorPermutation Make()
    {
        return BinaryOperatorPermutation(GetVariantType<X>(), GetVariantType<Y>(), GetVariantType<T>(),
            [](UpdateContext& context, ParticleGraphNodeInstance* instance, ParticleGraphPinRef* pinRefs)
            {
                RunUpdate<typename Node::Instance, X, Y, T>(
                    context, *static_cast<typename Node::Instance*>(instance), context.indices_.size(), pinRefs);
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
    explicit BinaryMathOperator(Context* context, const ea::vector<BinaryOperatorPermutation>& permutations);

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
    void Update(UpdateContext& context, Instance* instance);

protected:
    const ea::vector<BinaryOperatorPermutation>& permutations_;
    ParticleGraphPin pins_[3];
};

/// Add operator.
class URHO3D_API Add : public BinaryMathOperator
{
    URHO3D_OBJECT(Add, ParticleGraphNode)

public:
    class Instance final : public BinaryMathOperator::Instance
    {
    public:
        Instance(Add* node)
            : BinaryMathOperator::Instance(node)
        {
        }

        template <typename Tuple> void operator()(UpdateContext& context, unsigned numParticles, Tuple&& spans)
        {
            auto& x = ea::get<0>(spans);
            auto& y = ea::get<1>(spans);
            auto& out = ea::get<2>(spans);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = x[i] + y[i];
            }
        }
    };

public:
    /// Construct.
    explicit Add(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);
};

/// Subtract operator.
class URHO3D_API Subtract : public BinaryMathOperator
{
    URHO3D_OBJECT(Subtract, ParticleGraphNode)

public:
    class Instance final : public BinaryMathOperator::Instance
    {
    public:
        Instance(Subtract* node)
            : BinaryMathOperator::Instance(node)
        {
        }

        template <typename Tuple> void operator()(UpdateContext& context, unsigned numParticles, Tuple&& spans)
        {
            auto& x = ea::get<0>(spans);
            auto& y = ea::get<1>(spans);
            auto& out = ea::get<2>(spans);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = x[i] - y[i];
            }
        }
    };


public:
    /// Construct.
    explicit Subtract(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);
};

/// Multiply operator.
class URHO3D_API Multiply : public BinaryMathOperator
{
    URHO3D_OBJECT(Multiply, ParticleGraphNode)

public:
    class Instance final : public BinaryMathOperator::Instance
    {
    public:
        Instance(Multiply* node)
            : BinaryMathOperator::Instance(node)
        {
        }

        template <typename Tuple> void operator()(UpdateContext& context, unsigned numParticles, Tuple&& spans)
        {
            auto& x = ea::get<0>(spans);
            auto& y = ea::get<1>(spans);
            auto& out = ea::get<2>(spans);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = x[i] * y[i];
            }
        }
    };

public:
    /// Construct.
    explicit Multiply(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);
};

/// Divide operator.
class URHO3D_API Divide : public BinaryMathOperator
{
    URHO3D_OBJECT(Divide, ParticleGraphNode)

public:
    class Instance final : public BinaryMathOperator::Instance
    {
    public:
        Instance(Divide* node)
            : BinaryMathOperator::Instance(node)
        {
        }

        template <typename Tuple> void operator()(UpdateContext& context, unsigned numParticles, Tuple&& spans)
        {
            auto& x = ea::get<0>(spans);
            auto& y = ea::get<1>(spans);
            auto& out = ea::get<2>(spans);
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = x[i] / y[i];
            }
        }
    };


public:
    /// Construct.
    explicit Divide(Context* context);
    /// Register particle node factory.
    /// @nobind
    static void RegisterObject(ParticleGraphSystem* context);
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
