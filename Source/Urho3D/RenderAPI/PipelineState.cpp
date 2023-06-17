// Copyright (c) 2017-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/RenderAPI/PipelineState.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Thread.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/GAPIIncludes.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include <Diligent/Graphics/GraphicsEngine/interface/RenderDevice.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

void InitializeLayoutElementsMetadata(
    ea::vector<Diligent::LayoutElement>& result, ea::span<const InputLayoutElementDesc> vertexElements)
{
    static const unsigned numComponents[] = {
        1, // TYPE_INT
        1, // TYPE_FLOAT
        2, // TYPE_VECTOR2
        3, // TYPE_VECTOR3
        4, // TYPE_VECTOR4
        4, // TYPE_UBYTE4
        4, // TYPE_UBYTE4_NORM
    };

    static const Diligent::VALUE_TYPE valueTypes[] = {
        Diligent::VT_INT32, // TYPE_INT
        Diligent::VT_FLOAT32, // TYPE_FLOAT
        Diligent::VT_FLOAT32, // TYPE_VECTOR2
        Diligent::VT_FLOAT32, // TYPE_VECTOR3
        Diligent::VT_FLOAT32, // TYPE_VECTOR4
        Diligent::VT_UINT8, // TYPE_UBYTE4
        Diligent::VT_UINT8 // TYPE_UBYTE4_NORM
    };

    static const bool isNormalized[] = {
        false, // TYPE_INT
        false, // TYPE_FLOAT
        false, // TYPE_VECTOR2
        false, // TYPE_VECTOR3
        false, // TYPE_VECTOR4
        false, // TYPE_UBYTE4
        true // TYPE_UBYTE4_NORM
    };

    result.clear();
    result.reserve(vertexElements.size());

    for (const InputLayoutElementDesc& sourceElement : vertexElements)
    {
        Diligent::LayoutElement& destElement = result.emplace_back();

        destElement.InputIndex = M_MAX_UNSIGNED;
        destElement.RelativeOffset = sourceElement.elementOffset_;
        destElement.NumComponents = numComponents[sourceElement.elementType_];
        destElement.ValueType = valueTypes[sourceElement.elementType_];
        destElement.IsNormalized = isNormalized[sourceElement.elementType_];
        destElement.BufferSlot = sourceElement.bufferIndex_;
        destElement.Stride = sourceElement.bufferStride_;
        destElement.Frequency = sourceElement.instanceStepRate_ != 0 //
            ? Diligent::INPUT_ELEMENT_FREQUENCY_PER_INSTANCE
            : Diligent::INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
        destElement.InstanceDataStepRate = sourceElement.instanceStepRate_;
    }
}

void InitializeImmutableSampler(
    Diligent::ImmutableSamplerDesc& destSampler, const SamplerStateDesc& sourceSampler, const ea::string& samplerName)
{
    static const Diligent::FILTER_TYPE minMagFilter[][2] = {
        {Diligent::FILTER_TYPE_POINT, Diligent::FILTER_TYPE_COMPARISON_POINT}, // FILTER_NEAREST
        {Diligent::FILTER_TYPE_LINEAR, Diligent::FILTER_TYPE_COMPARISON_LINEAR}, // FILTER_BILINEAR
        {Diligent::FILTER_TYPE_LINEAR, Diligent::FILTER_TYPE_COMPARISON_LINEAR}, // FILTER_TRILINEAR
        {Diligent::FILTER_TYPE_ANISOTROPIC, Diligent::FILTER_TYPE_COMPARISON_ANISOTROPIC}, // FILTER_ANISOTROPIC
        {Diligent::FILTER_TYPE_POINT, Diligent::FILTER_TYPE_COMPARISON_POINT}, // FILTER_NEAREST_ANISOTROPIC
    };
    static const Diligent::FILTER_TYPE mipFilter[][2] = {
        {Diligent::FILTER_TYPE_POINT, Diligent::FILTER_TYPE_COMPARISON_POINT}, // FILTER_NEAREST
        {Diligent::FILTER_TYPE_POINT, Diligent::FILTER_TYPE_COMPARISON_POINT}, // FILTER_BILINEAR
        {Diligent::FILTER_TYPE_LINEAR, Diligent::FILTER_TYPE_COMPARISON_LINEAR}, // FILTER_TRILINEAR
        {Diligent::FILTER_TYPE_ANISOTROPIC, Diligent::FILTER_TYPE_COMPARISON_ANISOTROPIC}, // FILTER_ANISOTROPIC
        {Diligent::FILTER_TYPE_LINEAR, Diligent::FILTER_TYPE_LINEAR}, // FILTER_NEAREST_ANISOTROPIC
    };
    static const Diligent::TEXTURE_ADDRESS_MODE addressMode[] = {
        Diligent::TEXTURE_ADDRESS_WRAP, // ADDRESS_WRAP
        Diligent::TEXTURE_ADDRESS_MIRROR, // ADDRESS_MIRROR
        Diligent::TEXTURE_ADDRESS_CLAMP, // ADDRESS_CLAMP
        Diligent::TEXTURE_ADDRESS_BORDER // ADDRESS_BORDER
    };

    // TODO(diligent): Configure defaults
    const int anisotropy = sourceSampler.anisotropy_ ? sourceSampler.anisotropy_ : 4;
    const TextureFilterMode filterMode =
        sourceSampler.filterMode_ != FILTER_DEFAULT ? sourceSampler.filterMode_ : FILTER_TRILINEAR;

    destSampler.ShaderStages = Diligent::SHADER_TYPE_ALL_GRAPHICS;
    destSampler.SamplerOrTextureName = samplerName.c_str();
    destSampler.Desc.MinFilter = minMagFilter[filterMode][sourceSampler.shadowCompare_];
    destSampler.Desc.MagFilter = minMagFilter[filterMode][sourceSampler.shadowCompare_];
    destSampler.Desc.MipFilter = mipFilter[filterMode][sourceSampler.shadowCompare_];
    destSampler.Desc.AddressU = addressMode[sourceSampler.addressMode_[TextureCoordinate::U]];
    destSampler.Desc.AddressV = addressMode[sourceSampler.addressMode_[TextureCoordinate::V]];
    destSampler.Desc.AddressW = addressMode[sourceSampler.addressMode_[TextureCoordinate::W]];
    destSampler.Desc.MaxAnisotropy = anisotropy;
    destSampler.Desc.ComparisonFunc = Diligent::COMPARISON_FUNC_LESS_EQUAL;
    destSampler.Desc.MinLOD = -M_INFINITY;
    destSampler.Desc.MaxLOD = M_INFINITY;
    memcpy(&destSampler.Desc.BorderColor, sourceSampler.borderColor_.Data(), 4 * sizeof(float));
}

void InitializeImmutableSamplers(ea::vector<Diligent::ImmutableSamplerDesc>& result, const PipelineStateDesc& desc,
    const ShaderProgramReflection& reflection)
{
    static const auto defaultSampler = SamplerStateDesc::Bilinear();

    const ea::span<const StringHash> samplerNames{desc.samplers_.names_.data(), desc.samplers_.size_};
    for (const auto& [nameHash, resourceDesc] : reflection.GetShaderResources())
    {
        const auto iter = ea::find(samplerNames.begin(), samplerNames.end(), nameHash);
        const SamplerStateDesc* sourceSampler = &defaultSampler;
        if (iter != samplerNames.end())
        {
            const auto index = static_cast<unsigned>(iter - samplerNames.begin());
            sourceSampler = &desc.samplers_.desc_[index];
        }
        else
        {
            URHO3D_LOGWARNING("Default sampler is used for resource '{}'", resourceDesc.internalName_);
        }

        const ea::string& internalName = resourceDesc.internalName_;
        Diligent::ImmutableSamplerDesc& destSampler = result.emplace_back();
        InitializeImmutableSampler(destSampler, *sourceSampler, internalName);
    }
}

bool IsSameSematics(const InputLayoutElementDesc& lhs, const VertexShaderAttribute& rhs)
{
    return lhs.elementSemantic_ == rhs.semantic_ && lhs.elementSemanticIndex_ == rhs.semanticIndex_;
}

void FillLayoutElementIndices(ea::span<Diligent::LayoutElement> result,
    ea::span<const InputLayoutElementDesc> vertexElements, const VertexShaderAttributeVector& attributes)
{
    URHO3D_ASSERT(result.size() == vertexElements.size());

    const unsigned numExpectedAttributes = attributes.size();
    for (const VertexShaderAttribute& attribute : attributes)
    {
        // For each attribute, find the latest element in the layout that matches the attribute.
        // This is needed because the layout may contain multiple elements with the same semantic.
        const auto iter = ea::find(vertexElements.rbegin(), vertexElements.rend(), attribute, &IsSameSematics);
        if (iter != vertexElements.rend())
        {
            const unsigned elementIndex = vertexElements.rend() - iter - 1;
            result[elementIndex].InputIndex = attribute.inputIndex_;
        }
        else
        {
            URHO3D_LOGERROR("Attribute #{} with semantics '{}{}' is not found in the vertex layout",
                attribute.inputIndex_, ToShaderInputName(attribute.semantic_), attribute.semanticIndex_);
        }
    }
}

unsigned RemoveUnusedElements(ea::span<Diligent::LayoutElement> result)
{
    const auto isUnused = [](const Diligent::LayoutElement& element) { return element.InputIndex == M_MAX_UNSIGNED; };
    const auto iter = ea::remove_if(result.begin(), result.end(), isUnused);
    return iter - result.begin();
}

void InitializeLayoutElements(ea::vector<Diligent::LayoutElement>& result,
    ea::span<const InputLayoutElementDesc> vertexElements, const VertexShaderAttributeVector& vertexShaderAttributes)
{
    InitializeLayoutElementsMetadata(result, vertexElements);
    FillLayoutElementIndices(result, vertexElements, vertexShaderAttributes);
    const unsigned numElements = RemoveUnusedElements(result);
    result.resize(numElements);
}

#if GL_SUPPORTED || GLES_SUPPORTED

VertexShaderAttributeVector GetGLVertexAttributes(GLuint programObject)
{
    GLint numActiveAttribs = 0;
    GLint maxNameLength = 0;
    glGetProgramiv(programObject, GL_ACTIVE_ATTRIBUTES, &numActiveAttribs);
    glGetProgramiv(programObject, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength);

    ea::string attributeName;
    attributeName.resize(maxNameLength, '\0');

    VertexShaderAttributeVector result;
    for (int attribIndex = 0; attribIndex < numActiveAttribs; ++attribIndex)
    {
        GLint attributeSize = 0;
        GLenum attributeType = 0;
        glGetActiveAttrib(
            programObject, attribIndex, maxNameLength, nullptr, &attributeSize, &attributeType, attributeName.data());

        if (const auto element = ParseVertexAttribute(attributeName.c_str()))
        {
            const int location = glGetAttribLocation(programObject, attributeName.c_str());
            URHO3D_ASSERT(location != -1);

            result.push_back({element->semantic_, element->semanticIndex_, static_cast<unsigned>(location)});
        }
        else
        {
            URHO3D_LOGWARNING("Unknown vertex element semantic: {}", attributeName);
        }
    }

    return result;
}

#endif

} // namespace

PipelineState::PipelineState(PipelineStateCache* owner, const PipelineStateDesc& desc)
    : Object(owner->GetContext())
    , DeviceObject(owner->GetContext())
    , owner_(owner)
    , desc_(desc)
{
    // TODO(diligent): Get rid of copying the name
    SetDebugName(desc_.debugName_);
    CreateGPU();
}

PipelineState::~PipelineState()
{
    if (!Thread::IsMainThread())
    {
        URHO3D_LOGWARNING("Pipeline state should be released only from main thread");
        return;
    }

    if (PipelineStateCache* owner = owner_)
        owner->ReleasePipelineState(desc_);
}

void PipelineState::Invalidate()
{
    DestroyGPU();
}

void PipelineState::Restore()
{
    if (handle_)
        return;

    RestoreDependency(GetSubsystem<PipelineStateCache>());
    RestoreDependency(desc_.vertexShader_);
    RestoreDependency(desc_.pixelShader_);
    RestoreDependency(desc_.geometryShader_);
    RestoreDependency(desc_.hullShader_);
    RestoreDependency(desc_.domainShader_);

    CreateGPU();
}

void PipelineState::Destroy()
{
    DestroyGPU();
}

void PipelineState::CreateGPU()
{
    static const Diligent::PRIMITIVE_TOPOLOGY primitiveTopology[] = {
        Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // TRIANGLE_LIST
        Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST, // LINE_LIST
        Diligent::PRIMITIVE_TOPOLOGY_POINT_LIST, // POINT_LIST
        Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // TRIANGLE_STRIP
        Diligent::PRIMITIVE_TOPOLOGY_LINE_STRIP, // LINE_STRIP
        Diligent::PRIMITIVE_TOPOLOGY_UNDEFINED // TRIANGLE_FAN (not supported)
    };

    static const Diligent::COMPARISON_FUNCTION comparisonFunction[] = {
        Diligent::COMPARISON_FUNC_ALWAYS, // CMP_ALWAYS
        Diligent::COMPARISON_FUNC_EQUAL, // CMP_EQUAL
        Diligent::COMPARISON_FUNC_NOT_EQUAL, // CMP_NOTEQUAL
        Diligent::COMPARISON_FUNC_LESS, // CMP_LESS
        Diligent::COMPARISON_FUNC_LESS_EQUAL, // CMP_LESSEQUAL
        Diligent::COMPARISON_FUNC_GREATER, // CMP_GREATER
        Diligent::COMPARISON_FUNC_GREATER_EQUAL // CMP_GREATEREQUAL
    };

    static const bool isBlendEnabled[] = {
        false, // BLEND_REPLACE
        true, // BLEND_ADD
        true, // BLEND_MULTIPLY
        true, // BLEND_ALPHA
        true, // BLEND_ADDALPHA
        true, // BLEND_PREMULALPHA
        true, // BLEND_INVDESTALPHA
        true, // BLEND_SUBTRACT
        true, // BLEND_SUBTRACTALPHA
        true, // BLEND_DEFERRED_DECAL
    };

    static const Diligent::BLEND_FACTOR sourceBlend[] = {
        Diligent::BLEND_FACTOR_ONE, // BLEND_REPLACE
        Diligent::BLEND_FACTOR_ONE, // BLEND_ADD
        Diligent::BLEND_FACTOR_DEST_COLOR, // BLEND_MULTIPLY
        Diligent::BLEND_FACTOR_SRC_ALPHA, // BLEND_ALPHA
        Diligent::BLEND_FACTOR_SRC_ALPHA, // BLEND_ADDALPHA
        Diligent::BLEND_FACTOR_ONE, // BLEND_PREMULALPHA
        Diligent::BLEND_FACTOR_INV_DEST_ALPHA, // BLEND_INVDESTALPHA
        Diligent::BLEND_FACTOR_ONE, // BLEND_SUBTRACT
        Diligent::BLEND_FACTOR_SRC_ALPHA, // BLEND_SUBTRACTALPHA
        Diligent::BLEND_FACTOR_SRC_ALPHA, // BLEND_DEFERRED_DECAL
    };

    static const Diligent::BLEND_FACTOR destBlend[] = {
        Diligent::BLEND_FACTOR_ZERO, // BLEND_REPLACE
        Diligent::BLEND_FACTOR_ONE, // BLEND_ADD
        Diligent::BLEND_FACTOR_ZERO, // BLEND_MULTIPLY
        Diligent::BLEND_FACTOR_INV_SRC_ALPHA, // BLEND_ALPHA
        Diligent::BLEND_FACTOR_ONE, // BLEND_ADDALPHA
        Diligent::BLEND_FACTOR_INV_SRC_ALPHA, // BLEND_PREMULALPHA
        Diligent::BLEND_FACTOR_DEST_ALPHA, // BLEND_INVDESTALPHA
        Diligent::BLEND_FACTOR_ONE, // BLEND_SUBTRACT
        Diligent::BLEND_FACTOR_ONE, // BLEND_SUBTRACTALPHA
        Diligent::BLEND_FACTOR_INV_SRC_ALPHA, // BLEND_DEFERRED_DECAL
    };

    static const Diligent::BLEND_FACTOR sourceAlphaBlend[] = {
        Diligent::BLEND_FACTOR_ONE, // BLEND_REPLACE
        Diligent::BLEND_FACTOR_ONE, // BLEND_ADD
        Diligent::BLEND_FACTOR_DEST_COLOR, // BLEND_MULTIPLY
        Diligent::BLEND_FACTOR_SRC_ALPHA, // BLEND_ALPHA
        Diligent::BLEND_FACTOR_SRC_ALPHA, // BLEND_ADDALPHA
        Diligent::BLEND_FACTOR_ONE, // BLEND_PREMULALPHA
        Diligent::BLEND_FACTOR_INV_DEST_ALPHA, // BLEND_INVDESTALPHA
        Diligent::BLEND_FACTOR_ONE, // BLEND_SUBTRACT
        Diligent::BLEND_FACTOR_SRC_ALPHA, // BLEND_SUBTRACTALPHA
        Diligent::BLEND_FACTOR_ZERO, // BLEND_DEFERRED_DECAL
    };

    static const Diligent::BLEND_FACTOR destAlphaBlend[] = {
        Diligent::BLEND_FACTOR_ZERO, // BLEND_REPLACE
        Diligent::BLEND_FACTOR_ONE, // BLEND_ADD
        Diligent::BLEND_FACTOR_ZERO, // BLEND_MULTIPLY
        Diligent::BLEND_FACTOR_INV_SRC_ALPHA, // BLEND_ALPHA
        Diligent::BLEND_FACTOR_ONE, // BLEND_ADDALPHA
        Diligent::BLEND_FACTOR_INV_SRC_ALPHA, // BLEND_PREMULALPHA
        Diligent::BLEND_FACTOR_DEST_ALPHA, // BLEND_INVDESTALPHA
        Diligent::BLEND_FACTOR_ONE, // BLEND_SUBTRACT
        Diligent::BLEND_FACTOR_ONE, // BLEND_SUBTRACTALPHA
        Diligent::BLEND_FACTOR_ONE, // BLEND_DEFERRED_DECAL
    };

    static const Diligent::BLEND_OPERATION blendOperation[] = {
        Diligent::BLEND_OPERATION_ADD, // BLEND_REPLACE
        Diligent::BLEND_OPERATION_ADD, // BLEND_ADD
        Diligent::BLEND_OPERATION_ADD, // BLEND_MULTIPLY
        Diligent::BLEND_OPERATION_ADD, // BLEND_ALPHA
        Diligent::BLEND_OPERATION_ADD, // BLEND_ADDALPHA
        Diligent::BLEND_OPERATION_ADD, // BLEND_PREMULALPHA
        Diligent::BLEND_OPERATION_ADD, // BLEND_INVDESTALPHA
        Diligent::BLEND_OPERATION_REV_SUBTRACT, // BLEND_SUBTRACT
        Diligent::BLEND_OPERATION_REV_SUBTRACT, // BLEND_SUBTRACTALPHA
        Diligent::BLEND_OPERATION_ADD, // BLEND_DEFERRED_DECAL
    };

    static const Diligent::STENCIL_OP stencilOp[] = {
        Diligent::STENCIL_OP_KEEP, // OP_KEEP
        Diligent::STENCIL_OP_ZERO, // OP_ZERO
        Diligent::STENCIL_OP_REPLACE, // OP_REF
        Diligent::STENCIL_OP_INCR_WRAP, // OP_INCR
        Diligent::STENCIL_OP_DECR_WRAP, // OP_DECR
    };

    static const Diligent::CULL_MODE cullMode[] = {
        Diligent::CULL_MODE_NONE, // CULL_NONE
        Diligent::CULL_MODE_BACK, // CULL_CCW
        Diligent::CULL_MODE_FRONT // CULL_CW
    };

    static const Diligent::FILL_MODE fillMode[] = {

        Diligent::FILL_MODE_SOLID, // FILL_SOLID
        Diligent::FILL_MODE_WIREFRAME, // FILL_WIREFRAME
        Diligent::FILL_MODE_WIREFRAME, // FILL_POINT (not supported)
        // Point fill mode not supported
    };

    Diligent::IRenderDevice* renderDevice = renderDevice_->GetRenderDevice();
    const bool isOpenGL = renderDevice_->GetBackend() == RenderBackend::OpenGL;
    const bool hasSeparableShaderPrograms = renderDevice->GetDeviceInfo().Features.SeparablePrograms;
    URHO3D_ASSERT(isOpenGL || hasSeparableShaderPrograms);
    const ea::span<const InputLayoutElementDesc> vertexElements{
        desc_.inputLayout_.elements_.data(), desc_.inputLayout_.size_};

    Diligent::GraphicsPipelineStateCreateInfo ci;

    ea::vector<Diligent::LayoutElement> layoutElements;
    ea::vector<Diligent::ImmutableSamplerDesc> immutableSamplers;

    Diligent::IShader* vertexShader = desc_.vertexShader_ ? desc_.vertexShader_->GetHandle() : nullptr;
    Diligent::IShader* pixelShader = desc_.pixelShader_ ? desc_.pixelShader_->GetHandle() : nullptr;
    Diligent::IShader* domainShader = desc_.domainShader_ ? desc_.domainShader_->GetHandle() : nullptr;
    Diligent::IShader* hullShader = desc_.hullShader_ ? desc_.hullShader_->GetHandle() : nullptr;
    Diligent::IShader* geometryShader = desc_.geometryShader_ ? desc_.geometryShader_->GetHandle() : nullptr;

    // TODO(diligent): Revisit
    for (RawShader* shader :
        {desc_.vertexShader_, desc_.pixelShader_, desc_.domainShader_, desc_.hullShader_, desc_.geometryShader_})
    {
        if (shader)
            shader->OnReloaded.Subscribe(this, &PipelineState::Invalidate);
    }

    // On OpenGL, vertex layout initialization is postponed until the program is linked.
    if (!isOpenGL)
    {
        const VertexShaderAttributeVector& vertexShaderAttributes =
            desc_.vertexShader_->GetBytecode().vertexAttributes_;
        InitializeLayoutElements(layoutElements, vertexElements, vertexShaderAttributes);
        ci.GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
        ci.GraphicsPipeline.InputLayout.LayoutElements = layoutElements.data();
    }

    // On OpenGL, uniform layout initialization may be postponed.
    if (hasSeparableShaderPrograms)
    {
        Diligent::IShader* const shaders[] = {vertexShader, pixelShader, domainShader, hullShader, geometryShader};
        reflection_ = MakeShared<ShaderProgramReflection>(shaders);

        InitializeImmutableSamplers(immutableSamplers, desc_, *reflection_);
        ci.PSODesc.ResourceLayout.NumImmutableSamplers = immutableSamplers.size();
        ci.PSODesc.ResourceLayout.ImmutableSamplers = immutableSamplers.data();
    }

#ifdef URHO3D_DEBUG
    const ea::string debugName = Format("{}#{}", desc_.debugName_, desc_.ToHash());
    ci.PSODesc.Name = debugName.c_str();
#endif

    ci.GraphicsPipeline.PrimitiveTopology = primitiveTopology[desc_.primitiveType_];

    ci.GraphicsPipeline.NumRenderTargets = desc_.output_.numRenderTargets_;
    for (unsigned i = 0; i < desc_.output_.numRenderTargets_; ++i)
        ci.GraphicsPipeline.RTVFormats[i] = desc_.output_.renderTargetFormats_[i];
    ci.GraphicsPipeline.DSVFormat = desc_.output_.depthStencilFormat_;

    ci.pVS = vertexShader;
    ci.pPS = pixelShader;
    ci.pDS = domainShader;
    ci.pHS = hullShader;
    ci.pGS = geometryShader;

    ci.GraphicsPipeline.BlendDesc.AlphaToCoverageEnable = desc_.alphaToCoverageEnabled_;
    ci.GraphicsPipeline.BlendDesc.IndependentBlendEnable = false;
    if (ci.GraphicsPipeline.NumRenderTargets > 0)
    {
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = isBlendEnabled[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlend = sourceBlend[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlend = destBlend[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOp = blendOperation[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlendAlpha = sourceAlphaBlend[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlendAlpha = destAlphaBlend[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOpAlpha = blendOperation[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].RenderTargetWriteMask =
            desc_.colorWriteEnabled_ ? Diligent::COLOR_MASK_ALL : Diligent::COLOR_MASK_NONE;
    }

    ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
    ci.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = desc_.depthWriteEnabled_;
    ci.GraphicsPipeline.DepthStencilDesc.DepthFunc = comparisonFunction[desc_.depthCompareFunction_];
    ci.GraphicsPipeline.DepthStencilDesc.StencilEnable = desc_.stencilTestEnabled_;
    ci.GraphicsPipeline.DepthStencilDesc.StencilReadMask = desc_.stencilCompareMask_;
    ci.GraphicsPipeline.DepthStencilDesc.StencilWriteMask = desc_.stencilWriteMask_;
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFailOp = stencilOp[desc_.stencilOperationOnStencilFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilDepthFailOp = stencilOp[desc_.stencilOperationOnDepthFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilPassOp = stencilOp[desc_.stencilOperationOnPassed_];
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFunc = comparisonFunction[desc_.stencilCompareFunction_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFailOp = stencilOp[desc_.stencilOperationOnStencilFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilDepthFailOp = stencilOp[desc_.stencilOperationOnDepthFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilPassOp = stencilOp[desc_.stencilOperationOnPassed_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFunc = comparisonFunction[desc_.stencilCompareFunction_];

    unsigned depthBits = 24;
    if (ci.GraphicsPipeline.DSVFormat == Diligent::TEX_FORMAT_R16_TYPELESS)
        depthBits = 16;
    const int scaledDepthBias = isOpenGL ? 0 : (int)(desc_.constantDepthBias_ * (1 << depthBits));

    ci.GraphicsPipeline.RasterizerDesc.FillMode = fillMode[desc_.fillMode_];
    ci.GraphicsPipeline.RasterizerDesc.CullMode = cullMode[desc_.cullMode_];
    ci.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = false;
    ci.GraphicsPipeline.RasterizerDesc.DepthBias = scaledDepthBias;
    ci.GraphicsPipeline.RasterizerDesc.SlopeScaledDepthBias = desc_.slopeScaledDepthBias_;
    ci.GraphicsPipeline.RasterizerDesc.DepthClipEnable = true;
    ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = desc_.scissorTestEnabled_;
    ci.GraphicsPipeline.RasterizerDesc.AntialiasedLineEnable = !isOpenGL && desc_.lineAntiAlias_;

    ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = desc_.scissorTestEnabled_;

    ci.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

    PipelineStateCache* psoCache = GetSubsystem<PipelineStateCache>();
    ci.pPSOCache = psoCache->GetHandle();

#if GL_SUPPORTED || GLES_SUPPORTED
    auto patchInputLayout = [&](GLuint* programObjects, Diligent::Uint32 numPrograms)
    {
        const auto vertexShaderAttributes = GetGLVertexAttributes(programObjects[0]);

        InitializeLayoutElements(layoutElements, vertexElements, vertexShaderAttributes);
        ci.GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
        ci.GraphicsPipeline.InputLayout.LayoutElements = layoutElements.data();

        if (!hasSeparableShaderPrograms)
        {
            reflection_ = MakeShared<ShaderProgramReflection>(programObjects[0]);

            InitializeImmutableSamplers(immutableSamplers, desc_, *reflection_);
            ci.PSODesc.ResourceLayout.NumImmutableSamplers = immutableSamplers.size();
            ci.PSODesc.ResourceLayout.ImmutableSamplers = immutableSamplers.data();
        }
    };

    ci.GLProgramLinkedCallbackUserData = &patchInputLayout;
    ci.GLProgramLinkedCallback =
        [](Diligent::Uint32* programObjects, Diligent::Uint32 numProgramObjects, void* userData)
    {
        const auto& callback = *reinterpret_cast<decltype(patchInputLayout)*>(userData);
        callback(programObjects, numProgramObjects);
    };
#endif

    renderDevice->CreateGraphicsPipelineState(ci, &handle_);

    if (!handle_)
    {
        URHO3D_LOGERROR("Failed to create PipelineState '{}'", GetDebugName());
        return;
    }

    handle_->CreateShaderResourceBinding(&shaderResourceBinding_, true);
    reflection_->ConnectToShaderVariables(shaderResourceBinding_);
}

void PipelineState::DestroyGPU()
{
    handle_ = nullptr;
    shaderResourceBinding_ = nullptr;
    reflection_ = nullptr;
}

PipelineStateCache::PipelineStateCache(Context* context)
    : Object(context)
    , DeviceObject(context)
{
}

void PipelineStateCache::Initialize(const ByteVector& cachedData)
{
    cachedData_ = cachedData;

    Diligent::PipelineStateCacheCreateInfo ci;
    ci.Desc.Name = "PipelineStateCache";
    ci.CacheDataSize = cachedData_.size();
    ci.pCacheData = cachedData_.data();

    renderDevice_->GetRenderDevice()->CreatePipelineStateCache(ci, &handle_);
    if (!handle_)
        URHO3D_LOGERROR("Failed to create GPU Pipeline State Cache.");
    else
        URHO3D_LOGDEBUG("GPU Pipeline State Cache has been created.");
}

const ByteVector& PipelineStateCache::GetCachedData()
{
    UpdateCachedData();
    return cachedData_;
}

SharedPtr<PipelineState> PipelineStateCache::GetPipelineState(PipelineStateDesc desc)
{
    if (!desc.IsInitialized())
        return nullptr;

    desc.RecalculateHash();

    WeakPtr<PipelineState>& weakPipelineState = states_[desc];
    SharedPtr<PipelineState> pipelineState = weakPipelineState.Lock();
    if (!pipelineState)
    {
        pipelineState = MakeShared<PipelineState>(this, desc);
        weakPipelineState = pipelineState;
    }
    return pipelineState;
}

void PipelineStateCache::ReleasePipelineState(const PipelineStateDesc& desc)
{
    if (states_.erase(desc) != 1)
        URHO3D_LOGERROR("Unexpected call of PipelineStateCache::ReleasePipelineState");
}

void PipelineStateCache::UpdateCachedData()
{
    cachedData_.clear();
    if (handle_)
    {
        Diligent::RefCntAutoPtr<Diligent::IDataBlob> blob;
        handle_->GetData(&blob);

        if (blob)
        {
            cachedData_.resize(blob->GetSize());
            memcpy(cachedData_.data(), blob->GetDataPtr(), blob->GetSize());
        }
    }
}

} // namespace Urho3D
