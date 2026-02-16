// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Core/Signal.h>
#include <Urho3D/RmlUI/RmlUIComponent.h>

#include <RmlUi/Core/DataModelHandle.h>

using namespace Urho3D;

/// UI widget to manage server and client settings.
class AdvancedNetworkingUI : public RmlUIComponent
{
    URHO3D_OBJECT(AdvancedNetworkingUI, RmlUIComponent);

public:
    explicit AdvancedNetworkingUI(Context* context);

    Signal<void(unsigned short)> OnStartServer;
    Signal<void(const ea::string&, unsigned short)> OnConnectToServer;
    Signal<void()> OnStop;

    void SetServerRunning(bool value) { isServer_ = value; }
    void SetClientConnected(bool value) { isClient_ = value; }

    void StartServer();
    void ConnectToServer(const ea::string& address);
    void Stop() override;

    bool GetCheatAutoMovementCircle() const { return cheatAutoMovementCircle_; }
    bool GetCheatAutoAimHand() const { return cheatAutoAimHand_; }
    bool GetCheatAutoClick() const { return checkAutoClick_; }

private:
    void Update(float timeStep) override;
    void OnDataModelInitialized() override;

    int serverPort_{2345};
    Rml::String connectionAddress_{"localhost"};

    bool cheatAutoMovementCircle_{};
    bool cheatAutoAimHand_{};
    bool checkAutoClick_{};
    bool isServer_{};
    bool isClient_{};
};
