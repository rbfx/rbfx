// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Deserializer.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/Serializer.h"
#include "Urho3D/Resource/XMLElement.h"
#include "Urho3D/Resource/JSONValue.h"
#include "Urho3D/Scene/UnknownComponent.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

UnknownComponent::UnknownComponent(Context* context) :
    Component(context)
{
}

void UnknownComponent::RegisterObject(Context* context)
{
    context->AddFactoryReflection<UnknownComponent>();
}

bool UnknownComponent::Load(Deserializer& source)
{
    SetTemporary(true);
    return true;
}

bool UnknownComponent::LoadXML(const XMLElement& source)
{
    SetTemporary(true);
    return true;
}

bool UnknownComponent::LoadJSON(const JSONValue& source)
{
    SetTemporary(true);
    return true;
}

}
