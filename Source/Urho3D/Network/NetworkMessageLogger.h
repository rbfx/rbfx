// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/Network/PacketTypeFlags.h"
#include "Urho3D/Network/Protocol.h"

namespace Urho3D
{

class NetworkConnection;

/// Helper class for logging network messages.
///
/// NetworkMessageLogger provides functionality to log sent and received network messages
/// with configurable verbosity levels. It can be enabled/disabled at runtime and provides
/// virtual methods that can be overridden for custom logging behavior.
///
/// Usage Example:
/// @code
/// class MyConnection : public DataChannelConnection
/// {
/// private:
///     NetworkMessageLogger logger_;
///
/// public:
///     void SendMessage(NetworkMessageId messageId, const MemoryBuffer& data,
///         PacketTypeFlags type, ea::string_view debugInfo)
///     {
///         // Log the outgoing message before sending
///         logger_.LogSentMessage(messageId, data, type, debugInfo);
///         BaseClassName::SendMessage(messageId, data, type, debugInfo);
///     }
///
///     bool OnMessage(NetworkMessageId messageId, MemoryBuffer& msg)
///     {
///         // Log the received message
///         logger_.LogReceivedMessage(messageId, msg);
///         // Process the replicated message further
///     }
/// };
/// @endcode
class URHO3D_API NetworkMessageLogger
{
public:
    NetworkMessageLogger() = default;
    explicit NetworkMessageLogger(NetworkConnection* connection);

    /// Subscribe to connection signals for automatic logging.
    void Attach(NetworkConnection* connection);

    /// Enable or disable message logging.
    void SetEnabled(bool enabled) { isEnabled_ = enabled; }
    /// Check if message logging is currently enabled.
    bool IsEnabled() const { return isEnabled_; }
    /// Get the log level for a specific network message type.
    /// Can be overridden to provide custom log level per message type.
    virtual LogLevel GetMessageLogLevel(NetworkMessageId messageId) const;

    /// Log a sent network message.
    /// Can be overridden for custom logging behavior (e.g., file output, remote logging).
    virtual void LogSentMessage(NetworkMessageId messageId, const MemoryBuffer& data, PacketTypeFlags packetType,
        ea::string_view debugInfo = {});
    /// Log a received network message.
    /// Can be overridden for custom logging behavior (e.g., file output, remote logging).
    virtual void LogReceivedMessage(
        NetworkMessageId messageId, const MemoryBuffer& data, ea::string_view debugInfo = {});

private:
    bool isEnabled_ = true;
    WeakPtr<NetworkConnection> connection_{};
};

} // namespace Urho3D
