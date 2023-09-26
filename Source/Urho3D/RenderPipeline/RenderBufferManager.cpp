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

#include "../Precompiled.h"

#include "../Graphics/Geometry.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsUtils.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderSurface.h"
#include "../Graphics/Texture2D.h"
#include "../RenderPipeline/RenderBufferManager.h"
#include "../RenderPipeline/RenderPipelineDebugger.h"
#include "../RenderPipeline/RenderPipelineDefs.h"
#include "../RenderPipeline/ShaderConsts.h"
#include "../RenderAPI/DrawCommandQueue.h"
#include "../RenderAPI/RenderContext.h"
#include "../RenderAPI/RenderDevice.h"
#include "../RenderAPI/RenderAPIUtils.h"
#include "../RenderAPI/RenderScope.h"

#include <EASTL/optional.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

template <class T>
ea::span<const T> ToSpan(std::initializer_list<T> list)
{
    return { list.begin(), static_cast<eastl_size_t>(list.size()) };
}

Texture2D* GetParentTexture2D(RenderSurface* renderSurface)
{
    return renderSurface && renderSurface->GetParentTexture() ? renderSurface->GetParentTexture()->Cast<Texture2D>() : nullptr;
}

ea::optional<RenderSurface*> GetLinkedDepthStencil(RenderSurface* renderSurface)
{
    if (!renderSurface)
        return nullptr;
    if (RenderSurface* depthStencil = renderSurface->GetLinkedDepthStencil())
        return depthStencil;
    return ea::nullopt;
}

bool HasStencilBuffer(RenderSurface* renderSurface)
{
    // Assume that backbuffer always has stencil, otherwise we cannot do anything about it.
    if (!renderSurface)
        return true;

    const TextureFormat format = renderSurface->GetParentTexture()->GetFormat();
    return IsDepthStencilTextureFormat(format);
}

bool HasReadableDepth(RenderSurface* renderSurface)
{
    // Backbuffer depth is never readable
    if (!renderSurface)
        return false;

    return true;
}

TextureFormat GetColorTextureFormat(RenderPipelineColorSpace colorSpace, TextureFormat textureFormat)
{
    switch (colorSpace)
    {
    case RenderPipelineColorSpace::GammaLDR: return TextureFormat::TEX_FORMAT_RGBA8_UNORM;
    case RenderPipelineColorSpace::LinearLDR: return TextureFormat::TEX_FORMAT_RGBA8_UNORM_SRGB;
    case RenderPipelineColorSpace::LinearHDR: return TextureFormat::TEX_FORMAT_RGBA16_FLOAT;
    case RenderPipelineColorSpace::Optimized:
    default: return textureFormat;
    }
}

Vector4 CalculateViewportOffsetAndScale(
    RenderDevice* renderDevice, const IntVector2& textureSize, const IntRect& viewportRect)
{
    const Vector2 halfViewportScale = 0.5f * viewportRect.Size().ToVector2() / textureSize.ToVector2();
    const float xOffset = static_cast<float>(viewportRect.left_) / textureSize.x_ + halfViewportScale.x_;
    const float yOffset = static_cast<float>(viewportRect.top_) / textureSize.y_ + halfViewportScale.y_;
    // TODO: Do we want to flip this in OpenGL? It would make sense but it doesn't work.
    // const bool isOpenGL = renderDevice->GetBackend() == RenderBackend::OpenGL;
    // return { xOffset, 1.0f - yOffset, halfViewportScale.x_, halfViewportScale.y_ };
    return { xOffset, yOffset, halfViewportScale.x_, halfViewportScale.y_ };
}

GraphicsPipelineStateDesc GetClearPipelineStateDesc(Graphics* graphics, ClearTargetFlags flags)
{
    const ea::string shaderName = "v2/X_ClearFramebuffer";

    GraphicsPipelineStateDesc desc;
    desc.debugName_ = shaderName;
    desc.vertexShader_ = graphics->GetShader(VS, shaderName, "");
    desc.pixelShader_ = graphics->GetShader(PS, shaderName, "");

    desc.colorWriteEnabled_ = flags.Test(CLEAR_COLOR);
    desc.blendMode_ = BLEND_REPLACE;
    desc.depthWriteEnabled_ = flags.Test(CLEAR_DEPTH);
    desc.stencilWriteMask_ = flags.Test(CLEAR_STENCIL) ? 0xff : 0x00;
    desc.stencilOperationOnPassed_ = OP_REF;

    return desc;
}

}

RenderBufferManager::RenderBufferManager(RenderPipelineInterface* renderPipeline)
    : Object(renderPipeline->GetContext())
    , renderPipeline_(renderPipeline)
    , graphics_(GetSubsystem<Graphics>())
    , renderer_(GetSubsystem<Renderer>())
    , renderDevice_(GetSubsystem<RenderDevice>())
    , renderContext_(renderDevice_->GetRenderContext())
    , debugger_(renderPipeline_->GetDebugger())
    , drawQueue_(renderDevice_->GetDefaultQueue())
    , pipelineStates_(context_)
{
    // Order is important. RenderBufferManager should receive callbacks before any of render buffers
    renderPipeline_->OnPipelineStatesInvalidated.Subscribe(this, &RenderBufferManager::OnPipelineStatesInvalidated);
    renderPipeline_->OnRenderBegin.Subscribe(this, &RenderBufferManager::OnRenderBegin);
    renderPipeline_->OnRenderEnd.Subscribe(this, &RenderBufferManager::OnRenderEnd);

    viewportColorBuffer_ = MakeShared<ViewportColorRenderBuffer>(renderPipeline_);
    viewportDepthBuffer_ = MakeShared<ViewportDepthStencilRenderBuffer>(renderPipeline_);

    InitializePipelineStates();
}

RenderBufferManager::~RenderBufferManager()
{
}

void RenderBufferManager::SetSettings(const RenderBufferManagerSettings& settings)
{
    settings_ = settings;
}

void RenderBufferManager::SetFrameSettings(const RenderBufferManagerFrameSettings& frameSettings)
{
    frameSettings_ = frameSettings;
    if (frameSettings_.supportColorReadWrite_)
        frameSettings_.readableColor_ = true;
}

TextureFormat RenderBufferManager::GetOutputColorFormat() const
{
    return colorOutputParams_.textureFormat_;
}

TextureFormat RenderBufferManager::GetOutputDepthStencilFormat() const
{
    return static_cast<TextureFormat>(depthStencilOutputParams_.textureFormat_);
}

SharedPtr<RenderBuffer> RenderBufferManager::CreateColorBuffer(const RenderBufferParams& params, const Vector2& size)
{
    return MakeShared<TextureRenderBuffer>(renderPipeline_, params, size);
}

void RenderBufferManager::SwapColorBuffers(bool synchronizeContents)
{
    if (!frameSettings_.supportColorReadWrite_)
    {
        URHO3D_ASSERTLOG("Cannot call SwapColorBuffers if 'supportColorReadWrite' flag is not set");
        assert(0);
        return;
    }

    URHO3D_ASSERT(readableColorBuffer_ && writeableColorBuffer_);
    ea::swap(writeableColorBuffer_, readableColorBuffer_);

    if (synchronizeContents)
    {
        SetRenderTargets(depthStencilBuffer_, { writeableColorBuffer_ });
        DrawTexture("Synchronize readable and writeable color buffers", readableColorBuffer_->GetTexture());
    }
}

void RenderBufferManager::SetRenderTargetsRect(const IntRect& viewportRect,
    RenderBuffer* depthStencilBuffer, ea::span<RenderBuffer* const> colorBuffers, bool readOnlyDepth, CubeMapFace face)
{
    ea::optional<IntRect> defaultViewportRect;
    OptionalRawTextureRTV depthStencilRef;
    if (depthStencilBuffer)
    {
        depthStencilRef =
            readOnlyDepth ? depthStencilBuffer->GetReadOnlyDepthView(face) : depthStencilBuffer->GetView(face);
        defaultViewportRect = depthStencilBuffer->GetViewportRect();
    }

    ea::fixed_vector<RenderTargetView, MaxRenderTargets> colorRefs;
    for (RenderBuffer* colorBuffer : colorBuffers)
    {
        colorRefs.push_back(colorBuffer->GetView(face));
        if (!defaultViewportRect)
            defaultViewportRect = colorBuffer->GetViewportRect();
    }

    if (!defaultViewportRect || *defaultViewportRect == IntRect::ZERO)
    {
        URHO3D_LOGERROR("Cannot set null render targets");
        return;
    }

    renderContext_->SetRenderTargets(depthStencilRef, colorRefs);

    if (viewportRect == IntRect::ZERO)
        renderContext_->SetViewport(*defaultViewportRect);
    else
        renderContext_->SetViewport(viewportRect);
}

void RenderBufferManager::SetRenderTargets(RenderBuffer* depthStencilBuffer,
    ea::span<RenderBuffer* const> colorBuffers, bool readOnlyDepth, CubeMapFace face)
{
    SetRenderTargetsRect(IntRect::ZERO, depthStencilBuffer, colorBuffers, readOnlyDepth, face);
}

void RenderBufferManager::SetRenderTargetsRect(const IntRect& viewportRect, RenderBuffer* depthStencilBuffer,
    std::initializer_list<RenderBuffer*> colorBuffers, bool readOnlyDepth, CubeMapFace face)
{
    SetRenderTargetsRect(viewportRect, depthStencilBuffer, ToSpan(colorBuffers), readOnlyDepth, face);
}

void RenderBufferManager::SetRenderTargets(RenderBuffer* depthStencilBuffer,
    std::initializer_list<RenderBuffer*> colorBuffers, bool readOnlyDepth, CubeMapFace face)
{
    SetRenderTargetsRect(IntRect::ZERO, depthStencilBuffer, ToSpan(colorBuffers), readOnlyDepth, face);
}

void RenderBufferManager::SetOutputRenderTargetsRect(const IntRect& viewportRect, bool readOnlyDepth)
{
    SetRenderTargetsRect(viewportRect, GetDepthStencilOutput(), { GetColorOutput() }, readOnlyDepth);
}

void RenderBufferManager::SetOutputRenderTargets(bool readOnlyDepth)
{
    SetRenderTargetsRect(IntRect::ZERO, GetDepthStencilOutput(), { GetColorOutput() }, readOnlyDepth);
}

void RenderBufferManager::ClearDepthStencil(RenderBuffer* depthStencilBuffer,
    ClearTargetFlags flags, float depth, unsigned stencil, CubeMapFace face)
{
    const RenderTargetView depthStencil = depthStencilBuffer->GetView(face);
    renderContext_->SetRenderTargets(depthStencil, {});
    renderContext_->SetFullViewport();
    renderContext_->ClearDepthStencil(CLEAR_DEPTH | CLEAR_STENCIL, depth, stencil);
}

void RenderBufferManager::ClearColor(RenderBuffer* colorBuffer,
    const Color& color, CubeMapFace face)
{
    const RenderTargetView renderTargets[] = {colorBuffer->GetView(face)};
    renderContext_->SetRenderTargets(ea::nullopt, renderTargets);
    renderContext_->SetFullViewport();
    renderContext_->ClearRenderTarget(0, color);
}

void RenderBufferManager::ClearOutputRect(const IntRect& viewportRect, ClearTargetFlags flags,
    const Color& color, float depth, unsigned stencil)
{
    // Do it just in case because we cannot handle any noise below
    flags &= CLEAR_COLOR | CLEAR_DEPTH | CLEAR_STENCIL;

    SetRenderTargetsRect(viewportRect, GetDepthStencilOutput(), { GetColorOutput() });
    const IntRect fullViewportRect{IntVector2::ZERO, renderContext_->GetCurrentRenderTargetSize()};
    if (renderContext_->GetCurrentViewport() == fullViewportRect)
    {
        if (flags.Test(CLEAR_COLOR))
            renderContext_->ClearRenderTarget(0, color);
        if (flags.Test(CLEAR_DEPTH) || flags.Test(CLEAR_STENCIL))
            renderContext_->ClearDepthStencil(flags, depth, stencil);
    }
    else
    {
        const bool isOpenGL = renderContext_->GetRenderDevice()->GetBackend() == RenderBackend::OpenGL;
        const ShaderParameterDesc params[] = {
            {"Color", color},
            {"Depth", isOpenGL ? (depth * 2.0 - 1.0) : depth},
        };
        DrawViewportQuad("Clear output subregion", clearPipelineState_[flags.AsInteger()], {}, params);
    }
}

void RenderBufferManager::ClearOutput(ClearTargetFlags flags, const Color& color, float depth, unsigned stencil)
{
    ClearOutputRect(IntRect::ZERO, flags, color, depth, stencil);
}

void RenderBufferManager::ClearOutput(const Color& color, float depth, unsigned stencil)
{
    ClearOutput(CLEAR_COLOR | CLEAR_DEPTH | CLEAR_STENCIL, color, depth, stencil);
}

Vector4 RenderBufferManager::GetDefaultClipToUVSpaceOffsetAndScale() const
{
    const IntVector2 size = GetOutputSize();
    return CalculateViewportOffsetAndScale(renderDevice_, size, IntRect{ IntVector2::ZERO, size });
}

StaticPipelineStateId RenderBufferManager::CreateQuadPipelineState(GraphicsPipelineStateDesc desc)
{
    Geometry* quadGeometry = renderer_->GetQuadGeometry();

    InitializeInputLayoutAndPrimitiveType(desc, quadGeometry);
    desc.colorWriteEnabled_ = true;

    return pipelineStates_.CreateState(desc);
}

StaticPipelineStateId RenderBufferManager::CreateQuadPipelineState(BlendMode blendMode,
    const ea::string& shaderName, const ea::string& shaderDefines, ea::span<const NamedSamplerStateDesc> samplers)
{
    ea::string defines = shaderDefines;
    defines += " URHO3D_GEOMETRY_STATIC";

    GraphicsPipelineStateDesc desc;
    desc.debugName_ = Format("Quad with {}({})", shaderName, defines);
    desc.blendMode_ = blendMode;
    desc.vertexShader_ = graphics_->GetShader(VS, shaderName, defines);
    desc.pixelShader_ = graphics_->GetShader(PS, shaderName, defines);

    for (const auto& [name, samplerStateDesc] : samplers)
        desc.samplers_.Add(StringHash{name}, samplerStateDesc);

    return CreateQuadPipelineState(desc);
}

PipelineState* RenderBufferManager::GetQuadPipelineState(StaticPipelineStateId id)
{
    return pipelineStates_.GetState(id, renderContext_->GetCurrentRenderTargetsDesc());
}

void RenderBufferManager::DrawQuad(ea::string_view debugComment, const DrawQuadParams& params, bool flipVertical)
{
    PipelineState* pipelineState =
        params.pipelineState_ ? params.pipelineState_ : GetQuadPipelineState(params.pipelineStateId_);
    if (!pipelineState || !pipelineState->IsValid())
        return;

    const RenderScope renderScope(renderContext_, debugComment);

    Geometry* quadGeometry = renderer_->GetQuadGeometry();
    Matrix3x4 modelMatrix = Matrix3x4::IDENTITY;
    Matrix4 projection = Matrix4::IDENTITY;
    if (flipVertical)
        projection.m11_ = -1.0f;

    // OpenGL z range is [-1, 1] instead of [0, 1], draw quad at z = 0.5 for consistency
    const bool isOpenGL = renderDevice_->GetBackend() == RenderBackend::OpenGL;
    modelMatrix.m23_ = isOpenGL ? 0.0f : 0.5f;

    drawQueue_->Reset();
    drawQueue_->SetPipelineState(pipelineState);

    if (drawQueue_->BeginShaderParameterGroup(SP_FRAME))
    {
        drawQueue_->AddShaderParameter(ShaderConsts::Frame_DeltaTime, timeStep_);
        drawQueue_->CommitShaderParameterGroup(SP_FRAME);
    }

    if (drawQueue_->BeginShaderParameterGroup(SP_CAMERA))
    {
        drawQueue_->AddShaderParameter(ShaderConsts::Camera_GBufferOffsets, params.clipToUVOffsetAndScale_);
        drawQueue_->AddShaderParameter(ShaderConsts::Camera_GBufferInvSize, params.invInputSize_);
        drawQueue_->AddShaderParameter(ShaderConsts::Camera_ViewProj, projection);
        drawQueue_->CommitShaderParameterGroup(SP_CAMERA);
    }

    if (drawQueue_->BeginShaderParameterGroup(SP_OBJECT))
    {
        drawQueue_->AddShaderParameter(ShaderConsts::Object_Model, modelMatrix);
        drawQueue_->CommitShaderParameterGroup(SP_OBJECT);
    }

    if (drawQueue_->BeginShaderParameterGroup(SP_CUSTOM))
    {
        for (const ShaderParameterDesc& shaderParameter : params.parameters_)
            drawQueue_->AddShaderParameter(shaderParameter.name_, shaderParameter.value_);
        drawQueue_->CommitShaderParameterGroup(SP_CUSTOM);
    }

    if (params.bindSecondaryColorToDiffuse_)
    {
        if (RawTexture* secondaryColor = GetSecondaryColorTexture())
            drawQueue_->AddShaderResource(ShaderResources::Albedo, secondaryColor);
    }
    for (const ShaderResourceDesc& shaderResource : params.resources_)
    {
        const bool canBind = !params.bindSecondaryColorToDiffuse_ || shaderResource.name_ != ShaderResources::Albedo;
        if (canBind && shaderResource.texture_)
            drawQueue_->AddShaderResource(shaderResource.name_, shaderResource.texture_);
    }
    drawQueue_->CommitShaderResources();

    SetBuffersFromGeometry(*drawQueue_, quadGeometry);
    drawQueue_->DrawIndexed(quadGeometry->GetIndexStart(), quadGeometry->GetIndexCount());

    renderContext_->Execute(drawQueue_);

    if (RenderPipelineDebugger::IsSnapshotInProgress(debugger_))
    {
        debugger_->ReportQuad(debugComment, renderContext_->GetCurrentViewport().Size());
    }
}

void RenderBufferManager::DrawViewportQuad(ea::string_view debugComment,
    StaticPipelineStateId pipelineStateId, ea::span<const ShaderResourceDesc> resources,
    ea::span<const ShaderParameterDesc> parameters, bool flipVertical)
{
    DrawQuadParams params;
    params.pipelineStateId_ = pipelineStateId;
    params.clipToUVOffsetAndScale_ = GetDefaultClipToUVSpaceOffsetAndScale();
    params.invInputSize_ = GetInvOutputSize();
    params.resources_ = resources;
    params.parameters_ = parameters;
    DrawQuad(debugComment, params, flipVertical);
}

void RenderBufferManager::DrawFeedbackViewportQuad(ea::string_view debugComment,
    StaticPipelineStateId pipelineStateId, ea::span<const ShaderResourceDesc> resources,
    ea::span<const ShaderParameterDesc> parameters, bool flipVertical)
{
    DrawQuadParams params;
    params.pipelineStateId_ = pipelineStateId;
    params.clipToUVOffsetAndScale_ = GetDefaultClipToUVSpaceOffsetAndScale();
    params.invInputSize_ = GetInvOutputSize();
    params.bindSecondaryColorToDiffuse_ = true;
    params.resources_ = resources;
    params.parameters_ = parameters;
    DrawQuad(debugComment, params, flipVertical);
}

void RenderBufferManager::OnPipelineStatesInvalidated()
{
    pipelineStates_.Invalidate();
}

void RenderBufferManager::OnViewportDefined(RenderSurface* renderTarget, const IntRect& viewportRect)
{
    Texture2D* outputTexture = GetParentTexture2D(renderTarget);
    const ea::optional<RenderSurface*> outputDepthStencil = GetLinkedDepthStencil(renderTarget);

    const bool isBilinearFilteredOutput = outputTexture && outputTexture->GetFilterMode() != FILTER_NEAREST;
    const int outputMultiSample = RenderSurface::GetMultiSample(graphics_, renderTarget);
    const TextureFormat outputColorFormat = RenderSurface::GetColorFormat(graphics_, renderTarget);

    // Determine output format
    RenderBufferParams colorParams;
    colorParams.multiSampleLevel_ = settings_.inheritMultiSampleLevel_
        ? outputMultiSample : settings_.multiSampleLevel_;
    colorParams.textureFormat_ = GetColorTextureFormat(settings_.colorSpace_, outputColorFormat);
    colorParams.flags_.Set(RenderBufferFlag::BilinearFiltering, settings_.filteredColor_ || isBilinearFilteredOutput);

    RenderBufferParams depthParams = colorParams;
    depthParams.flags_ |= RenderBufferFlag::Persistent;
    depthParams.textureFormat_ = RenderSurface::GetDepthFormat(graphics_, outputDepthStencil.value_or(nullptr));

    if (colorOutputParams_ != colorParams || depthStencilOutputParams_ != depthParams)
    {
        colorOutputParams_ = colorParams;
        depthStencilOutputParams_ = depthParams;
        ResetCachedRenderBuffers();
    }

    // Only non-linear texture formats are ones that are explicitly non-sRGB,
    // or when it is explicitly specified for the texture.
    const bool isLinearTextureFormat =
        SetTextureFormatSRGB(colorOutputParams_.textureFormat_, true) == colorOutputParams_.textureFormat_;
    const bool isLinearTextureMetadata = outputTexture && outputTexture->GetLinear();
    linearColorSpace_ = isLinearTextureFormat || isLinearTextureMetadata;
}

void RenderBufferManager::OnRenderBegin(const CommonFrameInfo& frameInfo)
{
    timeStep_ = frameInfo.timeStep_;
    viewportRect_ = frameInfo.viewportRect_;

    // Get parameters of output render surface
    const TextureFormat outputFormat = RenderSurface::GetColorFormat(graphics_, frameInfo.renderTarget_);
    const int outputMultiSample = RenderSurface::GetMultiSample(graphics_, frameInfo.renderTarget_);

    const ea::optional<RenderSurface*> outputDepthStencil = GetLinkedDepthStencil(frameInfo.renderTarget_);
    const bool outputHasStencil = outputDepthStencil && HasStencilBuffer(*outputDepthStencil);
    const bool outputHasReadableDepth = outputDepthStencil && HasReadableDepth(*outputDepthStencil);

    Texture2D* outputTexture = GetParentTexture2D(frameInfo.renderTarget_);
    const bool isFullRectOutput =
        viewportRect_ == IntRect::ZERO || viewportRect_ == RenderSurface::GetRect(graphics_, frameInfo.renderTarget_);
    const bool isSimpleTextureOutput = outputTexture != nullptr && isFullRectOutput;
    const bool isBilinearFilteredOutput = outputTexture && outputTexture->GetFilterMode() != FILTER_NEAREST;

    // Check if need to allocate secondary color buffer, substitute primary color buffer or substitute depth buffer
    const bool needSimpleTexture = frameSettings_.readableColor_ || settings_.readableDepth_
        || settings_.colorUsableWithMultipleRenderTargets_;

    const bool isColorFormatMatching = outputFormat == colorOutputParams_.textureFormat_;
    const bool isMultiSampleMatching = outputMultiSample == colorOutputParams_.multiSampleLevel_;
    const bool isFilterMatching = isBilinearFilteredOutput == colorOutputParams_.flags_.Test(RenderBufferFlag::BilinearFiltering);
    const bool isColorUsageMatching = isSimpleTextureOutput || !needSimpleTexture;
    const bool isOutputMatching =
        isColorFormatMatching && isMultiSampleMatching && isFilterMatching && isColorUsageMatching;

    const bool needSecondaryBuffer = frameSettings_.supportColorReadWrite_;
    const bool needSubstituteDepthBuffer = !isMultiSampleMatching || !outputDepthStencil.has_value()
        || ((needSecondaryBuffer || !isOutputMatching) && outputDepthStencil.value() == nullptr)
        || (settings_.readableDepth_ && (!outputHasReadableDepth || !isSimpleTextureOutput))
        || (settings_.stencilBuffer_ && !outputHasStencil);
    const bool needSubstitutePrimaryBuffer = !isOutputMatching || needSubstituteDepthBuffer;

    // Allocate substitute buffers if necessary
    if (needSubstitutePrimaryBuffer && !substituteRenderBuffers_[0])
    {
        substituteRenderBuffers_[0] = MakeShared<TextureRenderBuffer>(renderPipeline_, colorOutputParams_);
    }
    if (needSecondaryBuffer && !substituteRenderBuffers_[1])
    {
        substituteRenderBuffers_[1] = MakeShared<TextureRenderBuffer>(renderPipeline_, colorOutputParams_);
    }
    if (needSubstituteDepthBuffer && !substituteDepthBuffer_)
    {
        substituteDepthBuffer_ = MakeShared<TextureRenderBuffer>(renderPipeline_, depthStencilOutputParams_);
    }

    depthStencilBuffer_ = needSubstituteDepthBuffer ? substituteDepthBuffer_.Get() : viewportDepthBuffer_.Get();
    writeableColorBuffer_ = needSubstitutePrimaryBuffer ? substituteRenderBuffers_[0].Get() : viewportColorBuffer_.Get();
    readableColorBuffer_ = needSecondaryBuffer ? substituteRenderBuffers_[1].Get() : nullptr;

    if (flipColorBuffersNextTime_ && readableColorBuffer_)
        ea::swap(writeableColorBuffer_, readableColorBuffer_);
}

void RenderBufferManager::OnRenderEnd(const CommonFrameInfo& frameInfo)
{
    if (writeableColorBuffer_ != viewportColorBuffer_.Get())
    {
        RawTexture* colorTexture = writeableColorBuffer_->GetTexture();
        CopyTextureRegion("Copy final color to output RenderSurface", colorTexture,
            IntRect{IntVector2::ZERO, colorTexture->GetParams().size_.ToIntVector2()}, viewportColorBuffer_->GetView(),
            viewportColorBuffer_->GetViewportRect(), ColorSpaceTransition::Automatic, false);

        // If viewport is reused for ping-ponging, optimize away final copy
        flipColorBuffersNextTime_ ^= viewportColorBuffer_ == readableColorBuffer_;
    }
}

void RenderBufferManager::ResetCachedRenderBuffers()
{
    substituteRenderBuffers_[0] = nullptr;
    substituteRenderBuffers_[1] = nullptr;
    substituteDepthBuffer_ = nullptr;
}

void RenderBufferManager::InitializePipelineStates()
{
    static const NamedSamplerStateDesc samplers[] = {{ShaderResources::Albedo, SamplerStateDesc::Bilinear()}};

    copyTexturePipelineState_ = CreateQuadPipelineState(BLEND_REPLACE, "v2/X_CopyFramebuffer", "", samplers);
    copyGammaToLinearTexturePipelineState_ =
        CreateQuadPipelineState(BLEND_REPLACE, "v2/X_CopyFramebuffer", "URHO3D_GAMMA_TO_LINEAR", samplers);
    copyLinearToGammaTexturePipelineState_ =
        CreateQuadPipelineState(BLEND_REPLACE, "v2/X_CopyFramebuffer", "URHO3D_LINEAR_TO_GAMMA", samplers);

    for (unsigned i = 0; i < MaxClearVariants; ++i)
    {
        const ClearTargetFlags flags{i};
        clearPipelineState_[i] = CreateQuadPipelineState(GetClearPipelineStateDesc(graphics_, flags));
    }
}

void RenderBufferManager::CopyTextureRegion(ea::string_view debugComment,
    RawTexture* sourceTexture, const IntRect& sourceRect,
    RenderTargetView destinationSurface, const IntRect& destinationRect, ColorSpaceTransition mode, bool flipVertical)
{
    renderContext_->SetRenderTargets(ea::nullopt, {&destinationSurface, 1});
    renderContext_->SetViewport(destinationRect);
    DrawTextureRegion(debugComment, sourceTexture, sourceRect, mode, flipVertical);
}

void RenderBufferManager::DrawTextureRegion(ea::string_view debugComment, RawTexture* sourceTexture,
    const IntRect& sourceRect, ColorSpaceTransition mode, bool flipVertical)
{
    if (sourceTexture->GetParams().type_ != TextureType::Texture2D)
    {
        URHO3D_LOGERROR("Draw texture is supported only for Texture2D");
        return;
    }

    const TextureFormat destinationFormat = renderContext_->GetCurrentRenderTargetsDesc().renderTargetFormats_[0];
    const bool isSRGBSource = IsTextureFormatSRGB(sourceTexture->GetParams().format_);
    const bool isSRGBDestination = IsTextureFormatSRGB(destinationFormat);

    DrawQuadParams callParams;

    if (mode == ColorSpaceTransition::None || isSRGBSource == isSRGBDestination)
        callParams.pipelineStateId_ = copyTexturePipelineState_;
    else if (isSRGBDestination)
        callParams.pipelineStateId_ = copyGammaToLinearTexturePipelineState_;
    else
        callParams.pipelineStateId_ = copyLinearToGammaTexturePipelineState_;

    const IntVector2 size = sourceTexture->GetParams().size_.ToIntVector2();
    callParams.invInputSize_ = Vector2::ONE / size.ToVector2();

    const IntRect effectiveSourceRect = sourceRect != IntRect::ZERO ? sourceRect : IntRect{IntVector2::ZERO, size};
    callParams.clipToUVOffsetAndScale_ = CalculateViewportOffsetAndScale(renderDevice_, size, effectiveSourceRect);

    const ShaderResourceDesc shaderResources[] = { { ShaderResources::Albedo, sourceTexture } };
    callParams.resources_ = shaderResources;

    DrawQuad(debugComment, callParams, flipVertical);
}

void RenderBufferManager::DrawTexture(ea::string_view debugComment, RawTexture* sourceTexture,
    ColorSpaceTransition mode, bool flipVertical)
{
    DrawTextureRegion(debugComment, sourceTexture, IntRect::ZERO, mode, flipVertical);
}

}
