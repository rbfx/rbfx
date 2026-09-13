// Copyright (c) 2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "AttributeAction.h"

namespace Urho3D
{
namespace Actions
{
/// Animate attribute between two values.
class URHO3D_API AttributeFromTo : public AttributeAction
{
    URHO3D_OBJECT(AttributeFromTo, AttributeAction)

public:
    /// Construct.
    explicit AttributeFromTo(Context* context);

    // Set "from" value.
    void SetFrom(const Variant&);
    // Get "to" value.
    void SetTo(const Variant&);

    // Get "from" value.
    const Variant& GetFrom() const { return from_; }
    // Get "to" value.
    const Variant& GetTo() const { return to_; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Variant from_;
    Variant to_;
};

/// Animate attribute between current and provided value.
class URHO3D_API AttributeTo : public AttributeAction
{
    URHO3D_OBJECT(AttributeTo, AttributeAction)

public:
    /// Construct.
    explicit AttributeTo(Context* context);

    // Get "to" value.
    void SetTo(const Variant&);

    // Get "to" value.
    const Variant& GetTo() const { return to_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Variant to_;
};

/// Animate attribute between two values.
class URHO3D_API AttributeBlink : public AttributeAction
{
    URHO3D_OBJECT(AttributeBlink, AttributeAction)

public:
    /// Construct.
    explicit AttributeBlink(Context* context);

    // Set "from" value.
    void SetFrom(const Variant&);
    // Get "to" value.
    void SetTo(const Variant&);
    /// Set number of blinks.
    void SetNumOfBlinks(unsigned times) { times_ = Max(1, times); };

    // Get "from" value.
    const Variant& GetFrom() const { return from_; }
    // Get "to" value.
    const Variant& GetTo() const { return to_; }
    /// Get number of blinks.
    unsigned GetNumOfBlinks() const { return times_; };

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Variant from_;
    Variant to_;
    unsigned times_{1};
};

} // namespace Actions
} // namespace Urho3D
