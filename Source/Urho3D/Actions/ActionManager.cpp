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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "ActionManager.h"

#include "Action.h"
#include "ActionState.h"
#include "Attribute.h"
#include "Move.h"
#include "Ease.h"
#include "ShaderParameterFromTo.h"
#include "../IO/Log.h"
#include "../Core/CoreEvents.h"

namespace Urho3D
{

ActionManager::ActionManager(Context* context)
    : ActionManager(context, true)
{
}

ActionManager::ActionManager(Context* context, bool autoupdate)
    : Object(context)
    , ObjectReflectionRegistry(context)
{
    RegisterActionLibrary(context, this);
    if (autoupdate)
    {
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ActionManager, HandleUpdate));
    }
}

ActionManager::~ActionManager()
{
    RemoveAllActions();
}

void RegisterActionLibrary(Context* context, ActionManager* manager)
{
    if (!context->GetObjectReflections().contains(Action::GetTypeStatic()))
    {
        Action::RegisterObject(context);
    }

    manager->AddFactoryReflection<BaseAction>();
    manager->AddFactoryReflection<FiniteTimeAction>();
    manager->AddFactoryReflection<MoveBy>();
    manager->AddFactoryReflection<MoveBy2D>();
    manager->AddFactoryReflection<AttributeFromTo>();
    manager->AddFactoryReflection<AttributeTo>();
    manager->AddFactoryReflection<ShaderParameterFromTo>();
    manager->AddFactoryReflection<EaseBackIn>();
}

void ActionManager::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Then execute user-defined update function
    Update(eventData[P_TIMESTEP].GetFloat());
}

void ActionManager::RemoveAllActions()
{
    if (targets_.empty())
        return;

    auto count = targets_.size();

    tmpKeysArray_.clear();
    tmpKeysArray_.reserve(targets_.size() * 2);

    for (auto target:targets_)
    {
        tmpKeysArray_.push_back(target.first);
    }

    for (auto target: tmpKeysArray_)
    {
        RemoveAllActionsFromTarget(target);
    }
}

void ActionManager::RemoveAllActionsFromTarget(Object* target)
{
    if (target == nullptr)
        return;

    auto targetIt = targets_.find(target);
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

void ActionManager::RemoveAction(ActionState* actionState)
{
    if (!actionState || !actionState->GetOriginalTarget())
        return;

    auto target = actionState->GetOriginalTarget();

    auto targetIt = targets_.find(target);
    if (targetIt != targets_.end())
    {
        HashElement& element = targetIt->second;
        ea::erase_if(element.ActionStates, [&](const SharedPtr<ActionState>& state) { return state.Get() == actionState; });
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
        element->Paused = paused;
        element->Target = target;
    }
    else
    {
        element = &existingTargetIt->second;
    }

    auto isActionRunning = false;
    for (auto& existingState: element->ActionStates)
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
        tmpKeysArray_.reserve(count*2);
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

        currentTarget_ = element.Target;
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
                    RemoveAction(actionState);
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

} // namespace Urho3D
