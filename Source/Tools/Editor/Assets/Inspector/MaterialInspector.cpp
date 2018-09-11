//
// Copyright (c) 2018 Rokas Kupstys
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
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include "MaterialInspector.h"

using namespace ImGui::litterals;

namespace Urho3D
{

// Should these be exported by the engine?
static const char* cullModeNames[] =
{
    "none",
    "ccw",
    "cw",
    nullptr
};
static const char* fillModeNames[] =
{
    "solid",
    "wireframe",
    "point",
    nullptr
};

const float attributeIndentLevel = 15.f;
const auto MAX_FILLMODES = IM_ARRAYSIZE(fillModeNames) - 1;

MaterialInspector::MaterialInspector(Context* context, Material* material)
    : ResourceInspector(context)
    , view_(context, {0, 0, 200, 200})
    , material_(material)
    , attributeInspector_(context)
    , autoColumn_(context)
{
    // Workaround: for some reason this overriden method of our class does not get called by SceneView constructor.
    CreateObjects();

    // Scene viewport renderpath must be same as material viewport renderpath
    Viewport* effectSource = nullptr;
    if (effectSource != nullptr)
    {
        auto path = effectSource->GetRenderPath();
        view_.GetViewport()->SetRenderPath(path);
        auto light = view_.GetCamera()->GetComponent<Light>();
        for (auto& command: path->commands_)
        {
            if (command.pixelShaderName_ == "PBRDeferred")
            {
                // Lights in PBR scenes need modifications, otherwise obects in material preview look very dark
                light->SetUsePhysicalValues(true);
                light->SetBrightness(5000);
                light->SetShadowCascade(CascadeParameters(10, 20, 30, 40, 10));
                break;
            }
        }
    }
}

void MaterialInspector::Render()
{
    auto size = static_cast<int>(ui::GetWindowWidth() - ui::GetCursorPosX());
    view_.SetSize({0, 0, size, size});
    ui::Image(view_.GetTexture(), ImVec2(view_.GetTexture()->GetWidth(), view_.GetTexture()->GetHeight()));
    ui::SetHelpTooltip("Drag resource here.\nClick to switch object.");
    Input* input = view_.GetCamera()->GetInput();
    bool rightMouseButtonDown = input->GetMouseButtonDown(MOUSEB_RIGHT);
    if (ui::IsItemHovered())
    {
        if (rightMouseButtonDown)
            SetGrab(true);
        else if (input->GetMouseButtonPress(MOUSEB_LEFT))
            ToggleModel();
    }

    if (mouseGrabbed_)
    {
        if (rightMouseButtonDown)
        {
            if (input->GetKeyPress(KEY_ESCAPE))
            {
                view_.GetCamera()->GetNode()->SetPosition(Vector3::BACK * distance_);
                view_.GetCamera()->GetNode()->LookAt(Vector3::ZERO);
            }
            else
            {
                IntVector2 delta = input->GetMouseMove();
                view_.GetCamera()->GetNode()->RotateAround(Vector3::ZERO,
                                      Quaternion(delta.x_ * 0.1f, view_.GetCamera()->GetNode()->GetUp()) *
                                      Quaternion(delta.y_ * 0.1f, view_.GetCamera()->GetNode()->GetRight()), TS_WORLD);
            }
        }
        else
            SetGrab(false);
    }

    ui::Indent(attributeIndentLevel);

    ui::TextUnformatted("Cull");
    autoColumn_.NextColumn();
    ui::PushItemWidth(-1);
    int valueInt = material_->GetCullMode();
    if (ui::Combo("###cull", &valueInt, cullModeNames, (int)MAX_CULLMODES))
    {
        material_->SetCullMode(static_cast<CullMode>(valueInt));
        material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
    }
    ui::PopItemWidth();

    ui::TextUnformatted("Shadow Cull");
    autoColumn_.NextColumn();
    ui::PushItemWidth(-1);
    valueInt = material_->GetShadowCullMode();
    if (ui::Combo("###shadowCull", &valueInt, cullModeNames, (int)MAX_CULLMODES))
    {
        material_->SetShadowCullMode(static_cast<CullMode>(valueInt));
        material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
    }
    ui::PopItemWidth();

    ui::TextUnformatted("Fill");
    autoColumn_.NextColumn();
    ui::PushItemWidth(-1);
    valueInt = material_->GetFillMode();
    if (ui::Combo("###fill", &valueInt, fillModeNames, MAX_FILLMODES))
    {
        material_->SetFillMode(static_cast<FillMode>(valueInt));
        material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
    }
    ui::PopItemWidth();

    auto bias = material_->GetDepthBias();
    ui::TextUnformatted("Constant Bias");
    autoColumn_.NextColumn();
    ui::PushItemWidth(-1);
    if (ui::DragFloat("###constantBias_", &bias.constantBias_, 0.1f, -1, 1))
    {
        material_->SetDepthBias(bias);
        material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
    }
    ui::PopItemWidth();

    ui::TextUnformatted("Slope Scaled Bias");
    autoColumn_.NextColumn();
    ui::PushItemWidth(-1);
    if (ui::DragFloat("###slopeScaledBias_", &bias.slopeScaledBias_, 1, -16, 16))
    {
        material_->SetDepthBias(bias);
        material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
    }
    ui::PopItemWidth();

    ui::TextUnformatted("Normal Offset");
    autoColumn_.NextColumn();
    ui::PushItemWidth(-1);
    if (ui::DragFloat("###normalOffset_", &bias.normalOffset_, 1, 0))
    {
        material_->SetDepthBias(bias);
        material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
    }
    ui::PopItemWidth();

    ui::TextUnformatted("Alpha To Coverage");
    autoColumn_.NextColumn();
    ui::PushItemWidth(-1);
    bool valueBool = material_->GetAlphaToCoverage();
    if (ui::Checkbox("###alphaToCoverage_", &valueBool))
    {
        material_->SetAlphaToCoverage(valueBool);
        material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
    }
    ui::PopItemWidth();

    ui::TextUnformatted("Line Anti-Alias");
    autoColumn_.NextColumn();
    ui::PushItemWidth(-1);
    valueBool = material_->GetLineAntiAlias();
    if (ui::Checkbox("###lineAntiAlias_", &valueBool))
    {
        material_->SetLineAntiAlias(valueBool);
        material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
    }
    ui::PopItemWidth();

    ui::TextUnformatted("Occlusion");
    autoColumn_.NextColumn();
    ui::PushItemWidth(-1);
    valueBool = material_->GetOcclusion();
    if (ui::Checkbox("###occlusion_", &valueBool))
    {
        material_->SetOcclusion(valueBool);
        material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
    }
    ui::PopItemWidth();

    ui::TextUnformatted("Render Order");
    autoColumn_.NextColumn();
    ui::PushItemWidth(-1);
    valueInt = material_->GetRenderOrder();
    if (ui::DragInt("###renderOrder_", &valueInt, 1, 0, 0xFF))
    {
        material_->SetRenderOrder(static_cast<unsigned char>(valueInt));
        material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
    }
    ui::PopItemWidth();

    auto handleDragAndDrop = [&](StringHash resourceType, SharedPtr<Resource>& resource)
    {
        bool dropped = false;
        if (ui::BeginDragDropTarget())
        {
            const Variant& payload = ui::AcceptDragDropVariant("path");
            if (!payload.IsEmpty())
            {
                resource = GetCache()->GetResource(resourceType, payload.GetString());
                dropped = resource.NotNull();
            }
            ui::EndDragDropTarget();
        }
        ui::SetHelpTooltip("Drag and drop resource here.");
        return dropped;
    };
    SharedPtr<Resource> resource;

    for (unsigned i = 0; i < material_->GetNumTechniques(); i++)
    {
        ui::PushID(i);
        auto& tech = const_cast<TechniqueEntry&>(material_->GetTechniqueEntry(i));

        bool open = ui::CollapsingHeaderSimple(ToString("Technique %d", i).CString());
        autoColumn_.NextColumn();
        String techName = tech.technique_->GetName();
        ui::PushItemWidth(material_->GetNumTechniques() > 1 ? -60_dpx : -30_dpx);
        ui::InputText("###techniqueName_", (char*)techName.CString(), techName.Length(),
                      ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
        ui::PopItemWidth();
        if (handleDragAndDrop(Technique::GetTypeStatic(), resource))
        {
            material_->SetTechnique(i, DynamicCast<Technique>(resource), tech.qualityLevel_, tech.lodDistance_);
            material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
            resource.Reset();
        }

        if (material_->GetNumTechniques() > 1)
        {
            ui::SameLine();
            if (ui::IconButton(ICON_FA_TRASH))
            {
                for (auto j = i + 1; j < material_->GetNumTechniques(); j++)
                    material_->SetTechnique(j - 1, material_->GetTechnique(j));
                material_->SetNumTechniques(material_->GetNumTechniques() - 1);
                material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
                ui::PopID();
                break;
            }
        }

        ui::SameLine();
        if (ui::IconButton(ICON_FA_CROSSHAIRS))
        {
            SendEvent(E_INSPECTORLOCATERESOURCE, InspectorLocateResource::P_NAME, material_->GetTechnique(i)->GetName());
        }
        ui::SetHelpTooltip("Locate resource");

        if (open)
        {
            ui::Indent(attributeIndentLevel);

            ui::TextUnformatted("LOD Distance");
            autoColumn_.NextColumn();
            ui::PushItemWidth(-1);
            if (ui::DragFloat("###lodDistance_", &tech.lodDistance_))
                material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
            ui::PopItemWidth();

            ui::TextUnformatted("Quality");
            autoColumn_.NextColumn();
            ui::PushItemWidth(-1);
            if (ui::DragInt("###qualityLevel_", (int*)&tech.qualityLevel_, 1, QUALITY_LOW, QUALITY_MAX))
                material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
            ui::PopItemWidth();

            ui::Unindent(attributeIndentLevel);
        }
        ui::PopID();
    }

    ui::PushItemWidth(-1);
    const char* newTechnique = "Add new technique";
    ui::InputText("###newTechnique_", (char*)newTechnique, strlen(newTechnique), ImGuiInputTextFlags_ReadOnly);
    if (handleDragAndDrop(Technique::GetTypeStatic(), resource))
    {
        material_->SetNumTechniques(material_->GetNumTechniques() + 1);
        material_->SetTechnique(material_->GetNumTechniques() - 1, dynamic_cast<Technique*>(resource.Get()));
        material_->SaveFile(GetCache()->GetResourceFileName(material_->GetName()));
    }
    ui::PopItemWidth();
    ui::Unindent(attributeIndentLevel);
}

void MaterialInspector::ToggleModel()
{
    auto model = node_->GetOrCreateComponent<StaticModel>();

    model->SetModel(node_->GetCache()->GetResource<Model>(ToString("Models/%s.mdl", figures_[figureIndex_])));
    model->SetMaterial(material_);
    auto bb = model->GetBoundingBox();
    auto scale = 1.f / Max(bb.Size().x_, Max(bb.Size().y_, bb.Size().z_));
    if (strcmp(figures_[figureIndex_], "Box") == 0)    // Box is rather big after autodetecting scale, but other figures
        scale *= 0.7f;                      // are ok. Patch the box then.
    else if (strcmp(figures_[figureIndex_], "TeaPot") == 0)    // And teapot is rather small.
        scale *= 1.2f;
    node_->SetScale(scale);
    node_->SetWorldPosition(node_->GetWorldPosition() - model->GetWorldBoundingBox().Center());

    figureIndex_ = ++figureIndex_ % figures_.Size();
}

void MaterialInspector::SetGrab(bool enable)
{
    if (mouseGrabbed_ == enable)
        return;

    mouseGrabbed_ = enable;
    Input* input = view_.GetCamera()->GetInput();
    if (enable && input->IsMouseVisible())
        input->SetMouseVisible(false);
    else if (!enable && !input->IsMouseVisible())
        input->SetMouseVisible(true);
}

void MaterialInspector::CreateObjects()
{
    view_.CreateObjects();
    node_ = view_.GetScene()->CreateChild("Sphere");
    ToggleModel();
    view_.GetCamera()->GetNode()->CreateComponent<Light>();
    view_.GetCamera()->GetNode()->SetPosition(Vector3::BACK * distance_);
    view_.GetCamera()->GetNode()->LookAt(Vector3::ZERO);
}

}
