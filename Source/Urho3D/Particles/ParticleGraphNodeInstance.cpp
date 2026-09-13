// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "ParticleGraphNodeInstance.h"

namespace Urho3D
{

ParticleGraphNodeInstance::ParticleGraphNodeInstance() = default;

ParticleGraphNodeInstance::~ParticleGraphNodeInstance() = default;

void ParticleGraphNodeInstance::Reset() {}

void ParticleGraphNodeInstance::CopyDrawableAttributes(Drawable* drawable, ParticleGraphEmitter* emitter)
{
    if (!drawable || !emitter)
        return;

    drawable->SetViewMask(emitter->GetViewMask());
    drawable->SetLightMask(emitter->GetLightMask());
    drawable->SetShadowMask(emitter->GetShadowMask());
    drawable->SetZoneMask(emitter->GetZoneMask());
}

/// Handle scene change in instance.
void ParticleGraphNodeInstance::OnSceneSet(Scene* scene) {}

/// Handle drawable attribute change.
void ParticleGraphNodeInstance::UpdateDrawableAttributes() {}

} // namespace Urho3D
