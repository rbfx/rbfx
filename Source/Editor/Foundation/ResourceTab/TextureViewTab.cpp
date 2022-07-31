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
}

TextureViewTab::~TextureViewTab()
{
}

bool TextureViewTab::CanOpenResource(const ResourceFileDescriptor& desc)
{
    return desc.HasObjectType<Texture>();
}


void TextureViewTab::RenderContent()
{
    if (!texture_)
        return;

    SharedPtr<Texture2D> texture2D;
    texture2D.DynamicCast(texture_);
    if (texture2D)
    {
        Widgets::Image(texture2D, ToImGui(GetContentSize()));
        return;
    }

    CustomSceneViewTab::RenderContent();
}

void TextureViewTab::OnResourceLoaded(const ea::string& resourceName)
{
    auto cache = GetSubsystem<ResourceCache>();
    texture_ = cache->GetResource<Texture2D>(resourceName);
}

void TextureViewTab::OnResourceUnloaded(const ea::string& resourceName)
{
    texture_.Reset();
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
