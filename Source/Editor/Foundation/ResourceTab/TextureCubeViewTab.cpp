//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "TextureCubeViewTab.h"

#include "../../Core/CommonEditorActions.h"
#include "../../Core/IniHelpers.h"

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/TextureCube.h>
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
    modelNode_ = GetScene()->CreateChild("Model");
    staticModel_ = modelNode_->CreateComponent<StaticModel>();
    staticModel_->SetCastShadows(true);
    material_ = MakeShared<Material>(context_);
    auto* cache = context->GetSubsystem<ResourceCache>();
    material_->SetTechnique(0, cache->GetResource<Technique>("Techniques/UnlitOpaque.xml"));
    staticModel_->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
    staticModel_->SetMaterial(material_);
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
    material_->SetTexture(TU_DIFFUSE, texture);
    CustomSceneViewTab::RenderContent();
}

void TextureCubeViewTab::RenderContent()
{
    if (textureCube_)
    {
        RenderTextureCube(textureCube_);
        return;
    }

}

void TextureCubeViewTab::OnResourceLoaded(const ea::string& resourceName)
{
    auto cache = GetSubsystem<ResourceCache>();
    textureCube_ = cache->GetResource<TextureCube>(resourceName);
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
