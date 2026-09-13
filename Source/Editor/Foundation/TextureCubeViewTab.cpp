// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Foundation/TextureCubeViewTab.h"

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

void Foundation_TextureCubeViewTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<TextureCubeViewTab>(context));
}

TextureCubeViewTab::TextureCubeViewTab(Context* context)
    : CustomSceneViewTab(context, "Cubemap", "d66bcf6d-9fe3-4e7c-a519-4b1ad5a0f89c",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault,
        EditorTabPlacement::DockCenter)
{
}

TextureCubeViewTab::~TextureCubeViewTab()
{
}

bool TextureCubeViewTab::CanOpenResource(const ResourceFileDescriptor& desc)
{
    return desc.HasObjectType<TextureCube>();
}

void TextureCubeViewTab::RenderTextureCube(TextureCube* texture)
{
    CustomSceneViewTab::RenderContent();
}

void TextureCubeViewTab::RenderContent()
{
    if (textureCube_)
    {
        BaseClassName::RenderContent();
    }

}

void TextureCubeViewTab::OnResourceLoaded(const ea::string& resourceName)
{
    auto cache = GetSubsystem<ResourceCache>();
    textureCube_ = cache->GetResource<TextureCube>(resourceName);
    preview_->SetSkyboxTexture(textureCube_);
}

void TextureCubeViewTab::OnResourceUnloaded(const ea::string& resourceName)
{
    textureCube_.Reset();
}

void TextureCubeViewTab::OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName)
{
}

void TextureCubeViewTab::OnResourceSaved(const ea::string& resourceName)
{
}

void TextureCubeViewTab::OnResourceShallowSaved(const ea::string& resourceName)
{
}

} // namespace Urho3D
