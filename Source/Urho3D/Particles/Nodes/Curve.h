// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
