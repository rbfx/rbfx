// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "ParticleGraphEmitter.h"

#include "../Core/Context.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ResourceEvents.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
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
    context->AddFactoryReflection<ParticleGraphEmitter>(Category_Geometry);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Effect", GetEffectAttr, SetEffectAttr, ResourceRef,
                                    ResourceRef(ParticleGraphEmitter::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("View Mask", GetViewMask, SetViewMask, unsigned, DEFAULT_VIEWMASK, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Light Mask", GetLightMask, SetLightMask, unsigned, DEFAULT_LIGHTMASK, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Shadow Mask", GetShadowMask, SetShadowMask, unsigned, DEFAULT_SHADOWMASK, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Zone Mask", GetZoneMask, SetZoneMask, unsigned, DEFAULT_ZONEMASK, AM_DEFAULT);
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
    for (auto& layer : layers_)
    {
        layer.Reset();
    }
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

    Reset();
}

void ParticleGraphEmitter::SetEmitting(bool enable)
{
    if (enable != emitting_)
    {
        emitting_ = enable;

        // If stopping emission now, and there are active particles, send finish event once they are gone
        //sendFinishedEvent_ = enable || CheckActiveParticles();
        //periodTimer_ = 0.0f;
    }
}

void ParticleGraphEmitter::RemoveAllParticles()
{
    for (unsigned i = 0; i < layers_.size(); ++i)
    {
        layers_[i].RemoveAllParticles();
    }
}

void ParticleGraphEmitter::SetEffect(ParticleGraphEffect* effect)
{
    if (effect == effect_)
        return;

    layers_.clear();

    // Unsubscribe from the reload event of previous effect (if any), then subscribe to the new
    if (effect_)
        UnsubscribeFromEvent(effect_, E_RELOADFINISHED);

    effect_ = effect;

    if (effect_)
        SubscribeToEvent(effect_, E_RELOADFINISHED, URHO3D_HANDLER(ParticleGraphEmitter, HandleEffectReloadFinished));

    ApplyEffect();
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

void ParticleGraphEmitter::SetViewMask(unsigned mask)
{
    if (viewMask_ != mask)
    {
        viewMask_ = mask;
        UpdateDrawables();
    }
}

void ParticleGraphEmitter::SetLightMask(unsigned mask)
{
    if (lightMask_ != mask)
    {
        lightMask_ = mask;
        UpdateDrawables();
    }
}

void ParticleGraphEmitter::SetShadowMask(unsigned mask)
{
    if (shadowMask_ != mask)
    {
        shadowMask_ = mask;
        UpdateDrawables();
    }
}

void ParticleGraphEmitter::SetZoneMask(unsigned mask)
{
    if (zoneMask_ != mask)
    {
        zoneMask_ = mask;
        UpdateDrawables();
    }
}

void ParticleGraphEmitter::UpdateDrawables()
{
    for (auto& layer : layers_)
    {
        layer.UpdateDrawables();
    }
}

ResourceRef ParticleGraphEmitter::GetEffectAttr() const
{
    return GetResourceRef(effect_, ParticleGraphEffect::GetTypeStatic());
}


void ParticleGraphEmitter::OnSceneSet(Scene* previousScene, Scene* scene)
{
    if (scene && IsEnabledEffective())
        SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(ParticleGraphEmitter, HandleScenePostUpdate));
    else if (!scene)
        UnsubscribeFromEvent(E_SCENEPOSTUPDATE);

    for (unsigned i = 0; i < layers_.size(); ++i)
    {
            layers_[i].OnSceneSet(scene);
    }
}

bool ParticleGraphEmitter::EmitNewParticle(unsigned layer)
{
    if (layer >= layers_.size())
        return false;

    layers_[layer].EmitNewParticles();

    return true;
}

void ParticleGraphEmitter::Tick(float timeStep)
{
    for (unsigned i = 0; i < layers_.size(); ++i)
    {
        layers_[i].Update(timeStep, emitting_);
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
        if (layers_[i].GetNumActiveParticles() > 0)
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
    layers_.clear();
    ApplyEffect();
}

} // namespace Urho3D
