//
// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022 the rbfx project.
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

#include "AdvancedNetworkingUI.h"

#include <Urho3D/Network/Network.h>
#include <Urho3D/RmlUI/RmlUI.h>
#include <Urho3D/Scene/Scene.h>

void AdvancedNetworkingUI::StartServer()
{
    Stop();

    Network* network = GetSubsystem<Network>();
    network->StartServer(serverPort_);
}

void AdvancedNetworkingUI::ConnectToServer(const ea::string& address)
{
    Stop();

    Network* network = GetSubsystem<Network>();
    network->Connect(address, serverPort_, GetScene());
}

void AdvancedNetworkingUI::Stop()
{
    Network* network = GetSubsystem<Network>();
    network->Disconnect();
    network->StopServer();
}

void AdvancedNetworkingUI::OnNodeSet(Node* node)
{
    BaseClassName::OnNodeSet(node);
    RmlUI* rmlUI = GetUI();
    Network* network = GetSubsystem<Network>();
    Rml::Context* rmlContext = rmlUI->GetRmlContext();

    if (node != nullptr && !model_)
    {
        Rml::DataModelConstructor constructor = rmlContext->CreateDataModel("AdvancedNetworkingUI_model");
        if (!constructor)
            return;

        constructor.Bind("port", &serverPort_);
        constructor.Bind("connectionAddress", &connectionAddress_);
        constructor.BindFunc("isServer", [=](Rml::Variant& result) { result = network->IsServerRunning(); });
        constructor.BindFunc("isClient", [=](Rml::Variant& result) { result = network->GetServerConnection() != nullptr; });
        constructor.Bind("cheatAutoMovementCircle", &cheatAutoMovementCircle_);
        constructor.Bind("cheatAutoAimHand", &cheatAutoAimHand_);
        constructor.Bind("cheatAutoClick", &checkAutoClick_);

        constructor.BindEventCallback("onStartServer",
            [=](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { StartServer(); });
        constructor.BindEventCallback("onConnectToServer",
            [=](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { ConnectToServer(connectionAddress_); });
        constructor.BindEventCallback("onStop",
            [=](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Stop(); });

        model_ = constructor.GetModelHandle();

        SetResource("UI/AdvancedNetworkingUI.rml");
        SetOpen(true);
    }
    else if (node == nullptr && model_)
    {
        rmlContext->RemoveDataModel("AdvancedNetworkingUI_model");
        model_ = nullptr;
    }
}

void AdvancedNetworkingUI::Update(float timeStep)
{
    model_.DirtyVariable("isServer");
    model_.DirtyVariable("isClient");
}
