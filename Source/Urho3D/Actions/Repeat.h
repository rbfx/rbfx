// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Actions/FiniteTimeAction.h"
#include <EASTL/array.h>

namespace Urho3D
{
namespace Actions
{

/// Repeat inner action several times.
class URHO3D_API Repeat : public FiniteTimeAction
{
    URHO3D_OBJECT(Repeat, FiniteTimeAction)
public:
    /// Construct.
    explicit Repeat(Context* context);

    /// Get action duration.
    float GetDuration() const override;

    /// Set inner action.
    void SetInnerAction(FiniteTimeAction* action);

    /// Get inner action.
    FiniteTimeAction* GetInnerAction() const { return innerAction_.Get(); }

    /// Set number of repetitions.
    void SetTimes(unsigned times);

    /// Get number of repetitions.
    unsigned GetTimes() const { return times_; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    unsigned times_;
    SharedPtr<FiniteTimeAction> innerAction_;
};

/// Repeat inner action forever.
class URHO3D_API RepeatForever : public FiniteTimeAction
{
    URHO3D_OBJECT(RepeatForever, FiniteTimeAction)
public:
    /// Construct.
    explicit RepeatForever(Context* context);

    /// Get action duration.
    float GetDuration() const override;

    /// Set inner action.
    void SetInnerAction(FiniteTimeAction* action);

    /// Get inner action.
    FiniteTimeAction* GetInnerAction() const { return innerAction_.Get(); }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    SharedPtr<FiniteTimeAction> innerAction_;
};

} // namespace Actions
} // namespace Urho3D
