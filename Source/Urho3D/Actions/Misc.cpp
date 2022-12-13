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
    SerializeOptionalValue(archive, "times", times_, 1);
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
