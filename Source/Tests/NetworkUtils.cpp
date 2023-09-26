//
// Copyright (c) 2017-2021 the rbfx project.
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

#include "NetworkUtils.h"
#include "CommonUtils.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Replica/StaticNetworkObject.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Resource/XMLFile.h>

namespace Tests
{

// For easier debugging
unsigned currentSimulationStep = 0;

unsigned ManualConnection::systemTime = 0;

ManualConnection::ManualConnection(Context* context, ReplicationManager* sink, unsigned seed)
    : AbstractConnection(context)
    , sink_(sink)
    , random_(seed)
{
}

void ManualConnection::IncrementTime(unsigned delta)
{
    currentTime_ += delta;

    SendOrderedMessages(messages_[false][true]);
    SendOrderedMessages(messages_[true][true]);

    SendUnorderedMessages(messages_[false][false]);
    SendUnorderedMessages(messages_[true][false]);
}

void ManualConnection::SendMessageInternal(NetworkMessageId messageId, const unsigned char* data, unsigned numBytes, PacketTypeFlags packetType)
{
    const double currentDropRatio = droppedMessages_ / ea::max(1.0, static_cast<double>(totalUnreliableMessages_));
    const double currentShuffleRatio = shuffledMessages_ / ea::max(1.0, static_cast<double>(totalUnorderedMessages_));
    const bool reliable = packetType & PacketType::Reliable;
    const bool inOrder = packetType & PacketType::Ordered;

    ++totalMessages_;
    if (!reliable)
        ++totalUnreliableMessages_;
    if (!inOrder)
        ++totalUnorderedMessages_;

    // Simulate message loss
    if (!reliable && currentDropRatio < quality_.dropRate_)
    {
        ++droppedMessages_;
        return;
    }

    // Simulate shuffle
    auto& outgoingQueue = messages_[reliable][inOrder];
    unsigned index = outgoingQueue.size();
    if (!inOrder && currentShuffleRatio < quality_.shuffleRate_)
    {
        index = random_.GetUInt(0, index);
        ++shuffledMessages_;
    }

    InternalMessage& msg = *outgoingQueue.emplace(outgoingQueue.begin() + index);
    msg.receiveTime_ = currentTime_ + GetPing();
    msg.messageId_ = messageId;
    msg.data_.assign(data, data + numBytes);
}

unsigned ManualConnection::GetPing()
{
    const float mean = (quality_.minPing_ + quality_.maxPing_) / 2;
    const float sigma = (quality_.maxPing_ - quality_.minPing_) / 2;
    // Take two to avoid clustering on min ping
    const float source1 = random_.GetNormalFloat(mean, sigma / 1.5);
    const float source2 = random_.GetNormalFloat(mean, sigma / 1.5);
    const float value = Clamp(source1 >= quality_.minPing_ ? source1 : source2, quality_.minPing_, quality_.spikePing_);
    return static_cast<unsigned>(value * NetworkSimulator::MillisecondsInFrame * NetworkSimulator::FramesInSecond);
}

void ManualConnection::DeliverMessage(const InternalMessage& msg)
{
    MemoryBuffer memoryBuffer(msg.data_);
    sink_->ProcessMessage(sinkConnection_, msg.messageId_, memoryBuffer);
}

void ManualConnection::SendOrderedMessages(ea::vector<InternalMessage>& messages)
{
    const auto firstIgnoredMessageIter = ea::find_if(messages.begin(), messages.end(),
        [&](const InternalMessage& msg) { return msg.receiveTime_ >= currentTime_; });
    ea::for_each(messages.begin(), firstIgnoredMessageIter,
        [&](const InternalMessage& msg) { DeliverMessage(msg); });
    messages.erase(messages.begin(), firstIgnoredMessageIter);
}

void ManualConnection::SendUnorderedMessages(ea::vector<InternalMessage>& messages)
{
    ea::erase_if(messages, [&](const InternalMessage& msg)
    {
        if (msg.receiveTime_ >= currentTime_)
            return false;

        DeliverMessage(msg);
        return true;
    });
}

double NetworkSimulator::QuantizeDuration(double duration, unsigned millisecondsInQuant)
{
    const unsigned numQuantsInSecond = 1000 / millisecondsInQuant;
    REQUIRE(numQuantsInSecond * millisecondsInQuant == 1000);
    return RoundToInt(duration * millisecondsInQuant) / static_cast<double>(millisecondsInQuant);
}

NetworkSimulator::NetworkSimulator(Scene* serverScene, unsigned seed)
    : context_(serverScene->GetContext())
    , network_(context_->GetSubsystem<Network>())
    , random_(seed)
    , serverScene_(serverScene)
{
    serverReplicationManager_ = serverScene_->GetOrCreateComponent<ReplicationManager>();
    serverReplicationManager_->StartServer();
}

NetworkSimulator::~NetworkSimulator()
{
}

void NetworkSimulator::AddClient(Scene* clientScene, const ConnectionQuality& quality)
{
    PerClient data;
    data.clientScene_ = clientScene;
    data.clientReplicationManager_ = clientScene->GetOrCreateComponent<ReplicationManager>();
    data.clientToServer_ = MakeShared<ManualConnection>(context_, serverReplicationManager_, random_.GetUInt());
    data.serverToClient_ = MakeShared<ManualConnection>(context_, data.clientReplicationManager_, random_.GetUInt());

    data.clientToServer_->SetSinkConnection(data.serverToClient_);
    data.clientToServer_->SetQuality(quality);
    data.serverToClient_->SetSinkConnection(data.clientToServer_);
    data.serverToClient_->SetQuality(quality);

    data.clientReplicationManager_->StartClient(data.clientToServer_);
    serverReplicationManager_->GetServerReplicator()->AddConnection(data.serverToClient_);

    clients_.push_back(data);
}

void NetworkSimulator::RemoveClient(Scene* clientScene)
{
    const auto iter = FindClientIter(clientScene);
    if (iter == clients_.end())
        return;

    iter->clientReplicationManager_->StartStandalone();
    serverReplicationManager_->GetServerReplicator()->RemoveConnection(iter->serverToClient_);
    clients_.erase(iter);
}

void NetworkSimulator::SimulateEngineFrame(float timeStep)
{
    const unsigned elapsedNetworkMilliseconds = static_cast<unsigned>(timeStep * MillisecondsInFrame * FramesInSecond);

    // Process client-to-server messages first so server can process them
    for (PerClient& data : clients_)
        data.clientToServer_->IncrementTime(elapsedNetworkMilliseconds);

    // Proces server-to-client messages after.
    // This may result in more client-to-server messages which will be ignored until next frame.
    for (PerClient& data : clients_)
        data.serverToClient_->IncrementTime(elapsedNetworkMilliseconds);

    SimulateEngineFrame(context_, timeStep);
}

void NetworkSimulator::SimulateEngineFrame(Context* context, float timeStep)
{
    auto time = context->GetSubsystem<Time>();
    auto engine = context->GetSubsystem<Engine>();

    // Update client time
    engine->SetNextTimeStep(timeStep);
    time->BeginFrame(timeStep);

    // Process new frame on server if applicable
    engine->Update();

    time->EndFrame();
}

void NetworkSimulator::SimulateTime(Context* context, float time, unsigned millisecondsInQuant)
{
    SimulateTimeCallback(time, millisecondsInQuant,
        [&](float timeStep)
    {
        SimulateEngineFrame(context, timeStep);
    });
}

void NetworkSimulator::SimulateTime(float time, unsigned millisecondsInQuant)
{
    SimulateTimeCallback(time, millisecondsInQuant,
        [&](float timeStep)
    {
        ManualConnection::systemTime += millisecondsInQuant;
        SimulateEngineFrame(timeStep);
    });
}

void NetworkSimulator::SimulateTimeCallback(
    float time, unsigned millisecondsInQuant, ea::function<void(float timeStep)> callback)
{
    REQUIRE(MillisecondsInFrame % millisecondsInQuant == 0);

    const float timeStep = millisecondsInQuant / 1000.0f;
    const float numStepsRaw = time / timeStep;
    const auto numSteps = RoundToInt(numStepsRaw);
    REQUIRE(numSteps == Catch::Approx(numStepsRaw).margin(0.001));

    for (unsigned i = 0; i < numSteps; ++i)
    {
        currentSimulationStep = i;
        callback(timeStep);
    }
}

AbstractConnection* NetworkSimulator::GetServerToClientConnection(Scene* clientScene)
{
    const auto iter = FindClientIter(clientScene);
    return iter != clients_.end() ? iter->serverToClient_ : nullptr;
}

Node* SpawnOnServer(Node* parent, StringHash objectType, PrefabResource* prefab, const ea::string& name,
    const Vector3& position, const Quaternion& rotation)
{
    Node* node = parent->InstantiatePrefab(prefab->GetNodePrefab(), position, rotation);
    node->SetName(name);

    auto networkObject = dynamic_cast<StaticNetworkObject*>(node->CreateComponent(objectType));
    networkObject->SetClientPrefab(prefab);
    return node;
}

}
