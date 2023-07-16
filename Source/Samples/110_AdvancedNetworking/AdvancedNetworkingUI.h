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

#pragma once

#include <Urho3D/Network/Connection.h>
#include <Urho3D/RmlUI/RmlUIComponent.h>

#include <RmlUi/Core/DataModelHandle.h>

using namespace Urho3D;

/// UI widget to manage server and client settings.
class AdvancedNetworkingUI : public RmlUIComponent
{
    URHO3D_OBJECT(AdvancedNetworkingUI, RmlUIComponent);

public:
    explicit AdvancedNetworkingUI(Context* context);

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
};
