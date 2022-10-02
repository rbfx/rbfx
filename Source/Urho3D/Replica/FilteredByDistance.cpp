//
// Copyright (c) 2017-2020 the rbfx project.
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

ea::optional<NetworkObjectRelevance> FilteredByDistance::GetRelevanceForClient(AbstractConnection* connection)
{
    // Never filter owned objects
    if (GetNetworkObject()->GetOwnerConnection() == connection)
        return ea::nullopt;

    ReplicationManager* replicationManager = GetNetworkObject()->GetReplicationManager();
    ServerReplicator* serverReplicator = replicationManager->GetServerReplicator();
    const auto& ownedObjects = serverReplicator->GetNetworkObjectsOwnedByConnection(connection);

    const Vector3 thisPosition = GetNode()->GetWorldPosition();
    float distanceToConnectionObjects = M_LARGE_VALUE;

    for (NetworkObject* networkObject : ownedObjects)
    {
        const Vector3 otherPosition = networkObject->GetNode()->GetWorldPosition();
        const float distance = (thisPosition - otherPosition).Length();
        distanceToConnectionObjects = ea::min(distanceToConnectionObjects, distance);
    }

    if (distanceToConnectionObjects < distance_)
        return ea::nullopt;

    if (!isRelevant_)
        return NetworkObjectRelevance::Irrelevant;

    static constexpr auto maxPeriod = static_cast<unsigned>(NetworkObjectRelevance::MaxPeriod);
    return static_cast<NetworkObjectRelevance>(ea::min(updatePeriod_, maxPeriod));
}

}
