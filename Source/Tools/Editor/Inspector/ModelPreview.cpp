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
#include "ModelPreview.h"
#include "Editor.h"


namespace Urho3D
{

ModelPreview::ModelPreview(Context* context)
    : Object(context)
    , view_(context, {0, 0, 200, 200})
{
    // Workaround: for some reason this overriden method of our class does not get called by SceneView constructor.
    CreateObjects();

    if (auto* sceneTab = GetSubsystem<Editor>()->GetTab<SceneTab>())
        SetEffectSource(sceneTab->GetViewport()->GetRenderPath());

    ToggleModel();
}

void ModelPreview::SetModel(Model* model)
{
    auto* staticModel = node_->GetOrCreateComponent<StaticModel>();
    if (staticModel->GetModel() == model)
        return;

    staticModel->SetModel(model);
    auto bb = model->GetBoundingBox();
    auto scale = 1.f / Max(bb.Size().x_, Max(bb.Size().y_, bb.Size().z_)) * 0.8f;
    node_->SetScale(scale);
    node_->SetWorldPosition(node_->GetWorldPosition() - staticModel->GetWorldBoundingBox().Center());
}

void ModelPreview::SetModel(const ea::string& resourceName)
{
    auto* cache = context_->GetSubsystem<ResourceCache>();
    SetModel(cache->GetResource<Model>(resourceName));
}

void ModelPreview::SetMaterial(const ea::string& resourceName, int index)
{
    auto* cache = context_->GetSubsystem<ResourceCache>();
    SetMaterial(cache->GetResource<Material>(resourceName), index);
}

void ModelPreview::SetMaterial(Material* material, int index)
{
    auto* model = node_->GetOrCreateComponent<StaticModel>();
    model->SetMaterial(index, material);
}

void ModelPreview::CreateObjects()
{
    view_.CreateObjects();
    node_ = view_.GetScene()->CreateChild("Preview");
    Node* node = view_.GetCamera()->GetNode();
    node->CreateComponent<Light>();
    node->SetPosition(Vector3::BACK * distance_);
    node->LookAt(Vector3::ZERO);
}

void ModelPreview::RenderPreview()
{
    auto* input = GetSubsystem<Input>();
#if 0
    const float dpi = ui::GetCurrentWindow()->Viewport->DpiScale;
#else
    const float dpi = 1.0f;
#endif
    float size = ui::GetWindowWidth() - ui::GetCursorPosX();
    view_.SetSize({0, 0, static_cast<int>(size * dpi), static_cast<int>(size * dpi)});
    ui::ImageItem(view_.GetTexture(), ImVec2(size, size));
    bool wasActive = mouseGrabbed_;
    mouseGrabbed_ = ui::ItemMouseActivation(MOUSEB_RIGHT, ImGuiItemMouseActivation_Dragging);
    if (wasActive != mouseGrabbed_)
        input->SetMouseVisible(!mouseGrabbed_);

    if (mouseGrabbed_)
    {
        Node* node = view_.GetCamera()->GetNode();
        if (ui::IsKeyPressed(KEY_ESCAPE))
        {
            node->SetPosition(Vector3::BACK * distance_);
            node->LookAt(Vector3::ZERO);
        }
        else
        {
            Vector2 delta = ui::GetMouseDragDelta(MOUSEB_RIGHT);
            Quaternion rotateDelta = Quaternion(delta.x_ * 0.1f, node->GetUp()) *
                Quaternion(delta.y_ * 0.1f, node->GetRight());
            node->RotateAround(Vector3::ZERO, rotateDelta, TS_WORLD);
        }
    }
}

void ModelPreview::SetEffectSource(RenderPath* renderPath)
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

void ModelPreview::ToggleModel()
{
    const char* currentModel = figures_[figureIndex_];
    ResourceRefList materials;
    if (auto* model = node_->GetComponent<StaticModel>())
        materials = model->GetMaterialsAttr();

    SetModel(ToString("Models/%s.mdl", currentModel));
    figureIndex_ = ++figureIndex_ % figures_.size();

    auto* cache = GetSubsystem<ResourceCache>();
    auto* model = node_->GetComponent<StaticModel>();
    for (int i = 0; i < materials.names_.size(); i++)
        model->SetMaterial(i, cache->GetResource<Material>(materials.names_[i]));

    auto bb = model->GetBoundingBox();
    auto scale = 1.f / Max(bb.Size().x_, Max(bb.Size().y_, bb.Size().z_));
    if (strcmp(currentModel, "Box") == 0)            // Box is rather big after autodetecting scale, but other
        scale *= 0.7f;                               // figures are ok. Patch the box then.
    else if (strcmp(currentModel, "TeaPot") == 0)    // And teapot is rather small.
        scale *= 1.2f;
    node_->SetScale(scale);
}

Material* ModelPreview::GetMaterial(int index)
{
    if (auto* model = node_->GetComponent<StaticModel>())
        return model->GetMaterial(index);
    return nullptr;
}

}
