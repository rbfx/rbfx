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

#if GL_SUPPORTED || GLES_SUPPORTED
    #include <Diligent/Graphics/GraphicsEngineOpenGL/interface/PipelineStateGL.h>
    #include <Diligent/Graphics/GraphicsEngineOpenGL/interface/ShaderGL.h>
#endif

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

void InitializeImmutableSampler(Diligent::ImmutableSamplerDesc& destSampler, const SamplerStateDesc& sourceSampler,
    const ea::string& samplerName, RenderDevice* renderDevice, Diligent::SHADER_TYPE shaderStages)
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
    };

    const int anisotropy =
        sourceSampler.anisotropy_ ? sourceSampler.anisotropy_ : renderDevice->GetDefaultTextureAnisotropy();
    const TextureFilterMode filterMode = sourceSampler.filterMode_ != FILTER_DEFAULT
        ? sourceSampler.filterMode_
        : renderDevice->GetDefaultTextureFilterMode();

    destSampler.ShaderStages = shaderStages;
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
}

void InitializeImmutableSamplers(ea::vector<Diligent::ImmutableSamplerDesc>& result, const ImmutableSamplersDesc& desc,
    const ShaderProgramReflection& reflection, RenderDevice* renderDevice, Diligent::SHADER_TYPE shaderStages)
{
    static const auto defaultSampler = SamplerStateDesc::Bilinear();

    const ea::span<const StringHash> samplerNames{desc.names_.data(), desc.size_};
    for (const auto& [nameHash, resourceDesc] : reflection.GetShaderResources())
    {
        const auto iter = ea::find(samplerNames.begin(), samplerNames.end(), nameHash);
        const SamplerStateDesc* sourceSampler = &defaultSampler;
        if (iter != samplerNames.end())
        {
            const auto index = static_cast<unsigned>(iter - samplerNames.begin());
            sourceSampler = &desc.desc_[index];
        }
        else
        {
            URHO3D_LOGWARNING("Default sampler is used for resource '{}'", resourceDesc.internalName_);
        }

        const ea::string& internalName = resourceDesc.internalName_;
        Diligent::ImmutableSamplerDesc& destSampler = result.emplace_back();
        InitializeImmutableSampler(destSampler, *sourceSampler, internalName, renderDevice, shaderStages);
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
    #define CHECK_ERROR_AND_RETURN(message) \
        if (glGetError() != GL_NO_ERROR) \
        { \
            URHO3D_ASSERTLOG(false, message); \
            return; \
        }

ea::pair<VertexShaderAttributeVector, StringVector> GetGLVertexAttributes(GLuint programObject)
{
    GLint numActiveAttribs = 0;
    GLint maxNameLength = 0;
    glGetProgramiv(programObject, GL_ACTIVE_ATTRIBUTES, &numActiveAttribs);
    glGetProgramiv(programObject, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength);

    ea::string attributeName;
    attributeName.resize(maxNameLength, '\0');

    VertexShaderAttributeVector result;
    StringVector resultNames;
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
            resultNames.push_back(attributeName);
        }
        else
        {
            URHO3D_LOGWARNING("Unknown vertex element semantic: {}", attributeName);
        }
    }

    return {result, resultNames};
}

class TemporaryGLProgram
{
public:
    TemporaryGLProgram(ea::span<Diligent::IShader* const> shaders, bool separablePrograms)
    {
        programObject_ = glCreateProgram();
        if (!programObject_)
        {
            URHO3D_ASSERTLOG(false, "glCreateProgram() failed");
            return;
        }

        if (separablePrograms)
            glProgramParameteri(programObject_, GL_PROGRAM_SEPARABLE, GL_TRUE);

        for (Diligent::IShader* shader : shaders)
        {
            // Link only vertex shader if separable shader programs are used.
            if (shader && (!separablePrograms || shader->GetDesc().ShaderType == Diligent::SHADER_TYPE_VERTEX))
            {
                glAttachShader(programObject_, static_cast<Diligent::IShaderGL*>(shader)->GetGLShaderHandle());
                CHECK_ERROR_AND_RETURN("glAttachShader() failed");
            }
        }

        glLinkProgram(programObject_);
        CHECK_ERROR_AND_RETURN("glLinkProgram() failed");

        int isLinked = GL_FALSE;
        glGetProgramiv(programObject_, GL_LINK_STATUS, &isLinked);
        CHECK_ERROR_AND_RETURN("glGetProgramiv() failed");

        if (!isLinked)
        {
            int lengthWithNull = 0;
            glGetProgramiv(programObject_, GL_INFO_LOG_LENGTH, &lengthWithNull);

            ea::vector<char> shaderProgramInfoLog(lengthWithNull);
            glGetProgramInfoLog(programObject_, lengthWithNull, nullptr, shaderProgramInfoLog.data());

            URHO3D_LOGERROR("Failed to link shader program:\n{}", shaderProgramInfoLog.data());
            return;
        }

        const auto [attributes, names] = GetGLVertexAttributes(programObject_);
        vertexAttributes_ = attributes;
        vertexAttributeNames_ = names;
    }

    ~TemporaryGLProgram()
    {
        if (programObject_)
        {
            glDeleteProgram(programObject_);
            if (glGetError() != GL_NO_ERROR)
                URHO3D_ASSERTLOG(false, "glDeleteProgram() failed");
        }
    }

    GLuint GetHandle() const { return programObject_; }
    const VertexShaderAttributeVector& GetVertexAttributes() const { return vertexAttributes_; }
    const StringVector& GetVertexAttributeNames() const { return vertexAttributeNames_; }

private:
    GLuint programObject_{};
    VertexShaderAttributeVector vertexAttributes_;
    StringVector vertexAttributeNames_;
};

    #undef CHECK_ERROR_AND_RETURN
#endif

} // namespace

const ea::string& PipelineStateDesc::GetDebugName() const
{
    const ea::string* result = nullptr;
    ea::visit([&result](const auto& desc) { result = &desc.debugName_; }, desc_);
    return *result;
}

const GraphicsPipelineStateDesc* PipelineStateDesc::AsGraphics() const
{
    return GetType() == PipelineStateType::Graphics ? &ea::get<0>(desc_) : nullptr;
}

const ComputePipelineStateDesc* PipelineStateDesc::AsCompute() const
{
    return GetType() == PipelineStateType::Compute ? &ea::get<1>(desc_) : nullptr;
}

PipelineState::PipelineState(PipelineStateCache* owner, const PipelineStateDesc& desc)
    : Object(owner->GetContext())
    , DeviceObject(owner->GetContext())
    , owner_(owner)
    , desc_(desc)
{
    SetDebugName(Format("{} #{}", desc_.GetDebugName(), desc_.ToHash()));
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

    if (renderDevice_)
        renderDevice_->QueuePipelineStateReload(this);
}

void PipelineState::Restore()
{
    if (handle_)
        return;

    RestoreDependency(GetSubsystem<PipelineStateCache>());

    if (const GraphicsPipelineStateDesc* graphicsDesc = desc_.AsGraphics())
    {
        RestoreDependency(graphicsDesc->vertexShader_);
        RestoreDependency(graphicsDesc->pixelShader_);
        RestoreDependency(graphicsDesc->geometryShader_);
        RestoreDependency(graphicsDesc->hullShader_);
        RestoreDependency(graphicsDesc->domainShader_);
    }

    if (const ComputePipelineStateDesc* computeDesc = desc_.AsCompute())
    {
        RestoreDependency(computeDesc->computeShader_);
    }

    CreateGPU();
}

void PipelineState::Destroy()
{
    DestroyGPU();
}

void PipelineState::CreateGPU()
{
    if (const GraphicsPipelineStateDesc* graphicsDesc = desc_.AsGraphics())
        CreateGPU(*graphicsDesc);
    else if (const ComputePipelineStateDesc* computeDesc = desc_.AsCompute())
        CreateGPU(*computeDesc);
}

void PipelineState::CreateGPU(const GraphicsPipelineStateDesc& desc)
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

    DestroyGPU();

    for (RawShader* shader :
        {desc.vertexShader_, desc.pixelShader_, desc.domainShader_, desc.hullShader_, desc.geometryShader_})
    {
        if (shader && !shader->GetHandle())
        {
            URHO3D_LOGERROR("Failed to create PipelineState '{}' due to failed {} shader compilation", GetDebugName(),
                ToString(shader->GetShaderType()));
            return;
        }
    }

    Diligent::IRenderDevice* renderDevice = renderDevice_->GetRenderDevice();
    const bool isOpenGL = renderDevice_->GetBackend() == RenderBackend::OpenGL;
    const bool hasSeparableShaderPrograms = renderDevice->GetDeviceInfo().Features.SeparablePrograms;
    URHO3D_ASSERT(isOpenGL || hasSeparableShaderPrograms);
    const ea::span<const InputLayoutElementDesc> vertexElements{
        desc.inputLayout_.elements_.data(), desc.inputLayout_.size_};

    Diligent::GraphicsPipelineStateCreateInfo ci;

    ea::vector<Diligent::LayoutElement> layoutElements;
    ea::vector<Diligent::ImmutableSamplerDesc> immutableSamplers;

    Diligent::IShader* vertexShader = desc.vertexShader_ ? desc.vertexShader_->GetHandle() : nullptr;
    Diligent::IShader* pixelShader = desc.pixelShader_ ? desc.pixelShader_->GetHandle() : nullptr;
    Diligent::IShader* domainShader = desc.domainShader_ ? desc.domainShader_->GetHandle() : nullptr;
    Diligent::IShader* hullShader = desc.hullShader_ ? desc.hullShader_->GetHandle() : nullptr;
    Diligent::IShader* geometryShader = desc.geometryShader_ ? desc.geometryShader_->GetHandle() : nullptr;
    Diligent::IShader* const shaderHandles[] = {vertexShader, pixelShader, domainShader, hullShader, geometryShader};

    for (RawShader* shader :
        {desc.vertexShader_, desc.pixelShader_, desc.domainShader_, desc.hullShader_, desc.geometryShader_})
    {
        if (shader)
            shader->OnReloaded.Subscribe(this, &PipelineState::Invalidate);
    }

    VertexShaderAttributeVector vertexAttributes;
    StringVector vertexAttributeNames;
    if (!isOpenGL)
    {
        // On all backends except OpenGL vertex input is precompiled.
        vertexAttributes = desc.vertexShader_->GetBytecode().vertexAttributes_;

        reflection_ = MakeShared<ShaderProgramReflection>(shaderHandles);
    }
    else
    {
#if GL_SUPPORTED || GLES_SUPPORTED
        // On OpenGL we should create temporary program and reflect vertex inputs.
        // If separable shader programs are not supported, we should also reflect everything else.
        TemporaryGLProgram glProgram{shaderHandles, hasSeparableShaderPrograms};

        vertexAttributes = glProgram.GetVertexAttributes();
        vertexAttributeNames = glProgram.GetVertexAttributeNames();

        if (hasSeparableShaderPrograms)
            reflection_ = MakeShared<ShaderProgramReflection>(shaderHandles);
        else
            reflection_ = MakeShared<ShaderProgramReflection>(glProgram.GetHandle());
#endif
    }

    InitializeLayoutElements(layoutElements, vertexElements, vertexAttributes);
    ci.GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
    ci.GraphicsPipeline.InputLayout.LayoutElements = layoutElements.data();

    InitializeImmutableSamplers(
        immutableSamplers, desc.samplers_, *reflection_, renderDevice_, Diligent::SHADER_TYPE_ALL_GRAPHICS);
    ci.PSODesc.ResourceLayout.NumImmutableSamplers = immutableSamplers.size();
    ci.PSODesc.ResourceLayout.ImmutableSamplers = immutableSamplers.data();

    ci.PSODesc.Name = GetDebugName().c_str();

    ci.GraphicsPipeline.PrimitiveTopology = primitiveTopology[desc.primitiveType_];

    ci.GraphicsPipeline.NumRenderTargets = desc.output_.numRenderTargets_;
    for (unsigned i = 0; i < desc.output_.numRenderTargets_; ++i)
        ci.GraphicsPipeline.RTVFormats[i] = desc.output_.renderTargetFormats_[i];
    ci.GraphicsPipeline.DSVFormat = desc.output_.depthStencilFormat_;
    ci.GraphicsPipeline.SmplDesc.Count = desc.output_.multiSample_;
    ci.GraphicsPipeline.ReadOnlyDSV = desc.readOnlyDepth_;

    ci.pVS = vertexShader;
    ci.pPS = pixelShader;
    ci.pDS = domainShader;
    ci.pHS = hullShader;
    ci.pGS = geometryShader;

    ci.GraphicsPipeline.BlendDesc.AlphaToCoverageEnable = desc.alphaToCoverageEnabled_;
    ci.GraphicsPipeline.BlendDesc.IndependentBlendEnable = false;
    if (ci.GraphicsPipeline.NumRenderTargets > 0)
    {
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = isBlendEnabled[desc.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlend = sourceBlend[desc.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlend = destBlend[desc.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOp = blendOperation[desc.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlendAlpha = sourceAlphaBlend[desc.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlendAlpha = destAlphaBlend[desc.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOpAlpha = blendOperation[desc.blendMode_];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].RenderTargetWriteMask =
            desc.colorWriteEnabled_ ? Diligent::COLOR_MASK_ALL : Diligent::COLOR_MASK_NONE;
    }

    ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
    ci.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = desc.depthWriteEnabled_;
    ci.GraphicsPipeline.DepthStencilDesc.DepthFunc = comparisonFunction[desc.depthCompareFunction_];
    ci.GraphicsPipeline.DepthStencilDesc.StencilEnable = desc.stencilTestEnabled_;
    ci.GraphicsPipeline.DepthStencilDesc.StencilReadMask = desc.stencilCompareMask_;
    ci.GraphicsPipeline.DepthStencilDesc.StencilWriteMask = desc.stencilWriteMask_;
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFailOp = stencilOp[desc.stencilOperationOnStencilFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilDepthFailOp = stencilOp[desc.stencilOperationOnDepthFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilPassOp = stencilOp[desc.stencilOperationOnPassed_];
    ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFunc = comparisonFunction[desc.stencilCompareFunction_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFailOp = stencilOp[desc.stencilOperationOnStencilFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilDepthFailOp = stencilOp[desc.stencilOperationOnDepthFailed_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilPassOp = stencilOp[desc.stencilOperationOnPassed_];
    ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFunc = comparisonFunction[desc.stencilCompareFunction_];

    unsigned depthBits = 24;
    if (ci.GraphicsPipeline.DSVFormat == Diligent::TEX_FORMAT_D16_UNORM)
        depthBits = 16;
    const int scaledDepthBias = isOpenGL ? 0 : (int)(desc.constantDepthBias_ * (1 << depthBits));

    ci.GraphicsPipeline.RasterizerDesc.FillMode = fillMode[desc.fillMode_];
    ci.GraphicsPipeline.RasterizerDesc.CullMode = cullMode[desc.cullMode_];
    ci.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = false;
    ci.GraphicsPipeline.RasterizerDesc.DepthBias = scaledDepthBias;
    ci.GraphicsPipeline.RasterizerDesc.SlopeScaledDepthBias = desc.slopeScaledDepthBias_;
    ci.GraphicsPipeline.RasterizerDesc.DepthClipEnable = true;
    ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = desc.scissorTestEnabled_;
    ci.GraphicsPipeline.RasterizerDesc.AntialiasedLineEnable = !isOpenGL && desc.lineAntiAlias_;

    ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = desc.scissorTestEnabled_;

    ci.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

    PipelineStateCache* psoCache = GetSubsystem<PipelineStateCache>();
    ci.pPSOCache = psoCache->GetHandle();

    renderDevice->CreateGraphicsPipelineState(ci, &handle_);

    if (!handle_)
    {
        DestroyGPU();

        URHO3D_LOGERROR("Failed to create PipelineState '{}'", GetDebugName());
        return;
    }

#if GL_SUPPORTED || GLES_SUPPORTED
    if (isOpenGL)
    {
        const auto handleGl = static_cast<Diligent::IPipelineStateGL*>(handle_.RawPtr());
        const GLuint programObject = handleGl->GetGLProgramHandle(Diligent::SHADER_TYPE_VERTEX);

        // TODO: Diligent should return null handle but it does not
        int isLinked = GL_FALSE;
        glGetProgramiv(programObject, GL_LINK_STATUS, &isLinked);
        if (!isLinked)
        {
            DestroyGPU();

            URHO3D_LOGERROR("Failed to create PipelineState '{}' due to OpenGL program linking error", GetDebugName());
            return;
        }

        for (unsigned i = 0; i < vertexAttributes.size(); ++i)
            glBindAttribLocation(programObject, vertexAttributes[i].inputIndex_, vertexAttributeNames[i].c_str());
    }
#endif

    handle_->CreateShaderResourceBinding(&shaderResourceBinding_, true);
    reflection_->ConnectToShaderVariables(desc_.GetType(), shaderResourceBinding_);
}

void PipelineState::CreateGPU(const ComputePipelineStateDesc& desc)
{
    DestroyGPU();

    if (desc.computeShader_ && !desc.computeShader_->GetHandle())
    {
        URHO3D_LOGERROR("Failed to create PipelineState '{}' due to failed {} shader compilation", GetDebugName(),
            ToString(desc.computeShader_->GetShaderType()));
        return;
    }

    Diligent::IRenderDevice* renderDevice = renderDevice_->GetRenderDevice();
    const bool isOpenGL = renderDevice_->GetBackend() == RenderBackend::OpenGL;
    const bool hasSeparableShaderPrograms = renderDevice->GetDeviceInfo().Features.SeparablePrograms;
    URHO3D_ASSERT(isOpenGL || hasSeparableShaderPrograms);

    Diligent::ComputePipelineStateCreateInfo ci;

    ea::vector<Diligent::ImmutableSamplerDesc> immutableSamplers;

    Diligent::IShader* computeShader = desc.computeShader_->GetHandle();
    Diligent::IShader* const shaderHandles[] = {computeShader};
    desc.computeShader_->OnReloaded.Subscribe(this, &PipelineState::Invalidate);

    if (hasSeparableShaderPrograms)
    {
        reflection_ = MakeShared<ShaderProgramReflection>(shaderHandles);
    }
    else
    {
#if GL_SUPPORTED || GLES_SUPPORTED
        TemporaryGLProgram glProgram{shaderHandles, hasSeparableShaderPrograms};
        reflection_ = MakeShared<ShaderProgramReflection>(glProgram.GetHandle());
#endif
    }

    InitializeImmutableSamplers(
        immutableSamplers, desc.samplers_, *reflection_, renderDevice_, Diligent::SHADER_TYPE_COMPUTE);
    ci.PSODesc.ResourceLayout.NumImmutableSamplers = immutableSamplers.size();
    ci.PSODesc.ResourceLayout.ImmutableSamplers = immutableSamplers.data();

    ci.PSODesc.Name = GetDebugName().c_str();

    ci.pCS = computeShader;

    ci.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

    PipelineStateCache* psoCache = GetSubsystem<PipelineStateCache>();
    ci.pPSOCache = psoCache->GetHandle();

    renderDevice->CreateComputePipelineState(ci, &handle_);

    if (!handle_)
    {
        DestroyGPU();

        URHO3D_LOGERROR("Failed to create PipelineState '{}'", GetDebugName());
        return;
    }

    handle_->CreateShaderResourceBinding(&shaderResourceBinding_, true);
    reflection_->ConnectToShaderVariables(desc_.GetType(), shaderResourceBinding_);
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

SharedPtr<PipelineState> PipelineStateCache::GetPipelineState(const PipelineStateDesc& desc)
{
    WeakPtr<PipelineState>& weakPipelineState = states_[desc];
    SharedPtr<PipelineState> pipelineState = weakPipelineState.Lock();
    if (!pipelineState)
    {
        pipelineState = MakeShared<PipelineState>(this, desc);
        weakPipelineState = pipelineState;
    }
    return pipelineState;
}

SharedPtr<PipelineState> PipelineStateCache::GetGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
{
    if (!desc.IsInitialized())
        return nullptr;

    return GetPipelineState(PipelineStateDesc{desc});
}

SharedPtr<PipelineState> PipelineStateCache::GetComputePipelineState(const ComputePipelineStateDesc& desc)
{
    if (!desc.IsInitialized())
        return nullptr;

    return GetPipelineState(PipelineStateDesc{desc});
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
