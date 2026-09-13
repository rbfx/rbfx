// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/InspectorTab/EmptyInspector.h"

namespace Urho3D
{

void Foundation_EmptyInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<EmptyInspector>(inspectorTab->GetProject());
}

EmptyInspector::EmptyInspector(Project* project)
    : Object(project->GetContext())
{
    project->OnRequest.Subscribe(this, &EmptyInspector::OnProjectRequest);
}

void EmptyInspector::OnProjectRequest(ProjectRequest* request)
{
    auto inspectRequest = dynamic_cast<BaseInspectRequest*>(request);
    if (!inspectRequest)
        return;

    request->QueueProcessCallback([=]()
    {
        OnActivated(this);
    }, M_MIN_INT);
}

void EmptyInspector::RenderContent()
{
}

void EmptyInspector::RenderContextMenuItems()
{
}

void EmptyInspector::RenderMenu()
{
}

void EmptyInspector::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

}
