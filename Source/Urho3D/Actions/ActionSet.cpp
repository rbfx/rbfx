// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Actions/ActionSet.h"

#include "Urho3D/Actions/ActionManager.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Deserializer.h"

namespace Urho3D
{
using namespace Actions;

ActionSet::ActionSet(Context* context)
    : Resource(context)
{
    SetDefaultAction(nullptr);
}

void ActionSet::RegisterObject(Context* context) { context->RegisterFactory<ActionSet>(); }

bool ActionSet::BeginLoad(Deserializer& source)
{
    ea::string extension = GetExtension(source.GetName());

    defaultAction_.Reset();

    const auto xmlFile = MakeShared<XMLFile>(context_);
    if (!xmlFile->Load(source))
        return false;

    return xmlFile->LoadObject("actions", *this);
}

/// Set action
void ActionSet::SetDefaultAction(BaseAction* action)
{
    defaultAction_ = (action) ? action : (BaseAction*)context_->GetSubsystem<Urho3D::ActionManager>()->GetEmptyAction();
}


bool ActionSet::Save(Serializer& dest) const
{
    const auto xmlFile = MakeShared<XMLFile>(context_);
    xmlFile->SaveObject("actions", *this);
    xmlFile->Save(dest);
    return true;
}

void ActionSet::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "default", defaultAction_);
}

} // namespace Urho3D
