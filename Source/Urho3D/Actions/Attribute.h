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
/// Animate attribute between two values.
class URHO3D_API AttributeFromTo : public FiniteTimeAction
{
    URHO3D_FINITETIMEACTION(AttributeFromTo, FiniteTimeAction)

public:
    /// Construct.
    explicit AttributeFromTo(
        Context* context, float duration, const ea::string& attribute, const Variant& from, const Variant& to);

    // Get "from" value.
    const Variant& GetFrom() const { return from_; }
    // Get "to" value.
    const Variant& GetTo() const { return to_; }
    // Get attribute parameter name.
    const ea::string& GetName() const { return name_; }

private:
    ea::string name_;
    Variant from_;
    Variant to_;
};

/// Animate attribute between current and provided value.
class URHO3D_API AttributeTo : public FiniteTimeAction
{
    URHO3D_FINITETIMEACTION(AttributeTo, FiniteTimeAction)

public:
    /// Construct.
    explicit AttributeTo(Context* context, float duration, const ea::string& attribute, const Variant& to);

    // Get "to" value.
    const Variant& GetTo() const { return to_; }
    // Get attribute parameter name.
    const ea::string& GetName() const { return name_; }

private:
    ea::string name_;
    Variant to_;
};

} // namespace Urho3D
