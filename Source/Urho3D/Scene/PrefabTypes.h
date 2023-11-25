//
// Copyright (c) 2022-2022 the rbfx project.
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

namespace Urho3D
{

/// Strongly typed attribute ID.
enum class AttributeId : unsigned
{
    None
};

/// Strongly typed serializable ID.
enum class SerializableId : unsigned
{
    None
};

/// Prefab archive format flags.
/// Flags must be the same on loading and saving.
/// Mismatch will cause serialization error.
enum class PrefabArchiveFlag
{
    None = 0,

    /// Whether to ignore ID of serializable object.
    IgnoreSerializableId = 1 << 0,
    /// Whether to ignore type of serializable object.
    IgnoreSerializableType = 1 << 1,
    /// Whether to compact type names to hashes.
    /// Useful for large structures not intended for readability.
    CompactTypeNames = 1 << 2,
    /// Whether to serialize temporary objects.
    /// Useful if the exact serialization is required.
    SerializeTemporary = 1 << 3,
};
URHO3D_FLAGSET(PrefabArchiveFlag, PrefabArchiveFlags);

/// Flags that control how prefab is saved.
enum class PrefabSaveFlag
{
    None = 0,

    /// Whether to compact attribute names to hashes.
    /// Useful for large structures not intended for readability.
    CompactAttributeNames = 1 << 0,
    /// Whether to treat enums as strings.
    /// Improves readability and portability of text formats.
    EnumsAsStrings = 1 << 1,
    /// Whether to save default attribute values.
    SaveDefaultValues = 1 << 2,
    /// Whether the prefab is saved. Attributes without AM_PREFAB flag will be ignored.
    Prefab = 1 << 3,
    /// Whether to save temporary objects and attributes.
    SaveTemporary = 1 << 4,
};
URHO3D_FLAGSET(PrefabSaveFlag, PrefabSaveFlags);

/// Flags that control how prefab is loaded.
enum class PrefabLoadFlag
{
    None = 0,

    /// Whether to check that serializable object type matches type in prefab.
    CheckSerializableType = 1 << 0,
    /// Whether to keep existing components of serializable object.
    KeepExistingComponents = 1 << 1,
    /// Whether to keep existing children of serializable object.
    KeepExistingChildren = 1 << 2,
    /// Whether to create temporary nodes and components instead of persistent ones.
    /// Useful for instantiating prefabs.
    LoadAsTemporary = 1 << 3,
    /// Whether to discard and reassign IDs.
    DiscardIds = 1 << 4,
    /// Whether to ignore attributes of the root node.
    IgnoreRootAttributes = 1 << 5,
    /// Keep "temporary" state as is.
    KeepTemporaryState = 1 << 6,
    /// Skip "ApplyAttributes" callback.
    SkipApplyAttributes = 1 << 7,
};
URHO3D_FLAGSET(PrefabLoadFlag, PrefabLoadFlags);

/// Return standard archive flags for node. Node doesn't need type and must have ID.
inline PrefabArchiveFlags ToNodeFlags(PrefabArchiveFlags flags)
{
    return (flags | PrefabArchiveFlag::IgnoreSerializableType) & ~PrefabArchiveFlag::IgnoreSerializableId;
}

/// Return standard archive flags for component. Component must have type and ID.
inline PrefabArchiveFlags ToComponentFlags(PrefabArchiveFlags flags)
{
    return flags & ~PrefabArchiveFlag::IgnoreSerializableType & ~PrefabArchiveFlag::IgnoreSerializableType;
}

} // namespace Urho3D
