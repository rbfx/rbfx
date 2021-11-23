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

#pragma once

#include "NetworkUtils.h"
#include "CommonUtils.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Scene/Scene.h>

namespace Tests
{

ManualConnection::ManualConnection(Context* context, NetworkManager* sink, unsigned seed)
    : AbstractConnection(context)
    , sink_(sink)
    , random_(seed)
{
}

void ManualConnection::IncrementTime(unsigned delta)
{
    currentTime_ += delta;
    for (auto& messagesByOrder : messages_)
    {
        for (auto& messages : messagesByOrder)
        {
            ea::erase_if(messages, [&](const InternalMessage& msg)
            {
                if (msg.sendingTime_ >= currentTime_)
                    return false;

                MemoryBuffer memoryBuffer(msg.data_);
                sink_->ProcessMessage(sinkConnection_, msg.messageId_, memoryBuffer);
                return true;
            });
        }
    }
}

void ManualConnection::SendMessageInternal(NetworkMessageId messageId, bool reliable, bool inOrder, const unsigned char* data, unsigned numBytes)
{
    const double currentDropRatio = droppedMessages_ / ea::max(1.0, static_cast<double>(totalMessages_));
    const double currentShuffleRatio = shuffledMessages_ / ea::max(1.0, static_cast<double>(totalMessages_));

    ++totalMessages_;

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
    msg.sendingTime_ = currentTime_ + GetPing();
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

NetworkSimulator::NetworkSimulator(Scene* serverScene, unsigned seed)
    : context_(serverScene->GetContext())
    , network_(context_->GetSubsystem<Network>())
    , random_(seed)
    , serverScene_(serverScene)
    , serverNetworkManager_(serverScene_->GetNetworkManager())
{
    serverNetworkManager_->MarkAsServer();
}

void NetworkSimulator::AddClient(Scene* clientScene, const ConnectionQuality& quality)
{
    PerClient data;
    data.clientScene_ = clientScene;
    data.clientNetworkManager_ = clientScene->GetNetworkManager();
    data.clientToServer_ = MakeShared<ManualConnection>(context_, serverNetworkManager_, random_.GetUInt());
    data.serverToClient_ = MakeShared<ManualConnection>(context_, data.clientNetworkManager_, random_.GetUInt());

    data.clientToServer_->SetSinkConnection(data.serverToClient_);
    data.clientToServer_->SetPing(quality);
    data.serverToClient_->SetSinkConnection(data.clientToServer_);
    data.serverToClient_->SetPing(quality);

    data.clientNetworkManager_->MarkAsClient(data.clientToServer_);
    serverNetworkManager_->AsServer().AddConnection(data.serverToClient_);
    serverNetworkManager_->AsServer().SetTestPing(data.serverToClient_, RoundToInt((quality.maxPing_ + quality.minPing_) / 2 * 1000));

    clientScenes_.push_back(data);
}

void NetworkSimulator::SimulateEngineFrame(float timeStep)
{
    auto time = context_->GetSubsystem<Time>();
    auto engine = context_->GetSubsystem<Engine>();

    const unsigned elapsedNetworkMilliseconds = static_cast<unsigned>(timeStep * MillisecondsInFrame * FramesInSecond);

    // Process client-to-server messages first so server can process them
    for (PerClient& data : clientScenes_)
        data.clientToServer_->IncrementTime(elapsedNetworkMilliseconds);

    // Proces server-to-client messages after.
    // This may result in more client-to-server messages which will be ignored until next frame.
    for (PerClient& data : clientScenes_)
        data.serverToClient_->IncrementTime(elapsedNetworkMilliseconds);

    // Update client time
    engine->SetNextTimeStep(timeStep);
    time->BeginFrame(timeStep);

    // Process new frame on server if applicable
    engine->Update();

    time->EndFrame();
}

void NetworkSimulator::SimulateTime(float time)
{
    const float timeStep = 1.0f / (FramesInSecond * MillisecondsInFrame);
    for (unsigned i = 0; i < time / timeStep; ++i)
        SimulateEngineFrame(timeStep);
}

}
