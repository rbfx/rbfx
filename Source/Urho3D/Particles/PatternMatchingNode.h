// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/Helpers.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphNode.h"
#include "Urho3D/Particles/ParticleGraphNodeInstance.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{
struct NodePattern
{
    static constexpr size_t ExpectedNumberOfPins = 4;

    typedef ea::function<void(UpdateContext& context, ParticleGraphPinRef*)> UpdateFunction;

    explicit NodePattern(UpdateFunction&& update);

    NodePattern& WithPin(ParticleGraphPin&& pin0);

    bool Match(const ea::span<ParticleGraphPin>& pins) const;

    VariantType EvaluateOutputPinType(const ea::span<ParticleGraphPin>& pins, const ParticleGraphPin& outputPin) const;

    template <typename T> void SetPins(T lastPin)
    {
        pins_.emplace_back(lastPin.GetFlags(), lastPin.GetName(), GetVariantType<typename T::Type>());
    }

    template <typename T, typename... Rest> void SetPins(T lastPin, Rest... restPins)
    {
        pins_.emplace_back(lastPin.GetFlags(), lastPin.GetName(), GetVariantType<typename T::Type>());
        SetPins(restPins...);
    }

    UpdateFunction updateFunction_;
    ea::fixed_vector<ParticleGraphPin, ExpectedNumberOfPins> pins_;
};

template <typename Lambda, typename... Args> NodePattern MakePattern(Lambda lambda, Args... args)
{
    auto l = [&](UpdateContext& context, ParticleGraphPinRef* pinRefs)
    { RunUpdate<Lambda, Args...>(context, lambda, pinRefs); };
    NodePattern pattern(l);
    pattern.SetPins(args...);
    return pattern;
}

/// Graph node that adapts to input pins dynamically.
class URHO3D_API PatternMatchingNode : public ParticleGraphNode
{
public:
    struct Instance : public ParticleGraphNodeInstance
    {
        /// Construct.
        explicit Instance(PatternMatchingNode* node, const NodePattern& pattern);

        /// Update particles.
        virtual void Update(UpdateContext& context);

        PatternMatchingNode* node_;
        const NodePattern& pattern_;
    };

    /// Construct.
    explicit PatternMatchingNode(Context* context, const ea::vector<NodePattern>& patterns);

    /// Get number of pins.
    unsigned GetNumPins() const override;

    /// Get pin by index.
    ParticleGraphPin& GetPin(unsigned index) override;

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override;

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override;

protected:
    /// Load input pin.
    ParticleGraphPin* LoadInputPin(ParticleGraphReader& reader, GraphInPin& pin) override;
    /// Load output pin.
    ParticleGraphPin* LoadOutputPin(ParticleGraphReader& reader, GraphOutPin& pin) override;

    /// Evaluate runtime output pin type.
    VariantType EvaluateOutputPinType(ParticleGraphPin& pin) override;

    /// Update particles.
    void Update(UpdateContext& context, const NodePattern& pattern);

protected:
    const ea::vector<NodePattern>& patterns_;
    ea::fixed_vector<ParticleGraphPin, NodePattern::ExpectedNumberOfPins> pins_;
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
