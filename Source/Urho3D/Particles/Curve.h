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
#include "../Graphics/AnimationTrack.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{
/// Sample curve operator.
class URHO3D_API Curve : public AbstractNode<Curve, float, float>
{
    URHO3D_OBJECT(Curve, ParticleGraphNode);

public:
    template <typename Tuple>
    static void Op(UpdateContext& context, Instance* instance, unsigned numParticles, Tuple&& spans)
    {
        auto* node = instance->GetNodeInstace();
        auto& t = ea::get<0>(spans);
        auto& out = ea::get<1>(spans);
        for (unsigned i = 0; i < numParticles; ++i)
        {
            out[i] = node->Sample(t[i]).Get<ea::remove_reference_t<decltype(out[0])>>();
        }
    }

public:
    /// Construct.
    explicit Curve(Context* context);

    Variant Sample(float time) const;

    bool LoadProperty(GraphNodeProperty& prop) override;

    bool SaveProperties(ParticleGraphWriter& writer, GraphNode& node) override;

private:
    float duration_;
    bool isLooped_;
    VariantAnimationTrack curve_;
};


} // namespace ParticleGraphNodes

} // namespace Urho3D
