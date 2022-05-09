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

#include "../Foundation/SceneViewTab.h"

#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

void Foundation_SceneViewTab(Context* context, ProjectEditor* projectEditor)
{
    projectEditor->AddTab(MakeShared<SceneViewTab>(context));
}

SceneViewTab::SceneViewTab(Context* context)
    : ResourceEditorTab(context, "Scene View", "9f4f7432-dd60-4c83-aecd-2f6cf69d3549",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault, EditorTabPlacement::DockCenter)
{
}

SceneViewTab::~SceneViewTab()
{
}

bool SceneViewTab::CanOpenResource(const OpenResourceRequest& request)
{
    return request.xmlFile_ && request.xmlFile_->GetRoot("scene");
}

void SceneViewTab::OnResourceLoaded(const ea::string& resourceName)
{
    auto cache = GetSubsystem<ResourceCache>();
    auto xmlFile = cache->GetResource<XMLFile>(resourceName);

    if (!xmlFile)
    {
        URHO3D_LOGERROR("Cannot load scene file '%s'", resourceName);
        return;
    }

    auto scene = MakeShared<Scene>(context_);
    scene->LoadXML(xmlFile->GetRoot());

    SceneData& data = scenes_[resourceName];
    data.scene_ = scene;
    data.renderer_ = MakeShared<SceneRendererToTexture>(scene);
}

void SceneViewTab::OnResourceUnloaded(const ea::string& resourceName)
{
    scenes_.erase(resourceName);
}

void SceneViewTab::OnActiveResourceChanged(const ea::string& resourceName)
{
    for (const auto& [name, data] : scenes_)
        data.renderer_->SetActive(name == resourceName);
}

void SceneViewTab::UpdateAndRenderContent()
{
    const auto iter = scenes_.find(GetActiveResourceName());
    if (iter == scenes_.end())
        return;

    SceneData& currentScene = iter->second;
    currentScene.renderer_->SetTextureSize(GetContentSize());
    currentScene.renderer_->Update();

    Texture2D* sceneTexture = currentScene.renderer_->GetTexture();
    ui::SetCursorPos({0, 0});
    ui::ImageItem(sceneTexture, ToImGui(sceneTexture->GetSize()));
}

IntVector2 SceneViewTab::GetContentSize() const
{
    const ImGuiContext& g = *GImGui;
    const ImGuiWindow* window = g.CurrentWindow;
    const ImRect rect = ImRound(window->ContentRegionRect);
    return {RoundToInt(rect.GetWidth()), RoundToInt(rect.GetHeight())};
}

}
