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

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderPipeline/RenderPipeline.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/RenderPipeline/DefaultRenderPipeline.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/ResourceEvents.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

static const ResourceRef defaultRenderPath{RenderPath::TypeId, "RenderPaths/Default.renderpath"};

static const ea::vector<ea::string> colorSpaceNames = {"LDR Gamma", "LDR Linear", "HDR Linear", "Optimized"};

static const ea::vector<ea::string> materialQualityNames = {
    "Low",
    "Medium",
    "High",
};

static const ea::vector<ea::string> specularQualityNames = {
    "Disabled",
    "Simple",
    "Antialiased",
};

static const ea::vector<ea::string> reflectionQualityNames = {
    "Vertex",
    "Pixel",
};

static const ea::vector<ea::string> ambientModeNames = {
    "Constant",
    "Flat",
    "Directional",
};

static const ea::vector<ea::string> ssaoModeNames = {
    "Combine",
    "PreviewRaw",
    "PreviewBlurred",
};

static const ea::vector<ea::string> directLightingModeNames = {
    "Forward",
    "Deferred Blinn-Phong",
    "Deferred PBR",
};

} // namespace

RenderPipelineView::RenderPipelineView(RenderPipeline* renderPipeline)
    : Object(renderPipeline->GetContext())
    , renderPipeline_(renderPipeline)
    , graphics_(context_->GetSubsystem<Graphics>())
    , renderer_(context_->GetSubsystem<Renderer>())
{
}

RenderPipeline::RenderPipeline(Context* context)
    : Component(context)
{
    SetRenderPathAttr(defaultRenderPath);

    // Enable instancing by default for default render pipeline
    settings_.instancingBuffer_.enableInstancing_ = true;
    settings_.Validate();
    settings_.AdjustToSupported(context_);
}

RenderPipeline::~RenderPipeline()
{
}

void RenderPipeline::RegisterObject(Context* context)
{
    context->AddFactoryReflection<RenderPipeline>();

    // clang-format off
    URHO3D_ACCESSOR_ATTRIBUTE("Render Path", GetRenderPathAttr, SetRenderPathAttr, ResourceRef, defaultRenderPath, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Render Passes", GetRenderPassesAttr, SetRenderPassesAttr, VariantVector, Variant::emptyVariantVector, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Render Path Parameters", GetRenderPathParameters, SetRenderPathParameters, StringVariantMap, Variant::emptyVariantMap, AM_DEFAULT)
        .SetMetadata(AttributeMetadata::DynamicMetadata, true);

    URHO3D_ENUM_ATTRIBUTE_EX("Color Space", settings_.renderBufferManager_.colorSpace_, MarkSettingsDirty, colorSpaceNames, RenderPipelineColorSpace::GammaLDR, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE_EX("Material Quality", settings_.sceneProcessor_.materialQuality_, MarkSettingsDirty, materialQualityNames, SceneProcessorSettings{}.materialQuality_, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE_EX("Specular Quality", settings_.sceneProcessor_.specularQuality_, MarkSettingsDirty, specularQualityNames, SceneProcessorSettings{}.specularQuality_, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE_EX("Reflection Quality", settings_.sceneProcessor_.reflectionQuality_, MarkSettingsDirty, reflectionQualityNames, SceneProcessorSettings{}.reflectionQuality_, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Readable Depth", bool, settings_.renderBufferManager_.readableDepth_, MarkSettingsDirty, RenderBufferManagerSettings{}.readableDepth_, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Max Vertex Lights", unsigned, settings_.sceneProcessor_.maxVertexLights_, MarkSettingsDirty, DrawableProcessorSettings{}.maxVertexLights_, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Max Pixel Lights", unsigned, settings_.sceneProcessor_.maxPixelLights_, MarkSettingsDirty, DrawableProcessorSettings{}.maxPixelLights_, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE_EX("Ambient Mode", settings_.sceneProcessor_.ambientMode_, MarkSettingsDirty, ambientModeNames, DrawableAmbientMode::Directional, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Enable Instancing", bool, settings_.instancingBuffer_.enableInstancing_, MarkSettingsDirty, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Depth Pre-Pass", bool, settings_.sceneProcessor_.depthPrePass_, MarkSettingsDirty, false, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE_EX("Lighting Mode", settings_.sceneProcessor_.lightingMode_, MarkSettingsDirty, directLightingModeNames, DirectLightingMode::Forward, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Enable Shadows", bool, settings_.sceneProcessor_.enableShadows_, MarkSettingsDirty, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Cubemap Box Projection", bool, settings_.sceneProcessor_.cubemapBoxProjection_, MarkSettingsDirty, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("PCF Kernel Size", unsigned, settings_.sceneProcessor_.pcfKernelSize_, MarkSettingsDirty, 1, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Use Variance Shadow Maps", bool, settings_.shadowMapAllocator_.enableVarianceShadowMaps_, MarkSettingsDirty, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("VSM Shadow Settings", Vector2, settings_.sceneProcessor_.varianceShadowMapParams_, MarkSettingsDirty, BatchRendererSettings{}.varianceShadowMapParams_, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("VSM Multi Sample", unsigned, settings_.shadowMapAllocator_.varianceShadowMapMultiSample_, MarkSettingsDirty, 1, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("16-bit Shadow Maps", bool, settings_.shadowMapAllocator_.use16bitShadowMaps_, MarkSettingsDirty, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Draw Debug Geometry", bool, settings_.drawDebugGeometry_, MarkSettingsDirty, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Depth Bias Scale", float, settings_.shadowMapAllocator_.depthBiasScale_, MarkSettingsDirty, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Depth Bias Offset", float, settings_.shadowMapAllocator_.depthBiasOffset_, MarkSettingsDirty, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Normal Offset Scale", float, settings_.sceneProcessor_.normalOffsetScale_, MarkSettingsDirty, 1.0f, AM_DEFAULT);
    // clang-format on
}

void RenderPipeline::SetSettings(const RenderPipelineSettings& settings)
{
    settings_ = settings;
    settings_.Validate();
    settings_.AdjustToSupported(context_);
    MarkSettingsDirty();
}

ResourceRef RenderPipeline::GetRenderPathAttr() const
{
    return GetResourceRef(renderPath_, RenderPath::TypeId);
}

void RenderPipeline::SetRenderPathAttr(const ResourceRef& value)
{
    // This function may be called from constructor, check for cache just in case.
    if (auto* cache = GetSubsystem<ResourceCache>())
        SetRenderPath(cache->GetResource<RenderPath>(value.name_));
}

void RenderPipeline::SetRenderPath(RenderPath* renderPath)
{
    if (renderPath_)
        UnsubscribeFromEvent(renderPath_, E_RELOADFINISHED);

    renderPath_ = renderPath;
    OnRenderPathReloaded();

    if (renderPath_)
        SubscribeToEvent(renderPath_, E_RELOADFINISHED, &RenderPipeline::OnRenderPathReloaded);
}

void RenderPipeline::SetRenderPasses(const EnabledRenderPasses& renderPasses)
{
    renderPasses_ = renderPasses;
    if (renderPath_)
        renderPasses_ = renderPath_->RepairEnabledRenderPasses(renderPasses_);
    OnParametersChanged(this);
}

const VariantVector& RenderPipeline::GetRenderPassesAttr() const
{
    static thread_local VariantVector result;
    result.clear();
    for (const auto& [name, enabled] : renderPasses_)
    {
        result.push_back(Variant{name});
        result.push_back(Variant{enabled});
    }
    return result;
}

void RenderPipeline::SetRenderPassesAttr(const VariantVector& value)
{
    const unsigned numPasses = value.size() / 2;

    EnabledRenderPasses renderPasses;
    for (unsigned i = 0; i < numPasses; ++i)
        renderPasses.emplace_back(value[i * 2].GetString(), value[i * 2 + 1].GetBool());
    SetRenderPasses(renderPasses);
}

void RenderPipeline::SetRenderPathParameters(const StringVariantMap& params)
{
    renderPathParameters_ = params;
    OnParametersChanged(this);
}

void RenderPipeline::UpdateRenderPathParameters(const VariantMap& params)
{
    bool modified = false;
    for (const auto& [nameHash, newValue] : params)
    {
        const auto iter = renderPathParameters_.find_by_hash(nameHash.Value());
        if (iter == renderPathParameters_.end())
            continue;

        if (iter->second != newValue)
        {
            iter->second = newValue;
            modified = true;
        }
    }
    if (modified)
        OnParametersChanged(this);
}

void RenderPipeline::SetRenderPassEnabled(const ea::string& passName, bool enabled)
{
    const auto isMatching = [&](const ea::pair<ea::string, bool>& item) { return item.first == passName; };
    const auto iter = ea::find_if(renderPasses_.begin(), renderPasses_.end(), isMatching);

    bool isUpdated = false;
    if (iter == renderPasses_.end())
    {
        isUpdated = true;
        renderPasses_.emplace_back(passName, enabled);
        if (renderPath_)
            renderPasses_ = renderPath_->RepairEnabledRenderPasses(renderPasses_);
    }
    else if (iter->second != enabled)
    {
        isUpdated = true;
        iter->second = enabled;
    }

    if (isUpdated)
        OnParametersChanged(this);
}

SharedPtr<RenderPipelineView> RenderPipeline::Instantiate()
{
    return MakeShared<DefaultRenderPipelineView>(this);
}

void RenderPipeline::MarkSettingsDirty()
{
    settings_.Validate();
    settings_.AdjustToSupported(context_);
    OnSettingsChanged(this, settings_);
}

void RenderPipeline::OnRenderPathReloaded()
{
    if (renderPath_)
    {
        renderPath_->CollectParameters(renderPathParameters_);
        renderPasses_ = renderPath_->RepairEnabledRenderPasses(renderPasses_);
    }

    OnRenderPathChanged(this, renderPath_);
}

} // namespace Urho3D
