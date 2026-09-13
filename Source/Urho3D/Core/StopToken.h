// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <EASTL/shared_ptr.h>

#include <atomic>

namespace Urho3D
{

/// Stop token used to thread-safely stop asynchronous task. This object can be passed
/// by value and all copies will share same internal state..
/// TODO: For better memory management split this class into StopSource with shared_ptr
/// and StopToken with weak_ptr, or reuse corresponding classes from C++20 standard library.
class StopToken
{
public:
    /// Construct default.
    StopToken() : stopped_(ea::make_shared<std::atomic<bool>>(false)) {}

    /// Signal stop.
    void Stop() { *stopped_ = true; }

    /// Check whether is stopped.
    bool IsStopped() const { return *stopped_; }

private:
    /// Whether the token is stopped.
    ea::shared_ptr<std::atomic<bool>> stopped_;
};

}
