// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Context.h"
#include "Urho3D/Particles/Helpers.h"
#include "Urho3D/Particles/ParticleGraphLayerInstance.h"
#include "Urho3D/Particles/ParticleGraphNode.h"
#include "Urho3D/Particles/ParticleGraphNodeInstance.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{

/// Template for a particle graph node class.
template <typename Instance, typename... Values> class TemplateNode : public ParticleGraphNode
{
public:
    typedef TemplateNode<Instance, Values...> BaseNodeType;
    static constexpr unsigned NumberOfPins = sizeof...(Values);
    typedef ea::array<ParticleGraphPin, NumberOfPins> PinArray;

protected:
    /// Helper methods to assign pin types based on template argument types
    template <typename LastValue> void SetPinTypes(ParticleGraphPin* pins, const ParticleGraphPin* src)
    {
        *pins = (*src).WithType(GetVariantType<LastValue>());
    }

    template <typename First, typename Second, typename... PinTypes>
    void SetPinTypes(ParticleGraphPin* pins, const ParticleGraphPin* src)
    {
        *pins = (*src).WithType(GetVariantType<First>());
        SetPinTypes<Second, PinTypes...>(pins + 1, src + 1);
    }

    /// Construct.
    explicit TemplateNode(Context* context, const PinArray& pins)
        : ParticleGraphNode(context)
    {
        SetPinTypes<Values...>(&pins_[0], &pins[0]);
    }

public:
    class InstanceBase : public ParticleGraphNodeInstance
    {
    public:
        /// Init instance.
        virtual void Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer)
        {
            assert(node != nullptr);
            node_ = node;
            layer_ = layer;
        }
        /// Update particles.
        void Update(UpdateContext& context) override
        {
            ParticleGraphPinRef pinRefs[NumberOfPins];
            for (unsigned i = 0; i < NumberOfPins; ++i)
            {
                pinRefs[i] = node_->GetPin(i).GetMemoryReference();
            }
            RunUpdate<Instance, Values...>(context, *static_cast<Instance*>(this), pinRefs);
        }

        /// Get graph node instance.
        ParticleGraphNode* GetGraphNode() const { return node_; }
        /// Get graph layer instance.
        ParticleGraphLayerInstance* GetLayerInstance() const { return layer_; }
        /// Get graph layer.
        ParticleGraphLayer* GetLayer() const { return layer_->GetLayer(); }
        /// Get emitter component.
        ParticleGraphEmitter* GetEmitter() const { return layer_->GetEmitter(); }
        /// Get scene node.
        Node* GetNode() const;
        /// Get engine context.
        Context* GetContext() const;
        /// Get scene.
        Scene* GetScene() const;

        /// Static assert to simplify compile type error description.
        //template <typename... args> void operator()(args... a) { static_assert(false, "Instance operator() is not defined or arguments doesn't match node pins"); }
    protected:
        /// Pointer to graph node instance.
        ParticleGraphNode* node_{nullptr};
        /// Pointer to graph layer instance.
        ParticleGraphLayerInstance* layer_{nullptr};
    };

public:
    /// Get number of pins.
    unsigned GetNumPins() const override { return NumberOfPins; }

    /// Get pin by index.
    ParticleGraphPin& GetPin(unsigned index) override { return pins_[index]; }

protected:
    /// Pins
    PinArray pins_;
};

template <typename Instance, typename... Values>
Urho3D::Node* TemplateNode<Instance, Values...>::InstanceBase::GetNode() const
{
    const auto* emitter = GetEmitter();
    return (emitter) ? emitter->GetNode() : nullptr;
}

template <typename Instance, typename... Values> Context* TemplateNode<Instance, Values...>::InstanceBase::GetContext() const
{
    const auto* emitter = GetEmitter();
    return (emitter) ? emitter->GetContext() : nullptr;
}

template <typename Instance, typename... Values> Scene* TemplateNode<Instance, Values...>::InstanceBase::GetScene() const
{
    const auto* emitter = GetEmitter();
    return (emitter) ? emitter->GetScene() : nullptr;
}
} // namespace ParticleGraphNodes

} // namespace Urho3D
