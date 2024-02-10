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
