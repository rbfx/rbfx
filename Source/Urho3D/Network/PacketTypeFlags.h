// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/FlagSet.h"

namespace Urho3D
{

/// Packet types for outgoing buffers. Outgoing messages are grouped by their type
enum PacketType : uint16_t
{
    Reliable            = 1u << 0,
    Ordered             = 1u << 1,
    UnreliableUnordered = 0,
    UnreliableOrdered   = Ordered,
    ReliableUnordered   = Reliable,
    ReliableOrdered     = Reliable | Ordered,
};
URHO3D_FLAGSET(PacketType, PacketTypeFlags);
//static_assert(sizeof(PacketType) == sizeof(NetworkMessageId));

}
