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

#include "Curve.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
Curve::Curve(Context* context)
    : AbstractNodeType(context, PinArray{
                                    ParticleGraphPin(PGPIN_INPUT, "t"),
                                    ParticleGraphPin(PGPIN_NONE, "out"),
                                })
    , duration_(1)
    , isLooped_(false)
{
}


Variant Curve::Sample(float time) const
{
    unsigned frameIndex;
    return curve_.Sample(time, duration_, isLooped_, frameIndex);
}

bool Curve::LoadProperty(GraphNodeProperty& prop)
{
    auto nameHash = prop.GetNameHash();
    if (nameHash == StringHash("duration"))
    {
        duration_ = prop.value_.GetFloat();
        return true;
    }
    if (nameHash == StringHash("isLooped"))
    {
        isLooped_ = prop.value_.GetBool();
        return true;
    }
    if (nameHash == StringHash("curve"))
    {
        curve_ = *(prop.value_.GetCustomVariantValuePtr()->GetValuePtr<ea::unique_ptr<VariantAnimationTrack>>()->get());
        return true;
    }

    return AbstractNodeType::LoadProperty(prop);
}

bool Curve::SaveProperties(ParticleGraphWriter& writer, GraphNode& node)
{
    node.GetOrAddProperty("duration") = duration_;
    node.GetOrAddProperty("isLooped") = isLooped_;
    node.GetOrAddProperty("curve").SetCustom(ea::make_unique<VariantAnimationTrack>(curve_));

    return AbstractNodeType::SaveProperties(writer, node);
}
} // namespace ParticleGraphNodes

} // namespace Urho3D
