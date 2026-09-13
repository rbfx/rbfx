// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/ResourceBrowserTab/MaterialFactory.h"

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace Urho3D
{

void Foundation_MaterialFactory(Context* context, ResourceBrowserTab* resourceBrowserTab)
{
    resourceBrowserTab->AddFactory(MakeShared<MaterialFactory>(context));
}

MaterialFactory::MaterialFactory(Context* context)
    : BaseResourceFactory(context, 0, "Material")
{
}

void MaterialFactory::RenderAuxilary()
{
    ui::Separator();

    ui::RadioButton("Opaque", &type_, Opaque);
    if (ui::IsItemHovered())
        ui::SetTooltip("Opaque material with solid surface.");

    ui::RadioButton("Alpha Mask", &type_, AlphaMask);
    if (ui::IsItemHovered())
        ui::SetTooltip("Opaque material with pixels discarded based on alpha channel in diffuse (albedo) texture.");

    ui::RadioButton("Transparent", &type_, Transparent);
    if (ui::IsItemHovered())
        ui::SetTooltip("Realistic transparent material like glass or plastic with specular highlights and reflections not affected by alpha value.");

    ui::RadioButton("Transparent Fade", &type_, TransparentFade);
    if (ui::IsItemHovered())
        ui::SetTooltip("Transparent material with specular highlights and reflections faded out by alpha value.");

    ui::Checkbox("Lit", &lit_);
    if (ui::IsItemHovered())
        ui::SetTooltip("Enable lighting for this material.");

    ui::BeginDisabled(!lit_);

    ui::Checkbox("PBR", &pbr_);
    if (ui::IsItemHovered())
        ui::SetTooltip("Use physically based rendering for this material.");

    ui::Checkbox("Normal Mapping", &normal_);
    if (ui::IsItemHovered())
        ui::SetTooltip("Use normal mapping for this material, if normal texture is provided.");

    ui::EndDisabled();

    ui::Separator();
}

void MaterialFactory::CommitAndClose()
{
    auto cache = GetSubsystem<ResourceCache>();

    const ea::string techniqueName = GetTechniqueName();
    ea::string vertexDefines;
    ea::string pixelDefines;
    if (lit_ && pbr_)
    {
        vertexDefines += "PBR ";
        pixelDefines += "PBR ";
    }
    if (type_ == AlphaMask)
        pixelDefines += "ALPHAMASK ";

    auto material = MakeShared<Material>(context_);

    material->SetTechnique(0, cache->GetResource<Technique>(techniqueName));
    material->SetVertexShaderDefines(pixelDefines);
    material->SetPixelShaderDefines(pixelDefines);

    material->SaveFile(GetFinalFileName());
}

ea::string MaterialFactory::GetTechniqueName() const
{
    if (!lit_)
    {
        if (type_ == Opaque || type_ == AlphaMask)
            return "Techniques/UnlitOpaque.xml";
        else
            return "Techniques/UnlitTransparent.xml";
    }
    else
    {
        if (normal_)
        {
            if (type_ == Opaque || type_ == AlphaMask)
                return "Techniques/LitOpaqueNormalMap.xml";
            else if (type_ == Transparent)
                return "Techniques/LitTransparentNormalMap.xml";
            else
                return "Techniques/LitTransparentFadeNormalMap.xml";
        }
        else
        {
            if (type_ == Opaque || type_ == AlphaMask)
                return "Techniques/LitOpaque.xml";
            else if (type_ == Transparent)
                return "Techniques/LitTransparent.xml";
            else
                return "Techniques/LitTransparentFade.xml";
        }
    }
}

}
