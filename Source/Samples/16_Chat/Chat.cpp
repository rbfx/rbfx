//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/IOEvents.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Network/Protocol.h>
#include <Urho3D/Network/Transport/DataChannel/DataChannelConnection.h>
#include <Urho3D/Network/Transport/DataChannel/DataChannelServer.h>
#include <Urho3D/Network/Transport/NetworkConnection.h>
#include <Urho3D/Network/URL.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "Chat.h"

#include <Urho3D/DebugNew.h>

// Undefine Windows macro, as our Connection class has a function called SendMessage
#ifdef SendMessage
#undef SendMessage
#endif

// Identifier for the chat network messages
const auto MSG_CHAT = static_cast<NetworkMessageId>(MSG_USER + 0);
// UDP port we will use
const unsigned short CHAT_SERVER_PORT = 2345;


Chat::Chat(Context* context) :
    Sample(context)
{
}

void Chat::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Create the user interface
    CreateUI();

    // Subscribe to UI and network events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);
}

void Chat::Stop()
{
    HandleDisconnect({}, GetEventDataMap());
    BaseClassName::Stop();
}

void Chat::CreateUI()
{
    SetLogoVisible(false); // We need the full rendering window

    auto* graphics = GetSubsystem<Graphics>();
    UIElement* root = GetUIRoot();
    auto* cache = GetSubsystem<ResourceCache>();
    auto* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    chatHistoryText_ = root->CreateChild<Text>();
    chatHistoryText_->SetFont(font, 12);

    buttonContainer_ = root->CreateChild<UIElement>();
    buttonContainer_->SetFixedSize(graphics->GetWidth(), 20);
    buttonContainer_->SetPosition(0, graphics->GetHeight() - 20);
    buttonContainer_->SetLayoutMode(LM_HORIZONTAL);

    textEdit_ = buttonContainer_->CreateChild<LineEdit>();
    textEdit_->SetStyleAuto();

    sendButton_ = CreateButton("Send", 70);
    connectButton_ = CreateButton("Connect", 90);
    disconnectButton_ = CreateButton("Disconnect", 100);
    startServerButton_ = CreateButton("Start Server", 110);

    UpdateButtons();

    float rowHeight = chatHistoryText_->GetRowHeight();
    // Row height would be zero if the font failed to load
    if (rowHeight)
    {
        float numberOfRows = (graphics->GetHeight() - 100) / rowHeight;
        chatHistory_.resize(static_cast<unsigned int>(numberOfRows));
    }

    // No viewports or scene is defined. However, the default zone's fog color controls the fill color
    GetSubsystem<Renderer>()->GetDefaultZone()->SetFogColor(Color(0.0f, 0.0f, 0.1f));
}

void Chat::SubscribeToEvents()
{
    // Subscribe to UI element events
    SubscribeToEvent(textEdit_, E_TEXTFINISHED, URHO3D_HANDLER(Chat, HandleSend));
    SubscribeToEvent(sendButton_, E_RELEASED, URHO3D_HANDLER(Chat, HandleSend));
    SubscribeToEvent(connectButton_, E_RELEASED, URHO3D_HANDLER(Chat, HandleConnect));
    SubscribeToEvent(disconnectButton_, E_RELEASED, URHO3D_HANDLER(Chat, HandleDisconnect));
    SubscribeToEvent(startServerButton_, E_RELEASED, URHO3D_HANDLER(Chat, HandleStartServer));

    // Subscribe to log messages so that we can pipe them to the chat window
    SubscribeToEvent(E_LOGMESSAGE, URHO3D_HANDLER(Chat, HandleLogMessage));
}

Button* Chat::CreateButton(const ea::string& text, int width)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

    auto* button = buttonContainer_->CreateChild<Button>();
    button->SetStyleAuto();
    button->SetFixedWidth(width);

    auto* buttonText = button->CreateChild<Text>();
    buttonText->SetFont(font, 12);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetText(text);

    return button;
}

void Chat::ShowChatText(const ea::string& row)
{
    chatHistory_.pop_front();
    chatHistory_.push_back(row);

    // Concatenate all the rows in history
    ea::string allRows;
    for (unsigned i = 0; i < chatHistory_.size(); ++i)
        allRows += chatHistory_[i] + "\n";

    chatHistoryText_->SetText(allRows);
}

void Chat::UpdateButtons()
{
    const bool clientConnected = clientConnection_ && clientConnection_->IsConnected();
    const bool serverRunning = server_ && server_->IsListening();

    // Show and hide buttons so that eg. Connect and Disconnect are never shown at the same time
    sendButton_->SetVisible(clientConnected);
    connectButton_->SetVisible(!clientConnected && !serverRunning);
    disconnectButton_->SetVisible(clientConnected || serverRunning);
    startServerButton_->SetVisible(!clientConnected && !serverRunning);
}

void Chat::HandleLogMessage(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace LogMessage;

    ShowChatText(eventData[P_MESSAGE].GetString());
}

void Chat::HandleSend(StringHash /*eventType*/, VariantMap& eventData)
{
    ea::string text = textEdit_->GetText();
    if (text.empty())
        return; // Do not send an empty message

    if (clientConnection_ && clientConnection_->IsConnected())
    {
        // A VectorBuffer object is convenient for constructing a message to send
        VectorBuffer msg;
        msg.WriteString(text);
        // Send the chat message as in-order and reliable
        clientConnection_->SendMessage(MSG_CHAT, msg);
        // Empty the text edit after sending
        textEdit_->SetText(EMPTY_STRING);
    }
}

void Chat::HandleConnect(StringHash /*eventType*/, VariantMap& eventData)
{
    ea::string address = textEdit_->GetText();
    address.trim();
    if (address.empty())
        address = "localhost"; // Use localhost to connect if nothing else specified
    // Empty the text edit after reading the address to connect to
    textEdit_->SetText(EMPTY_STRING);

    if (!clientConnection_)
    {
        clientConnection_ = MakeShared<DataChannelConnection>(context_);
        clientConnection_->onConnected_.Subscribe(this, &Chat::HandleClientConnected);
        clientConnection_->onDisconnected_.Subscribe(this, &Chat::HandleClientDisconnected);
        clientConnection_->onMessage_.SubscribeWithSender(this, &Chat::HandleNetworkMessage);
    }

    if (!clientConnection_->IsDisconnected())
        return;

    clientConnection_->Connect(URL(Format("{}:{}", address, CHAT_SERVER_PORT)));

    UpdateButtons();
}

void Chat::HandleDisconnect(StringHash /*eventType*/, VariantMap& eventData)
{
    if (clientConnection_ && !clientConnection_->IsDisconnected())
    {
        clientConnection_->Disconnect();
    }
    else if (server_ && server_->IsListening())
    {
        for (const auto& connection : serverConnections_)
        {
            if (auto connectionPtr = connection.Lock())
                connectionPtr->Disconnect();
        }
        serverConnections_.clear();
        server_->Stop();
    }

    UpdateButtons();
}

void Chat::HandleStartServer(StringHash /*eventType*/, VariantMap& eventData)
{
    if (!server_)
    {
        server_ = MakeShared<DataChannelServer>(context_);
        server_->onConnected_.Subscribe(this, &Chat::HandleServerConnected);
        server_->onDisconnected_.Subscribe(this, &Chat::HandleServerDisconnected);
    }

    const URL listenUrl(Format("ws://0.0.0.0:{}", CHAT_SERVER_PORT));
    server_->Listen(listenUrl);

    UpdateButtons();
}

void Chat::HandleServerConnected(NetworkConnection* connection)
{
    serverConnections_.push_back(WeakPtr<NetworkConnection>{});
    serverConnections_.back() = connection;
    connection->onMessage_.SubscribeWithSender(this, &Chat::HandleNetworkMessage);
    UpdateButtons();
}

void Chat::HandleServerDisconnected(NetworkConnection* connection)
{
    for (unsigned i = 0; i < serverConnections_.size();)
    {
        if (serverConnections_[i].Get() == connection)
            serverConnections_.erase(serverConnections_.begin() + i);
        else
            ++i;
    }
    UpdateButtons();
}

void Chat::HandleClientConnected()
{
    UpdateButtons();
}

void Chat::HandleClientDisconnected()
{
    UpdateButtons();
}

void Chat::HandleNetworkMessage(NetworkConnection* connection, NetworkMessageId messageId, MemoryBuffer& message, bool& handled)
{
    if (messageId != MSG_CHAT)
        return;

    ea::string text = message.ReadString();

    if (server_ && server_->IsListening() && IsServerConnection(connection))
    {
        ea::string senderLabel = connection->GetAddress();
        if (senderLabel.empty())
            senderLabel = "Unknown";
        if (connection->GetPort() != 0)
            senderLabel = Format("{}:{}", senderLabel, connection->GetPort());

        text = senderLabel + " " + text;

        VectorBuffer sendMsg;
        sendMsg.WriteString(text);
        for (const auto& serverConnection : serverConnections_)
        {
            if (auto connectionPtr = serverConnection.Lock())
                connectionPtr->SendMessage(MSG_CHAT, sendMsg);
        }
    }

    ShowChatText(text);
    handled = true;
}

bool Chat::IsServerConnection(NetworkConnection* connection) const
{
    for (const auto& serverConnection : serverConnections_)
    {
        if (serverConnection.Get() == connection)
            return true;
    }
    return false;
}
