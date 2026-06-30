// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"
#include <Urho3D/Network/Protocol.h>

namespace Urho3D
{

class Button;
class DataChannelConnection;
class DataChannelServer;
class LineEdit;
class MemoryBuffer;
class NetworkConnection;
class Text;
class UIElement;

/// WebRTC STUN/TURN connectivity test with chat, UPnP, and relay signaling.
class WebRTCSTUN : public Sample
{
    URHO3D_OBJECT(WebRTCSTUN, Sample)

public:
    explicit WebRTCSTUN(Context* context);
    void Start() override;
    void Stop() override;

private:
    void CreateUI();
    void SubscribeToEvents();
    Button* CreateButton(const ea::string& text, int width, int height);
    void ShowChatText(const ea::string& row);
    void UpdateButtons();

    void HandleLogMessage(StringHash, VariantMap&);
    void HandleSend(StringHash, VariantMap&);
    void HandleConnect(StringHash, VariantMap&);
    void HandleDisconnect(StringHash, VariantMap&);
    void HandleStartServer(StringHash, VariantMap&);
    void HandleStartRelay(StringHash, VariantMap&);
    void HandleConnectRelay(StringHash, VariantMap&);

    void HandleServerListenStart();
    void HandleServerListenStop();
    void HandleServerConnected(NetworkConnection* connection);
    void HandleServerDisconnected(NetworkConnection* connection);
    void HandleClientConnected();
    void HandleClientDisconnected();
    void HandleNetworkMessage(NetworkConnection*, NetworkMessageId, MemoryBuffer&, bool&);

    void TryConnect(const ea::string& address);
    void TryConnectRelay(const ea::string& roomId, const ea::string& relayUrl);
    void ScheduleRetry();
    void HandleRetryUpdate(VariantMap&);

    void HookIceLogging(const SharedPtr<DataChannelConnection>& conn);

    ea::vector<ea::string> chatHistory_;
    SharedPtr<Text> chatHistoryText_;
    SharedPtr<UIElement> controlPanel_;
    SharedPtr<LineEdit> textEdit_;
    SharedPtr<Button> sendButton_, connectButton_, disconnectButton_;
    SharedPtr<Button> startServerButton_, startRelayButton_, connectRelayButton_;

    SharedPtr<DataChannelServer> server_;
    SharedPtr<DataChannelConnection> clientConnection_;
    ea::vector<WeakPtr<NetworkConnection>> serverConnections_;
    ea::vector<SharedPtr<DataChannelConnection>> relayConnections_;
    ea::vector<unsigned> serverClientIds_;
    unsigned nextClientId_ = 1;

    ea::string retryAddress_;
    ea::string retryRelayUrl_;
    int retryCount_ = 0;
    bool wasEverConnected_ = false;
    bool retrying_ = false;
    bool isRelayMode_ = false;
    Timer retryTimer_;

    static const int MaxRetries = 5;
    static const float RetryDelay;
};

} // namespace Urho3D