// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Urho2D/StretchableSprite2D.h"
#include "Urho3D/Urho2D/AnimatedSprite2D.h"
#include "Urho3D/Urho2D/AnimationSet2D.h"
#include "Urho3D/Urho2D/ParticleEffect2D.h"
#include "Urho3D/Urho2D/ParticleEmitter2D.h"
#include "Urho3D/Urho2D/Renderer2D.h"
#include "Urho3D/Urho2D/Sprite2D.h"
#include "Urho3D/Urho2D/SpriteSheet2D.h"
#include "Urho3D/Urho2D/TileMap2D.h"
#include "Urho3D/Urho2D/TileMapLayer2D.h"
#include "Urho3D/Urho2D/TmxFile2D.h"
#include "Urho3D/Urho2D/Urho2D.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

void RegisterUrho2DLibrary(Context* context)
{
    Renderer2D::RegisterObject(context);

    Sprite2D::RegisterObject(context);
    SpriteSheet2D::RegisterObject(context);

    // Must register objects from base to derived order
    Drawable2D::RegisterObject(context);
    StaticSprite2D::RegisterObject(context);

    StretchableSprite2D::RegisterObject(context);

    AnimationSet2D::RegisterObject(context);
    AnimatedSprite2D::RegisterObject(context);

    ParticleEffect2D::RegisterObject(context);
    ParticleEmitter2D::RegisterObject(context);

    TmxFile2D::RegisterObject(context);
    TileMap2D::RegisterObject(context);
    TileMapLayer2D::RegisterObject(context);
}

}
