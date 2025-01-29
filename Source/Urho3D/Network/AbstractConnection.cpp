// Copyright (c) 2020-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Network/AbstractConnection.h"

namespace Urho3D
{

AbstractConnection::AbstractConnection(Context* context)
    : Object(context)
{
}

unsigned AbstractConnection::GetMaxPacketSize() const
{
    return maxPacketSize_;
}

unsigned AbstractConnection::GetMaxMessageSize() const
{
    return ea::min(MaxNetworkMessageSize, maxPacketSize_ - NetworkMessageHeaderSize);
}

void AbstractConnection::SetMaxPacketSize(unsigned limit)
{
    maxPacketSize_ = limit;
}

void AbstractConnection::SendMessage(
    NetworkMessageId messageId, ConstByteSpan payload, PacketTypeFlags packetType, ea::string_view debugInfo)
{
    if (payload.size() > GetMaxMessageSize())
    {
        URHO3D_LOGERROR("{}: Message #{} ({} bytes) is too big to send", ToString(), static_cast<unsigned>(messageId),
            payload.size());
        return;
    }

    SendMessageInternal(messageId, payload.data(), payload.size(), packetType);

    Log::GetLogger().Write(GetMessageLogLevel(messageId), "{}: Message #{} ({} bytes) sent{}{}{}{}", ToString(),
        static_cast<unsigned>(messageId), payload.size(), (packetType & PacketType::Reliable) ? ", reliable" : "",
        (packetType & PacketType::Ordered) ? ", ordered" : "", debugInfo.empty() ? "" : ": ", debugInfo);
}

void AbstractConnection::SendMessage(
    NetworkMessageId messageId, const VectorBuffer& msg, PacketTypeFlags packetType, ea::string_view debugInfo)
{
    SendMessage(messageId, msg.GetBuffer(), packetType, debugInfo);
}

void AbstractConnection::LogReceivedMessage(NetworkMessageId messageId, ea::string_view debugInfo) const
{
    Log::GetLogger().Write(GetMessageLogLevel(messageId), "{}: Message #{} received: {}", ToString(),
        static_cast<unsigned>(messageId), debugInfo);
}

LogLevel AbstractConnection::GetMessageLogLevel(NetworkMessageId messageId) const
{
    static const ea::unordered_set<NetworkMessageId> debugMessages = {
        MSG_IDENTITY,
        MSG_SCENELOADED,
        MSG_REQUESTPACKAGE,

        MSG_LOADSCENE,
        MSG_SCENECHECKSUMERROR,
        MSG_PACKAGEINFO,

        MSG_CONFIGURE,
        MSG_SYNCHRONIZED,
    };
    return debugMessages.contains(messageId) ? LOG_DEBUG : LOG_TRACE;
}

} // namespace Urho3D
