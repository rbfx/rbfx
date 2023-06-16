#include "../PipelineState.h"
#include "../RenderSurface.h"
#include "../Shader.h"
#include "DiligentGraphicsImpl.h"
#include "DiligentLookupSettings.h"

#include "Urho3D/Core/Macros.h"
#include "Urho3D/Core/StringUtils.h"
#include "Urho3D/RenderAPI/GAPIIncludes.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"

#include <Diligent/Graphics/GraphicsEngine/interface/PipelineState.h>

#include <EASTL/optional.h>
#include <EASTL/span.h>

namespace Urho3D
{

namespace
{

void InitializeLayoutElementsMetadata(
    ea::vector<Diligent::LayoutElement>& result, ea::span<const VertexElementInBuffer> vertexElements)
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

    for (const VertexElementInBuffer& sourceElement : vertexElements)
    {
        Diligent::LayoutElement& destElement = result.emplace_back();

        destElement.InputIndex = M_MAX_UNSIGNED;
        destElement.RelativeOffset = sourceElement.offset_;
        destElement.NumComponents = numComponents[sourceElement.type_];
        destElement.ValueType = valueTypes[sourceElement.type_];
        destElement.IsNormalized = isNormalized[sourceElement.type_];
        destElement.BufferSlot = sourceElement.bufferIndex_;
        destElement.Stride = sourceElement.bufferStride_;
        // TODO(xr): Patch this place
        destElement.Frequency =
            sourceElement.perInstance_ ? INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
    }
}

void InitializeImmutableSampler(
    Diligent::ImmutableSamplerDesc& destSampler, const SamplerStateDesc& sourceSampler, const ea::string& samplerName)
{
    static const Diligent::FILTER_TYPE minMagFilter[][2] = {
        {FILTER_TYPE_POINT, FILTER_TYPE_COMPARISON_POINT}, // FILTER_NEAREST
        {FILTER_TYPE_LINEAR, FILTER_TYPE_COMPARISON_LINEAR}, // FILTER_BILINEAR
        {FILTER_TYPE_LINEAR, FILTER_TYPE_COMPARISON_LINEAR}, // FILTER_TRILINEAR
        {FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_COMPARISON_ANISOTROPIC}, // FILTER_ANISOTROPIC
        {FILTER_TYPE_POINT, FILTER_TYPE_COMPARISON_POINT}, // FILTER_NEAREST_ANISOTROPIC
    };
    static const Diligent::FILTER_TYPE mipFilter[][2] = {
        {FILTER_TYPE_POINT, FILTER_TYPE_COMPARISON_POINT}, // FILTER_NEAREST
        {FILTER_TYPE_POINT, FILTER_TYPE_COMPARISON_POINT}, // FILTER_BILINEAR
        {FILTER_TYPE_LINEAR, FILTER_TYPE_COMPARISON_LINEAR}, // FILTER_TRILINEAR
        {FILTER_TYPE_ANISOTROPIC, FILTER_TYPE_COMPARISON_ANISOTROPIC}, // FILTER_ANISOTROPIC
        {FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR}, // FILTER_NEAREST_ANISOTROPIC
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

    destSampler.ShaderStages = SHADER_TYPE_ALL_GRAPHICS;
    destSampler.SamplerOrTextureName = samplerName.c_str();
    destSampler.Desc.MinFilter = minMagFilter[filterMode][sourceSampler.shadowCompare_];
    destSampler.Desc.MagFilter = minMagFilter[filterMode][sourceSampler.shadowCompare_];
    destSampler.Desc.MipFilter = mipFilter[filterMode][sourceSampler.shadowCompare_];
    destSampler.Desc.AddressU = addressMode[sourceSampler.addressMode_[TextureCoordinate::U]];
    destSampler.Desc.AddressV = addressMode[sourceSampler.addressMode_[TextureCoordinate::V]];
    destSampler.Desc.AddressW = addressMode[sourceSampler.addressMode_[TextureCoordinate::W]];
    destSampler.Desc.MaxAnisotropy = anisotropy;
    destSampler.Desc.ComparisonFunc = COMPARISON_FUNC_LESS_EQUAL;
    destSampler.Desc.MinLOD = -M_INFINITY;
    destSampler.Desc.MaxLOD = M_INFINITY;
    memcpy(&destSampler.Desc.BorderColor, sourceSampler.borderColor_.Data(), 4 * sizeof(float));
}

void InitializeImmutableSamplers(ea::vector<Diligent::ImmutableSamplerDesc>& result, const PipelineStateDesc& desc,
    const ShaderProgramReflection& reflection)
{
    static const auto defaultSampler = SamplerStateDesc::Bilinear();

    const auto samplerNames = desc.GetSamplerNames();
    for (const auto& [nameHash, resourceDesc] : reflection.GetShaderResources())
    {
        const auto iter = ea::find(samplerNames.begin(), samplerNames.end(), nameHash);
        const SamplerStateDesc* sourceSampler = &defaultSampler;
        if (iter != samplerNames.end())
        {
            const auto index = static_cast<unsigned>(iter - samplerNames.begin());
            sourceSampler = &desc.samplers_[index];
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

bool IsSameSematics(const VertexElementInBuffer& lhs, const VertexShaderAttribute& rhs)
{
    return lhs.semantic_ == rhs.semantic_ && lhs.index_ == rhs.semanticIndex_;
}

void FillLayoutElementIndices(ea::span<Diligent::LayoutElement> result,
    ea::span<const VertexElementInBuffer> vertexElements, const VertexShaderAttributeVector& attributes)
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
    ea::span<const VertexElementInBuffer> vertexElements, const VertexShaderAttributeVector& vertexShaderAttributes)
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
        glGetActiveAttrib(programObject, attribIndex, maxNameLength, nullptr, &attributeSize, &attributeType,
            attributeName.data());

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

}

using namespace Diligent;
bool PipelineState::BuildPipeline(Graphics* graphics)
{
    Diligent::IRenderDevice* renderDevice = graphics->GetImpl()->GetDevice();
    const bool isOpenGL = graphics->GetRenderBackend() == RenderBackend::OpenGL;
    const bool hasSeparableShaderPrograms = renderDevice->GetDeviceInfo().Features.SeparablePrograms;
    URHO3D_ASSERT(isOpenGL || hasSeparableShaderPrograms);

    GraphicsPipelineStateCreateInfo ci;

    ea::vector<LayoutElement> layoutElements;
    ea::vector<Diligent::ImmutableSamplerDesc> immutableSamplers;

    Diligent::IShader* vertexShader = desc_.vertexShader_ ? desc_.vertexShader_->GetHandle() : nullptr;
    Diligent::IShader* pixelShader = desc_.pixelShader_ ? desc_.pixelShader_->GetHandle() : nullptr;
    Diligent::IShader* domainShader = desc_.domainShader_ ? desc_.domainShader_->GetHandle() : nullptr;
    Diligent::IShader* hullShader = desc_.hullShader_ ? desc_.hullShader_->GetHandle() : nullptr;
    Diligent::IShader* geometryShader = desc_.geometryShader_ ? desc_.geometryShader_->GetHandle() : nullptr;

    // TODO(diligent): Revisit
    for (RawShader* shader : {desc_.vertexShader_, desc_.pixelShader_, desc_.domainShader_, desc_.hullShader_, desc_.geometryShader_})
    {
        if (shader)
            shader->OnReloaded.Subscribe(this, &PipelineState::ResetCachedState);
    }

    // On OpenGL, vertex layout initialization is postponed until the program is linked.
    if (!isOpenGL)
    {
        const VertexShaderAttributeVector& vertexShaderAttributes =
            desc_.vertexShader_->GetBytecode().vertexAttributes_;
        InitializeLayoutElements(layoutElements, desc_.GetVertexElements(), vertexShaderAttributes);
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

    ci.GraphicsPipeline.PrimitiveTopology = DiligentPrimitiveTopology[desc_.primitiveType_];

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
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = DiligentBlendEnable[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlend = DiligentSrcBlend[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlend = DiligentDestBlend[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOp = DiligentBlendOp[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlendAlpha = DiligentSrcAlphaBlend[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlendAlpha = DiligentDestAlphaBlend[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOpAlpha = DiligentBlendOp[desc_.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].RenderTargetWriteMask =
            desc_.colorWriteEnabled_ ? COLOR_MASK_ALL : COLOR_MASK_NONE;
    }

    ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
    ci.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = desc_.depthWriteEnabled_;
    ci.GraphicsPipeline.DepthStencilDesc.DepthFunc = DiligentCmpFunc[desc_.depthCompareFunction_];
    ci.GraphicsPipeline.DepthStencilDesc.StencilEnable = desc_.stencilTestEnabled_;
    ci.GraphicsPipeline.DepthStencilDesc.StencilReadMask = desc_.stencilCompareMask_;
    ci.GraphicsPipeline.DepthStencilDesc.StencilWriteMask = desc_.stencilWriteMask_;
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFailOp = sStencilOp[desc_.stencilOperationOnStencilFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilDepthFailOp =
        sStencilOp[desc_.stencilOperationOnDepthFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilPassOp = sStencilOp[desc_.stencilOperationOnPassed_];
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFunc = DiligentCmpFunc[desc_.stencilCompareFunction_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFailOp = sStencilOp[desc_.stencilOperationOnStencilFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilDepthFailOp = sStencilOp[desc_.stencilOperationOnDepthFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilPassOp = sStencilOp[desc_.stencilOperationOnPassed_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFunc = DiligentCmpFunc[desc_.stencilCompareFunction_];

    unsigned depthBits = 24;
    if (ci.GraphicsPipeline.DSVFormat == TEX_FORMAT_R16_TYPELESS)
        depthBits = 16;
    const int scaledDepthBias = isOpenGL ? 0 : (int)(desc_.constantDepthBias_ * (1 << depthBits));

    ci.GraphicsPipeline.RasterizerDesc.FillMode = DiligentFillMode[desc_.fillMode_];
    ci.GraphicsPipeline.RasterizerDesc.CullMode = DiligentCullMode[desc_.cullMode_];
    ci.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = false;
    ci.GraphicsPipeline.RasterizerDesc.DepthBias = scaledDepthBias;
    ci.GraphicsPipeline.RasterizerDesc.SlopeScaledDepthBias = desc_.slopeScaledDepthBias_;
    ci.GraphicsPipeline.RasterizerDesc.DepthClipEnable = true;
    ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = desc_.scissorTestEnabled_;
    ci.GraphicsPipeline.RasterizerDesc.AntialiasedLineEnable = !isOpenGL && desc_.lineAntiAlias_;

    ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = desc_.scissorTestEnabled_;

    ci.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

    PipelineStateCache* psoCache = graphics->GetSubsystem<PipelineStateCache>();
    if (psoCache->object_)
        ci.pPSOCache = psoCache->object_.Cast<IPipelineStateCache>(IID_PipelineStateCache);

#if GL_SUPPORTED || GLES_SUPPORTED
    auto patchInputLayout = [&](GLuint* programObjects, Diligent::Uint32 numPrograms)
    {
        const auto vertexShaderAttributes = GetGLVertexAttributes(programObjects[0]);

        InitializeLayoutElements(layoutElements, desc_.GetVertexElements(), vertexShaderAttributes);
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
        URHO3D_LOGERROR("Error has ocurred at Pipeline Build. ({})", desc_.ToHash());
        return false;
    }

    handle_->CreateShaderResourceBinding(&shaderResourceBinding_, true);
    reflection_->ConnectToShaderVariables(shaderResourceBinding_);

    URHO3D_LOGDEBUG("Created Graphics Pipeline ({})", desc_.ToHash());
    return true;
}

void PipelineState::ReleasePipeline()
{
    handle_ = nullptr;
}

} // namespace Urho3D
