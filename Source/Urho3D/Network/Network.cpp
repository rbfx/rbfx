//
// Copyright (c) 2008-2022 the Urho3D project.
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
#include "../Core/CoreEvents.h"
#include "../Core/Profiler.h"
#include "../Engine/EngineEvents.h"
#include "../Network/Network.h"
#include "../Network/NetworkEvents.h"
#include "../Network/Transport/DataChannel/DataChannelConnection.h"
#include "../Network/Transport/DataChannel/DataChannelServer.h"
#include "../Replica/BehaviorNetworkObject.h"
#include "../Replica/FilteredByDistance.h"
#include "../Replica/FilteredByOwner.h"
#include "../Replica/NetworkObject.h"
#include "../Replica/PredictedKinematicController.h"
#include "../Replica/ReplicatedAnimation.h"
#include "../Replica/ReplicatedParent.h"
#include "../Replica/ReplicatedTransform.h"
#include "../Replica/ReplicationManager.h"
#include "../Replica/StaticNetworkObject.h"
#include "../Replica/TrackedAnimatedModel.h"

#include "../DebugNew.h"

namespace Urho3D
{

Network::Network(Context* context)
    : Object(context)
{
    // Register Network library object factories
    RegisterNetworkLibrary(context_);

    SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(Network, HandleBeginFrame));
    SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(Network, HandleRenderUpdate));
}

Network::~Network()
{
}

void Network::SetUpdateFps(unsigned fps)
{
    updateFps_ = Max(fps, 1);
    updateInterval_ = 1.0f / (float)updateFps_;
    updateAcc_ = 0.0f;
}

void Network::Update(float timeStep)
{
    URHO3D_PROFILE("UpdateNetwork");

    // Check if periodic update should happen now
    updateAcc_ += timeStep;
    updateNow_ = updateAcc_ >= updateInterval_;
    if (updateNow_)
        updateAcc_ = fmodf(updateAcc_, updateInterval_);

    {
        using namespace NetworkInputProcessed;
        VariantMap& eventData = GetEventDataMap();
        eventData[P_TIMESTEP] = timeStep;
        SendEvent(E_NETWORKINPUTPROCESSED, eventData);
    }
}

void Network::PostUpdate(float timeStep)
{
    URHO3D_PROFILE("PostUpdateNetwork");

    // Update periodically on the server
    if (updateNow_)
    {
        SendNetworkUpdateEvent(E_NETWORKUPDATE, true);
        SendNetworkUpdateEvent(E_NETWORKUPDATESENT, true);
    }

    // Always update on the client
    SendNetworkUpdateEvent(E_NETWORKUPDATE, false);
    SendNetworkUpdateEvent(E_NETWORKUPDATESENT, false);
}

void Network::HandleBeginFrame(VariantMap& eventData)
{
    using namespace BeginFrame;

    Update(eventData[P_TIMESTEP].GetFloat());
}

void Network::HandleRenderUpdate(VariantMap& eventData)
{
    using namespace RenderUpdate;

    PostUpdate(eventData[P_TIMESTEP].GetFloat());
}

void Network::SendNetworkUpdateEvent(StringHash eventType, bool isServer)
{
    using namespace NetworkUpdate;
    auto& eventData = GetEventDataMap();
    eventData[P_ISSERVER] = isServer;
    SendEvent(eventType, eventData);
}

void RegisterNetworkLibrary(Context* context)
{
    NetworkObjectRegistry::RegisterObject(context);
    ReplicationManager::RegisterObject(context);

    NetworkObject::RegisterObject(context);
    StaticNetworkObject::RegisterObject(context);
    BehaviorNetworkObject::RegisterObject(context);

    NetworkBehavior::RegisterObject(context);
    ReplicatedAnimation::RegisterObject(context);
    ReplicatedParent::RegisterObject(context);
    ReplicatedTransform::RegisterObject(context);
    TrackedAnimatedModel::RegisterObject(context);
    FilteredByDistance::RegisterObject(context);
    FilteredByOwner::RegisterObject(context);
#ifdef URHO3D_PHYSICS
    PredictedKinematicController::RegisterObject(context);
#endif

    DataChannelConnection::RegisterObject(context);
    DataChannelServer::RegisterObject(context);
}

}
