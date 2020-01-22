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
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Graphics/StaticModel.h>
#include "Tabs/Scene/SceneTab.h"
#include "PreviewInspector.h"
#include "Editor.h"


namespace Urho3D
{

PreviewInspector::PreviewInspector(Context* context)
    : InspectorProvider(context)
    , view_(context, {0, 0, 200, 200})
{
    // Workaround: for some reason this overriden method of our class does not get called by SceneView constructor.
    CreateObjects();

    if (SceneTab* sceneTab = GetSubsystem<Editor>()->GetTab<SceneTab>())
        SetEffectSource(sceneTab->GetViewport()->GetRenderPath());
}

void PreviewInspector::SetModel(Model* model)
{
    auto staticModel = node_->GetOrCreateComponent<StaticModel>();

    staticModel->SetModel(model);
    auto bb = model->GetBoundingBox();
    auto scale = 1.f / Max(bb.Size().x_, Max(bb.Size().y_, bb.Size().z_)) * 0.8f;
    node_->SetScale(scale);
    node_->SetWorldPosition(node_->GetWorldPosition() - staticModel->GetWorldBoundingBox().Center());
}

void PreviewInspector::SetModel(const ea::string& resourceName)
{
    SetModel(context_->GetCache()->GetResource<Model>(resourceName));
}

void PreviewInspector::CreateObjects()
{
    view_.CreateObjects();
    node_ = view_.GetScene()->CreateChild("Preview");
    view_.GetCamera()->GetNode()->CreateComponent<Light>();
    view_.GetCamera()->GetNode()->SetPosition(Vector3::BACK * distance_);
    view_.GetCamera()->GetNode()->LookAt(Vector3::ZERO);
}

void PreviewInspector::RenderPreview()
{
    auto* input = GetSubsystem<Input>();
    auto size = static_cast<int>(ui::GetWindowWidth() - ui::GetCursorPosX());
    view_.SetSize({0, 0, size, size});
    ui::ImageItem(view_.GetTexture(), ImVec2(size, size));
    bool wasActive = mouseGrabbed_;
    mouseGrabbed_ = ui::ItemMouseActivation(MOUSEB_RIGHT) && ui::IsMouseDragging(MOUSEB_RIGHT);
    if (wasActive != mouseGrabbed_)
        input->SetMouseVisible(!mouseGrabbed_);

    if (mouseGrabbed_)
    {
        ui::SetMouseCursor(ImGuiMouseCursor_None);
        Node* node = view_.GetCamera()->GetNode();
        if (input->GetKeyPress(KEY_ESCAPE))
        {
            node->SetPosition(Vector3::BACK * distance_);
            node->LookAt(Vector3::ZERO);
        }
        else
        {
            Vector2 delta(input->GetMouseMove());
            Quaternion rotateDelta = Quaternion(delta.x_ * 0.1f, node->GetUp()) *
                Quaternion(delta.y_ * 0.1f, node->GetRight());
            node->RotateAround(Vector3::ZERO, rotateDelta, TS_WORLD);
        }
    }
}

void PreviewInspector::SetEffectSource(RenderPath* renderPath)
{
    if (renderPath == nullptr)
        return;

    view_.GetViewport()->SetRenderPath(renderPath);
    auto* light = view_.GetCamera()->GetComponent<Light>();
    for (auto& command: renderPath->commands_)
    {
        if (command.pixelShaderName_.starts_with("PBR"))
        {
            // Lights in PBR scenes need modifications, otherwise obects in material preview look very dark
            light->SetUsePhysicalValues(true);
            light->SetBrightness(5000);
            light->SetShadowCascade(CascadeParameters(10, 20, 30, 40, 10));
            return;
        }
    }

    light->SetUsePhysicalValues(false);
    light->SetBrightness(DEFAULT_BRIGHTNESS);
    light->SetShadowCascade(CascadeParameters(DEFAULT_SHADOWSPLIT, 0.0f, 0.0f, 0.0f, DEFAULT_SHADOWFADESTART));
}

}
