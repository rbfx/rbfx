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

#include "TextureViewTab.h"

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

void Foundation_TextureViewTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<TextureViewTab>(context));
}

TextureViewTab::TextureViewTab(Context* context)
    : CustomSceneViewTab(context, "Texture", "2a3032e6-541a-42fe-94c3-8baf96604690",
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

TextureViewTab::~TextureViewTab()
{
}

bool TextureViewTab::CanOpenResource(const ResourceFileDescriptor& desc)
{
    return desc.HasObjectType<Texture>();
}

void TextureViewTab::RenderTexture2D(Texture2D* texture)
{
    const ImVec2 basePosition = ui::GetCursorPos();

    RenderTitle();

    const ImVec2 contentPosition = ui::GetCursorPos();
    const IntVector2 minSize = IntVector2(1, 1);
    const auto contentSize = VectorMax(GetContentSize() - IntVector2(0, contentPosition.y - basePosition.y), minSize);
    const auto imageSize = VectorMax(texture->GetSize(), minSize);
    const float contentAspect = contentSize.x_ / static_cast<float>(contentSize.y_);
    const float imageAspect = imageSize.x_ / static_cast<float>(imageSize.y_);
    IntVector2 previewSize;
    if (contentAspect > imageAspect)
    {
        previewSize = IntVector2(Max(static_cast<int>(contentSize.y_ * imageAspect), 1), contentSize.y_);
    }
    else
    {
        previewSize = IntVector2(contentSize.x_, Max(static_cast<int>(contentSize.x_ / imageAspect), 1));
    }
    Widgets::Image(texture, ToImGui(previewSize));
}

void TextureViewTab::RenderTextureCube(TextureCube* texture)
{
    material_->SetTexture(TU_DIFFUSE, texture);
    CustomSceneViewTab::RenderContent();
}

void TextureViewTab::RenderContent()
{
    if (textureCube_)
    {
        RenderTextureCube(textureCube_);
        return;
    }

    if (texture2D_)
    {
        RenderTexture2D(texture2D_);
        return;
    }
}

void TextureViewTab::OnResourceLoaded(const ea::string& resourceName)
{
    auto cache = GetSubsystem<ResourceCache>();
    texture2D_ = cache->GetResource<Texture2D>(resourceName);
    textureCube_ = cache->GetResource<TextureCube>(resourceName);
}

void TextureViewTab::OnResourceUnloaded(const ea::string& resourceName)
{
    texture2D_.Reset();
    textureCube_.Reset();
}

void TextureViewTab::OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName)
{
}

void TextureViewTab::OnResourceSaved(const ea::string& resourceName)
{
}

void TextureViewTab::OnResourceShallowSaved(const ea::string& resourceName)
{
}

} // namespace Urho3D
