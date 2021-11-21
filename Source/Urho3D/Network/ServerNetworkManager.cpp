//
// Copyright (c) 2008-2020 the Urho3D project.
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
#include "../Core/Exception.h"
#include "../IO/Log.h"
#include "../Network/Connection.h"
#include "../Network/Network.h"
#include "../Network/NetworkEvents.h"
#include "../Network/NetworkComponent.h"
#include "../Network/ServerNetworkManager.h"
#include "../Scene/Scene.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

ServerNetworkManager::ServerNetworkManager(Scene* scene)
    : Object(scene->GetContext())
    , network_(GetSubsystem<Network>())
    , scene_(scene)
{
    SubscribeToEvent(network_, E_NETWORKUPDATE, [this](StringHash, VariantMap&)
    {
        BeginNetworkFrame();
        for (const auto& [connection, data] : connections_)
            SendUpdate(connection);
    });
}

void ServerNetworkManager::BeginNetworkFrame()
{
    const float timeStep = 1.0f / updateFrequency_;
    updateFrequency_ = network_->GetUpdateFps();
    ++currentFrame_;

    for (auto& [connection, data] : connections_)
    {
        if (!data.synchronized_)
            return;

        data.pingAccumulator_ += timeStep;
        data.clockAccumulator_ += timeStep;
    }
}

void ServerNetworkManager::AddConnection(AbstractConnection* connection)
{
    if (connections_.contains(connection))
    {
        URHO3D_LOGWARNING("Connection is already added");
        assert(0);
    }

    ClientConnectionData& data = connections_[connection];
    data = ClientConnectionData{};

    data.connection_ = connection;
    data.comfirmedPings_.set_capacity(settings_.numInitialPings_);
    data.comfirmedPingsSorted_.set_capacity(settings_.numInitialPings_);
    data.pendingPings_.set_capacity(settings_.maxOngoingPings_);
}

void ServerNetworkManager::RemoveConnection(AbstractConnection* connection)
{
    if (!connections_.contains(connection))
    {
        URHO3D_LOGWARNING("Connection is not added");
        assert(0);
    }

    connections_.erase(connection);
}

void ServerNetworkManager::SendUpdate(AbstractConnection* connection)
{
    ClientConnectionData& data = GetConnection(connection);

    // Initialization phase: send "reliable" pings one by one
    if (!data.comfirmedPings_.full() && data.pendingPings_.empty())
    {
        const unsigned magic = GetMagic(true);
        data.pendingPings_.push_back(ClientPing{ magic });

        connection->SendMessage(MSG_PING, MsgPingPong{ magic }, NetworkMessageFlag::Reliable);
        return;
    }

    // Synchronization phase.
    if (!data.synchronized_)
    {
        if (!data.pendingSynchronization_)
        {
            const unsigned magic = GetMagic();
            const unsigned ping = data.averagePing_;

            data.pendingSynchronization_ = magic;
            const MsgSynchronize msg{ magic, currentFrame_, ping, updateFrequency_,
                settings_.clockBufferSize_, settings_.clockBufferSkippedTailsLength_ };
            connection->SendMessage(MSG_SYNCHRONIZE, msg, NetworkMessageFlag::Reliable);
        }
        return;
    }

    // Send periodic ping
    if (data.pingAccumulator_ >= settings_.pingIntervalMs_ / 1000.0f)
    {
        data.pingAccumulator_ = 0.0f;

        const unsigned magic = GetMagic();
        data.pendingPings_.push_back(ClientPing{ magic });

        connection->SendMessage(MSG_PING, MsgPingPong{ magic }, NetworkMessageFlag::None);
    }

    // Send periodic clock
    if (data.clockAccumulator_ >= settings_.clockIntervalMs_ / 1000.0f)
    {
        data.clockAccumulator_ = 0.0f;
        connection->SendMessage(MSG_CLOCK, MsgClock{ currentFrame_, data.averagePing_ }, NetworkMessageFlag::None);
    }
}

void ServerNetworkManager::ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    switch (messageId)
    {
    case MSG_PONG:
    {
        ClientConnectionData& data = GetConnection(connection);
        const auto& msg = ReadNetworkMessage<MsgPingPong>(messageData);
        connection->OnMessageReceived(messageId, msg);

        const auto iter = ea::find_if(data.pendingPings_.begin(), data.pendingPings_.end(),
            [&](const ClientPing& ping) { return ping.magic_ == msg.magic_; });
        if (iter == data.pendingPings_.end())
        {
            URHO3D_LOGWARNING("{}: Unexpected pong received", connection->ToString(), msg.magic_);
            break;
        }

        ClientPing& pendingPing = *iter;
        const auto pingMs = pendingPing.timer_.GetUSec(false) / 1000 / 2;
        data.pendingPings_.erase(iter);

        if (data.comfirmedPings_.full())
            data.comfirmedPings_.pop_front();
        data.comfirmedPings_.push_back(pingMs);

        RecalculateAvergagePing(data);
        URHO3D_LOGDEBUG("{}: Ping of {}ms added to buffer, average is {}ms", connection->ToString(), msg.magic_, pingMs, data.averagePing_);
        break;
    }

    case MSG_SYNCHRONIZE_ACK:
    {
        ClientConnectionData& data = GetConnection(connection);
        const auto& msg = ReadNetworkMessage<MsgSynchronizeAck>(messageData);
        connection->OnMessageReceived(messageId, msg);

        if (!data.pendingSynchronization_ || data.pendingSynchronization_ != msg.magic_)
        {
            URHO3D_LOGWARNING("{}: Unexpected synchronization ack received", connection->ToString(), msg.magic_);
            break;
        }

        data.pendingSynchronization_ = ea::nullopt;
        data.synchronized_ = true;
    }

    default:
        break;
    }
}

void ServerNetworkManager::SetTestPing(AbstractConnection* connection, unsigned ping)
{
    ClientConnectionData& data = GetConnection(connection);
    data.overridePing_ = ping;
    RecalculateAvergagePing(data);
}

void ServerNetworkManager::SetCurrentFrame(unsigned frame)
{
    currentFrame_ = frame;
}

ClientConnectionData& ServerNetworkManager::GetConnection(AbstractConnection* connection)
{
    const auto iter = connections_.find(connection);
    assert(iter != connections_.end());
    return iter->second;
}

void ServerNetworkManager::RecalculateAvergagePing(ClientConnectionData& data)
{
    if (data.overridePing_)
    {
        data.averagePing_ = *data.overridePing_;
        return;
    }

    if (data.comfirmedPings_.empty())
    {
        data.averagePing_ = M_MAX_UNSIGNED;
        return;
    }

    data.comfirmedPingsSorted_.assign(data.comfirmedPings_.begin(), data.comfirmedPings_.end());
    ea::sort(data.comfirmedPingsSorted_.begin(), data.comfirmedPingsSorted_.end());

    const unsigned numPings = data.comfirmedPingsSorted_.size();
    const unsigned tailsLength = ea::min(settings_.pingsBufferSkippedTailsLength_, (numPings - 1) / 2);
    const auto beginIter = std::next(data.comfirmedPingsSorted_.begin(), tailsLength);
    const auto endIter = std::prev(data.comfirmedPingsSorted_.end(), tailsLength);
    data.averagePing_ = ea::accumulate(beginIter, endIter, 0) / ea::distance(beginIter, endIter);
}

unsigned ServerNetworkManager::GetMagic(bool reliable) const
{
    // TODO: It doesn't look secure.
    const unsigned value = Rand();
    return reliable ? (value | 1u) : (value & ~1u);
}

ea::string ServerNetworkManager::ToString() const
{
    const ea::string& sceneName = scene_->GetName();
    return Format("Scene '{}': Frame #{}",
        !sceneName.empty() ? sceneName : "Unnamed",
        currentFrame_);
}

}
