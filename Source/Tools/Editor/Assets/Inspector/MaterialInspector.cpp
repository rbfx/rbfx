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

const float attributeIndentLevel = 15.f;
const auto MAX_FILLMODES = IM_ARRAYSIZE(fillModeNames) - 1;

MaterialInspector::MaterialInspector(Context* context, Material* material)
    : ResourceInspector(context)
    , view_(context, {0, 0, 200, 200})
    , inspectable_(new Inspectable::Material(material))
    , attributeInspector_(context)
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

    undo_.Connect(&attributeInspector_);
}

void MaterialInspector::RenderInspector(const char* filter)
{
    if (RenderAttributes(inspectable_, filter, &attributeInspector_))
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
    }
}

void MaterialInspector::ToggleModel()
{
    auto model = node_->GetOrCreateComponent<StaticModel>();

    model->SetModel(node_->GetCache()->GetResource<Model>(ToString("Models/%s.mdl", figures_[figureIndex_])));
    model->SetMaterial(inspectable_->GetMaterial());
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

void MaterialInspector::Save()
{
    inspectable_->GetMaterial()->SaveFile(GetCache()->GetResourceFileName(inspectable_->GetMaterial()->GetName()));
}

Inspectable::Material::Material(Urho3D::Material* material)
    : Serializable(material->GetContext())
    , material_(material)
{
}

void Inspectable::Material::RegisterObject(Context* context)
{
    // TODO: Depth Bias
    // TODO: Techniques
    // TODO: Textures
    // TODO: UV transforms
    // TODO: Shader Parameters

    // Cull Mode
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                value = material->GetCullMode();
        };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                material->SetCullMode((CullMode) value.GetUInt());
        };
        URHO3D_CUSTOM_ENUM_ATTRIBUTE("Cull", getter, setter, cullModeNames, CULL_NONE, AM_DEFAULT);
    }

    // Shadow Cull Mode
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                value = material->GetShadowCullMode();
        };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                material->SetShadowCullMode((CullMode) value.GetUInt());
        };
        URHO3D_CUSTOM_ENUM_ATTRIBUTE("Shadow Cull", getter, setter, cullModeNames, CULL_NONE, AM_DEFAULT);
    }

    // Fill Mode
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                value = material->GetFillMode();
        };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                material->SetFillMode((FillMode) value.GetUInt());
        };
        URHO3D_CUSTOM_ENUM_ATTRIBUTE("Fill", getter, setter, fillModeNames, FILL_SOLID, AM_DEFAULT);
    }

    // Alpha To Coverage
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                value = material->GetAlphaToCoverage();
        };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                material->SetAlphaToCoverage(value.GetBool());
        };
        URHO3D_CUSTOM_ATTRIBUTE("Alpha To Coverage", getter, setter, bool, false, AM_DEFAULT);
    }

    // Line Anti Alias
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                value = material->GetLineAntiAlias();
        };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                material->SetLineAntiAlias(value.GetBool());
        };
        URHO3D_CUSTOM_ATTRIBUTE("Line Anti Alias", getter, setter, bool, false, AM_DEFAULT);
    }

    // Render Order
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                value = material->GetRenderOrder();
        };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                material->SetRenderOrder(static_cast<unsigned char>(value.GetUInt()));
        };
        URHO3D_CUSTOM_ATTRIBUTE("Render Order", getter, setter, unsigned, 128, AM_DEFAULT);
    }

    // Occlusion
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                value = material->GetOcclusion();
        };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)
        {
            if (auto* material = inspectable.GetMaterial())
                material->SetOcclusion(value.GetBool());
        };
        URHO3D_CUSTOM_ATTRIBUTE("Occlusion", getter, setter, bool, false, AM_DEFAULT);
    }
}

}
