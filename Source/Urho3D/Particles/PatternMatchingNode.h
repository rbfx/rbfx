//
// Copyright (c) 2021-2022 the rbfx project.
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
#include "../Core/Context.h"
#include "ParticleGraphLayerInstance.h"
#include "ParticleGraphNode.h"
#include "ParticleGraphNodeInstance.h"

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
