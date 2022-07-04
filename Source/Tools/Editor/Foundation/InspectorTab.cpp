//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Foundation/InspectorTab.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

void Foundation_InspectorTab(Context* context, ProjectEditor* projectEditor)
{
    projectEditor->AddTab(MakeShared<InspectorTab_>(context));
}

InspectorAddon::InspectorAddon(InspectorTab_* owner)
    : Object(owner->GetContext())
    , owner_(owner)
{
}

InspectorAddon::~InspectorAddon()
{
}

void InspectorAddon::Activate()
{
    if (owner_)
        owner_->ConnectToSource(this, this);
}

InspectorTab_::InspectorTab_(Context* context)
    : EditorTab(context, "Inspector", "bd959865-8929-4f92-a20f-97ff867d6ba6",
        EditorTabFlag::OpenByDefault, EditorTabPlacement::DockRight)
{
}

void InspectorTab_::RegisterAddon(const SharedPtr<InspectorAddon>& addon)
{
    addons_.push_back(addon);
}

void InspectorTab_::ConnectToSource(Object* source, InspectorSource* sourceInterface)
{
    source_ = source;
    sourceInterface_ = sourceInterface;
}

void InspectorTab_::RenderMenu()
{
    if (source_)
        sourceInterface_->RenderMenu();
}

void InspectorTab_::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    if (source_)
        sourceInterface_->ApplyHotkeys(hotkeyManager);
}

void InspectorTab_::RenderContent()
{
    if (source_)
        sourceInterface_->RenderContent();
}

void InspectorTab_::RenderContextMenuItems()
{
    if (source_)
        sourceInterface_->RenderContextMenuItems();
}

}
