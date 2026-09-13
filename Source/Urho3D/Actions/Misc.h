// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "AttributeAction.h"

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
class URHO3D_API Show : public AttributeAction
{
    URHO3D_OBJECT(Show, AttributeAction)
public:
    /// Construct.
    explicit Show(Context* context);

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// Hide target. The Target should have "Is Visible" attribute.
class URHO3D_API Hide : public AttributeAction
{
    URHO3D_OBJECT(Hide, AttributeAction)
public:
    /// Construct.
    explicit Hide(Context* context);

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};


/// Show target. The Target should have "Is Enabled" attribute.
class URHO3D_API Enable : public AttributeAction
{
    URHO3D_OBJECT(Enable, AttributeAction)
public:
    /// Construct.
    explicit Enable(Context* context);

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// Hide target. The Target should have "Is Enabled" attribute.
class URHO3D_API Disable : public AttributeAction
{
    URHO3D_OBJECT(Disable, AttributeAction)
public:
    /// Construct.
    explicit Disable(Context* context);

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// Blink target by toggling "Is Enabled" attribute. The Target should have "Is Enabled" attribute.
class URHO3D_API Blink : public AttributeAction
{
    URHO3D_OBJECT(Blink, AttributeAction)
public:

    /// Construct.
    explicit Blink(Context* context);

    /// Get number of blinks.
    unsigned GetNumOfBlinks() const { return times_; };
    /// Set number of blinks.
    void SetNumOfBlinks(unsigned times) { times_ = Max(1, times); };

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    unsigned times_{1};
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
