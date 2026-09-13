// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../IO/Deserializer.h"
#include "../IO/Log.h"
#include "../IO/Serializer.h"
#include "../Resource/XMLElement.h"
#include "../Resource/JSONValue.h"
#include "../Scene/UnknownComponent.h"

#include "../DebugNew.h"

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
