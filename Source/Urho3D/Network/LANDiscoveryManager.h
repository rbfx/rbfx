//
// Copyright (c) 2017-2022 the Urho3D project.
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

#include <Urho3D/Container/FlagSet.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/IO/VectorBuffer.h>

namespace Urho3D
{

enum class LANDiscoveryMode
{
    /// Discovery beacons are broadcast to localhost.
    Local = 1,
    /// Discovery beacons are broadcast to LAN.
    LAN = 2,
    /// Discovery beacons are broadcast to localhost and LAN.
    All = Local | LAN,
};
URHO3D_FLAGSET(LANDiscoveryMode, LANDiscoveryModeFlags);

class URHO3D_API LANDiscoveryManager : public Object
{
    URHO3D_OBJECT(LANDiscoveryManager, Object);
public:
    explicit LANDiscoveryManager(Context* context);

    /// Specify data, which will be broadcast to the network for other nodes to be discovered.
    void SetBroadcastData(const VariantMap& data) { broadcastData_ = data; }
    /// If broadcast data is set, service will broadcast it periodically. If no data is set, service will discover other hosts broadcasting discovery data.
    bool Start(unsigned short port, LANDiscoveryModeFlags mode = LANDiscoveryMode::All);
    /// Stop discovery service.
    void Stop();
    /// Get current broadcast interval, which is defined in milliseconds.
    unsigned GetBroadcastTimeMs() const { return broadcastTimeMs_; }
    /// Set new broadcast interval, which is defined in in milliseconds.
    void SetBroadcastTimeMs(unsigned time) { broadcastTimeMs_ = time; }

private:
    VariantMap broadcastData_ = {};
    int socket_ = -1;
    Timer timer_;
    VectorBuffer buffer_;
    unsigned broadcastTimeMs_ = 5000;
};

}
