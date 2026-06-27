// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

// WebRTC STUN/TURN connectivity test demonstrating NAT traversal fallback options:
// 1. Direct connection (requires port forwarding or local network)
// 2. UPnP port mapping (requires a cross platform upnp library like miniupnp)
// 3. Relay + STUN (requires VPS with relay server)
// 4. Relay + TURN (requires VPS with relay and TURN servers)

#include "WebRTCSTUN.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/IO/IOEvents.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Network/Protocol.h>
#include <Urho3D/Network/Transport/DataChannel/DataChannelConnection.h>
#include <Urho3D/Network/Transport/DataChannel/DataChannelServer.h>
#include <Urho3D/Network/Transport/NetworkConnection.h>
#include <Urho3D/Network/URL.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include <rtc/peerconnection.hpp>
#include <rtc/websocket.hpp>

#include <Urho3D/DebugNew.h>

namespace Urho3D
{

const float WebRTCSTUN::RetryDelay = 2.0f;

namespace
{

static const ea::vector<ea::string> IceServers = {
    "stun:stun.l.google.com:19302",
    "stun:stun1.l.google.com:19302",
    "stun:stun2.l.google.com:19302",
    "stun:stun3.l.google.com:19302",
    "stun:stun4.l.google.com:19302",
};
static const unsigned short DefaultPort = 2345;
static const unsigned short DataPort = 2346;
static const auto MSG_CHAT = static_cast<NetworkMessageId>(MSG_USER + 0);
static const auto MSG_SYS = static_cast<NetworkMessageId>(MSG_USER + 1);
static const char* RelayUrl = "ws://localhost:9876";

// ---- ICE server loading ----
static ea::vector<ea::string> LoadIceServers(Context* ctx)
{
    return IceServers;
}

// ---- ICE enum descriptions ----
static ea::string IceStateStr(rtc::PeerConnection::State s)
{
    switch (s)
    {
    case rtc::PeerConnection::State::New: return "New";
    case rtc::PeerConnection::State::Connecting: return "Connecting";
    case rtc::PeerConnection::State::Connected: return "Connected";
    case rtc::PeerConnection::State::Disconnected: return "Disconnected";
    case rtc::PeerConnection::State::Failed: return "Failed";
    case rtc::PeerConnection::State::Closed: return "Closed";
    default: return "?";
    }
}
static ea::string GatherStr(rtc::PeerConnection::GatheringState s)
{
    switch (s)
    {
    case rtc::PeerConnection::GatheringState::New: return "New";
    case rtc::PeerConnection::GatheringState::InProgress: return "InProgress";
    case rtc::PeerConnection::GatheringState::Complete: return "Complete";
    default: return "?";
    }
}
static ea::string CandStr(rtc::Candidate::Type t)
{
    switch (t)
    {
    case rtc::Candidate::Type::Host: return "Host";
    case rtc::Candidate::Type::ServerReflexive: return "ServerReflexive";
    case rtc::Candidate::Type::PeerReflexive: return "PeerReflexive";
    case rtc::Candidate::Type::Relayed: return "Relayed";
    default: return "?";
    }
}
static ea::string ClassifyAddr(const ea::string& a)
{
    if (a.empty())
        return "?";
    if (a == "::1" || a.starts_with("::1%") || a.starts_with("127."))
        return "localhost";
    if (a.starts_with("10.") || a.starts_with("192.168."))
        return "LAN";
    if (a.starts_with("172."))
    {
        auto p = a.substr(4).split('.');
        if (!p.empty())
        {
            int n = ToInt(p[0]);
            if (n >= 16 && n <= 31)
                return "LAN";
        }
    }
    if (a.starts_with("fc") || a.starts_with("fd") || a.starts_with("fe80"))
        return "LAN";
    return "WAN";
}

/// Convert vector<string> to vector<string_view> for passing to span-based APIs.
static ea::vector<ea::string_view> AsViews(const ea::vector<ea::string>& v)
{
    return {v.begin(), v.end()};
}
} // namespace

WebRTCSTUN::WebRTCSTUN(Context* c)
    : Sample(c)
{
}

void WebRTCSTUN::Start()
{
    Sample::Start();
    GetSubsystem<Input>()->SetMouseVisible(true);
    CreateUI();
    SubscribeToEvents();
    SetMouseMode(MM_FREE);
}

void WebRTCSTUN::Stop()
{
    HandleDisconnect({}, GetEventDataMap());
    BaseClassName::Stop();
}

void WebRTCSTUN::CreateUI()
{
    SetLogoVisible(false);
    auto* g = GetSubsystem<Graphics>();
    UIElement* root = GetUIRoot();
    auto* cache = GetSubsystem<ResourceCache>();
    root->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    int sw = g->GetWidth(), sh = g->GetHeight();

    chatHistoryText_ = root->CreateChild<Text>();
    chatHistoryText_->SetFont(font, 12);
    chatHistoryText_->SetPosition(5, 5);
    chatHistoryText_->SetSize(sw - 10, sh - 90);

    float rh = chatHistoryText_->GetRowHeight();
    if (rh)
        chatHistory_.resize(static_cast<unsigned>((sh - 100) / rh));

    int pw = 700, ph = 75, px = (sw - pw) / 2, py = sh - ph - 5;
    controlPanel_ = root->CreateChild<UIElement>();
    controlPanel_->SetPosition(px, py);
    controlPanel_->SetFixedSize(pw, ph);
    controlPanel_->SetLayoutMode(LM_VERTICAL);
    controlPanel_->SetLayoutSpacing(4);

    textEdit_ = controlPanel_->CreateChild<LineEdit>();
    textEdit_->SetStyleAuto();
    textEdit_->SetFixedHeight(28);

    auto* br = controlPanel_->CreateChild<UIElement>();
    br->SetLayoutMode(LM_HORIZONTAL);
    br->SetLayoutSpacing(6);
    br->SetAlignment(HA_CENTER, VA_CENTER);
    br->SetFixedHeight(32);

    startServerButton_ = CreateButton("Start Server", 130, 30);
    startRelayButton_ = CreateButton("Start Relay", 120, 30);
    connectButton_ = CreateButton("Connect", 100, 30);
    connectRelayButton_ = CreateButton("Connect Relay", 120, 30);
    disconnectButton_ = CreateButton("Disconnect", 110, 30);
    sendButton_ = CreateButton("Send", 90, 30);

    br->AddChild(startServerButton_);
    br->AddChild(startRelayButton_);
    br->AddChild(connectButton_);
    br->AddChild(connectRelayButton_);
    br->AddChild(disconnectButton_);
    br->AddChild(sendButton_);

    UpdateButtons();
    GetSubsystem<Renderer>()->GetDefaultZone()->SetFogColor(Color(0.0f, 0.0f, 0.1f));

    URHO3D_LOGINFO("=== WebRTC STUN/TURN Test ===");
    URHO3D_LOGINFO("Signaling port: {} (TCP) | Data port: {} (UDP, muxed)", DefaultPort, DataPort);
    URHO3D_LOGINFO("Direct: type IP[:port] -> Connect | Start Server [port]");
    URHO3D_LOGINFO("Relay: room[@host:port] -> Start/Connect Relay (default: {})", RelayUrl);
}

void WebRTCSTUN::SubscribeToEvents()
{
    SubscribeToEvent(textEdit_, E_TEXTFINISHED, URHO3D_HANDLER(WebRTCSTUN, HandleSend));
    SubscribeToEvent(sendButton_, E_RELEASED, URHO3D_HANDLER(WebRTCSTUN, HandleSend));
    SubscribeToEvent(connectButton_, E_RELEASED, URHO3D_HANDLER(WebRTCSTUN, HandleConnect));
    SubscribeToEvent(disconnectButton_, E_RELEASED, URHO3D_HANDLER(WebRTCSTUN, HandleDisconnect));
    SubscribeToEvent(startServerButton_, E_RELEASED, URHO3D_HANDLER(WebRTCSTUN, HandleStartServer));
    SubscribeToEvent(startRelayButton_, E_RELEASED, URHO3D_HANDLER(WebRTCSTUN, HandleStartRelay));
    SubscribeToEvent(connectRelayButton_, E_RELEASED, URHO3D_HANDLER(WebRTCSTUN, HandleConnectRelay));
    SubscribeToEvent(E_LOGMESSAGE, URHO3D_HANDLER(WebRTCSTUN, HandleLogMessage));
}

Button* WebRTCSTUN::CreateButton(const ea::string& text, int width, int height)
{
    auto* font = GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    auto* button = new Button(context_);
    button->SetStyleAuto();
    button->SetFixedSize(width, height);
    auto* bt = button->CreateChild<Text>();
    bt->SetFont(font, 14);
    bt->SetAlignment(HA_CENTER, VA_CENTER);
    bt->SetText(text);
    return button;
}

void WebRTCSTUN::ShowChatText(const ea::string& row)
{
    chatHistory_.pop_front();
    chatHistory_.push_back(row);
    ea::string all;
    for (const auto& x : chatHistory_)
        all += x + "\n";
    chatHistoryText_->SetText(all);
}

void WebRTCSTUN::UpdateButtons()
{
    bool cc = clientConnection_ && clientConnection_->IsConnected();
    bool sr = server_ && (server_->IsListening() || !relayConnections_.empty());
    bool chat = cc || (sr && !serverConnections_.empty());
    bool idle = !cc && !sr;

    sendButton_->SetVisible(chat);
    startServerButton_->SetVisible(idle);
    startRelayButton_->SetVisible(idle);
    connectButton_->SetVisible(idle);
    connectRelayButton_->SetVisible(idle);
    disconnectButton_->SetVisible(!idle);
}

void WebRTCSTUN::HandleLogMessage(StringHash, VariantMap& eventData)
{
    using namespace LogMessage;
    ShowChatText(eventData[P_MESSAGE].GetString());
}

void WebRTCSTUN::HandleSend(StringHash, VariantMap&)
{
    ea::string text = textEdit_->GetText();
    if (text.empty())
        return;

    VectorBuffer msg;
    msg.WriteString(text);

    if (clientConnection_ && clientConnection_->IsConnected())
    {
        clientConnection_->SendMessage(MSG_CHAT, msg);
        ShowChatText("[You -> Server] " + text);
    }
    else if (server_ && (server_->IsListening() || !relayConnections_.empty()))
    {
        for (const auto& weakConn : serverConnections_)
        {
            if (auto conn = weakConn.Lock())
                conn->SendMessage(MSG_CHAT, msg);
        }
        ShowChatText("[Server -> All] " + text);
    }

    textEdit_->SetText(EMPTY_STRING);
}

void WebRTCSTUN::HandleConnect(StringHash, VariantMap&)
{
    ea::string addr = textEdit_->GetText().trimmed();
    if (addr.empty())
        addr = "localhost";
    textEdit_->SetText(EMPTY_STRING);

    if (clientConnection_ && !clientConnection_->IsDisconnected())
        clientConnection_->Disconnect();

    retryAddress_ = addr;
    retryCount_ = 0;
    wasEverConnected_ = false;
    isRelayMode_ = false;
    TryConnect(addr);
}

void WebRTCSTUN::TryConnect(const ea::string& addr)
{
    if (!clientConnection_)
    {
        clientConnection_ = MakeShared<DataChannelConnection>(context_);
        clientConnection_->SetIceServers(AsViews(LoadIceServers(context_)));
        clientConnection_->onConnected_.Subscribe(this, &WebRTCSTUN::HandleClientConnected);
        clientConnection_->onDisconnected_.Subscribe(this, &WebRTCSTUN::HandleClientDisconnected);
        clientConnection_->onMessage_.SubscribeWithSender(this, &WebRTCSTUN::HandleNetworkMessage);
    }

    // Allow "host:port" syntax, default to DefaultPort
    ea::string host = addr;
    unsigned short port = DefaultPort;
    const auto colonPos = addr.find(':');
    if (colonPos != ea::string::npos)
    {
        host = addr.substr(0, colonPos);
        port = static_cast<unsigned short>(ToInt(addr.substr(colonPos + 1)));
    }

    const ea::string url = Format("ws://{}:{}", host, port);
    URHO3D_LOGINFO("Connecting to {} ... (attempt {})", url, retryCount_ + 1);
    clientConnection_->Connect(URL(url));
    HookIceLogging(clientConnection_);
    UpdateButtons();
}

void WebRTCSTUN::HandleStartServer(StringHash, VariantMap&)
{
    if (!server_)
    {
        server_ = MakeShared<DataChannelServer>(context_);
        server_->SetIceServers(AsViews(LoadIceServers(context_)));
        server_->onListenStart_.Subscribe(this, &WebRTCSTUN::HandleServerListenStart);
        server_->onListenStop_.Subscribe(this, &WebRTCSTUN::HandleServerListenStop);
        server_->onConnected_.Subscribe(this, &WebRTCSTUN::HandleServerConnected);
        server_->onDisconnected_.Subscribe(this, &WebRTCSTUN::HandleServerDisconnected);
    }

    // Allow optional port override from text field
    unsigned short signalingPort = DefaultPort;
    unsigned short dataPort = DataPort;
    ea::string input = textEdit_->GetText().trimmed();
    if (!input.empty())
    {
        int port = ToInt(input);
        if (port > 0 && port < 65536)
        {
            signalingPort = static_cast<unsigned short>(port);
            dataPort = signalingPort + 1;
        }
    }
    textEdit_->SetText(EMPTY_STRING);

    // Single UDP port for all WebRTC data traffic — only one port to forward
    server_->SetIceUdpMux(true);
    server_->SetPortRange(dataPort, dataPort);

    const URL listenUrl(Format("ws://0.0.0.0:{}", signalingPort));
    URHO3D_LOGINFO("Signaling: {} | WebRTC data: UDP :{} (single port, muxed)", listenUrl.ToString(), dataPort);
    URHO3D_LOGINFO("Port-forward TCP {} and UDP {} for direct connections.", signalingPort, dataPort);
    server_->Listen(listenUrl);
    // This is where you would attempt UPnP port mapping as a fallback for NAT traversal
    // UPnPMapPort(DefaultPort);
    UpdateButtons();
}

// ---- Relay mode ----
void WebRTCSTUN::HandleStartRelay(StringHash, VariantMap&)
{
    ea::string input = textEdit_->GetText().trimmed();
    if (input.empty())
        input = "default";
    textEdit_->SetText(EMPTY_STRING);

    // Parse "room@host:port" or just "room"
    ea::string rid = input;
    ea::string relayUrl = RelayUrl;
    const auto atPos = input.find('@');
    if (atPos != ea::string::npos)
    {
        rid = input.substr(0, atPos);
        ea::string hostPort = input.substr(atPos + 1);
        if (hostPort.find(':') == ea::string::npos)
            hostPort += ":9876";
        relayUrl = Format("ws://{}", hostPort);
    }

    if (!server_)
    {
        server_ = MakeShared<DataChannelServer>(context_);
        server_->SetIceServers(AsViews(LoadIceServers(context_)));
        server_->onListenStart_.Subscribe(this, &WebRTCSTUN::HandleServerListenStart);
        server_->onListenStop_.Subscribe(this, &WebRTCSTUN::HandleServerListenStop);
        server_->onConnected_.Subscribe(this, &WebRTCSTUN::HandleServerConnected);
        server_->onDisconnected_.Subscribe(this, &WebRTCSTUN::HandleServerDisconnected);
    }

    URHO3D_LOGINFO("Joining relay room '{}' as host via {}...", rid, relayUrl);

    auto ws = std::make_shared<rtc::WebSocket>();
    ws->onOpen([this, rid, ws]()
    {
        JSONFile jsonFile(context_);
        jsonFile.GetRoot()["type"] = "join";
        jsonFile.GetRoot()["room"] = rid;
        const ea::string join = jsonFile.ToString("");
        ws->send(reinterpret_cast<const std::byte*>(join.c_str()), join.length());
    });
    ws->onMessage([this, ws](rtc::binary data)
    {
        const ea::string json(reinterpret_cast<const char*>(data.data()), data.size());
        JSONValue jsonValue;
        if (JSONFile::ParseJSON(json, jsonValue) && jsonValue["type"].GetString() == "paired")
        {
            URHO3D_LOGINFO("Relay: Paired!");
            auto dc = MakeShared<DataChannelConnection>(context_);
            dc->SetIceServers(AsViews(LoadIceServers(context_)));
            dc->onMessage_.SubscribeWithSender(this, &WebRTCSTUN::HandleNetworkMessage);
            dc->InitializeWithWebSocket(ws, server_);
            HookIceLogging(dc);
            relayConnections_.push_back(dc);
            // HandleServerConnected will add to serverConnections_ when data channels open
        }
    }, [](rtc::string) {});
    ws->open(std::string(relayUrl.c_str()));
    UpdateButtons();
}

void WebRTCSTUN::HandleConnectRelay(StringHash, VariantMap&)
{
    ea::string input = textEdit_->GetText().trimmed();
    if (input.empty())
        input = "default";
    textEdit_->SetText(EMPTY_STRING);

    // Parse "room@host:port" or just "room"
    ea::string rid = input;
    ea::string relayUrl = RelayUrl;
    const auto atPos = input.find('@');
    if (atPos != ea::string::npos)
    {
        rid = input.substr(0, atPos);
        ea::string hostPort = input.substr(atPos + 1);
        if (hostPort.find(':') == ea::string::npos)
            hostPort += ":9876";
        relayUrl = Format("ws://{}", hostPort);
    }

    if (clientConnection_ && !clientConnection_->IsDisconnected())
        clientConnection_->Disconnect();

    retryAddress_ = rid;
    retryRelayUrl_ = relayUrl;
    retryCount_ = 0;
    wasEverConnected_ = false;
    isRelayMode_ = true;
    TryConnectRelay(rid, relayUrl);
}

void WebRTCSTUN::TryConnectRelay(const ea::string& rid, const ea::string& relayUrl)
{
    if (!clientConnection_)
    {
        clientConnection_ = MakeShared<DataChannelConnection>(context_);
        clientConnection_->SetIceServers(AsViews(LoadIceServers(context_)));
        clientConnection_->onConnected_.Subscribe(this, &WebRTCSTUN::HandleClientConnected);
        clientConnection_->onDisconnected_.Subscribe(this, &WebRTCSTUN::HandleClientDisconnected);
        clientConnection_->onMessage_.SubscribeWithSender(this, &WebRTCSTUN::HandleNetworkMessage);
    }

    URHO3D_LOGINFO("Joining relay room '{}' as client via {}... (attempt {})", rid, relayUrl, retryCount_ + 1);

    auto ws = std::make_shared<rtc::WebSocket>();
    ws->onOpen([this, rid, ws]()
    {
        JSONFile jsonFile(context_);
        jsonFile.GetRoot()["type"] = "join";
        jsonFile.GetRoot()["room"] = rid;
        const ea::string join = jsonFile.ToString("");
        ws->send(reinterpret_cast<const std::byte*>(join.c_str()), join.length());
    });
    ws->onMessage([this, ws](rtc::binary data)
    {
        const ea::string json(reinterpret_cast<const char*>(data.data()), data.size());
        JSONValue jsonValue;
        if (JSONFile::ParseJSON(json, jsonValue) && jsonValue["type"].GetString() == "paired")
        {
            URHO3D_LOGINFO("Relay: Paired!");
            clientConnection_->InitializeWithWebSocket(ws);
            HookIceLogging(clientConnection_);
        }
    }, [](rtc::string) {});
    ws->open(std::string(relayUrl.c_str()));
    UpdateButtons();
}

void WebRTCSTUN::ScheduleRetry()
{
    if (retryCount_ >= MaxRetries || wasEverConnected_)
        return;

    ++retryCount_;
    URHO3D_LOGINFO("Connection failed. Retrying in {:.0f}s... ({}/{})", RetryDelay, retryCount_, MaxRetries);
    retrying_ = true;
    retryTimer_.Reset();
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(WebRTCSTUN, HandleRetryUpdate));
}

void WebRTCSTUN::HandleRetryUpdate(VariantMap&)
{
    if (retryTimer_.GetMSec(false) < RetryDelay * 1000.0f)
        return;

    UnsubscribeFromEvent(E_UPDATE);
    retrying_ = false;

    if (retryCount_ < MaxRetries && !wasEverConnected_)
    {
        if (isRelayMode_)
            TryConnectRelay(retryAddress_, retryRelayUrl_);
        else
            TryConnect(retryAddress_);
    }
}

void WebRTCSTUN::HandleDisconnect(StringHash, VariantMap&)
{
    retryCount_ = MaxRetries;
    retrying_ = false;

    if (clientConnection_ && !clientConnection_->IsDisconnected())
    {
        URHO3D_LOGINFO("Disconnecting client...");
        clientConnection_->Disconnect();
    }

    if (server_ && server_->IsListening())
    {
        URHO3D_LOGINFO("Stopping server...");
        // This is where you would attempt unmapping the port
        // UPnPUnmapPort(DefaultPort);
        for (const auto& weakConn : serverConnections_)
        {
            if (auto conn = weakConn.Lock())
                conn->Disconnect();
        }
        serverConnections_.clear();
        serverClientIds_.clear();
        server_->Stop();
    }

    for (auto& conn : relayConnections_)
    {
        if (conn && !conn->IsDisconnected())
            conn->Disconnect();
    }
    relayConnections_.clear();

    UpdateButtons();
}

void WebRTCSTUN::HandleServerListenStart()
{
    URHO3D_LOGINFO("Server is listening. Give this machine's IP to the client.");
    URHO3D_LOGINFO("For direct connections, forward TCP signaling port and UDP data port.");
}

void WebRTCSTUN::HandleServerListenStop()
{
    URHO3D_LOGINFO("Server stopped listening.");
}

void WebRTCSTUN::HandleServerConnected(NetworkConnection* connection)
{
    // NOTE: WebRTC callbacks may execute on different threads. This sample does not implement
    // full thread synchronization for simplicity. In production code, protect shared state
    // like serverConnections_ with mutexes or use thread-safe data structures.
    const unsigned clientId = nextClientId_++;
    serverConnections_.push_back(WeakPtr<NetworkConnection>{});
    serverConnections_.back() = connection;
    serverClientIds_.push_back(clientId);
    connection->onMessage_.SubscribeWithSender(this, &WebRTCSTUN::HandleNetworkMessage);

    if (auto* dc = dynamic_cast<DataChannelConnection*>(connection))
    {
        HookIceLogging(SharedPtr<DataChannelConnection>(dc));
        if (auto peer = dc->GetPeer())
        {
            rtc::Candidate local, remote;
            if (peer->getSelectedCandidatePair(&local, &remote))
                URHO3D_LOGINFO("ICE selected pair: local={} ({}), remote={} ({})", CandStr(local.type()),
                    static_cast<int>(local.type()), CandStr(remote.type()), static_cast<int>(remote.type()));
        }
    }

    const ea::string& addr = connection->GetAddress();
    const ea::string type = ClassifyAddr(addr);
    if (!addr.empty())
        URHO3D_LOGINFO(
            "Server: Client {} connected! {} ({}) ({} total)", clientId, addr.c_str(), type, serverConnections_.size());
    else
        URHO3D_LOGINFO("Server: Client {} connected! ({} total)", clientId, serverConnections_.size());

    VectorBuffer sysMsg;
    sysMsg.WriteString(Format("Client {} joined from {} ({} online)", clientId, type, serverConnections_.size()));
    for (const auto& weakConn : serverConnections_)
    {
        auto conn = weakConn.Lock();
        if (conn && conn.Get() != connection)
            conn->SendMessage(MSG_SYS, sysMsg);
    }

    UpdateButtons();
}

void WebRTCSTUN::HandleServerDisconnected(NetworkConnection* connection)
{
    unsigned disconnectedId = 0;
    for (unsigned i = 0; i < serverConnections_.size();)
    {
        if (serverConnections_[i].Get() == connection)
        {
            disconnectedId = serverClientIds_[i];
            serverConnections_.erase(serverConnections_.begin() + i);
            serverClientIds_.erase(serverClientIds_.begin() + i);
        }
        else
            ++i;
    }

    URHO3D_LOGINFO("Server: Client {} disconnected. ({} remaining)", disconnectedId, serverConnections_.size());

    if (!serverConnections_.empty())
    {
        VectorBuffer sysMsg;
        sysMsg.WriteString(Format("Client {} left ({} online)", disconnectedId, serverConnections_.size()));
        for (const auto& weakConn : serverConnections_)
        {
            if (auto conn = weakConn.Lock())
                conn->SendMessage(MSG_SYS, sysMsg);
        }
    }

    UpdateButtons();
}

void WebRTCSTUN::HandleClientConnected()
{
    wasEverConnected_ = true;
    retryCount_ = MaxRetries;

    const ea::string& addr = clientConnection_->GetAddress();
    if (!addr.empty())
        URHO3D_LOGINFO("Connected! Remote peer: {} ({})", addr.c_str(), ClassifyAddr(addr));
    else
        URHO3D_LOGINFO("Connected! (Web platform - address unavailable)");

    if (auto peer = clientConnection_->GetPeer())
    {
        rtc::Candidate local, remote;
        if (peer->getSelectedCandidatePair(&local, &remote))
            URHO3D_LOGINFO("ICE selected pair: local={} ({}), remote={} ({})", CandStr(local.type()),
                static_cast<int>(local.type()), CandStr(remote.type()), static_cast<int>(remote.type()));
    }

    UpdateButtons();
}

void WebRTCSTUN::HandleClientDisconnected()
{
    if (!wasEverConnected_)
        ScheduleRetry();
    else
        URHO3D_LOGINFO("Disconnected from server.");
    UpdateButtons();
}

void WebRTCSTUN::HandleNetworkMessage(
    NetworkConnection* connection, NetworkMessageId messageId, MemoryBuffer& message, bool& handled)
{
    if (messageId == MSG_SYS)
    {
        ShowChatText(message.ReadString());
        handled = true;
        return;
    }

    if (messageId != MSG_CHAT)
        return;

    ea::string text = message.ReadString();
    bool isServer = server_ && (server_->IsListening() || !relayConnections_.empty());

    if (isServer)
    {
        unsigned senderId = 0;
        for (unsigned i = 0; i < serverConnections_.size(); ++i)
        {
            if (serverConnections_[i].Get() == connection)
            {
                senderId = serverClientIds_[i];
                break;
            }
        }

        ShowChatText(Format("Client {}: {}", senderId, text));

        VectorBuffer sendMsg;
        sendMsg.WriteString(Format("Client {}: {}", senderId, text));
        for (const auto& weakConn : serverConnections_)
        {
            auto conn = weakConn.Lock();
            if (conn && conn.Get() != connection)
                conn->SendMessage(MSG_CHAT, sendMsg);
        }
    }
    else
    {
        ShowChatText("[Server] " + text);
    }

    handled = true;
}

void WebRTCSTUN::HookIceLogging(const SharedPtr<DataChannelConnection>& conn)
{
    if (auto peer = conn->GetPeer())
    {
        peer->onStateChange([](rtc::PeerConnection::State s)
        { URHO3D_LOGINFO("ICE state: {} ({})", IceStateStr(s), static_cast<int>(s)); });
        peer->onGatheringStateChange([](rtc::PeerConnection::GatheringState s)
        { URHO3D_LOGINFO("ICE gathering: {} ({})", GatherStr(s), static_cast<int>(s)); });
    }
}

} // namespace Urho3D