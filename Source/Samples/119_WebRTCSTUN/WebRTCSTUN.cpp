// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

// WebRTC STUN/TURN connectivity test demonstrating NAT traversal fallback options:
// 1. Direct connection (requires port forwarding or local network)
// 2. UPnP port mapping (Windows-only, deprecated API, may not work on modern systems)
// 3. Relay + STUN (requires VPS with relay server)
// 4. Relay + TURN (requires VPS with relay and TURN servers)

#include "WebRTCSTUN.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
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
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include <rtc/peerconnection.hpp>
#include <rtc/websocket.hpp>

#ifdef _WIN32
    #include <comdef.h>
    #include <natupnp.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#include <Urho3D/DebugNew.h>

namespace Urho3D
{

const float WebRTCSTUN::RetryDelay = 2.0f;

namespace
{

static const ea::vector<ea::string> FallbackIceServers = {
    "stun:stun.l.google.com:19302",
    "stun:stun1.l.google.com:19302",
};
static const unsigned short DefaultPort = 2345;
static const auto MSG_CHAT = static_cast<NetworkMessageId>(MSG_USER + 0);
static const auto MSG_SYS = static_cast<NetworkMessageId>(MSG_USER + 1);
static const char* RelayUrl = "ws://localhost:9876";

// ---- UPnP helpers ----
// NOTE: This uses the deprecated Windows natupnp.h API for educational purposes.
// Modern applications should use a proper UPnP library like miniupnpc.
// This implementation is Windows-only and may not work on modern Windows systems.
static bool upnpMappingSucceeded = false;

static bool UPnPMapPort(unsigned short port)
{
#ifdef _WIN32
    // Initialize COM once per thread
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool comInitialized = SUCCEEDED(hrInit) || hrInit == RPC_E_CHANGED_MODE;

    IUPnPNAT* nat = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(UPnPNAT), nullptr, CLSCTX_ALL, __uuidof(IUPnPNAT), (void**)&nat)) || !nat)
    {
        URHO3D_LOGWARNING("UPnP: Failed to create UPnPNAT instance");
        if (comInitialized)
            CoUninitialize();
        return false;
    }

    IStaticPortMappingCollection* mappings = nullptr;
    if (FAILED(nat->get_StaticPortMappingCollection(&mappings)) || !mappings)
    {
        URHO3D_LOGWARNING("UPnP: Failed to get port mapping collection");
        nat->Release();
        if (comInitialized)
            CoUninitialize();
        return false;
    }

    // Use getaddrinfo instead of deprecated gethostbyname
    char hn[256];
    if (gethostname(hn, sizeof(hn)) != 0)
    {
        URHO3D_LOGWARNING("UPnP: Failed to get hostname");
        mappings->Release();
        nat->Release();
        if (comInitialized)
            CoUninitialize();
        return false;
    }

    addrinfo hints = {};
    hints.ai_family = AF_INET; // IPv4 only for UPnP
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    if (getaddrinfo(hn, nullptr, &hints, &result) != 0 || !result)
    {
        URHO3D_LOGWARNING("UPnP: Failed to resolve hostname");
        mappings->Release();
        nat->Release();
        if (comInitialized)
            CoUninitialize();
        return false;
    }

    sockaddr_in* addr = (sockaddr_in*)result->ai_addr;
    const char* ip = inet_ntoa(addr->sin_addr);

    HRESULT hr = mappings->Add(port, _bstr_t("TCP"), port, _bstr_t(ip), VARIANT_TRUE, _bstr_t("rbfx WebRTC"), nullptr);

    freeaddrinfo(result);
    mappings->Release();
    nat->Release();

    if (comInitialized)
        CoUninitialize();

    if (SUCCEEDED(hr))
    {
        URHO3D_LOGINFO("UPnP: Mapped TCP {} -> {}:{}", port, ip, port);
        upnpMappingSucceeded = true;
        return true;
    }
    else
    {
        URHO3D_LOGWARNING("UPnP: Failed to map port (HRESULT: 0x{:08X})", static_cast<unsigned>(hr));
        return false;
    }
#else
    URHO3D_LOGINFO("UPnP: Not supported on this platform (Windows-only)");
    return false;
#endif
}

static void UPnPUnmapPort(unsigned short port)
{
#ifdef _WIN32
    if (!upnpMappingSucceeded)
    {
        URHO3D_LOGINFO("UPnP: Skipping unmap (no successful mapping)");
        return;
    }

    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool comInitialized = SUCCEEDED(hrInit) || hrInit == RPC_E_CHANGED_MODE;

    IUPnPNAT* nat = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(UPnPNAT), nullptr, CLSCTX_ALL, __uuidof(IUPnPNAT), (void**)&nat)) || !nat)
    {
        URHO3D_LOGWARNING("UPnP: Failed to create UPnPNAT instance for unmap");
        if (comInitialized)
            CoUninitialize();
        return;
    }

    IStaticPortMappingCollection* mappings = nullptr;
    if (SUCCEEDED(nat->get_StaticPortMappingCollection(&mappings)) && mappings)
    {
        HRESULT hr = mappings->Remove(port, _bstr_t("TCP"));
        if (SUCCEEDED(hr))
            URHO3D_LOGINFO("UPnP: Unmapped TCP port {}", port);
        else
            URHO3D_LOGWARNING("UPnP: Failed to unmap port (HRESULT: 0x{:08X})", static_cast<unsigned>(hr));
        mappings->Release();
    }
    else
    {
        URHO3D_LOGWARNING("UPnP: Failed to get port mapping collection for unmap");
    }

    nat->Release();
    if (comInitialized)
        CoUninitialize();

    upnpMappingSucceeded = false;
#endif
}

// ---- ICE server loading ----
static ea::vector<ea::string> LoadIceServers(Context* ctx)
{
    const ea::string path = ctx->GetSubsystem<FileSystem>()->GetProgramDir() + "webrtc_ice_servers.txt";
    File file(ctx, path);
    if (!file.IsOpen())
        return FallbackIceServers;
    ea::vector<ea::string> s;
    while (!file.IsEof())
    {
        auto l = file.ReadLine().trimmed();
        if (!l.empty() && !l.starts_with("#"))
            s.push_back(l);
    }
    if (s.empty())
        return FallbackIceServers;
    URHO3D_LOGINFO("Loaded {} ICE server(s) from file:", s.size());
    for (const auto& server : s)
        URHO3D_LOGINFO("  {}", server.c_str());
    return s;
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
    URHO3D_LOGINFO("Direct: IP -> Connect | Relay: room ID -> Start/Connect Relay");
    URHO3D_LOGINFO("Relay URL: {}  |  Port: {}", RelayUrl, DefaultPort);
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
    bool sr = server_ && server_->IsListening();
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
    else if (server_ && server_->IsListening())
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

    const ea::string url = Format("ws://{}:{}", addr, DefaultPort);
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

    const URL listenUrl(Format("ws://0.0.0.0:{}", DefaultPort));
    URHO3D_LOGINFO("Starting server on {} ...", listenUrl.ToString());
    server_->Listen(listenUrl);
    // Attempt UPnP port mapping as a fallback for NAT traversal
    UPnPMapPort(DefaultPort);
    UpdateButtons();
}

// ---- Relay mode ----
void WebRTCSTUN::HandleStartRelay(StringHash, VariantMap&)
{
    ea::string rid = textEdit_->GetText().trimmed();
    if (rid.empty())
        rid = "default";
    textEdit_->SetText(EMPTY_STRING);

    if (!server_)
    {
        server_ = MakeShared<DataChannelServer>(context_);
        server_->SetIceServers(AsViews(LoadIceServers(context_)));
        server_->onListenStart_.Subscribe(this, &WebRTCSTUN::HandleServerListenStart);
        server_->onListenStop_.Subscribe(this, &WebRTCSTUN::HandleServerListenStop);
        server_->onConnected_.Subscribe(this, &WebRTCSTUN::HandleServerConnected);
        server_->onDisconnected_.Subscribe(this, &WebRTCSTUN::HandleServerDisconnected);
    }

    URHO3D_LOGINFO("Joining relay room '{}' as host...", rid);

    auto ws = std::make_shared<rtc::WebSocket>();
    ws->onOpen([this, rid, ws]()
    {
        ea::string join = Format(R"({"type":"join","room":"{}"})", rid);
        ws->send(reinterpret_cast<const std::byte*>(join.c_str()), join.length());
    });
    ws->onMessage([this, ws](rtc::binary data)
    {
        ea::string json(reinterpret_cast<const char*>(data.data()), data.size());
        if (json.find("\"paired\"") != ea::string::npos)
        {
            URHO3D_LOGINFO("Relay: Paired!");
            server_->onListenStart_(server_);
            auto dc = MakeShared<DataChannelConnection>(context_);
            dc->SetIceServers(AsViews(LoadIceServers(context_)));
            dc->InitializeWithWebSocket(ws);
        }
    }, [](rtc::string) {});
    ws->onClosed([this]()
    {
        if (server_ && server_->IsListening())
            server_->onListenStop_(server_);
    });
    ws->open(RelayUrl);
    UpdateButtons();
}

void WebRTCSTUN::HandleConnectRelay(StringHash, VariantMap&)
{
    ea::string rid = textEdit_->GetText().trimmed();
    if (rid.empty())
        rid = "default";
    textEdit_->SetText(EMPTY_STRING);

    if (clientConnection_ && !clientConnection_->IsDisconnected())
        clientConnection_->Disconnect();

    retryAddress_ = rid;
    retryCount_ = 0;
    wasEverConnected_ = false;
    isRelayMode_ = true;
    TryConnectRelay(rid);
}

void WebRTCSTUN::TryConnectRelay(const ea::string& rid)
{
    if (!clientConnection_)
    {
        clientConnection_ = MakeShared<DataChannelConnection>(context_);
        clientConnection_->SetIceServers(AsViews(LoadIceServers(context_)));
        clientConnection_->onConnected_.Subscribe(this, &WebRTCSTUN::HandleClientConnected);
        clientConnection_->onDisconnected_.Subscribe(this, &WebRTCSTUN::HandleClientDisconnected);
        clientConnection_->onMessage_.SubscribeWithSender(this, &WebRTCSTUN::HandleNetworkMessage);
    }

    URHO3D_LOGINFO("Joining relay room '{}' as client... (attempt {})", rid, retryCount_ + 1);

    auto ws = std::make_shared<rtc::WebSocket>();
    ws->onOpen([this, rid, ws]()
    {
        ea::string join = Format(R"({"type":"join","room":"{}"})", rid);
        ws->send(reinterpret_cast<const std::byte*>(join.c_str()), join.length());
    });
    ws->onMessage([this, ws](rtc::binary data)
    {
        ea::string json(reinterpret_cast<const char*>(data.data()), data.size());
        if (json.find("\"paired\"") != ea::string::npos)
        {
            URHO3D_LOGINFO("Relay: Paired!");
            clientConnection_->InitializeWithWebSocket(ws);
            HookIceLogging(clientConnection_);
        }
    }, [](rtc::string) {});
    ws->open(RelayUrl);
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
            TryConnectRelay(retryAddress_);
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
        UPnPUnmapPort(DefaultPort);
        for (const auto& weakConn : serverConnections_)
        {
            if (auto conn = weakConn.Lock())
                conn->Disconnect();
        }
        serverConnections_.clear();
        serverClientIds_.clear();
        server_->Stop();
    }

    UpdateButtons();
}

void WebRTCSTUN::HandleServerListenStart()
{
    URHO3D_LOGINFO("Server is now listening on port {}", DefaultPort);
    URHO3D_LOGINFO("Give this machine's IP address to the client.");
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
    bool isServer = server_ && server_->IsListening();

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
        peer->onLocalCandidate([](rtc::Candidate c)
        { URHO3D_LOGINFO("ICE local candidate: {} ({})", CandStr(c.type()), static_cast<int>(c.type())); });
    }
}

} // namespace Urho3D