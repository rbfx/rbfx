// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Actions/BaseAction.h"

namespace Urho3D
{
namespace Actions
{
/// Finite time action.
class URHO3D_API FiniteTimeAction : public BaseAction
{
    URHO3D_OBJECT(FiniteTimeAction, BaseAction)

public:
    /// Construct.
    FiniteTimeAction(Context* context);

    /// Set action duration.
    void SetDuration(float duration);
    /// Get action duration.
    virtual float GetDuration() const;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Get action from argument or empty action.
    FiniteTimeAction* GetOrDefault(FiniteTimeAction* action) const;

    /// Create reversed action.
    virtual SharedPtr<FiniteTimeAction> Reverse() const;

private:
    float duration_{ea::numeric_limits<float>::epsilon()};
};

} // namespace Actions

void SerializeValue(Archive& archive, const char* name, SharedPtr<Actions::FiniteTimeAction>& value);

} // namespace Urho3D
