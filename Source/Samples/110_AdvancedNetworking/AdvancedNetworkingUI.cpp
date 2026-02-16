// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "AdvancedNetworkingUI.h"

#include <Urho3D/RmlUI/RmlUI.h>
#include <Urho3D/Scene/Scene.h>

AdvancedNetworkingUI::AdvancedNetworkingUI(Context* context)
    : RmlUIComponent(context)
{
    SetResource("UI/AdvancedNetworkingUI.rml");
}

void AdvancedNetworkingUI::StartServer()
{
    OnStartServer(this, static_cast<unsigned short>(serverPort_));
}

void AdvancedNetworkingUI::ConnectToServer(const ea::string& address)
{
    OnConnectToServer(this, address, static_cast<unsigned short>(serverPort_));
}

void AdvancedNetworkingUI::Stop()
{
    OnStop(this);
}

void AdvancedNetworkingUI::OnDataModelInitialized()
{
    auto constructor = GetDataModelConstructor();

    constructor->Bind("port", &serverPort_);
    constructor->Bind("connectionAddress", &connectionAddress_);
    constructor->BindFunc("isServer", [=](Rml::Variant& result) { result = isServer_; });
    constructor->BindFunc("isClient", [=](Rml::Variant& result) { result = isClient_; });
    constructor->Bind("cheatAutoMovementCircle", &cheatAutoMovementCircle_);
    constructor->Bind("cheatAutoAimHand", &cheatAutoAimHand_);
    constructor->Bind("cheatAutoClick", &checkAutoClick_);

    constructor->BindEventCallback("onStartServer",
        [=](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { StartServer(); });
    constructor->BindEventCallback("onConnectToServer",
        [=](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { ConnectToServer(connectionAddress_); });
    constructor->BindEventCallback("onStop",
        [=](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Stop(); });
}

void AdvancedNetworkingUI::Update(float timeStep)
{
    BaseClassName::Update(timeStep);

    DirtyVariable("isServer");
    DirtyVariable("isClient");
}
