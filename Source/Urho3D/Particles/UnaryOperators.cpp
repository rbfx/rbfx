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

#include "UnaryOperators.h"
#include "ParticleGraphSystem.h"

#include "Urho3D/Core/Context.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{

namespace
{

const static ea::vector NegatePins{
    UnaryOperatorPermutation::Make<Negate, float, float>(),
};

const static ea::vector TimeStepScalePins{
    UnaryOperatorPermutation::Make<TimeStepScale, float, float>(),
};

} // namespace

UnaryOperatorPermutation::UnaryOperatorPermutation(VariantType x, VariantType out, Lambda lambda)
    : x_(x)
    , out_(out)
    , lambda_(lambda)
{
}

UnaryMathOperator::Instance::Instance(UnaryMathOperator* op)
    : operator_(op)
{
}

void UnaryMathOperator::Instance::Update(UpdateContext& context) { operator_->Update(context); }

UnaryMathOperator::UnaryMathOperator(Context* context, const ea::vector<UnaryOperatorPermutation>& permutations)
    : ParticleGraphNode(context)
    , permutations_(permutations)
    , pins_{ParticleGraphPin(PGPIN_INPUT | PGPIN_TYPE_MUTABLE, "x"), ParticleGraphPin(PGPIN_TYPE_MUTABLE, "out")}
{
}

unsigned UnaryMathOperator::GetNumPins() const { return static_cast<unsigned>(ea::size(pins_)); }

ParticleGraphPin& UnaryMathOperator::GetPin(unsigned index) { return pins_[index]; }

unsigned UnaryMathOperator::EvaluateInstanceSize() { return sizeof(Instance); }

ParticleGraphNodeInstance* UnaryMathOperator::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    return new (ptr) Instance(this);
}

VariantType UnaryMathOperator::EvaluateOutputPinType(ParticleGraphPin& pin)
{
    for (const UnaryOperatorPermutation& p : permutations_)
    {
        if (p.x_ == pins_[0].GetValueType())
        {
            return p.out_;
        }
    }
    return VAR_NONE;
}

void UnaryMathOperator::Update(UpdateContext& context)
{
    ParticleGraphPinRef pinRefs[2];
    for (unsigned i = 0; i < 2; ++i)
    {
        pinRefs[i] = pins_[i].GetMemoryReference();
    }

    for (const UnaryOperatorPermutation& p : permutations_)
    {
        if (p.x_ == pins_[0].GetValueType())
        {
            return p.lambda_(context, pinRefs);
        }
    }
}

Negate::Negate(Context* context)
    : UnaryMathOperator(context, NegatePins)
{
}

void Negate::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<Negate>(); }

TimeStepScale::TimeStepScale(Context* context)
    : UnaryMathOperator(context, TimeStepScalePins)
{
}

void TimeStepScale::RegisterObject(ParticleGraphSystem* context) { context->AddReflection<TimeStepScale>(); }

} // namespace ParticleGraphNodes

} // namespace Urho3D
