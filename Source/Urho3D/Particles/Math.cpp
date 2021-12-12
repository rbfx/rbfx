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
#include "Math.h"
#include "ParticleGraphSystem.h"

#include "Urho3D/Core/Context.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{
BinaryOperatorPermutation::BinaryOperatorPermutation(VariantType x, VariantType y, VariantType out, Lambda lambda)
    : x_(x)
    , y_(y)
    , out_(out)
    , lambda_(lambda)
{
}

BinaryMathOperator::Instance::Instance(BinaryMathOperator* op)
    : operator_(op)
{
}

void BinaryMathOperator::Instance::Update(UpdateContext& context)
{
    operator_->Update(context);
}

BinaryMathOperator::BinaryMathOperator(Context* context, ea::vector<BinaryOperatorPermutation>&& permutations)
    : ParticleGraphNode(context)
    , permutations_(ea::move(permutations))
    , pins_{
        ParticleGraphPin(PGPIN_INPUT | PGPIN_TYPE_MUTABLE, "x"),
          ParticleGraphPin(PGPIN_INPUT | PGPIN_TYPE_MUTABLE, "y"),
          ParticleGraphPin(PGPIN_TYPE_MUTABLE, "out")
      }
{
}

unsigned BinaryMathOperator::NumPins() const
{
    return 3;
}

ParticleGraphPin& BinaryMathOperator::GetPin(unsigned index)
{
    return pins_[index];
}

unsigned BinaryMathOperator::EvaluateInstanceSize() { return sizeof(Instance); }

ParticleGraphNodeInstance* BinaryMathOperator::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    return new (ptr) Instance(this);
}

VariantType BinaryMathOperator::EvaluateOutputPinType(ParticleGraphPin& pin)
{
    for (BinaryOperatorPermutation& p: permutations_)
    {
        if (p.x_ == pins_[0].GetValueType() && p.y_ == pins_[1].GetValueType())
        {
            return p.out_;
        }
    }
    return VAR_NONE;
}

void BinaryMathOperator::Update(UpdateContext& context)
{
    ParticleGraphPinRef pinRefs[3];
    for (unsigned i = 0; i < 3; ++i)
    {
        pinRefs[i] = pins_[i].GetMemoryReference();
    }

    for (BinaryOperatorPermutation& p : permutations_)
    {
        if (p.x_ == pins_[0].GetValueType() && p.y_ == pins_[1].GetValueType())
        {
            return p.lambda_(context, pinRefs);
        }
    }
}

Add::Add(Context* context)
    : BinaryMathOperator(context,
        {
            BinaryOperatorPermutation::Make<Add,float, float, float>(),
                                      BinaryOperatorPermutation::Make<Add, Vector2, Vector2, Vector2>(),
                                      BinaryOperatorPermutation::Make<Add, Vector3, Vector3, Vector3>(),
                                      BinaryOperatorPermutation::Make<Add, Vector4, Vector4, Vector4>(),
                                  })
{
}

void Add::RegisterObject(ParticleGraphSystem* context)
{
    context->RegisterParticleGraphNodeFactory<Add>();
}

Slerp::Slerp(Context* context)
    : AbstractNodeType(context, PinArray{
                                    ParticleGraphPin(PGPIN_INPUT, "x"),
                                    ParticleGraphPin(PGPIN_INPUT, "y"),
                                    ParticleGraphPin(PGPIN_INPUT, "t"),
                                    ParticleGraphPin(PGPIN_NONE, "out"),
                                })
{
}

void Slerp::RegisterObject(ParticleGraphSystem* context)
{
    context->RegisterParticleGraphNodeFactory<Slerp>();
}

}

} // namespace Urho3D
