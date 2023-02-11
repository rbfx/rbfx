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

#include "../../Core/IniHelpers.h"
#include "../../Foundation/SceneViewTab/SceneSelectionRenderer.h"

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Scene/Node.h>

namespace Urho3D
{

void Foundation_SceneSelectionRenderer(Context* context, SceneViewTab* sceneViewTab)
{
    auto project = sceneViewTab->GetProject();
    auto settingsManager = project->GetSettingsManager();

    auto settingsPage = MakeShared<SceneSelectionRenderer::SettingsPage>(context);
    settingsManager->AddPage(settingsPage);

    sceneViewTab->RegisterAddon<SceneSelectionRenderer>(settingsPage);
}

void SceneSelectionRenderer::Settings::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "DirectSelectionColor", directSelectionColor_, Settings{}.directSelectionColor_);
    SerializeOptionalValue(archive, "IndirectSelectionColor", indirectSelectionColor_, Settings{}.indirectSelectionColor_);
}

void SceneSelectionRenderer::Settings::RenderSettings()
{
    ui::ColorEdit3("Direct Selection", &directSelectionColor_.r_);
    ui::ColorEdit3("Indirect Selection", &indirectSelectionColor_.r_);
}

SceneSelectionRenderer::SceneSelectionRenderer(SceneViewTab* owner, SettingsPage* settings)
    : SceneViewAddon(owner)
    , settings_(settings)
{
}

void SceneSelectionRenderer::Render(SceneViewPage& scenePage)
{
    const Settings& cfg = settings_->GetValues();
    PageState& state = GetOrInitializeState(scenePage);

    if (PrepareInternalComponents(scenePage, state))
    {
        state.directSelection_->SetColor(cfg.directSelectionColor_);
        state.indirectSelection_->SetColor(cfg.indirectSelectionColor_);

        if (state.currentRevision_ != scenePage.selection_.GetRevision())
        {
            state.currentRevision_ = scenePage.selection_.GetRevision();
            UpdateInternalComponents(scenePage, state);
        }
    }

    Scene* scene = scenePage.scene_;
    for (Node* node : scenePage.selection_.GetNodes())
    {
        if (node)
            DrawNodeSelection(scene, node, node != node->GetScene());
    }

    for (Component* component : scenePage.selection_.GetComponents())
    {
        if (component)
            DrawComponentSelection(scene, component);
    }
}

bool SceneSelectionRenderer::RenderTabContextMenu()
{
    if (ui::MenuItem("Draw Debug Geometry", nullptr, drawDebugGeometry_))
        drawDebugGeometry_ = !drawDebugGeometry_;
    return true;
}

void SceneSelectionRenderer::WriteIniSettings(ImGuiTextBuffer& output)
{
    WriteIntToIni(output, "SceneSelectionRenderer.DrawDebugGeometry", drawDebugGeometry_);
}

void SceneSelectionRenderer::ReadIniSettings(const char* line)
{
    if (const auto value = ReadIntFromIni(line, "SceneSelectionRenderer.DrawDebugGeometry"))
        drawDebugGeometry_ = *value != 0;
}

SceneSelectionRenderer::PageState& SceneSelectionRenderer::GetOrInitializeState(SceneViewPage& scenePage) const
{
    ea::any& stateWrapped = scenePage.GetAddonData(*this);
    if (!stateWrapped.has_value())
        stateWrapped = PageState{};
    return ea::any_cast<PageState&>(stateWrapped);
}

bool SceneSelectionRenderer::PrepareInternalComponents(SceneViewPage& scenePage, PageState& state) const
{
    Scene* scene = scenePage.scene_;
    if (!state.directSelection_ || !state.indirectSelection_)
    {
        if (state.directSelection_)
            state.directSelection_->Remove();
        if (state.indirectSelection_)
            state.indirectSelection_->Remove();

        state.currentRevision_ = 0;

        state.directSelection_ = scene->CreateComponent<OutlineGroup>();
        state.directSelection_->SetRenderOrder(DirectSelectionRenderOrder);
        state.directSelection_->SetTemporary(true);

        state.indirectSelection_ = scene->CreateComponent<OutlineGroup>();
        state.indirectSelection_->SetRenderOrder(IndirectSelectionRenderOrder);
        state.indirectSelection_->SetTemporary(true);
    }

    return state.directSelection_ && state.indirectSelection_;
}

void SceneSelectionRenderer::UpdateInternalComponents(SceneViewPage& scenePage, PageState& state) const
{
    Scene* scene = scenePage.scene_;

    state.directSelection_->ClearDrawables();

    for (Component* component : scenePage.selection_.GetComponents())
    {
        if (Node* node = component->GetNode())
            AddNodeDrawablesToGroup(node, state.directSelection_);
    }

    for (Node* node : scenePage.selection_.GetNodes())
        AddNodeDrawablesToGroup(node, state.directSelection_);

    state.indirectSelection_->ClearDrawables();

    for (Component* component : scenePage.selection_.GetComponents())
    {
        if (Node* node = component->GetNode(); node && node != scene)
            AddNodeChildrenDrawablesToGroup(node, state.indirectSelection_, state.directSelection_);
    }

    for (Node* node : scenePage.selection_.GetNodes())
    {
        if (node != scene)
            AddNodeChildrenDrawablesToGroup(node, state.indirectSelection_, state.directSelection_);
    }
}

void SceneSelectionRenderer::AddNodeDrawablesToGroup(const Node* node, OutlineGroup* group, OutlineGroup* excludeGroup) const
{
    for (Component* outlinedComponent : node->GetComponents())
    {
        if (auto drawable = outlinedComponent->Cast<Drawable>())
        {
            if (excludeGroup && excludeGroup->ContainsDrawable(drawable))
                continue;
            group->AddDrawable(drawable);
        }
    }
}

void SceneSelectionRenderer::AddNodeChildrenDrawablesToGroup(const Node* node, OutlineGroup* group, OutlineGroup* excludeGroup) const
{
    for (Node* child : node->GetChildren())
    {
        AddNodeDrawablesToGroup(child, group, excludeGroup);
        AddNodeChildrenDrawablesToGroup(child, group, excludeGroup);
    }
}

void SceneSelectionRenderer::DrawNodeSelection(Scene* scene, Node* node, bool recursive)
{
    for (Component* component : node->GetComponents())
        DrawComponentSelection(scene, component);

    if (recursive)
    {
        for (Node* child : node->GetChildren())
            DrawNodeSelection(scene, child, true);
    }
}

void SceneSelectionRenderer::DrawComponentSelection(Scene* scene, Component* component)
{
    if (!drawDebugGeometry_)
        return;
    auto debugRenderer = scene->GetComponent<DebugRenderer>();
    component->DrawDebugGeometry(debugRenderer, true);
}

}
