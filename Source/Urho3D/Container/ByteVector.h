// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <EASTL/shared_ptr.h>
#include <EASTL/span.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Vector of bytes.
using ByteVector = ea::vector<unsigned char>;

/// Shared vector of bytes.
using SharedByteVector = ea::shared_ptr<ByteVector>;

/// Span of bytes (mutable).
using ByteSpan = ea::span<unsigned char>;

/// Span of bytes (immutable).
using ConstByteSpan = ea::span<const unsigned char>;

}
