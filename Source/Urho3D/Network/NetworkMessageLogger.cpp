// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Network/NetworkMessageLogger.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/Network/Transport/NetworkConnection.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

NetworkMessageLogger::NetworkMessageLogger(NetworkConnection* connection)
{
    Attach(connection);
}

void NetworkMessageLogger::Attach(NetworkConnection* connection)
{
    if (!connection || connection_.Get() == connection)
        return;

    connection_ = connection;

    connection->onSendMessage_.SubscribeWithSender(connection,
        [this](NetworkConnection* sender, NetworkMessageId messageId, const MemoryBuffer& data, PacketTypeFlags type, bool& handled)
    {
        if (handled)
            return;

        LogSentMessage(messageId, data, type, sender->GetDebugInfoBuffer());
    });

    connection->onMessage_.SubscribeWithSender(connection,
        [this](NetworkConnection* sender, NetworkMessageId messageId, MemoryBuffer& data, bool& handled)
    {
        (void)sender;
        (void)handled;
        LogReceivedMessage(messageId, data);
    });
}

LogLevel NetworkMessageLogger::GetMessageLogLevel(NetworkMessageId messageId) const
{
    if (!isEnabled_)
        return LOG_NONE;

    switch (messageId)
    {
    case MSG_CONFIGURE:
    case MSG_SYNCHRONIZED: return LOG_DEBUG;
    default: return LOG_TRACE;
    };

    return LOG_NONE;
}

void NetworkMessageLogger::LogSentMessage(
    NetworkMessageId messageId, const MemoryBuffer& data, PacketTypeFlags packetType, ea::string_view debugInfo)
{
    const LogLevel logLevel = GetMessageLogLevel(messageId);
    if (logLevel == LOG_NONE)
        return;

    Log::GetLogger().Write(logLevel, "Message #{} ({} bytes) sent{}{}{}{}", static_cast<unsigned>(messageId),
        data.GetSize(), (packetType & PacketType::Reliable) ? ", reliable" : "",
        (packetType & PacketType::Ordered) ? ", ordered" : "", debugInfo.empty() ? "" : ": ", debugInfo);
}

void NetworkMessageLogger::LogReceivedMessage(NetworkMessageId messageId, const MemoryBuffer& data, ea::string_view debugInfo)
{
    const LogLevel logLevel = GetMessageLogLevel(messageId);
    if (logLevel == LOG_NONE)
        return;

    Log::GetLogger().Write(logLevel, "Message #{} ({} bytes) received{}{}", static_cast<unsigned>(messageId),
        data.GetSize(), debugInfo.empty() ? "" : ": ", debugInfo);
}

} // namespace Urho3D
