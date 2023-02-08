//
// Copyright (c) 2021 the rbfx project.
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

#include "ActionSet.h"

#include "ActionManager.h"
#include "../Core/Context.h"
#include "../Resource/XMLFile.h"
#include "../IO/FileSystem.h"
#include "../IO/Deserializer.h"

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
