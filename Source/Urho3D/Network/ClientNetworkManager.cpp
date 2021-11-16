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
#include "../Network/NetworkComponent.h"
#include "../Network/ClientNetworkManager.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

ClientNetworkManager::ClientNetworkManager(Scene* scene)
    : Object(scene->GetContext())
    , network_(GetSubsystem<Network>())
    , scene_(scene)
{
}

void ClientNetworkManager::ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData)
{
    if (messageId == MSG_FRAME_SYNC)
    {
        const auto msg = ReadNetworkMessage<MsgFrameSync>(messageData);
        updateFrequency_ = msg.updateFrequency_;
        latestServerFrame_ += ea::max(0, static_cast<int>(msg.lastFrame_ - latestServerFrame_));
    }
}

ea::string ClientNetworkManager::ToString() const
{
    const ea::string& sceneName = scene_->GetName();
    return Format("Scene '{}': Estimated frame #{}",
        !sceneName.empty() ? sceneName : "Unnamed",
        latestServerFrame_);
}

}
