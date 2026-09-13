// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "ParticleGraphEmitter.h"
#include "ParticleGraphPin.h"
#include "../Core/NonCopyable.h"

namespace Urho3D
{

class URHO3D_API ParticleGraphNodeInstance : public NonCopyable
{
public:
    ParticleGraphNodeInstance();
    virtual ~ParticleGraphNodeInstance();

    virtual void Update(UpdateContext& context) = 0;
    /// Handle scene change in instance.
    virtual void OnSceneSet(Scene* scene);
    /// Handle drawable attribute change.
    virtual void UpdateDrawableAttributes();

    virtual void Reset();
protected:
    /// Copy drawable attributes from emitter.
    void CopyDrawableAttributes(Drawable* drawable, ParticleGraphEmitter* emitter);
};

} // namespace Urho3D
