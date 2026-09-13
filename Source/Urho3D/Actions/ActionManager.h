// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Context.h"
#include "../Core/Object.h"

namespace Urho3D
{
namespace Actions
{
class ActionState;
class BaseAction;
class FiniteTimeAction;
} // namespace Actions

/// Action manager.
class URHO3D_API ActionManager
    : public Object
    , public ObjectReflectionRegistry
{
    URHO3D_OBJECT(ActionManager, Object)
private:
    struct HashElement
    {
        int ActionIndex {};
        ea::vector<SharedPtr<Actions::ActionState>> ActionStates;
        SharedPtr<Actions::ActionState> CurrentActionState;
        bool CurrentActionSalvaged {};
        bool Paused {};
        WeakPtr<Object> Target;
    };

public:
    ActionManager(Context* context);

    ActionManager(Context* context, bool autoupdate);

    ~ActionManager() override;

    void CompleteAllActions();

    void CancelAllActions();

    void CompleteAllActionsOnTarget(Object* target);

    void CancelAllActionsFromTarget(Object* target);

    void CancelAction(Actions::ActionState* actionState);

    unsigned GetNumActions(Object* target) const;

    /// Add action to the action manager.
    Actions::ActionState* AddAction(Actions::BaseAction* action, Object* target, bool paused = false);

    void Update(float dt);

    Actions::FiniteTimeAction* GetEmptyAction();

private:
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

private:
    // Current target strong pointer to keep target alive while manager operates on actions.
    SharedPtr<Object> currentTarget_{};
    bool currentTargetSalvaged_{};
    ea::unordered_map<Object*, HashElement> targets_;
    ea::vector<Object*> tmpKeysArray_;
    SharedPtr<Actions::FiniteTimeAction> emptyAction_;
};

/// Register Particle Graph library objects.
void URHO3D_API RegisterActionLibrary(Context* context, ActionManager* manager);

} // namespace Urho3D
