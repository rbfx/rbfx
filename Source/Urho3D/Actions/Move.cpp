//
// Copyright (c) 2015 Xamarin Inc.
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

#include "Actions.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "AttributeActionState.h"
#include "FiniteTimeActionState.h"
#include "Urho3D/IO/ArchiveSerializationBasic.h"

namespace Urho3D
{
namespace Actions
{

/// ------------------------------------------------------------------------------
void MoveBy::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<MoveBy*>(action)->SetDelta(-delta_);
}

/// ------------------------------------------------------------------------------
void MoveByQuadratic::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<MoveByQuadratic*>(action)->SetDelta(-GetDelta());
    static_cast<MoveByQuadratic*>(action)->SetControl(-GetControl());
}

/// ------------------------------------------------------------------------------
void JumpBy::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<JumpBy*>(action)->SetDelta(-delta_);
}

/// ------------------------------------------------------------------------------
///
void ScaleBy::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<ScaleBy*>(action)->SetDelta(Vector3(1.0f / delta_.x_, 1.0f / delta_.y_, 1.0f / delta_.z_));
}

/// ------------------------------------------------------------------------------
void RotateBy::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<RotateBy*>(action)->SetDelta(delta_.Inverse());
}


/// ------------------------------------------------------------------------------

void RotateAround::ReverseImpl(FiniteTimeAction* action) const
{
    BaseClassName::ReverseImpl(action);
    static_cast<RotateAround*>(action)->SetDelta(delta_.Inverse());
    static_cast<RotateAround*>(action)->SetPivot(GetPivot());
}

} // namespace Actions
} // namespace Urho3D

