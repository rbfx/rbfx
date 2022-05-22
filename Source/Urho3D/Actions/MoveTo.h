//
// Copyright (c) 2022 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "FiniteTimeAction.h"

namespace Urho3D
{
/// Move to 3D postion action. Target should have attribute "Position" of type Vector3 or IntVector3.
class URHO3D_API MoveTo : public FiniteTimeAction
{
    URHO3D_OBJECT(MoveTo, FiniteTimeAction)

public:
    /// Construct.
    explicit MoveTo(Context* context);
    /// Construct.
    explicit MoveTo(Context* context, float duration, const Vector3& position);
    /// Destruct.
    ~MoveTo() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Get position delta.
    const Vector3& GetPosition() const { return position_; }

protected:
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Vector3 position_{Vector3::ZERO};
};

} // namespace Urho3D
