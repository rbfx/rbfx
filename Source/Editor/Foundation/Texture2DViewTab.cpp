// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Foundation/Texture2DViewTab.h"

#include "../Core/CommonEditorActions.h"
#include "../Core/IniHelpers.h"

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/Widgets.h>

namespace Urho3D
{

namespace
{
}

void Foundation_Texture2DViewTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<Texture2DViewTab>(context));
}

Texture2DViewTab::Texture2DViewTab(Context* context)
    : CustomSceneViewTab(context, "Texture", "2a3032e6-541a-42fe-94c3-8baf96604690",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault,
        EditorTabPlacement::DockCenter)
{
}

Texture2DViewTab::~Texture2DViewTab()
{
}

bool Texture2DViewTab::CanOpenResource(const ResourceFileDescriptor& desc)
{
    return desc.HasObjectType<Texture2D>();
}

void Texture2DViewTab::RenderContent()
{
    if (preview_)
    {
        RenderTitle();
        if (ui::BeginChild("content", ImVec2(0,0)))
        {
            preview_->RenderContent();
        }
        ui::EndChild();
    }
}

void Texture2DViewTab::OnResourceLoaded(const ea::string& resourceName)
{
    auto cache = GetSubsystem<ResourceCache>();
    preview_ = MakeShared<Texture2DWidget>(context_, cache->GetResource<Texture2D>(resourceName));
}

void Texture2DViewTab::OnResourceUnloaded(const ea::string& resourceName)
{
    preview_.Reset();
}

void Texture2DViewTab::OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName)
{
}

void Texture2DViewTab::OnResourceSaved(const ea::string& resourceName)
{
}

void Texture2DViewTab::OnResourceShallowSaved(const ea::string& resourceName)
{
}

} // namespace Urho3D
