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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "ActionManager.h"

#include "../Core/CoreEvents.h"
#include "../IO/Log.h"
#include "ActionSet.h"
#include "ActionState.h"
#include "Attribute.h"
#include "Ease.h"
#include "FiniteTimeActionState.h"
#include "Move.h"
#include "Parallel.h"
#include "Repeat.h"
#include "Sequence.h"
#include "ShaderParameter.h"

namespace Urho3D
{
using namespace Urho3D::Actions;

namespace
{
struct EmptyState : public FiniteTimeActionState
{
    EmptyState(FiniteTimeAction* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
    }

    /// Gets a value indicating whether this instance is done.
    bool IsDone() const override { return true; }

    /// Called every frame with it's delta time.
    void Step(float dt) override {}
};

class EmptyAction : public FiniteTimeAction
{
    URHO3D_OBJECT(EmptyAction, FiniteTimeAction)

public:
    EmptyAction(Context* context)
        : FiniteTimeAction(context)
    {
        state_ = MakeShared<EmptyState>(this, nullptr);
    }

    /// Get action duration.
    float GetDuration() const override { return ea::numeric_limits<float>::epsilon(); }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override
    {
        // Empty action is immutable so we can reuse it.
        return SharedPtr<FiniteTimeAction>(const_cast<EmptyAction*>(this));
    }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override { return state_; }

    SharedPtr<EmptyState> state_;
};
} // namespace

ActionManager::ActionManager(Context* context)
    : ActionManager(context, true)
{
}

ActionManager::ActionManager(Context* context, bool autoupdate)
    : Object(context)
    , ObjectReflectionRegistry(context)
    , emptyAction_(MakeShared<EmptyAction>(context))
{
    RegisterActionLibrary(context, this);
    if (autoupdate)
    {
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ActionManager, HandleUpdate));
    }
}

ActionManager::~ActionManager() { CancelAllActions(); }

void RegisterActionLibrary(Context* context, ActionManager* manager)
{
    if (!context->GetObjectReflections().contains(ActionSet::GetTypeStatic()))
    {
        ActionSet::RegisterObject(context);
    }

    manager->AddFactoryReflection<EmptyAction>();
    manager->AddFactoryReflection<BaseAction>();
    manager->AddFactoryReflection<FiniteTimeAction>();
    manager->AddFactoryReflection<MoveBy>();
    manager->AddFactoryReflection<JumpBy>();
    manager->AddFactoryReflection<RotateBy>();
    manager->AddFactoryReflection<RotateAround>();
    manager->AddFactoryReflection<AttributeFromTo>();
    manager->AddFactoryReflection<AttributeTo>();
    manager->AddFactoryReflection<AttributeBlink>();
    manager->AddFactoryReflection<ShaderParameterFromTo>();
    manager->AddFactoryReflection<EaseBackIn>();
    manager->AddFactoryReflection<EaseBackInOut>();
    manager->AddFactoryReflection<EaseBackOut>();
    manager->AddFactoryReflection<EaseElasticIn>();
    manager->AddFactoryReflection<EaseElasticInOut>();
    manager->AddFactoryReflection<EaseElasticOut>();
    manager->AddFactoryReflection<EaseBounceIn>();
    manager->AddFactoryReflection<EaseBounceInOut>();
    manager->AddFactoryReflection<EaseBounceOut>();
    manager->AddFactoryReflection<EaseSineIn>();
    manager->AddFactoryReflection<EaseSineInOut>();
    manager->AddFactoryReflection<EaseSineOut>();
    manager->AddFactoryReflection<EaseExponentialIn>();
    manager->AddFactoryReflection<EaseExponentialInOut>();
    manager->AddFactoryReflection<EaseExponentialOut>();
    manager->AddFactoryReflection<Sequence>();
    manager->AddFactoryReflection<Parallel>();
    manager->AddFactoryReflection<Repeat>();
    manager->AddFactoryReflection<RepeatForever>();
}

void ActionManager::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Then execute user-defined update function
    Update(eventData[P_TIMESTEP].GetFloat());
}

void ActionManager::CompleteAllActions()
{
    if (targets_.empty())
        return;

    auto count = targets_.size();

    tmpKeysArray_.clear();
    tmpKeysArray_.reserve(targets_.size() * 2);

    for (auto target : targets_)
    {
        tmpKeysArray_.push_back(target.first);
    }

    for (auto target : tmpKeysArray_)
    {
        CompleteAllActionsOnTarget(target);
    }
}

void ActionManager::CancelAllActions()
{
    if (targets_.empty())
        return;

    auto count = targets_.size();

    tmpKeysArray_.clear();
    tmpKeysArray_.reserve(targets_.size() * 2);

    for (auto target : targets_)
    {
        tmpKeysArray_.push_back(target.first);
    }

    for (auto target : tmpKeysArray_)
    {
        CancelAllActionsFromTarget(target);
    }
}

void ActionManager::CancelAllActionsFromTarget(Object* target)
{
    if (target == nullptr)
        return;

    const auto targetIt = targets_.find(target);
    if (targetIt != targets_.end())
    {
        HashElement& element = targetIt->second;

        if (element.ActionStates.contains(element.CurrentActionState) && !element.CurrentActionSalvaged)
            element.CurrentActionSalvaged = true;

        element.ActionStates.clear();
        if (currentTarget_ == target)
        {
            currentTargetSalvaged_ = true;
        }
        else
        {
            targets_.erase(targetIt);
        }
    }
}

void ActionManager::CompleteAllActionsOnTarget(Object* target)
{
    if (target == nullptr)
        return;

    const auto targetIt = targets_.find(target);
    if (targetIt != targets_.end())
    {
        HashElement& element = targetIt->second;

        if (element.ActionStates.contains(element.CurrentActionState) && !element.CurrentActionSalvaged)
            element.CurrentActionSalvaged = true;

        for (const auto& action: element.ActionStates)
        {
            // Do a zero step to be sure that the action is initialized.
            action->Step(0.0f);
            float duration = ea::numeric_limits<float>::max();
            const auto finiteTimeAction = dynamic_cast<FiniteTimeAction*>(action->GetAction());
            if (finiteTimeAction)
            {
                duration = finiteTimeAction->GetDuration();
                if (duration >= ea::numeric_limits<float>::max())
                {
                    duration = 0.0f;
                }
            }
            // Do a step beyond duration to complete the action.
            action->Step(duration * 2.0f);
        }
        element.ActionStates.clear();
        if (currentTarget_ == target)
        {
            currentTargetSalvaged_ = true;
        }
        else
        {
            targets_.erase(targetIt);
        }
    }
}
void ActionManager::CancelAction(ActionState* actionState)
{
    if (!actionState || !actionState->GetOriginalTarget())
        return;

    auto target = actionState->GetOriginalTarget();

    auto targetIt = targets_.find(target);
    if (targetIt != targets_.end())
    {
        HashElement& element = targetIt->second;
        ea::erase_if(
            element.ActionStates, [&](const SharedPtr<ActionState>& state) { return state.Get() == actionState; });
    }
}

unsigned ActionManager::GetNumActions(Object* target) const
{
    if (!target)
        return 0;
    const auto existingTargetIt = targets_.find(target);
    if (existingTargetIt == targets_.end())
        return 0;
    return existingTargetIt->second.ActionStates.size();
}

ActionState* ActionManager::AddAction(BaseAction* action, Object* target, bool paused)
{
    if (action == nullptr)
    {
        URHO3D_LOGERROR("Action parameter is nullptr");
        return nullptr;
    }
    if (target == nullptr)
    {
        URHO3D_LOGERROR("Target parameter is nullptr");
        return nullptr;
    }

    const auto existingTargetIt = targets_.find(target);
    HashElement* element = nullptr;
    // Check if element is missing or if we have a record with conflicting pointer (happens in unit tests).
    if (existingTargetIt == targets_.end() || existingTargetIt->second.Target.Expired())
    {
        element = &targets_[target];
        // Hack to handle expired target with reused pointer.
        element->ActionStates.clear();
        element->Paused = paused;
        element->Target = target;
    }
    else
    {
        element = &existingTargetIt->second;
    }

    auto isActionRunning = false;
    for (auto& existingState : element->ActionStates)
    {
        if (existingState->GetAction() == action)
        {
            URHO3D_LOGERROR("Action is already running for this target.");
            return nullptr;
        }
    }
    auto state = action->StartAction(target);
    element->ActionStates.push_back(state);

    return state;
}

void ActionManager::Update(float dt)
{
    if (targets_.empty())
        return;

    const auto count = targets_.size();

    tmpKeysArray_.clear();
    if (tmpKeysArray_.capacity() < count)
        tmpKeysArray_.reserve(count * 2);
    for (auto target : targets_)
    {
        tmpKeysArray_.push_back(target.first);
    }

    for (unsigned i = 0; i < count; i++)
    {
        auto elementlt = targets_.find(tmpKeysArray_[i]);
        if (elementlt == targets_.end())
            continue;

        auto& element = elementlt->second;
        if (element.Target.Expired())
        {
            targets_.erase(elementlt);
            continue;
        }

        currentTarget_ = element.Target.Get();
        currentTargetSalvaged_ = false;

        if (!element.Paused)
        {
            // The 'actions' may change while inside this loop.
            for (element.ActionIndex = 0; element.ActionIndex < element.ActionStates.size(); element.ActionIndex++)
            {
                element.CurrentActionState = element.ActionStates[element.ActionIndex];
                if (!element.CurrentActionState)
                    continue;
                if (element.Target.Expired())
                    break;

                element.CurrentActionSalvaged = false;

                element.CurrentActionState->Step(dt);

                if (element.CurrentActionSalvaged)
                {
                    // The currentAction told the node to remove it. To prevent the action from
                    // accidentally deallocating itself before finishing its step, we retained
                    // it. Now that step is done, it's safe to release it.

                    // currentTarget->currentAction->release();
                }
                else if (element.CurrentActionState->IsDone())
                {
                    element.CurrentActionState->Stop();

                    auto actionState = element.CurrentActionState;
                    // Make currentAction nil to prevent removeAction from salvaging it.
                    element.CurrentActionState.Reset();
                    CancelAction(actionState);
                }

                element.CurrentActionState.Reset();
            }
        }

        // only delete currentTarget if no actions were scheduled during the cycle (issue #481)
        if (currentTargetSalvaged_ && element.ActionStates.empty())
            targets_.erase(elementlt);
    }

    currentTarget_ = nullptr;
}

FiniteTimeAction* ActionManager::GetEmptyAction() { return emptyAction_; }

void SerializeValue(Archive& archive, const char* name, SharedPtr<BaseAction>& value)
{
    const bool loading = archive.IsInput();
    ArchiveBlock block = archive.OpenUnorderedBlock(name);

    StringHash type{};
    ea::string_view typeName{};
    if (!loading && value)
    {
        type = value->GetType();
        typeName = value->GetTypeName();
    }

    SerializeStringHash(archive, "type", type, typeName);

    if (loading)
    {
        // Serialize null object
        if (type == StringHash{} || type == EmptyAction::GetTypeStatic())
        {
            value = archive.GetContext()->GetSubsystem<ActionManager>()->GetEmptyAction();
            return;
        }

        // Create instance
        Context* context = archive.GetContext();
        ActionManager* actionManager = context->GetSubsystem<ActionManager>();
        value.StaticCast(actionManager->CreateObject(type));
        if (!value)
        {
            throw ArchiveException("Failed to create action '{}/{}' of type {}", archive.GetCurrentBlockPath(), name,
                type.ToDebugString());
        }
        if (archive.HasElementOrBlock("args"))
        {
            SerializeValue(archive, "args", *value);
        }
    }
    else if (value && value->GetType() != EmptyAction::GetTypeStatic())
    {
        SerializeValue(archive, "args", *value);
    }
}

void SerializeValue(Archive& archive, const char* name, SharedPtr<FiniteTimeAction>& value)
{
    SharedPtr<BaseAction> baseAction = value;
    SerializeValue(archive, name, baseAction);
    if (archive.IsInput())
    {
        value.DynamicCast(baseAction);
    }
}


} // namespace Urho3D
