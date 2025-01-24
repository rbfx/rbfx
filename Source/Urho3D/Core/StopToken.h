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
