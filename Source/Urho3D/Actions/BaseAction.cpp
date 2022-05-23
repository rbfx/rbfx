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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "BaseAction.h"

#include "ActionManager.h"
#include "ActionState.h"

#include "../Core/Context.h"
#include "Urho3D/IO/Archive.h"
#include "Urho3D/IO/ArchiveSerializationBasic.h"

using namespace Urho3D;

namespace 
{
struct NoActionState: public ActionState
{
    NoActionState(BaseAction* action, Object* target)
        : ActionState(action, target)
    {
    }
};

}
/// Construct.
BaseAction::BaseAction(Context* context)
    : Serializable(context)
{
}

/// Destruct.
BaseAction::~BaseAction() {}

/// Serialize content from/to archive. May throw ArchiveException.
void BaseAction::SerializeInBlock(Archive& archive) {
    
}

/// Create new action state from the action.
SharedPtr<ActionState> BaseAction::StartAction(Object* target)
{
    return MakeShared<NoActionState>(this, target);
}

void Urho3D::SerializeValue(Archive& archive, const char* name, SharedPtr<BaseAction>& value)
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
        if (type == StringHash{})
        {
            value = nullptr;
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
    else if (value)
    {
        SerializeValue(archive, "args", *value);
    }
}
