// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

namespace Urho3D
{

/// Helper to declare non-copyable and non-movable class.
class NonCopyable
{
protected:
    NonCopyable() = default;

    /// Disable copy, move and assignment.
    /// @{
    NonCopyable(const NonCopyable& other) = delete;
    NonCopyable(NonCopyable && other) = delete;
    NonCopyable& operator=(const NonCopyable& other) = delete;
    NonCopyable& operator=(NonCopyable && other) = delete;
    /// @}
};

/// Helper to declare non-copyable but movable class.
class MovableNonCopyable
{
protected:
    MovableNonCopyable() = default;
    MovableNonCopyable(MovableNonCopyable && other) = default;
    MovableNonCopyable& operator=(MovableNonCopyable && other) = default;

    /// Disable copy and copy-assignment.
    /// @{
    MovableNonCopyable(const MovableNonCopyable& other) = delete;
    MovableNonCopyable& operator=(const MovableNonCopyable& other) = delete;
    /// @}
};

}
