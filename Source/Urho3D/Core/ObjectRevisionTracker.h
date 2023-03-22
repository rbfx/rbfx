//
// Copyright (c) 2023-2023 the rbfx project.
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

#include <Urho3D/Urho3D.h>

namespace Urho3D
{

/// Utility to keep track of object revisions.
/// Revision is never zero, so it can be used as sentinel value to save space.
class URHO3D_API ObjectRevisionTracker
{
public:
    static constexpr unsigned InvalidRevision = 0;

    /// Return object revision.
    unsigned GetRevision() const { return revision_; }

protected:
    /// Mark object as changed.
    void MarkRevisionUpdated()
    {
        ++revision_;
        if (revision_ == InvalidRevision)
            ++revision_;
    }

private:
    /// Object revision, used for detecting changes in animation tracks by the external user.
    unsigned revision_{1};
};

}
