//
// Copyright (c) 2017-2020 the rbfx project.
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
