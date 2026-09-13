// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Scene/TrackedComponent.h>

namespace Urho3D
{

/// ID used to identify unique NetworkObject within Scene.
using NetworkId = ComponentReference;

/// Relevance of the NetworkObject.
/// If greater than 0, indicates the period of unreliable updates of the NetworkObject.
/// Therefore, it's safe to use any positive number as NetworkObjectRelevance.
enum class NetworkObjectRelevance : signed char
{
    Irrelevant = -1,
    NoUpdates = 0,
    NormalUpdates = 1,

    MaxPeriod = 127
};

/// Network frame that represents discrete time on the server.
/// It's usually non-negative, but it's signed for simpler maths.
enum class NetworkFrame : long long
{
    Min = ea::numeric_limits<long long>::min(),
    Max = ea::numeric_limits<long long>::max()
};

inline NetworkFrame& operator++(NetworkFrame& frame)
{
    URHO3D_ASSERT(frame != NetworkFrame::Max);
    frame = static_cast<NetworkFrame>(static_cast<long long>(frame) + 1);
    return frame;
}

inline NetworkFrame& operator--(NetworkFrame& frame)
{
    URHO3D_ASSERT(frame != NetworkFrame::Min);
    frame = static_cast<NetworkFrame>(static_cast<long long>(frame) - 1);
    return frame;
}

inline long long operator-(NetworkFrame lhs, NetworkFrame rhs)
{
    return static_cast<long long>(lhs) - static_cast<long long>(rhs);
}

inline NetworkFrame operator+(NetworkFrame lhs, long long rhs)
{
    return static_cast<NetworkFrame>(static_cast<long long>(lhs) + rhs);
}

inline NetworkFrame operator-(NetworkFrame lhs, long long rhs)
{
    return static_cast<NetworkFrame>(static_cast<long long>(lhs) - rhs);
}

/// Marker used to synchronize the beginning of the fixed-step event (like physical world update)
/// and the network frame.
struct NetworkFrameSync
{
    /// Synchronized network frame.
    NetworkFrame networkFrame_{};
    /// Index of the fixed-step event that corresponds to the beginning of the synchronized network frame.
    /// If network update rate is the same as fixed-step event rate, index is always zero.
    unsigned offset_{};
};

} // namespace Urho3D
