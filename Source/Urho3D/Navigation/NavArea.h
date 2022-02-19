//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Math/BoundingBox.h"
#include "../Scene/Component.h"

namespace Urho3D
{

class URHO3D_API NavArea : public Component
{
    URHO3D_OBJECT(NavArea, Component);

public:
    /// Construct.
    explicit NavArea(Context* context);
    /// Destruct.
    ~NavArea() override;
    /// Register object factory and attributes.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Render debug geometry for the bounds.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Get the area id for this volume.
    /// @property
    unsigned GetAreaID() const { return (unsigned)areaID_; }

    /// Set the area id for this volume.
    /// @property
    void SetAreaID(unsigned newID);

    /// Get the bounding box of this navigation area, in local space.
    /// @property
    BoundingBox GetBoundingBox() const { return boundingBox_; }

    /// Set the bounding box of this area, in local space.
    /// @property
    void SetBoundingBox(const BoundingBox& bnds) { boundingBox_ = bnds; }

    /// Get the bounds of this navigation area in world space.
    /// @property
    BoundingBox GetWorldBoundingBox() const;

private:
    /// Bounds of area to mark.
    BoundingBox boundingBox_;
    /// Area id to assign to the marked area.
    unsigned char areaID_;
};

}
