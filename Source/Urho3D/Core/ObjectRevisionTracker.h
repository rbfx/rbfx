// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
