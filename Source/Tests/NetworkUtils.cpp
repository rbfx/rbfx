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
#include <Urho3D/Network/Network.h>
#include <Urho3D/Scene/Scene.h>

namespace Tests
{

ManualConnection::ManualConnection(Context* context, NetworkManager* sink)
    : AbstractConnection(context)
    , sink_(sink)
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
                if (msg.sendingTime_ + ping_ >= currentTime_)
                    return false;

                MemoryBuffer memoryBuffer(msg.data_);
                sink_->ProcessMessage(sinkConnection_, msg.messageId_, memoryBuffer);
                return true;
            });
        }
    }
}

void ManualConnection::SendMessage(NetworkMessageId messageId, bool reliable, bool inOrder, const unsigned char* data, unsigned numBytes)
{
    InternalMessage& msg = messages_[reliable][inOrder].emplace_back();
    msg.sendingTime_ = currentTime_;
    msg.messageId_ = messageId;
    msg.data_.assign(data, data + numBytes);
}

NetworkSimulator::NetworkSimulator(Scene* serverScene)
    : context_(serverScene->GetContext())
    , network_(context_->GetSubsystem<Network>())
    , serverScene_(serverScene)
    , serverNetworkManager_(serverScene_->GetNetworkManager())
{
    serverNetworkManager_->MarkAsServer();
}

void NetworkSimulator::AddClient(Scene* clientScene)
{
    PerClient data;
    data.clientScene_ = clientScene;
    data.clientToServer_ = MakeShared<ManualConnection>(context_, serverScene_->GetNetworkManager());
    data.serverToClient_ = MakeShared<ManualConnection>(context_, clientScene->GetNetworkManager());

    clientScene->GetNetworkManager()->MarkAsClient();
    serverNetworkManager_->AsServer().AddConnection(data.serverToClient_);

    clientScenes_.push_back(data);
}

void NetworkSimulator::SimulateFrame()
{
    network_->PostUpdate(1.0f / network_->GetUpdateFps());

    for (PerClient& data : clientScenes_)
    {
        serverNetworkManager_->AsServer().SendUpdate(data.serverToClient_);
        data.serverToClient_->IncrementTime(QuantsInFrame);
    }
}

void NetworkSimulator::SimulateFrames(unsigned count)
{
    for (unsigned i = 0; i < count; ++i)
        SimulateFrame();
}

SharedPtr<Context> CreateNetworkSimulatorContext()
{
    auto context = Tests::CreateCompleteTestContext();
    auto network = context->GetSubsystem<Network>();
    network->SetUpdateFps(NetworkSimulator::TestFrequency);
    network->PostUpdate(0.5f / NetworkSimulator::TestFrequency);
    return context;
}

}
