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
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/IOEvents.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Network/LANDiscoveryManager.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "LANDiscovery.h"

#include <Urho3D/DebugNew.h>

// Undefine Windows macro, as our Connection class has a function called SendMessage
#ifdef SendMessage
#undef SendMessage
#endif

static const int SERVER_PORT = 54654;

LANDiscovery::LANDiscovery(Context* context)
    : Sample(context)
    , lanDiscovery_(MakeShared<LANDiscoveryManager>(context))
{
}

void LANDiscovery::Start()
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

void LANDiscovery::CreateUI()
{
    SetLogoVisible(true); // We need the full rendering window

    auto* graphics = GetSubsystem<Graphics>();
    UIElement* root = GetUIRoot();
    auto* cache = GetSubsystem<ResourceCache>();
    auto* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    int marginTop = 20;
    CreateLabel("1. Start server", IntVector2(20, marginTop-20));
    startServer_ = CreateButton("Start server", 160, IntVector2(20, marginTop));
    stopServer_ = CreateButton("Stop server", 160, IntVector2(20, marginTop));
    stopServer_->SetVisible(false);

    // Create client connection related fields
    marginTop += 80;
    CreateLabel("2. Discover LAN servers", IntVector2(20, marginTop-20));
    refreshServerList_ = CreateButton("Search...", 160, IntVector2(20, marginTop));

    marginTop += 80;
    CreateLabel("Local servers:", IntVector2(20, marginTop - 20));
    serverList_ = CreateLabel("", IntVector2(20, marginTop));

    // No viewports or scene is defined. However, the default zone's fog color controls the fill color
    GetSubsystem<Renderer>()->GetDefaultZone()->SetFogColor(Color(0.0f, 0.0f, 0.1f));
}

void LANDiscovery::SubscribeToEvents()
{
    SubscribeToEvent(E_NETWORKHOSTDISCOVERED, URHO3D_HANDLER(LANDiscovery, HandleNetworkHostDiscovered));
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(LANDiscovery, HandleExpireServers));

    SubscribeToEvent(startServer_, "Released", URHO3D_HANDLER(LANDiscovery, HandleStartServer));
    SubscribeToEvent(stopServer_, "Released", URHO3D_HANDLER(LANDiscovery, HandleStopServer));
    SubscribeToEvent(refreshServerList_, "Released", URHO3D_HANDLER(LANDiscovery, HandleDoNetworkDiscovery));
}

Button* LANDiscovery::CreateButton(const ea::string& text, int width, IntVector2 position)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

    auto* button = GetUIRoot()->CreateChild<Button>();
    button->SetStyleAuto();
    button->SetFixedWidth(width);
    button->SetFixedHeight(30);
    button->SetPosition(position);

    auto* buttonText = button->CreateChild<Text>();
    buttonText->SetFont(font, 12);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetText(text);

    return button;
}

Text* LANDiscovery::CreateLabel(const ea::string& text, IntVector2 pos)
{
    auto* cache = GetSubsystem<ResourceCache>();
    // Create log element to view latest logs from the system
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    auto* label = GetUIRoot()->CreateChild<Text>();
    label->SetFont(font, 12);
    label->SetColor(Color(0.0f, 1.0f, 0.0f));
    label->SetPosition(pos);
    label->SetText(text);
    return label;
}

void LANDiscovery::HandleNetworkHostDiscovered(StringHash eventType, VariantMap& eventData)
{
    using namespace NetworkHostDiscovered;

    VariantMap data = eventData[P_BEACON].GetVariantMap();
    ea::string name = data["Name"].GetString();

    // Refresh server that reannounced itself
    {
        auto it = serverListItems_.find(name);
        if (it != serverListItems_.end())
        {
            URHO3D_LOGINFO("Server {} reannounced itself!", name);
            it->second.lastSeen_ = Time::GetTimeSinceEpoch();
        }
        else
        {
            URHO3D_LOGINFO("Server {} discovered!", name);
            serverListItems_[name] = {
                name, data["Players"].GetInt(), eventData[P_ADDRESS].GetString(), (unsigned short)eventData[P_PORT].GetUInt(),
                Time::GetTimeSinceEpoch()};
        }
    }

    FormatServerListUI();
}

void LANDiscovery::HandleStartServer(StringHash eventType, VariantMap& eventData)
{
    VariantMap data;
    data["Name"] = "Test server";
    data["Players"] = 100;

    // Set data which will be sent to all who requests LAN network discovery
    lanDiscovery_->SetBroadcastData(data);
    if (lanDiscovery_->Start(SERVER_PORT))
    {
        startServer_->SetVisible(false);
        stopServer_->SetVisible(true);
    }
}

void LANDiscovery::HandleStopServer(StringHash eventType, VariantMap& eventData)
{
    lanDiscovery_->Stop();
    startServer_->SetVisible(true);
    stopServer_->SetVisible(false);
}

void LANDiscovery::HandleDoNetworkDiscovery(StringHash eventType, VariantMap& eventData)
{
    /// Pass in the port that should be checked
    lanDiscovery_->Start(SERVER_PORT);
    serverList_->SetText("");
}

void LANDiscovery::HandleExpireServers(StringHash eventType, VariantMap& eventData)
{
    // Delete expired servers after 10s of not seeing an announcement
    for (auto it = serverListItems_.begin(); it != serverListItems_.end();)
    {
        if (Time::GetTimeSinceEpoch() - it->second.lastSeen_ > 10)
        {
            URHO3D_LOGINFO("Server {} expired!", it->second.name_);
            it = serverListItems_.erase(it);
            FormatServerListUI();
        }
        else
            ++it;
    }
}

void LANDiscovery::FormatServerListUI()
{
    ea::string text;
    for (auto&[name, item] : serverListItems_)
    {
        text += Format("\n{} ({}) {}:{}", item.name_, item.players_, item.address_, item.port_);
    }
    serverList_->SetText(text);
}
