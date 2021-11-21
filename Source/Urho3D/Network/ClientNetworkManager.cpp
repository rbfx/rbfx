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
#include "../Network/NetworkEvents.h"
#include "../Network/ClientNetworkManager.h"
#include "../Scene/Scene.h"

#include <EASTL/numeric.h>

namespace Urho3D
{

namespace
{

template <class T, class Iterator>
T StandardDeviation(Iterator begin, Iterator end, T mean)
{
    const auto size = ea::distance(begin, end);
    if (size <= 1)
        return M_LARGE_VALUE;

    T accum = 0.0;
    for (auto iter = begin; iter != end; ++iter)
    {
        const T delta = *iter - mean;
        accum += delta * delta;
    }

    return std::sqrt(accum / (size - 1));
}

}

ClientNetworkManager::ClientNetworkManager(Scene* scene, AbstractConnection* connection)
    : Object(scene->GetContext())
    , network_(GetSubsystem<Network>())
    , scene_(scene)
    , connection_(connection)
{
    SubscribeToEvent(network_, E_NETWORKINPUTPROCESSED, [this](StringHash, VariantMap&)
    {
        OnInputProcessed();
    });
}

void ClientNetworkManager::ProcessMessage(NetworkMessageId messageId, MemoryBuffer& messageData)
{
    switch (messageId)
    {
    case MSG_PING:
    {
        const auto msg = ReadNetworkMessage<MsgPingPong>(messageData);
        connection_->OnMessageReceived(messageId, msg);

        const bool isReliable = !!(msg.magic_ & 1u);
        connection_->SendMessage(MSG_PONG, MsgPingPong{ msg.magic_ }, isReliable ? NetworkMessageFlag::Reliable : NetworkMessageFlag::None);
        break;
    }

    case MSG_SYNCHRONIZE:
    {
        const auto msg = ReadNetworkMessage<MsgSynchronize>(messageData);
        connection_->OnMessageReceived(messageId, msg);

        clock_ = ClientClock{};
        clock_->updateFrequency_ = msg.updateFrequency_;
        clock_->latestServerFrame_ = msg.lastFrame_;
        clock_->ping_ = msg.ping_;

        clock_->currentFrame_ = clock_->latestServerFrame_ + GetPingInFrames();
        clock_->frameDuration_ = 1.0f / clock_->updateFrequency_;
        clock_->subFrameTime_ = clock_->frameDuration_ * 0.5f;

        clock_->lastSynchronizationFrame_ = clock_->currentFrame_;
        clock_->synchronizationErrors_.set_capacity(msg.clockBufferSize_);
        clock_->synchronizationErrorsSorted_.set_capacity(msg.clockBufferSize_);
        clock_->skippedTailsLength_ = msg.clockBufferSkippedTailsLength_;

        connection_->SendMessage(MSG_SYNCHRONIZE_ACK, MsgSynchronizeAck{ msg.magic_ }, NetworkMessageFlag::Reliable);
        URHO3D_LOGINFO("Client clock is started from frame #{}", clock_->currentFrame_);
        break;
    }

    case MSG_CLOCK:
    {
        const auto msg = ReadNetworkMessage<MsgClock>(messageData);
        if (!clock_)
        {
            URHO3D_LOGWARNING("Client clock update received before synchronization");
            break;
        }

        clock_->ping_ = msg.ping_;
        clock_->latestServerFrame_ = msg.lastFrame_;
        ProcessTimeCorrection();
        break;
    }

    default:
        break;
    }
}

void ClientNetworkManager::ProcessTimeCorrection()
{
    // Difference between client predicted time and server time
    const unsigned currentServerTime = clock_->latestServerFrame_ + GetPingInFrames();
    const double clockError = GetCurrentFrameDeltaRelativeTo(currentServerTime);

    auto& buffer = clock_->synchronizationErrors_;
    if (buffer.full())
        buffer.pop_front();
    buffer.push_back(clockError);

    // If buffer is filled, try to synchronize
    if (buffer.full())
    {
        auto& bufferSorted = clock_->synchronizationErrorsSorted_;
        bufferSorted.assign(buffer.begin(), buffer.end());
        ea::sort(bufferSorted.begin(), bufferSorted.end());

        const auto beginIter = std::next(bufferSorted.begin(), clock_->skippedTailsLength_);
        const auto endIter = std::prev(bufferSorted.end(), clock_->skippedTailsLength_);
        const double averageError = ea::accumulate(beginIter, endIter, 0.0) / ea::distance(beginIter, endIter);
        const double errorDeviation = StandardDeviation(beginIter, endIter, averageError);

        // Adjust time if error is too big
        if (std::abs(averageError) > ea::max(errorDeviation, clockErrorTolerance_))
        {
            const unsigned oldCurrentFrame = clock_->currentFrame_;
            const float oldSubFrameTime = clock_->subFrameTime_;

            const int integerError = RoundToInt(averageError);
            const float subFrameError = static_cast<float>(averageError - integerError) * clock_->frameDuration_;
            clock_->currentFrame_ += integerError;
            clock_->subFrameTime_ += subFrameError;
            if (clock_->subFrameTime_ < 0.0f)
            {
                clock_->subFrameTime_ += clock_->frameDuration_;
                --clock_->currentFrame_;
            }
            else if (clock_->subFrameTime_ >= clock_->frameDuration_)
            {
                clock_->subFrameTime_ -= clock_->frameDuration_;
                ++clock_->currentFrame_;
            }

            for (double& error : buffer)
                error -= averageError;

            clock_->lastSynchronizationFrame_ = clock_->currentFrame_;
            URHO3D_LOGINFO("Client clock is rewound from frame #{}:{} to frame #{}:{}",
                oldCurrentFrame, oldSubFrameTime,
                clock_->currentFrame_, clock_->subFrameTime_);
        }
    }
}

unsigned ClientNetworkManager::GetPingInFrames() const
{
    if (!clock_)
        return 0;

    // ms * (frames / s) * (s / ms)
    return RoundToInt(clock_->ping_ * clock_->updateFrequency_ * 0.001f);
}

double ClientNetworkManager::GetCurrentFrameDeltaRelativeTo(unsigned referenceFrame) const
{
    if (!clock_)
        return M_LARGE_VALUE;

    const auto frameDeltaInt = static_cast<int>(referenceFrame - clock_->currentFrame_);
    return static_cast<double>(frameDeltaInt) - clock_->subFrameTime_ * clock_->updateFrequency_;
}

ea::string ClientNetworkManager::ToString() const
{
    const ea::string& sceneName = scene_->GetName();
    return Format("Scene '{}': Estimated frame #{}",
        !sceneName.empty() ? sceneName : "Unnamed",
        GetCurrentFrame());
}

void ClientNetworkManager::OnInputProcessed()
{
    if (!clock_)
        return;

    auto time = GetSubsystem<Time>();
    const float timeStep = time->GetTimeStep();

    clock_->subFrameTime_ += timeStep;
    while (clock_->subFrameTime_ >= clock_->frameDuration_)
    {
        clock_->subFrameTime_ -= clock_->frameDuration_;
        ++clock_->currentFrame_;
    }
}

}
