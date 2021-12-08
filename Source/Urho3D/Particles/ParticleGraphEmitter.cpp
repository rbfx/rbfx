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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "ParticleGraphEmitter.h"

#include "../Resource/ResourceCache.h"
#include "../Resource/ResourceEvents.h"
#include "ParticleGraphLayer.h"
#include "ParticleGraphLayerInstance.h"

namespace Urho3D
{


ParticleGraphEmitter::ParticleGraphEmitter(Context* context)
    : Component(context)
{
}

ParticleGraphEmitter::~ParticleGraphEmitter() = default;

void ParticleGraphEmitter::RegisterObject(Context* context)
{
    context->RegisterFactory<ParticleGraphEmitter>();

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Effect", GetEffectAttr, SetEffectAttr, ResourceRef,
                                    ResourceRef(ParticleGraphEmitter::GetTypeStatic()), AM_DEFAULT);
}

void ParticleGraphEmitter::OnSetEnabled()
{
    Component::OnSetEnabled();

    Scene* scene = GetScene();
    if (scene)
    {
        if (IsEnabledEffective())
            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(ParticleGraphEmitter, HandleScenePostUpdate));
        else
            UnsubscribeFromEvent(scene, E_SCENEPOSTUPDATE);
    }
}

void ParticleGraphEmitter::Reset()
{
    layers_.clear();
}

void ParticleGraphEmitter::ApplyEffect()
{
    if (!effect_)
        return;

    const auto numLayers = effect_->GetNumLayers();
    layers_.resize(numLayers);

    for (unsigned i = 0; i < numLayers; ++i)
    {
        layers_[i].SetEmitter(this);
        layers_[i].Apply(effect_->GetLayer(i));
    }
}

void ParticleGraphEmitter::SetEffect(ParticleGraphEffect* effect)
{
    if (effect == effect_)
        return;

    Reset();

    // Unsubscribe from the reload event of previous effect (if any), then subscribe to the new
    if (effect_)
        UnsubscribeFromEvent(effect_, E_RELOADFINISHED);

    effect_ = effect;

    if (effect_)
        SubscribeToEvent(effect_, E_RELOADFINISHED, URHO3D_HANDLER(ParticleGraphEmitter, HandleEffectReloadFinished));

    ApplyEffect();
    MarkNetworkUpdate();
}

ParticleGraphEffect* ParticleGraphEmitter::GetEffect() const
{
    return effect_;
}

void ParticleGraphEmitter::SetEffectAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetEffect(cache->GetResource<ParticleGraphEffect>(value.name_));
}

ResourceRef ParticleGraphEmitter::GetEffectAttr() const
{
    return GetResourceRef(effect_, ParticleGraphEffect::GetTypeStatic());
}


void ParticleGraphEmitter::OnSceneSet(Scene* scene)
{
    Component::OnSceneSet(scene);

    if (scene && IsEnabledEffective())
        SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(ParticleGraphEmitter, HandleScenePostUpdate));
    else if (!scene)
        UnsubscribeFromEvent(E_SCENEPOSTUPDATE);
}

bool ParticleGraphEmitter::EmitNewParticle(unsigned layer)
{
    if (layer >= layers_.size())
        return false;

    layers_[layer].EmitNewParticle();

    return true;
}

void ParticleGraphEmitter::Tick(float timeStep)
{
    for (unsigned i = 0; i < layers_.size(); ++i)
    {
        layers_[i].Update(timeStep);
    }
}

const ParticleGraphLayerInstance* ParticleGraphEmitter::GetLayer(unsigned layer) const
{
    if (layer >= layers_.size())
        return nullptr;
    return &layers_[layer];
}

ParticleGraphLayerInstance* ParticleGraphEmitter::GetLayer(unsigned layer)
{
    if (layer >= layers_.size())
        return nullptr;
    return &layers_[layer];
}

bool ParticleGraphEmitter::CheckActiveParticles() const
{
    for (unsigned i = 0; i < layers_.size(); ++i)
    {
        if (layers_[i].CheckActiveParticles())
            return true;
    }

    return false;
}

void ParticleGraphEmitter::HandleScenePostUpdate(StringHash eventType, VariantMap& eventData)
{
    // Store scene's timestep and use it instead of global timestep, as time scale may be other than 1
    using namespace ScenePostUpdate;

    lastTimeStep_ = eventData[P_TIMESTEP].GetFloat();


    Tick(lastTimeStep_);

    //// If no invisible update, check that the billboardset is in view (framenumber has changed)
    //if ((effect_ && effect_->GetUpdateInvisible()) || viewFrameNumber_ != lastUpdateFrameNumber_)
    //{
    //    lastUpdateFrameNumber_ = viewFrameNumber_;
    //    needUpdate_ = true;
    //    MarkForUpdate();
    //}

    //// Send finished event only once all particles are gone
    //if (node_ && !emitting_ && sendFinishedEvent_ && !CheckActiveParticles())
    //{
    //    sendFinishedEvent_ = false;

    //    // Make a weak pointer to self to check for destruction during event handling
    //    WeakPtr<ParticleGraphEmitter> self(this);

    //    using namespace ParticleEffectFinished;

    //    VariantMap& eventData = GetEventDataMap();
    //    eventData[P_NODE] = node_;
    //    eventData[P_EFFECT] = effect_;

    //    node_->SendEvent(E_PARTICLEEFFECTFINISHED, eventData);

    //    if (self.Expired())
    //        return;

    //    DoAutoRemove(autoRemove_);
    //}
}

void ParticleGraphEmitter::HandleEffectReloadFinished(StringHash eventType, VariantMap& eventData)
{
    // When particle effect file is live-edited, remove existing particles and reapply the effect parameters
    Reset();
    ApplyEffect();
}

} // namespace Urho3D
