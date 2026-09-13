// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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

#define URHO3D_NETWORK_SETTING(name, type, defaultValue) \
    URHO3D_GLOBAL_CONSTANT(NetworkSetting name = (NetworkSetting{ConstString{#name}, type{defaultValue}})) \

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

/// Set network setting in the map to default value.
inline void SetDefaultNetworkSetting(VariantMap& map, const NetworkSetting& key)
{
    map[key.name_] = key.defaultValue_;
}

/// Network parameters supported by the engine
namespace NetworkSettings
{

/// Internal properties. Do not override.
/// @{

/// Version of internal protocol.
URHO3D_NETWORK_SETTING(InternalProtocolVersion, unsigned, 1);
/// Update frequency of the server, frames per second.
URHO3D_NETWORK_SETTING(UpdateFrequency, unsigned, 30);
/// Connection ID of current client.
URHO3D_NETWORK_SETTING(ConnectionId, unsigned, 0);

/// @}

/// Common properties between client and server.
/// @{

/// Maximum allowed delay between server time and replica time. Client must extrapolate if its delay is bigger.
URHO3D_NETWORK_SETTING(InterpolationLimit, float, 0.25f);
/// Maximum number of input frames tracked by the client.
URHO3D_NETWORK_SETTING(MaxInputFrames, unsigned, 256);
/// Maximum number of input frames sent to server including relevant frame.
URHO3D_NETWORK_SETTING(MaxInputRedundancy, unsigned, 32);

/// @}

/// Server-only properties ignored by the client.
/// @{

/// Interval in seconds between periodic clock updates.
URHO3D_NETWORK_SETTING(PeriodicClockInterval, float, 1.0f);
/// Number of clock ticks used to filter input delay.
URHO3D_NETWORK_SETTING(InputDelayFilterBufferSize, unsigned, 11);
/// Number of clock ticks used to filter input buffer.
URHO3D_NETWORK_SETTING(InputBufferingFilterBufferSize, unsigned, 11);
/// Number of frames used to evaluate recommended input buffering.
URHO3D_NETWORK_SETTING(InputBufferingWindowSize, unsigned, 128);
/// Input buffering is calculated as `clamp(round(x*tweakA + tweakB), min, max)`,
/// where x is magical statistics that roughly corresponds to the max amount of consecutive frame loss.
URHO3D_NETWORK_SETTING(InputBufferingTweakA, float, 1.3f);
URHO3D_NETWORK_SETTING(InputBufferingTweakB, float, 1.0f);
URHO3D_NETWORK_SETTING(InputBufferingMin, unsigned, 0);
URHO3D_NETWORK_SETTING(InputBufferingMax, unsigned, 8);
/// Interval in seconds between NetworkObject becoming unneeded for client and replication stopped.
URHO3D_NETWORK_SETTING(RelevanceTimeout, float, 5.0f);
/// Duration in seconds of value tracking on server. Used for lag compensation.
URHO3D_NETWORK_SETTING(ServerTracingDuration, float, 5.0f);

/// @}

/// Client-only properties ignored by the server.
/// @{

/// Minimal time error that is not ignored.
URHO3D_NETWORK_SETTING(TimeErrorTolerance, float, 0.002f);
/// Limit of smooth time adjustment. Larger errors are corrected immediately.
URHO3D_NETWORK_SETTING(TimeSnapThreshold, float, 2.5f);
/// Minimal time dilation factor.
URHO3D_NETWORK_SETTING(MinTimeDilation, float, 0.7f);
/// Maximal time dilation factor.
URHO3D_NETWORK_SETTING(MaxTimeDilation, float, 1.5f);
/// Delay in seconds before delivered updates are used for replica interpolation.
URHO3D_NETWORK_SETTING(InterpolationDelay, float, 0.1f);
/// Duration in seconds of value tracking on client. Used for interpolation.
URHO3D_NETWORK_SETTING(ClientTracingDuration, float, 3.0f);
/// Duration in seconds of value extrapolation. Beyond this limit the value stays fixed.
URHO3D_NETWORK_SETTING(ExtrapolationLimit, float, 0.5f);

/// @}

}

#undef URHO3D_NETWORK_SETTING

}
