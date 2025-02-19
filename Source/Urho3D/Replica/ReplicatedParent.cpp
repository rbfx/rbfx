// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Replica/ReplicatedParent.h"

#include "Urho3D/Core/Context.h"

namespace Urho3D
{

ReplicatedParent::ReplicatedParent(Context* context)
    : NetworkBehavior(context, CallbackMask)
{
}

ReplicatedParent::~ReplicatedParent()
{
}

void ReplicatedParent::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ReplicatedParent>(Category_Network);

    URHO3D_COPY_BASE_ATTRIBUTES(NetworkBehavior);
}

void ReplicatedParent::InitializeOnServer()
{
    latestSentParentObject_ = GetNetworkObject()->GetParentNetworkId();
}

bool ReplicatedParent::PrepareReliableDelta(NetworkFrame frame)
{
    const auto parentObject = GetNetworkObject()->GetParentNetworkId();
    const bool needUpdate = latestSentParentObject_ != parentObject;
    latestSentParentObject_ = parentObject;
    return needUpdate;
}

void ReplicatedParent::WriteReliableDelta(NetworkFrame frame, Serializer& dest)
{
    dest.WriteUInt(static_cast<unsigned>(latestSentParentObject_));
}

void ReplicatedParent::ReadReliableDelta(NetworkFrame frame, Deserializer& src)
{
    const auto parentObject = static_cast<NetworkId>(src.ReadUInt());
    GetNetworkObject()->SetParentNetworkObject(parentObject);
}

} // namespace Urho3D
