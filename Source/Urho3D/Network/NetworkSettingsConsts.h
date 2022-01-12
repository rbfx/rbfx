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

#pragma once

#include "../Container/ConstString.h"
#include "../Core/Variant.h"

namespace Urho3D
{

struct NetworkSetting
{
    ConstString name_;
    Variant defaultValue_;
};

#define URHO3D_NETWORK_SETTING(name, defaultValue) \
    URHO3D_GLOBAL_CONSTANT(NetworkSetting name = (NetworkSetting{#name, defaultValue})) \

/// Return network setting or default value.
inline const Variant& GetNetworkSetting(const VariantMap& map, const NetworkSetting& key)
{
    const auto iter = map.find(key.name_.GetHash());
    return iter != map.end() ? iter->second : key.defaultValue_;
}

/// Set network setting in the map.
inline void SetNetworkSetting(VariantMap& map, const NetworkSetting& key, const Variant& value)
{
    map[key.name_] = value;
}

/// Network parameters supported by the engine
namespace NetworkSettings
{

/// Internal properties. Do not override.
/// @{

/// Update frequency of the server, frames per second.
URHO3D_NETWORK_SETTING(UpdateFrequency, 30);
/// Connection ID of current client.
URHO3D_NETWORK_SETTING(ConnectionId, 0);

/// @}

/// Server-only properties ignored by the client
/// @{

/// Max number of pending pings. Keep this value high enough not to miss any pings.
URHO3D_NETWORK_SETTING(MaxPendingPings, 100);
/// Interval in seconds between initial pings sent to client.
URHO3D_NETWORK_SETTING(InitialPingInterval, 0.1f);
/// Number of initial pings.
URHO3D_NETWORK_SETTING(NumInitialPings, 10);
/// Interval in seconds between periodic pings.
URHO3D_NETWORK_SETTING(PeriodicPingInterval, 0.25f);
/// Number of averaged pings to maintain smooth ping value.
URHO3D_NETWORK_SETTING(NumAveragedPings, 20);
/// Number of highest pings skipped when averaged.
URHO3D_NETWORK_SETTING(NumAveragedPingsTrimmed, 3);
/// Interval in seconds between periodic clock updates.
URHO3D_NETWORK_SETTING(PeriodicClockInterval, 0.25f);

/// @}

/// Client-only properties ignored by the server
/// @{

/// Size of ring buffer used to keep server time synchronized.
/// Server time is expected to stay unchanged most of the time since client and server work at the same rate.
/// Error may occur is case of client or server lag.
/// Prefer larger buffer to prevent random fluctuations.
URHO3D_NETWORK_SETTING(ClockBufferSize, 21);
/// Number of server time samples to skip on server time evaluation.
URHO3D_NETWORK_SETTING(ClockBufferNumTrimmed, 3);

/// @}

}

}
