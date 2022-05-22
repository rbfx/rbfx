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
#include "../Scene/Node.h"

namespace Urho3D
{
/// Action manager.
class URHO3D_API MoveBy : public FiniteTimeAction
{
    URHO3D_OBJECT(MoveBy, FiniteTimeAction)

public:
    /// Construct.
    explicit MoveBy(Context* context);
    /// Construct.
    explicit MoveBy(Context* context, float duration, const Vector3& position);

    /// Destruct.
    ~MoveBy() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    const Vector3& GetPositionDelta() const { return position_; }

    FiniteTimeAction* Reverse() const;

protected:
    virtual SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Vector3 position_{Vector3::ZERO};
};


} // namespace Urho3D
