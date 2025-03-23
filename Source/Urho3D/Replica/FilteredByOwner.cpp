// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Replica/FilteredByOwner.h"

#include "Urho3D/Core/Context.h"

namespace Urho3D
{

FilteredByOwner::FilteredByOwner(Context* context)
    : NetworkBehavior(context, CallbackMask)
{
}

FilteredByOwner::~FilteredByOwner()
{
}

void FilteredByOwner::RegisterObject(Context* context)
{
    context->AddFactoryReflection<FilteredByOwner>(Category_Network);

    URHO3D_COPY_BASE_ATTRIBUTES(NetworkBehavior);
}

ea::optional<NetworkObjectRelevance> FilteredByOwner::GetRelevanceForClient(AbstractConnection* connection)
{
    const bool isOwner = GetNetworkObject()->GetOwnerConnection() == connection;
    return !isOwner ? ea::make_optional<NetworkObjectRelevance>(NetworkObjectRelevance::Irrelevant) : ea::nullopt;
}

} // namespace Urho3D
