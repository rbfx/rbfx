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
namespace Actions
{
/// Animate shader parameter.
class URHO3D_API ShaderParameterAction : public FiniteTimeAction
{
    URHO3D_OBJECT(ShaderParameterAction, FiniteTimeAction)

public:
    /// Construct.
    explicit ShaderParameterAction(Context* context);

        // Get shader parameter name.
    void SetName(ea::string_view parameter);

        // Get shader parameter name.
    const ea::string& GetName() const { return name_; }

        /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

private:
    ea::string name_;
};


/// Animate shader parameter from current value to another.
class URHO3D_API ShaderParameterTo : public ShaderParameterAction
{
    URHO3D_OBJECT(ShaderParameterTo, ShaderParameterAction)

public:
    /// Construct.
    explicit ShaderParameterTo(Context* context);

    // Get "to" value.
    void SetTo(const Variant&);

    // Get "to" value.
    const Variant& GetTo() const { return to_; }

protected:
    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Variant to_;
};

/// Animate shader parameter from one value to another.
class URHO3D_API ShaderParameterFromTo : public ShaderParameterTo
{
    URHO3D_OBJECT(ShaderParameterFromTo, ShaderParameterTo)

public:
    /// Construct.
    explicit ShaderParameterFromTo(Context* context);

    // Set "from" value.
    void SetFrom(const Variant&);

    // Get "from" value.
    const Variant& GetFrom() const { return from_; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

protected:
    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Variant from_;
};

} // namespace Actions
} // namespace Urho3D
