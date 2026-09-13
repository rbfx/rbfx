// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Audio/SoundListener.h"
#include "../Core/Context.h"

namespace Urho3D
{

SoundListener::SoundListener(Context* context) :
    Component(context)
{
}

SoundListener::~SoundListener() = default;

void SoundListener::RegisterObject(Context* context)
{
    context->AddFactoryReflection<SoundListener>(Category_Audio);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
}

}
