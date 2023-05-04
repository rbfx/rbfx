#include "../PipelineState.h"
#include "../RenderSurface.h"
#include "../Shader.h"
#include "DiligentGraphicsImpl.h"
#include "DiligentLookupSettings.h"

#include "Urho3D/Core/Macros.h"
#include "Urho3D/Core/StringUtils.h"
#include "Urho3D/RenderAPI/OpenGLIncludes.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <Diligent/Graphics/GraphicsEngine/interface/PipelineState.h>

#include <EASTL/optional.h>
#include <EASTL/span.h>

namespace Urho3D
{

namespace
{

const ea::string elementSemanticNames[] = {
    "iPos", // SEM_POSITION
    "iNormal", // SEM_NORMAL
    "iBinormal", // SEM_BINORMAL
    "iTangent", // SEM_TANGENT
    "iTexCoord", // SEM_TEXCOORD
    "iColor", // SEM_COLOR
    "iBlendWeights", // SEM_BLENDWEIGHTS
    "iBlendIndices", // SEM_BLENDINDICES
};

#if GL_SUPPORTED || GLES_SUPPORTED

ea::optional<VertexShaderAttribute> ParseGLVertexAttribute(const ea::string& name)
{
    for (unsigned index = 0; index < URHO3D_ARRAYSIZE(elementSemanticNames); ++index)
    {
        const ea::string& semanticName = elementSemanticNames[index];
        VertexElementSemantic semanticValue = static_cast<VertexElementSemantic>(index);

        const unsigned semanticPos = name.find(semanticName);
        if (semanticPos == ea::string::npos)
            continue;

        const unsigned semanticIndexPos = semanticPos + semanticName.length();
        const unsigned semanticIndex = ToUInt(&name[semanticIndexPos]);
        return VertexShaderAttribute{semanticValue, semanticIndex};
    }
    return ea::nullopt;
}

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

        if (const auto element = ParseGLVertexAttribute(attributeName))
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

void InitializeLayoutElements(
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
                attribute.inputIndex_, elementSemanticNames[attribute.semantic_], attribute.semanticIndex_);
        }
    }
}

unsigned RemoveUnusedElements(ea::span<Diligent::LayoutElement> result)
{
    const auto isUnused = [](const Diligent::LayoutElement& element) { return element.InputIndex == M_MAX_UNSIGNED; };
    const auto iter = ea::remove_if(result.begin(), result.end(), isUnused);
    return iter - result.begin();
}

// TODO(diligent): Remove this function, we should store VertexShaderAttributeVector directly.
VertexShaderAttributeVector ToAttributes(const ea::vector<VertexElement>& vertexElements)
{
    VertexShaderAttributeVector result;
    result.reserve(vertexElements.size());

    unsigned inputIndex = 0;
    for (const VertexElement& element : vertexElements)
        result.push_back(VertexShaderAttribute{element.semantic_, element.index_, inputIndex++});

    return result;
}

}

using namespace Diligent;
bool PipelineState::BuildPipeline(Graphics* graphics)
{
    // Check for depth and rt formats
    // If something is wrong, fix then.
    // rbfx will try to make things work, but is developer
    // responsability to fill correctly rts and depth formats.
    bool invalidFmt = false;
    TEXTURE_FORMAT currDepthFmt =
        (TEXTURE_FORMAT)(graphics->GetDepthStencil() ? RenderSurface::GetFormat(graphics, graphics->GetDepthStencil())
                                                     : graphics->GetSwapChainDepthFormat());
    if (currDepthFmt != desc_.depthStencilFormat_)
    {
#ifdef URHO3D_DEBUG
        ea::string log = "Current Binded Depth Format does not correspond to PipelineDesc Depth Format.\n";
        log += "Always fill correctly depth format to pipeline.\n";
        log += "rbfx will fix depth format for you. Invalid format can lead to unexpected issues.\n";
        log += Format("PipelineName: {} | Hash: {} | DepthFormat: {} | Expected Format: {}",
            desc_.debugName_.empty() ? desc_.debugName_ : ea::string("Unknow"), desc_.hash_, desc_.depthStencilFormat_,
            currDepthFmt);
        URHO3D_LOGWARNING(log);
#endif
        invalidFmt = true;
        desc_.depthStencilFormat_ = currDepthFmt;
    }
    for (uint8_t i = 0; i < desc_.renderTargetsFormats_.size(); ++i)
    {
        TEXTURE_FORMAT rtFmt = graphics->GetRenderTarget(i)
            ? (TEXTURE_FORMAT)RenderSurface::GetFormat(graphics, graphics->GetRenderTarget(i))
            : TEX_FORMAT_UNKNOWN;
        if (i == 0 && rtFmt == TEX_FORMAT_UNKNOWN)
            rtFmt = (TEXTURE_FORMAT)graphics->GetSwapChainRTFormat();
        if (rtFmt != desc_.renderTargetsFormats_[i])
        {
#ifdef URHO3D_DEBUG
            ea::string log = Format(
                "Current Binded Render Target Format at {} slot, does not corresponde to currently binded Render "
                "Target.\n",
                i);
            log += "Always fill correctly render target formats.\n";
            log += "rbfx will fix render target format for you. Invalid formats can lead to unexpected issues.\n";
            log += Format("PipelineName: {} | Hash: {} | RTFormat: {} | Expected Format: {}",
                desc_.debugName_.empty() ? desc_.debugName_ : ea::string("Unknow"), desc_.hash_,
                desc_.renderTargetsFormats_[i], desc_.depthStencilFormat_);
            URHO3D_LOGWARNING(log);
#endif
            invalidFmt = true;
            desc_.renderTargetsFormats_[i] = rtFmt;
        }
    }

    if (invalidFmt)
        pipeline_ = nullptr;
    if (pipeline_)
        return true;

    ea::vector<LayoutElement> layoutElements;
    InitializeLayoutElements(layoutElements, desc_.GetVertexElements());

    // On OpenGL layout initialization is postponed until the program is linked.
    if (graphics->GetRenderBackend() != RENDER_GL)
    {
        const ea::vector<VertexElement>& vertexShaderAttributes = desc_.vertexShader_->GetVertexElements();
        FillLayoutElementIndices(layoutElements, desc_.GetVertexElements(), ToAttributes(vertexShaderAttributes));
        const unsigned numElements = RemoveUnusedElements(layoutElements);
        layoutElements.resize(numElements);
    }

    GraphicsPipelineStateCreateInfo ci;
#ifdef URHO3D_DEBUG
    if (desc_.debugName_.empty())
        URHO3D_LOGWARNING(
            "PipelineState doesn't have a debug name on your desc. This is critical but is recommended to have a debug "
            "name.");
    ea::string debugName = Format("{}#{}", desc_.debugName_, desc_.ToHash());
    ci.PSODesc.Name = debugName.c_str();
#endif
    ci.GraphicsPipeline.PrimitiveTopology = DiligentPrimitiveTopology[desc_.primitiveType_];

    ci.GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
    ci.GraphicsPipeline.InputLayout.LayoutElements = layoutElements.data();

    ci.GraphicsPipeline.NumRenderTargets = desc_.renderTargetsFormats_.size();
    for (unsigned i = 0; i < ci.GraphicsPipeline.NumRenderTargets; ++i)
        ci.GraphicsPipeline.RTVFormats[i] = (TEXTURE_FORMAT)desc_.renderTargetsFormats_[i];
    ci.GraphicsPipeline.DSVFormat = (TEXTURE_FORMAT)desc_.depthStencilFormat_;

    if (desc_.vertexShader_)
        ci.pVS = desc_.vertexShader_->GetGPUObject().Cast<IShader>(IID_Shader);
    if (desc_.pixelShader_)
        ci.pPS = desc_.pixelShader_->GetGPUObject().Cast<IShader>(IID_Shader);
    if (desc_.domainShader_)
        ci.pDS = desc_.domainShader_->GetGPUObject().Cast<IShader>(IID_Shader);
    if (desc_.hullShader_)
        ci.pHS = desc_.hullShader_->GetGPUObject().Cast<IShader>(IID_Shader);
    if (desc_.geometryShader_)
        ci.pGS = desc_.geometryShader_->GetGPUObject().Cast<IShader>(IID_Shader);

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
    int scaledDepthBias = (int)(desc_.constantDepthBias_ * (1 << depthBits));

    ci.GraphicsPipeline.RasterizerDesc.FillMode = DiligentFillMode[desc_.fillMode_];
    ci.GraphicsPipeline.RasterizerDesc.CullMode = DiligentCullMode[desc_.cullMode_];
    ci.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = false;
    ci.GraphicsPipeline.RasterizerDesc.DepthBias = scaledDepthBias;
    ci.GraphicsPipeline.RasterizerDesc.DepthBiasClamp = M_INFINITY;
    ci.GraphicsPipeline.RasterizerDesc.SlopeScaledDepthBias = desc_.slopeScaledDepthBias_;
    ci.GraphicsPipeline.RasterizerDesc.DepthClipEnable = true;
    ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = desc_.scissorTestEnabled_;
    ci.GraphicsPipeline.RasterizerDesc.AntialiasedLineEnable = desc_.lineAntiAlias_;

    ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = desc_.scissorTestEnabled_;

    ci.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

    PipelineStateCache* psoCache = graphics->GetSubsystem<PipelineStateCache>();
    if (psoCache->object_)
        ci.pPSOCache = psoCache->object_.Cast<IPipelineStateCache>(IID_PipelineStateCache);

#if GL_SUPPORTED || GLES_SUPPORTED
    auto patchInputLayout = [&](GLuint programObject, Diligent::Uint32& numElements, Diligent::LayoutElement* elements)
    {
        const ea::span<Diligent::LayoutElement> elementsSpan{elements, numElements};
        const auto vertexAttributes = GetGLVertexAttributes(programObject);

        FillLayoutElementIndices(elementsSpan, desc_.GetVertexElements(), vertexAttributes);
        numElements = RemoveUnusedElements(elementsSpan);
    };

    ci.GLPatchVertexLayoutCallbackUserData = &patchInputLayout;
    ci.GLPatchVertexLayoutCallback = [](
        GLuint programObject, Diligent::Uint32* numElements, Diligent::LayoutElement* elements, void* userData)
    {
        const auto& callback = *reinterpret_cast<decltype(patchInputLayout)*>(userData);
        callback(programObject, *numElements, elements);
    };
#endif

    graphics->GetImpl()->GetDevice()->CreateGraphicsPipelineState(ci, &pipeline_);

    if (!pipeline_)
    {
        URHO3D_LOGERROR("Error has ocurred at Pipeline Build. ({})", desc_.ToHash());
        return false;
    }

    URHO3D_LOGDEBUG("Created Graphics Pipeline ({})", desc_.ToHash());
    return true;
}

void PipelineState::ReleasePipeline()
{
    pipeline_ = nullptr;
}

ShaderResourceBinding* PipelineState::CreateInternalSRB()
{
    assert(pipeline_);

    IPipelineState* pipeline = static_cast<IPipelineState*>(pipeline_);
    IShaderResourceBinding* srb = nullptr;
    pipeline->CreateShaderResourceBinding(&srb, false);
    assert(srb);

    return new ShaderResourceBinding(owner_->GetSubsystem<Graphics>(), srb);
}
} // namespace Urho3D
