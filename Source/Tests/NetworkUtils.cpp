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
    InternalMessage& msg = messages_[reliable][inOrder].emplace_back();
    msg.sendingTime_ = currentTime_ + random_.GetUInt(minPing_, maxPing_);
    msg.messageId_ = messageId;
    msg.data_.assign(data, data + numBytes);
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

void NetworkSimulator::AddClient(Scene* clientScene, float minPing, float maxPing)
{
    PerClient data;
    data.clientScene_ = clientScene;
    data.clientNetworkManager_ = clientScene->GetNetworkManager();
    data.clientToServer_ = MakeShared<ManualConnection>(context_, serverNetworkManager_, random_.GetUInt());
    data.serverToClient_ = MakeShared<ManualConnection>(context_, data.clientNetworkManager_, random_.GetUInt());

    const auto minPingMs = static_cast<unsigned>(minPing * MillisecondsInFrame * FramesInSecond);
    const auto maxPingMs = static_cast<unsigned>(maxPing * MillisecondsInFrame * FramesInSecond);
    data.clientToServer_->SetSinkConnection(data.serverToClient_);
    data.clientToServer_->SetPing(minPingMs, maxPingMs);
    data.serverToClient_->SetSinkConnection(data.clientToServer_);
    data.serverToClient_->SetPing(minPingMs, maxPingMs);

    data.clientNetworkManager_->MarkAsClient(data.clientToServer_);
    serverNetworkManager_->AsServer().AddConnection(data.serverToClient_);
    serverNetworkManager_->AsServer().SetTestPing(data.serverToClient_, RoundToInt((maxPing + minPing) / 2 * 1000));

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

SharedPtr<Context> CreateNetworkSimulatorContext()
{
    auto context = Tests::CreateCompleteTestContext();
    auto network = context->GetSubsystem<Network>();
    network->SetUpdateFps(NetworkSimulator::FramesInSecond);
    network->PostUpdate(0.5f / NetworkSimulator::FramesInSecond);
    return context;
}

}
