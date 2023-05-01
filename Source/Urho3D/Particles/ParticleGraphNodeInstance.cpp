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
