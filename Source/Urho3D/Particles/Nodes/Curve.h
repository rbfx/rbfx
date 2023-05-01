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
#include "../Helpers.h"
#include "../ParticleGraphNode.h"
#include "../ParticleGraphNodeInstance.h"
#include "../../Graphics/AnimationTrack.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{
/// Sample curve operator.
class URHO3D_API Curve : public ParticleGraphNode
{
    URHO3D_OBJECT(Curve, ParticleGraphNode)

public:
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(Curve* node);
        void Update(UpdateContext& context) override;
        Curve* GetNodeInstace() { return node_; }

        template <typename Out>
        void operator()(
            const UpdateContext& context, unsigned numParticles, const SparseSpan<float>& t, const SparseSpan<Out>& out)
        {
            auto* node = GetNodeInstace();
            for (unsigned i = 0; i < numParticles; ++i)
            {
                out[i] = node->Sample(t[i]).template Get<ea::remove_reference_t<decltype(out[0])>>();
            }
        }
    protected:
        Curve* node_;
    };


public:
    /// Construct.
    explicit Curve(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

    /// Get number of pins.
    unsigned GetNumPins() const override { return static_cast<unsigned>(ea::size(pins_)); }

    /// Get pin by index.
    ParticleGraphPin& GetPin(unsigned index) override { return pins_[index]; }

    float GetDuration() const { return duration_; }
    void SetDuration(float duration) { duration_ = duration; }
    bool IsLooped() const { return isLooped_; }
    void SetLooped(bool isLooped) { isLooped_ = isLooped; }
    const VariantCurve& GetCurve() const { return curve_; }
    void SetCurve(const VariantCurve& curve);

    Variant Sample(float time) const;

    VariantType EvaluateOutputPinType(ParticleGraphPin& pin) override;

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;

private:
    float duration_;
    bool isLooped_;
    VariantCurve curve_;
    ParticleGraphPin pins_[2];
};


} // namespace ParticleGraphNodes

} // namespace Urho3D
