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

#include "BinaryOperators.h"
#include "ParticleGraphSystem.h"

#include "Urho3D/Core/Context.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{

namespace
{
const static ea::vector AddPins{
    BinaryOperatorPermutation::Make<Add, float, float, float>(),
    BinaryOperatorPermutation::Make<Add, Vector2, Vector2, Vector2>(),
    BinaryOperatorPermutation::Make<Add, Vector3, Vector3, Vector3>(),
    BinaryOperatorPermutation::Make<Add, Vector4, Vector4, Vector4>(),
};

const static ea::vector SubtractPins{
    BinaryOperatorPermutation::Make<Subtract, float, float, float>(),
    BinaryOperatorPermutation::Make<Subtract, Vector2, Vector2, Vector2>(),
    BinaryOperatorPermutation::Make<Subtract, Vector3, Vector3, Vector3>(),
    BinaryOperatorPermutation::Make<Subtract, Vector4, Vector4, Vector4>(),
};

const static ea::vector MultiplyPins{
    BinaryOperatorPermutation::Make<Multiply, float, float, float>(),
    BinaryOperatorPermutation::Make<Multiply, float, Vector3, Vector3>(),
    BinaryOperatorPermutation::Make<Multiply, Vector3, float, Vector3>(),
    BinaryOperatorPermutation::Make<Multiply, Color, Color, Color>(),
};

const static ea::vector DividePins{
    BinaryOperatorPermutation::Make<Divide, float, float, float>(),
};

} // namespace

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

void BinaryMathOperator::Instance::Update(UpdateContext& context) { operator_->Update(context); }

BinaryMathOperator::BinaryMathOperator(Context* context, const ea::vector<BinaryOperatorPermutation>& permutations)
    : ParticleGraphNode(context)
    , permutations_(permutations)
    , pins_{ParticleGraphPin(ParticleGraphPinFlag::Input | ParticleGraphPinFlag::MutableType, "x"),
          ParticleGraphPin(ParticleGraphPinFlag::Input | ParticleGraphPinFlag::MutableType, "y"), ParticleGraphPin(ParticleGraphPinFlag::MutableType, "out")}
{
}

unsigned BinaryMathOperator::GetNumPins() const { return static_cast<unsigned>(ea::size(pins_)); }

ParticleGraphPin& BinaryMathOperator::GetPin(unsigned index) { return pins_[index]; }

unsigned BinaryMathOperator::EvaluateInstanceSize() { return sizeof(Instance); }

ParticleGraphNodeInstance* BinaryMathOperator::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    return new (ptr) Instance(this);
}

VariantType BinaryMathOperator::EvaluateOutputPinType(ParticleGraphPin& pin)
{
    for (const BinaryOperatorPermutation& p : permutations_)
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

    for (const BinaryOperatorPermutation& p : permutations_)
    {
        if (p.x_ == pins_[0].GetValueType() && p.y_ == pins_[1].GetValueType())
        {
            return p.lambda_(context, pinRefs);
        }
    }
}

Add::Add(Context* context)
    : BinaryMathOperator(context, AddPins)
{
}

void Add::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<Add>(); }

Subtract::Subtract(Context* context)
    : BinaryMathOperator(context, SubtractPins)
{
}

void Subtract::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<Subtract>(); }

Multiply::Multiply(Context* context)
    : BinaryMathOperator(context, MultiplyPins)
{
}

void Multiply::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<Multiply>(); }

Divide::Divide(Context* context)
    : BinaryMathOperator(context, DividePins)
{
}

void Divide::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<Divide>(); }

} // namespace ParticleGraphNodes

} // namespace Urho3D
