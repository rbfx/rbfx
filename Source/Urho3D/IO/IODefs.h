// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Variant.h"

namespace Urho3D
{

/// Vector encoding for serialization/deserialization.
/// Encoding parameter may be provided.
enum class VectorBinaryEncoding
{
    /// 32-bit float components.
    Float,
    /// 64-bit float components.
    Double,
    /// 32-bit integer components, range is normalized to [-param; param].
    Int32,
    /// 16-bit integer components, range is normalized to [-param; param].
    Int16,

    Count,
};

} // namespace Urho3D
