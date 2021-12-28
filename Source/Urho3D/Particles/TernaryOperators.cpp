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

#include "TernaryOperators.h"
#include "ParticleGraphSystem.h"

#include "Urho3D/Core/Context.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{

namespace
{
const static ea::vector LerpPins{
    TernaryOperatorPermutation::Make<Lerp, float, float, float, float>(),
    TernaryOperatorPermutation::Make<Lerp, Vector2, Vector2, float, Vector2>(),
    TernaryOperatorPermutation::Make<Lerp, Vector3, Vector3, float, Vector3>(),
    TernaryOperatorPermutation::Make<Lerp, Vector4, Vector4, float, Vector4>(),
    TernaryOperatorPermutation::Make<Lerp, Color, Color, float, Color>(),
};

} // namespace

TernaryOperatorPermutation::TernaryOperatorPermutation(
    VariantType x, VariantType y, VariantType z, VariantType out, Lambda lambda)
    : x_(x)
    , y_(y)
    , z_(z)
    , out_(out)
    , lambda_(lambda)
{
}

TernaryMathOperator::Instance::Instance(TernaryMathOperator* op)
    : operator_(op)
{
}

void TernaryMathOperator::Instance::Update(UpdateContext& context) { operator_->Update(context); }

TernaryMathOperator::TernaryMathOperator(Context* context, const char* zName, const ea::vector<TernaryOperatorPermutation>& permutations)
    : ParticleGraphNode(context)
    , permutations_(permutations)
    , pins_{ParticleGraphPin(ParticleGraphPinFlag::Input | ParticleGraphPinFlag::MutableType, "x"),
          ParticleGraphPin(ParticleGraphPinFlag::Input | ParticleGraphPinFlag::MutableType, "y"),
          ParticleGraphPin(ParticleGraphPinFlag::Input | ParticleGraphPinFlag::MutableType, zName), ParticleGraphPin(ParticleGraphPinFlag::MutableType, "out")}
{
}

unsigned TernaryMathOperator::GetNumPins() const { return static_cast<unsigned>(ea::size(pins_)); }

ParticleGraphPin& TernaryMathOperator::GetPin(unsigned index) { return pins_[index]; }

unsigned TernaryMathOperator::EvaluateInstanceSize() { return sizeof(Instance); }

ParticleGraphNodeInstance* TernaryMathOperator::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    return new (ptr) Instance(this);
}

VariantType TernaryMathOperator::EvaluateOutputPinType(ParticleGraphPin& pin)
{
    for (const TernaryOperatorPermutation& p : permutations_)
    {
        if (p.x_ == pins_[0].GetValueType() && p.y_ == pins_[1].GetValueType())
        {
            return p.out_;
        }
    }
    return VAR_NONE;
}

void TernaryMathOperator::Update(UpdateContext& context)
{
    ParticleGraphPinRef pinRefs[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        pinRefs[i] = pins_[i].GetMemoryReference();
    }

    for (const TernaryOperatorPermutation& p : permutations_)
    {
        if (p.x_ == pins_[0].GetValueType() && p.y_ == pins_[1].GetValueType() && p.z_ == pins_[2].GetValueType())
        {
            return p.lambda_(context, pinRefs);
        }
    }
}

Lerp::Lerp(Context* context)
    : TernaryMathOperator(context, "t", LerpPins)
{
}

void Lerp::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<Lerp>(); }

} // namespace ParticleGraphNodes

} // namespace Urho3D
