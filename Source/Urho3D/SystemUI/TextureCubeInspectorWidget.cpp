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

#include "Urho3D/SystemUI/TextureCubeInspectorWidget.h"

#include "Urho3D/Graphics/Geometry.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/GraphicsEvents.h"
#include "Urho3D/Graphics/GraphicsUtils.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/Graphics/TextureCube.h"
#include "Urho3D/RenderAPI/PipelineState.h"
#include "Urho3D/RenderAPI/RenderContext.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderAPI/RenderScope.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/SystemUI/SystemUI.h"

namespace Urho3D
{

namespace
{
const StringVector textureFilterModes{
    "NEAREST",
    "BILINEAR",
    "TRILINEAR",
    "ANISOTROPIC",
    "NEAREST_ANISOTROPIC",
    "DEFAULT",
};
const StringVector textureAddressMode{
    "WRAP",
    "MIRROR",
    "CLAMP",
    "BORDER",
};

Diligent::RefCntAutoPtr<Diligent::ITextureView> CreateCubemapView(TextureCube* texture, CubeMapFace face)
{
    Diligent::TextureViewDesc viewDesc = {};
    const ea::string name = Format("{}: face #{}", texture->GetName(), face);
    viewDesc.Name = name.c_str();
    viewDesc.ViewType = Diligent::TEXTURE_VIEW_SHADER_RESOURCE;
    viewDesc.TextureDim = Diligent::RESOURCE_DIM_TEX_2D;
    viewDesc.Format = texture->GetFormat();
    viewDesc.FirstArraySlice = face;
    viewDesc.NumArraySlices = 1;

    Diligent::RefCntAutoPtr<Diligent::ITextureView> view;
    texture->GetHandles().texture_->CreateView(viewDesc, &view);
    if (!view)
    {
        URHO3D_LOGERROR("Failed to create UAV for texture");
        return {};
    }

    return view;
}

} // namespace

const ea::vector<ResourceInspectorWidget::PropertyDesc> TextureCubeInspectorWidget::properties{
    {
        "SRGB",
        Variant{false},
        [](const Resource* resource) { return Variant{static_cast<const TextureCube*>(resource)->GetSRGB()}; },
        [](Resource* resource, const Variant& value) { static_cast<TextureCube*>(resource)->SetSRGB(value.GetBool()); },
        "SRGB",
    },
    {
        "Linear",
        Variant{false},
        [](const Resource* resource) { return Variant{static_cast<const TextureCube*>(resource)->GetLinear()}; },
        [](Resource* resource, const Variant& value) { static_cast<TextureCube*>(resource)->SetLinear(value.GetBool()); },
        "Linear color space",
    },
    {
        "Filter Mode",
        Variant{TextureFilterMode::FILTER_DEFAULT},
        [](const Resource* resource)
        { return Variant{static_cast<int>(static_cast<const TextureCube*>(resource)->GetFilterMode())}; },
        [](Resource* resource, const Variant& value)
        { static_cast<TextureCube*>(resource)->SetFilterMode(static_cast<TextureFilterMode>(value.GetInt())); },
        "Texture Filter Mode",
        Widgets::EditVariantOptions{}.Enum(textureFilterModes),
    },
    {
        "U Address Mode",
        Variant{ADDRESS_WRAP},
        [](const Resource* resource)
        { return Variant{static_cast<int>(static_cast<const TextureCube*>(resource)->GetAddressMode(TextureCoordinate::U))}; },
        [](Resource* resource, const Variant& value) {
            static_cast<TextureCube*>(resource)->SetAddressMode(TextureCoordinate::U, static_cast<TextureAddressMode>(value.GetInt()));
        },
        "U texture coordinate address mode",
        Widgets::EditVariantOptions{}.Enum(textureAddressMode),
    },
    {
        "V Address Mode",
        Variant{ADDRESS_WRAP},
        [](const Resource* resource)
        { return Variant{static_cast<int>(static_cast<const TextureCube*>(resource)->GetAddressMode(TextureCoordinate::V))}; },
        [](Resource* resource, const Variant& value) {
            static_cast<TextureCube*>(resource)->SetAddressMode(TextureCoordinate::V, static_cast<TextureAddressMode>(value.GetInt()));
        },
        "V texture coordinate address mode",
        Widgets::EditVariantOptions{}.Enum(textureAddressMode),
    },
    {
        "W Address Mode",
        Variant{ADDRESS_WRAP},
        [](const Resource* resource)
        { return Variant{static_cast<int>(static_cast<const TextureCube*>(resource)->GetAddressMode(TextureCoordinate::W))}; },
        [](Resource* resource, const Variant& value) {
            static_cast<TextureCube*>(resource)->SetAddressMode(
                TextureCoordinate::W, static_cast<TextureAddressMode>(value.GetInt()));
        },
        "W texture coordinate address mode",
        Widgets::EditVariantOptions{}.Enum(textureAddressMode),
    },
};

SphericalHarmonicsGenerator::SphericalHarmonicsGenerator(Context* context)
    : Object(context)
{
    auto renderer = GetSubsystem<Renderer>();
    quadGeometry_ = renderer->GetQuadGeometry();
}

SphericalHarmonicsGenerator::~SphericalHarmonicsGenerator()
{
}

void SphericalHarmonicsGenerator::InitializePipelineStates()
{
    auto graphics = GetSubsystem<Graphics>();
    auto renderer = GetSubsystem<Renderer>();
    auto pipelineStateCache = GetSubsystem<PipelineStateCache>();

    GraphicsPipelineStateDesc desc;
    desc.debugName_ = "Copy texture";
    desc.vertexShader_ = graphics->GetShader(VS, "v2/X_CopyTexture", "");
    desc.pixelShader_ = graphics->GetShader(PS, "v2/X_CopyTexture", "");
    desc.colorWriteEnabled_ = true;
    desc.samplers_.Add("Albedo", SamplerStateDesc::Trilinear());

    Geometry* quadGeometry = renderer->GetQuadGeometry();
    InitializeInputLayoutAndPrimitiveType(desc, quadGeometry);

    copyTexturePipelineState_ = pipelineStateCache->GetGraphicsPipelineState(desc);
}

void SphericalHarmonicsGenerator::InitializeTextures()
{
    tempTexture_ = MakeShared<Texture2D>(context_);
    tempTexture_->SetSize(textureSize_, textureSize_ * MAX_CUBEMAP_FACES, TextureFormat::TEX_FORMAT_RGBA32_FLOAT,
        TextureFlag::BindRenderTarget);

    tempTextureData_.resize(textureSize_ * textureSize_ * MAX_CUBEMAP_FACES);
}

SphericalHarmonicsColor9 SphericalHarmonicsGenerator::Generate(TextureCube* texture)
{
    if (!copyTexturePipelineState_)
        InitializePipelineStates();
    if (!tempTexture_)
        InitializeTextures();

    if (!copyTexturePipelineState_ || !copyTexturePipelineState_->IsValid() || !tempTexture_)
        return {};

    auto renderDevice = GetSubsystem<RenderDevice>();
    RenderContext* renderContext = renderDevice->GetRenderContext();
    DrawCommandQueue* drawQueue = renderDevice->GetDefaultQueue();

    ea::vector<Diligent::RefCntAutoPtr<Diligent::ITextureView>> faceViews;
    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
    {
        const auto view = CreateCubemapView(texture, static_cast<CubeMapFace>(face));
        if (!view)
            return {};
        faceViews.push_back(view);
    }

    const RenderScope renderScope(renderContext, "SphericalHarmonicsGenerator::Generate");
    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
    {
        const IntVector2 viewportOffset{0, static_cast<int>(textureSize_ * face)};
        const IntVector2 viewportSize = IntVector2::ONE * static_cast<int>(textureSize_);

        const RenderTargetView renderTargets[] = {RenderTargetView::Texture(tempTexture_)};
        renderContext->SetRenderTargets(ea::nullopt, renderTargets);
        renderContext->SetViewport({viewportOffset, viewportOffset + viewportSize});

        drawQueue->Reset();
        drawQueue->SetPipelineState(copyTexturePipelineState_);

        drawQueue->AddShaderResource("Albedo", faceViews[face]);
        drawQueue->CommitShaderResources();

        SetBuffersFromGeometry(*drawQueue, quadGeometry_);

        drawQueue->DrawIndexed(quadGeometry_->GetIndexStart(), quadGeometry_->GetIndexCount());
        drawQueue->ExecuteInContext(renderContext);
    }

    tempTexture_->Read(0, 0, tempTextureData_.data(), tempTextureData_.size() * sizeof(Vector4));

    SphericalHarmonicsColor9 result;
    float weightSum = 0.0f;

    const bool isGammaSpace = !texture->GetSRGB() && !texture->GetLinear();
    const auto textureWidth = static_cast<float>(textureSize_);
    for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
    {
        const auto face = static_cast<CubeMapFace>(i);
        for (unsigned y = 0; y < textureSize_; ++y)
        {
            for (unsigned x = 0; x < textureSize_; ++x)
            {
                const Vector4 rawSample = tempTextureData_[x + (y + i * textureSize_) * textureSize_];
                const Vector3 sample =
                    isGammaSpace ? Color(rawSample).GammaToLinear().ToVector3() : rawSample.ToVector3();
                const Vector2 uv = {(x + 0.5f) / textureWidth, (y + 0.5f) / textureWidth};
                const Vector3 offset = ImageCube::ProjectUVOnCube(face, uv);
                const float distance = offset.Length();
                const float weight = 1.0f / (distance * distance * distance);
                const Vector3 direction = offset / distance;

                result += SphericalHarmonicsColor9(direction, sample) * weight;
                weightSum += weight;
            }
        }
    }

    result *= 4.0f * M_PI / weightSum;
    return result;
}

void SphericalHarmonicsGenerator::Generate(TextureCube* texture, XMLFile* imageXml)
{
    const SphericalHarmonicsColor9 rawSH = Generate(texture);
    const SphericalHarmonicsDot9 packedSH{rawSH};

    XMLElement rootElement = imageXml->GetRoot();
    while (rootElement.RemoveChild("sh"))
        ;

    XMLElement shElement = rootElement.CreateChild("sh");
    shElement.SetVector4("ar", packedSH.Ar_);
    shElement.SetVector4("ag", packedSH.Ag_);
    shElement.SetVector4("ab", packedSH.Ab_);
    shElement.SetVector4("br", packedSH.Br_);
    shElement.SetVector4("bg", packedSH.Bg_);
    shElement.SetVector4("bb", packedSH.Bb_);
    shElement.SetVector4("c", packedSH.C_);

    imageXml->SaveFile(imageXml->GetAbsoluteFileName());
}

TextureCubeInspectorWidget::TextureCubeInspectorWidget(Context* context, const ResourceVector& resources)
    : BaseClassName(context, resources, ea::span(properties.begin(), properties.end()))
    , generator_(MakeShared<SphericalHarmonicsGenerator>(context))
{
    SubscribeToEvent(E_BEGINRENDERING, &TextureCubeInspectorWidget::GeneratePendingSH);
}

TextureCubeInspectorWidget::~TextureCubeInspectorWidget()
{
}

void TextureCubeInspectorWidget::RenderContent()
{
    BaseClassName::RenderContent();

    if (ui::Button("Generate Spherical Harmonics"))
    {
        for (Resource* resource : GetResources())
            texturesToGenerateSH_.push_back(resource->GetName());
    }
}

void TextureCubeInspectorWidget::GeneratePendingSH()
{
    auto cache = GetSubsystem<ResourceCache>();
    for (const ea::string& textureName : texturesToGenerateSH_)
    {
        auto texture = cache->GetResource<TextureCube>(textureName);
        auto imageXml = cache->GetResource<XMLFile>(textureName);
        if (!texture || !imageXml)
        {
            URHO3D_LOGERROR("Cannot generate Spherical Harmonics for texture {}: must be valid XML file", textureName);
            continue;
        }
        generator_->Generate(texture, imageXml);
    }
    texturesToGenerateSH_.clear();
}

} // namespace Urho3D
