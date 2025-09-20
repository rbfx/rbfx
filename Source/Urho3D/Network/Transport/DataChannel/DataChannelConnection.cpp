// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Network/Transport/DataChannel/DataChannelConnection.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Timer.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/Network/Transport/DataChannel/DataChannelServer.h"

#include <rtc/candidate.hpp>
#include <rtc/description.hpp>
#include <rtc/peerconnection.hpp>
#include <rtc/websocket.hpp>

namespace Urho3D
{

DataChannelConnection::DataChannelConnection(Context* context)
    : NetworkConnection(context)
{
}

DataChannelConnection::~DataChannelConnection()
{
    URHO3D_ASSERT(state_ == NetworkConnection::State::Disconnected);
}

void DataChannelConnection::RegisterObject(Context* context)
{
    context->AddAbstractReflection<DataChannelConnection>(Category_Network);
}

bool DataChannelConnection::Connect(const URL& url)
{
    URL finalUrl = url;
    if (finalUrl.scheme_.empty())
        finalUrl.scheme_ = "ws";    // A well-formed URL is required in web builds
#if URHO3D_PLATFORM_WEB
    auto websocket = std::make_shared<rtc::WebSocket>();
#else
    rtc::WebSocket::Configuration config = {};
    config.disableTlsVerification = finalUrl.scheme_ == "ws";
    auto websocket = std::make_shared<rtc::WebSocket>(config);
#endif
    InitializeFromSocket(nullptr, websocket);
    state_ = State::Connecting;
    websocket->open(finalUrl.ToString().data());
    return true;
}

void DataChannelConnection::Disconnect()
{
    if (peer_)
    {
        selfHolder_ = this; // Ensure this object is alive until all callbacks are done executing.
        state_ = State::Disconnecting;
#ifndef URHO3D_PLATFORM_WEB
        peer_->resetCallbacks();
        peer_->close();
#endif
        peer_ = nullptr;
    }
}

void DataChannelConnection::SendMessage(ea::string_view data, PacketTypeFlags type)
{
    if (state_ != NetworkConnection::State::Connected)
    {
        URHO3D_LOGDEBUG("Network message was not sent: connection is not connected.");
        return;
    }

    if (data.size() > GetMaxMessageSize())
    {
        URHO3D_LOGERROR("DataChannel tried to send {} bytes of data, which is more than max allowed {} bytes of data per message.",
            data.size(), GetMaxMessageSize());
        return;
    }

    if (auto* dc = dataChannels_[type].get())
    {
        if (dc->isOpen())
        {
            dc->send((const std::byte*)data.data(), data.size());
            return;
        }
    }
    else
    {
        URHO3D_LOGERROR("DataChannel {} is not connected!", (int)type);
        Disconnect();
    }
}

unsigned DataChannelConnection::GetMaxMessageSize() const
{
    if (state_ != State::Connected || !dataChannels_[0])
        return 0;

#if URHO3D_PLATFORM_WEB
    return MaxNetworkPacketSize;
#else
    return static_cast<unsigned>(dataChannels_[0]->maxMessageSize());
#endif
}

void DataChannelConnection::OnDataChannelConnected(int index)
{
    // Web builds may call this callback multiple times for an already open datachannel!
    if (state_ == State::Connected)
        return;

    for (int i = 0; i < URHO3D_ARRAYSIZE(dataChannels_); i++)
    {
        if (!dataChannels_[i] || !dataChannels_[i]->isOpen())
            return;
    }

    state_ = State::Connected;
#ifndef URHO3D_PLATFORM_WEB
    if (peer_->remoteAddress().has_value())
        address_ = peer_->remoteAddress().value().c_str();
#endif

    if (onConnected_)
        onConnected_();

    if (server_)
        server_->onConnected_(this);

    // Signaling server connection is no longer needed.
    websocket_->close();
    websocket_ = nullptr;
}

void DataChannelConnection::OnDataChannelDisconnected(int index)
{
    // Web builds may call this callback multiple times for an already open datachannel!
    if (state_ == State::Disconnected)
        return;
#ifndef URHO3D_PLATFORM_WEB
    dataChannels_[index]->resetCallbacks();
#endif
    dataChannels_[index] = nullptr;
    for (int i = 0; i < URHO3D_ARRAYSIZE(dataChannels_); i++)
    {
        if (dataChannels_[i])
            return;
    }

    // All data channels were closed, finalize disconnect.
    bool userRequestedDisconnect = state_ == State::Disconnecting;
    state_ = State::Disconnected;
    if (userRequestedDisconnect)
    {
        if (onDisconnected_)
            onDisconnected_();
    }
    else
    {
        if (onError_)
            onError_();
    }
    if (server_)
        server_->OnDisconnected(this);
    if (websocket_)
    {
        websocket_->close();
        websocket_ = nullptr;
    }
    peer_ = nullptr;
    server_ = nullptr;
    selfHolder_ = nullptr;
}

void DataChannelConnection::InitializeFromSocket(DataChannelServer* server, std::shared_ptr<rtc::WebSocket> websocket)
{
    server_ = WeakPtr(server);
    websocket_ = websocket;
    websocketWasOpened_ = server != nullptr;

    rtc::Configuration config = {};
    peer_ = std::make_shared<rtc::PeerConnection>(config);
    peer_->onLocalDescription([this](rtc::Description desc)
    {
        VectorBuffer msg;
        msg.WriteString(desc.typeString());
        msg.WriteString(std::string(desc));
        websocket_->send(reinterpret_cast<const std::byte*>(msg.GetData()), msg.GetSize());
        //URHO3D_LOGDEBUG("onLocalDescription: type={}, sdp={}", desc.typeString(), std::string(desc));
    });
    peer_->onLocalCandidate([this](rtc::Candidate candidate)
    {
        VectorBuffer msg;
        msg.WriteString("candidate");
        msg.WriteString(std::string(candidate));
        msg.WriteString(candidate.mid());
        websocket_->send(reinterpret_cast<const std::byte*>(msg.GetData()), msg.GetSize());
        //URHO3D_LOGDEBUG("onLocalCandidate: type=candidate, mid={}, sdp={}", candidate.mid(), std::string(candidate));
    });

    // Server initializes data channels
    if (server != nullptr)
    {
        using namespace std::chrono_literals;
        dataChannels_[PacketType::UnreliableUnordered] = peer_->createDataChannel("uu", {{rtc::Reliability::Type::Rexmit, true, 0}});
        dataChannels_[PacketType::Reliable] = peer_->createDataChannel("ru", {{rtc::Reliability::Type::Reliable, true}});
        dataChannels_[PacketType::Ordered] = peer_->createDataChannel("uo", {{rtc::Reliability::Type::Rexmit, false, 0}});
        dataChannels_[PacketType::ReliableOrdered] = peer_->createDataChannel("ro", {{rtc::Reliability::Type::Reliable, false}});
        for (int i = 0; i < URHO3D_ARRAYSIZE(dataChannels_); i++)
        {
            auto& dc = dataChannels_[i];
            dc->onOpen(std::bind(&DataChannelConnection::OnDataChannelConnected, this, i));
            dc->onClosed(std::bind(&DataChannelConnection::OnDataChannelDisconnected, this, i));
            dc->onMessage([this](const rtc::binary& data)
            {
                if (onMessage_)
                    onMessage_(ea::string_view{(const char*)data.data(), data.size()});
            }, [](rtc::string) {});
        }
    }

    peer_->onDataChannel([this](std::shared_ptr<rtc::DataChannel> dc)
    {
        rtc::Reliability reliability = dc->reliability();
        PacketTypeFlags packetType = {};
        if (dc->reliability().type == rtc::Reliability::Type::Reliable)
            packetType |= PacketType::Reliable;
        if (!dc->reliability().unordered)
            packetType |= PacketType::Ordered;

        URHO3D_ASSERT(dataChannels_[packetType] == nullptr);

        dataChannels_[packetType] = dc;
        dc->onOpen(std::bind(&DataChannelConnection::OnDataChannelConnected, this, packetType));
        dc->onClosed(std::bind(&DataChannelConnection::OnDataChannelDisconnected, this, packetType));
        dc->onMessage([this](const rtc::binary& data)
        {
            if (onMessage_)
                onMessage_(ea::string_view{(const char*)data.data(), data.size()});
        }, [](rtc::string) {});
    });
    websocket->onOpen([this]() { websocketWasOpened_ = true; });
    websocket->onMessage([this](rtc::binary data)
    {
        MemoryBuffer msg(data.data(), static_cast<unsigned>(data.size()));
        ea::string type = msg.ReadString();
        if (type == "offer" || type == "answer")
        {
            ea::string sdp = msg.ReadString();
            peer_->setRemoteDescription(rtc::Description(sdp.c_str(), type.c_str()));
            //URHO3D_LOGDEBUG("setRemoteDescription: type={}, sdp={}", type.c_str(), sdp.c_str());
        }
        else if (type == "candidate")
        {
            auto sdp = msg.ReadString();
            auto mid = msg.ReadString();
            peer_->addRemoteCandidate(rtc::Candidate(sdp.c_str(), mid.c_str()));
            //URHO3D_LOGDEBUG("setRemoteDescription: type={}, mid={}, sdp={}", type.c_str(), mid.c_str(), sdp.c_str());
        }
    }, [](rtc::string) {});
    websocket->onClosed([this]()
    {
        if (!websocketWasOpened_)
        {
            state_ = State::Disconnected;
            if (onError_)
                onError_();
            if (server_)
                server_->OnDisconnected(this);
            for (auto& dc : dataChannels_)
                dc = nullptr;
            peer_ = nullptr;
            server_ = nullptr;
            URHO3D_LOGDEBUG("Websocket failed to connect. Signaling server may be offline.");
        }
    });
}

}   // namespace Urho3D
