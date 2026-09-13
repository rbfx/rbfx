// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Scene/Serializable.h"

namespace Urho3D
{
class ActionManager;

namespace Actions
{

class ActionState;

/// Base action state.
class URHO3D_API BaseAction : public Serializable
{
    URHO3D_OBJECT(BaseAction, Serializable)

public:
    /// Construct.
    BaseAction(Context* context);
    /// Destruct.
    ~BaseAction() override;

    /// Get action from argument or empty action.
    BaseAction* GetOrDefault(BaseAction* action) const;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    virtual SharedPtr<ActionState> StartAction(Object* target);

    friend class Urho3D::ActionManager;
    friend class ActionState;
};

} // namespace Actions

void SerializeValue(Archive& archive, const char* name, SharedPtr<Actions::BaseAction>& value);

} // namespace Urho3D
