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

/// \file

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

} // namespace Urho3D
