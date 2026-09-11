// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/Ptr.h"

namespace Urho3D
{

class ReplicatedPeer;
using ReplicatedPeerPtr = SharedPtr<ReplicatedPeer, RefCounted>;
using ReplicatedPeerWeakPtr = WeakPtr<ReplicatedPeer, RefCounted>;

} // namespace Urho3D
