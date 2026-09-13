// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/AnimationController.h"
#include "../Replica/FilteredByDistance.h"
#include "../Replica/ReplicationManager.h"
#include "../Replica/ServerReplicator.h"

namespace Urho3D
{

FilteredByDistance::FilteredByDistance(Context* context)
    : NetworkBehavior(context, CallbackMask)
{
}

FilteredByDistance::~FilteredByDistance()
{
}

void FilteredByDistance::RegisterObject(Context* context)
{
    context->AddFactoryReflection<FilteredByDistance>(Category_Network);

    URHO3D_COPY_BASE_ATTRIBUTES(NetworkBehavior);

    URHO3D_ATTRIBUTE("Is Relevant", bool, isRelevant_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Update Period", unsigned, updatePeriod_, 0, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Distance", float, distance_, DefaultDistance, AM_DEFAULT);
}

ea::optional<NetworkObjectRelevance> FilteredByDistance::GetRelevanceForClient(ReplicatedPeer* connection)
{
    // Never filter owned objects
    if (GetNetworkObject()->GetOwnerConnection() == connection)
        return ea::nullopt;

    ReplicationManager* replicationManager = GetNetworkObject()->GetReplicationManager();
    ServerReplicator* serverReplicator = replicationManager->GetServerReplicator();
    const auto& ownedObjects = serverReplicator->GetNetworkObjectsOwnedByConnection(connection);

    float distanceToConnectionObjects = M_LARGE_VALUE;
    for (NetworkObject* networkObject : ownedObjects)
    {
        if (const auto distance = networkObject->CalculateDistanceForFiltering(GetNetworkObject()))
            distanceToConnectionObjects = ea::min(distanceToConnectionObjects, *distance);
    }

    if (distanceToConnectionObjects < distance_)
        return ea::nullopt;

    if (!isRelevant_)
        return NetworkObjectRelevance::Irrelevant;

    static constexpr auto maxPeriod = static_cast<unsigned>(NetworkObjectRelevance::MaxPeriod);
    return static_cast<NetworkObjectRelevance>(ea::min(updatePeriod_, maxPeriod));
}

}
