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

#pragma once

#include "FiniteTimeAction.h"

namespace Urho3D
{
namespace Actions
{

/// Remove self from parent. The Target of the action should be either Node or UIElement.
class URHO3D_API RemoveSelf : public FiniteTimeAction
{
    URHO3D_OBJECT(RemoveSelf, FiniteTimeAction)
public:
    /// Construct.
    explicit RemoveSelf(Context* context);

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// Show target. The Target should have "Is Visible" attribute.
class URHO3D_API Show : public FiniteTimeAction
{
    URHO3D_OBJECT(Show, FiniteTimeAction)
public:
    /// Construct.
    explicit Show(Context* context);

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// Hide target. The Target should have "Is Visible" attribute.
class URHO3D_API Hide : public FiniteTimeAction
{
    URHO3D_OBJECT(Hide, FiniteTimeAction)
public:
    /// Construct.
    explicit Hide(Context* context);

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};


/// Show target. The Target should have "Is Enabled" attribute.
class URHO3D_API Enable : public FiniteTimeAction
{
    URHO3D_OBJECT(Enable, FiniteTimeAction)
public:
    /// Construct.
    explicit Enable(Context* context);

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// Hide target. The Target should have "Is Enabled" attribute.
class URHO3D_API Disable : public FiniteTimeAction
{
    URHO3D_OBJECT(Disable, FiniteTimeAction)
public:
    /// Construct.
    explicit Disable(Context* context);

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// Blink target by toggling "Is Enabled" attribute. The Target should have "Is Enabled" attribute.
class URHO3D_API Blink : public FiniteTimeAction
{
    URHO3D_OBJECT(Blink, FiniteTimeAction)
public:

    /// Construct.
    explicit Blink(Context* context);

    /// Get number of blinks.
    unsigned GetNumOfBlinks() const { return times_; };
    /// Set number of blinks.
    void SetNumOfBlinks(unsigned times) { times_ = Max(1, times); };

    /// Get target attibute name.
    const ea::string& GetAttributeName() const { return attribute_; };
    /// Set target attibute name.
    void SetAttributeName(ea::string_view attribute) { attribute_ = attribute; };

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    unsigned times_{1};
    ea::string attribute_{"Is Enabled"};
};


/// Action that does nothing but waits.
class URHO3D_API DelayTime : public FiniteTimeAction
{
    URHO3D_OBJECT(DelayTime, FiniteTimeAction)
public:
    /// Construct.
    explicit DelayTime(Context* context);

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

} // namespace Actions
} // namespace Urho3D
