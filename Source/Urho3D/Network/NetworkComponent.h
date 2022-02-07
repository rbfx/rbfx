//
// Copyright (c) 2008-2020 the Urho3D project.
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

/// \file

#pragma once

#include "../Container/FlagSet.h"
#include "../Scene/Component.h"
#include "../Network/NetworkManager.h"

namespace Urho3D
{

/// Helper base class for user-defined network replication logic.
class URHO3D_API NetworkComponent : public Component
{
    URHO3D_OBJECT(NetworkComponent, Component);

public:
    explicit NetworkComponent(Context* context);
    ~NetworkComponent() override;

    static void RegisterObject(Context* context);

    /// Set network ID. For internal use only.
    void SetNetworkID(unsigned networkId) { networkId_ = networkId; }
    /// Return current network ID or M_MAX_UNSIGNED if not registered.
    unsigned GetNetworkID() const { return networkId_; }

    /// Prepare network update at server side.
    //virtual void PrepareServerUpdate();

protected:
    /// Component implementation
    /// @{
    void OnSceneSet(Scene* scene) override;
    /// @}

private:
    /// Network ID. Unique withing Scene.
    unsigned networkId_{ M_MAX_UNSIGNED };
};

}
