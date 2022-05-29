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

#include "ParticleGraphEffect.h"
#include "../Graphics/Drawable.h"
#include "../Scene/Component.h"

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

class ParticleGraphLayerInstance;
class ParticleGraphNodeInstance;

/// %Particle graph emitter component.
class URHO3D_API ParticleGraphEmitter : public Component
{
    URHO3D_OBJECT(ParticleGraphEmitter, Component)

public:
    /// Construct.
    explicit ParticleGraphEmitter(Context* context);
    /// Destruct.
    ~ParticleGraphEmitter() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Set particle effect.
    void SetEffect(ParticleGraphEffect* effect);
    /// Reset the particle emitter completely. Removes current particles, sets emitting state on, and resets the
    /// emission timer.
    void Reset();
    /// Apply not continuously updated values such as the material, the number of particles and sorting mode from the
    /// particle effect. Call this if you change the effect programmatically.
    void ApplyEffect();

    /// Set whether should be emitting. If the state was changed, also resets the emission period timer.
    /// @property
    void SetEmitting(bool enable);
    /// Return whether is currently emitting.
    /// @property
    bool IsEmitting() const { return emitting_; }

    /// Set to remove either the emitter component or its owner node from the scene automatically on particle effect
    /// completion. Disabled by default.
    /// @property
    void SetAutoRemoveMode(AutoRemoveMode mode);
    /// Return automatic removal mode on particle effect completion.
    /// @property
    AutoRemoveMode GetAutoRemoveMode() const { return autoRemove_; }
    /// Remove all current particles.
    void RemoveAllParticles();


    /// Return particle effect.
    ParticleGraphEffect* GetEffect() const;

    /// Set particles effect attribute.
    void SetEffectAttr(const ResourceRef& value);
    /// Set particles effect attribute.
    ResourceRef GetEffectAttr() const;

    /// Create a new particle. Return true if there was room.
    bool EmitNewParticle(unsigned layer);

    /// Manually update emitter.
    void Tick(float timeStep);

    /// Get layer by index.
    const ParticleGraphLayerInstance* GetLayer(unsigned layer) const;

    /// Get layer by index.
    ParticleGraphLayerInstance* GetLayer(unsigned layer);

    /// Return whether has active particles.
    bool CheckActiveParticles() const;

protected:
    /// Handle scene being assigned.
    void OnSceneSet(Scene* scene) override;

private:
    /// Handle scene post-update event.
    void HandleScenePostUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle live reload of the particle effect.
    void HandleEffectReloadFinished(StringHash eventType, VariantMap& eventData);

    /// Particle effect.
    SharedPtr<ParticleGraphEffect> effect_;

    ea::vector<ParticleGraphLayerInstance> layers_;

    /// Last scene timestep.
    float lastTimeStep_{};

    /// Currently emitting flag.
    bool emitting_{true};
    /// Automatic removal mode.
    AutoRemoveMode autoRemove_{REMOVE_DISABLED};
};

}
