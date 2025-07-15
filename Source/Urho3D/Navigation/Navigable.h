// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Navigation/NavigationDefs.h"
#include "Urho3D/Scene/Component.h"

namespace Urho3D
{

/// Component which tags geometry for inclusion in the navigation mesh. Optionally auto-includes geometry from child
/// nodes.
class URHO3D_API Navigable : public Component
{
    URHO3D_OBJECT(Navigable, Component);

public:
    /// Construct.
    explicit Navigable(Context* context);
    /// Destruct.
    ~Navigable() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Set whether geometry is automatically collected from child nodes. Default true.
    void SetRecursive(bool enable) { recursive_ = enable; }
    /// Return whether geometry is automatically collected from child nodes.
    bool IsRecursive() const { return recursive_; }

    /// Set whether geometry is walkable. Default true.
    void SetWalkable(bool enable) { walkable_ = enable; }
    /// Return whether geometry is walkable.
    bool IsWalkable() const { return walkable_; }

    /// Set area ID of geometry. Deduced by default.
    void SetAreaId(unsigned char enable) { areaId_ = enable; }
    /// Return area ID of geometry.
    unsigned char GetAreaId() const { return areaId_; }

    /// Return effective area ID for navigation mesh builder.
    unsigned char GetEffectiveAreaId() const { return areaId_ != DeduceAreaId || walkable_ ? areaId_ : 0; }

private:
    /// Recursive flag.
    bool recursive_{true};
    /// Walkable flag.
    bool walkable_{true};
    /// Area ID.
    unsigned char areaId_{DeduceAreaId};
};

} // namespace Urho3D
