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

#include "../../Precompiled.h"

#include "Random.h"

#include "../Helpers.h"
#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphNodeInstance.h"
#include "../ParticleGraphSystem.h"
#include "../UpdateContext.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
namespace
{
template <typename T>
struct Generate
{
    void operator()(const UpdateContext& context, const ParticleGraphPin& pin0, const Variant& min, const Variant& max)
    {
        auto span = context.GetSpan<T>(pin0.GetMemoryReference());
        for (unsigned i = 0; i < context.indices_.size(); ++i)
        {
            span[i] = min.Lerp(max, Urho3D::Random()).Get<T>();
        }
    }
};
}

Random::Random(Context* context)
    : ParticleGraphNode(context)
    , pins_{ParticleGraphPin(ParticleGraphPinFlag::MutableType, "out", ParticleGraphContainerType::Span)}
{
}

void Random::RegisterObject(ParticleGraphSystem* context)
{
    auto* reflection = context->AddReflection<Random>();
    reflection->AddAttribute(
        Urho3D::AttributeInfo(VAR_FLOAT, "Min",
                              Urho3D::MakeVariantAttributeAccessor<Random>(
                                  [](const Random& self, Urho3D::Variant& value) { value = self.GetMin(); },
                                  [](Random& self, const Urho3D::Variant& value) { self.SetMin(value); }),
                              nullptr, Variant(), AM_DEFAULT));
    reflection->AddAttribute(
        Urho3D::AttributeInfo(VAR_FLOAT, "Max",
                              Urho3D::MakeVariantAttributeAccessor<Random>(
                                  [](const Random& self, Urho3D::Variant& value) { value = self.GetMax(); },
                                  [](Random& self, const Urho3D::Variant& value) { self.SetMax(value); }),
                              nullptr, Variant(), AM_DEFAULT));
}

Random::Instance::Instance(Random* node)
    : node_(node)
{
}

void Random::Instance::Update(UpdateContext& context)
{
    const auto& pin0 = node_->pins_[0];
    SelectByVariantType<Generate>(pin0.GetValueType(), context, pin0, node_->min_, node_->max_);
};

} // namespace ParticleGraphNodes
} // namespace Urho3D
