// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Misc.h"

#include "../Core/Context.h"
#include "../IO/ArchiveSerializationBasic.h"
#include "../IO/Log.h"
#include "../Scene/Node.h"
#include "../UI/UIElement.h"
#include "AttributeActionState.h"
#include "FiniteTimeActionState.h"

namespace Urho3D
{
namespace Actions
{

namespace
{
class RemoveSelfState : public FiniteTimeActionState
{
public:
    RemoveSelfState(RemoveSelf* action, Object* target)
        : FiniteTimeActionState(action, target)
    {
    }

    void Update(float time) override
    {
        const auto target = GetTarget();
        if (!target)
        {
            return;
        }
        if (Node* node = target->Cast<Node>())
        {
            node->Remove();
        }
        else if (UIElement* element = target->Cast<UIElement>())
        {
            element->Remove();
        }
    }
};

} // namespace

/// Construct.
RemoveSelf::RemoveSelf(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> RemoveSelf::StartAction(Object* target) { return MakeShared<RemoveSelfState>(this, target); }

/// Construct.
Hide::Hide(Context* context)
    : BaseClassName(context, ISVISIBLE_ATTRIBUTE)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> Hide::StartAction(Object* target)
{
    return MakeShared<SetAttributeState>(this, target, GetAttribute(target), false);
}

/// Construct.
Show::Show(Context* context)
    : BaseClassName(context, ISVISIBLE_ATTRIBUTE)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> Show::StartAction(Object* target)
{
    return MakeShared<SetAttributeState>(this, target, GetAttribute(target), true);
}

/// Construct.
Disable::Disable(Context* context)
    : BaseClassName(context, ISENABLED_ATTRIBUTE)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> Disable::StartAction(Object* target)
{
    return MakeShared<SetAttributeState>(this, target, GetAttribute(target), false);
}

/// Construct.
Enable::Enable(Context* context)
    : BaseClassName(context, ISENABLED_ATTRIBUTE)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> Enable::StartAction(Object* target)
{
    return MakeShared<SetAttributeState>(this, target, GetAttribute(target), true);
}

/// Construct.
Blink::Blink(Context* context)
    : BaseClassName(context, ISENABLED_ATTRIBUTE)
{
}

/// Serialize content from/to archive. May throw ArchiveException.
void Blink::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "times", times_, 1u);
}

/// Create new action state from the action.
SharedPtr<ActionState> Blink::StartAction(Object* target) { return MakeShared<AttributeBlinkState>(this, target, GetAttribute(target), false, true, times_); }


/// Construct.
DelayTime::DelayTime(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> DelayTime::StartAction(Object* target)
{
    return MakeShared<FiniteTimeActionState>(this, target);
}

} // namespace Actions
} // namespace Urho3D
