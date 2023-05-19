#include "../PipelineState.h"
#include "../RenderSurface.h"
#include "../Shader.h"
#include "DiligentGraphicsImpl.h"
#include "DiligentLookupSettings.h"

#include "Urho3D/Core/Macros.h"
#include "Urho3D/Core/StringUtils.h"
#include "Urho3D/RenderAPI/OpenGLIncludes.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"

#include <Diligent/Graphics/GraphicsEngine/interface/PipelineState.h>

#include <EASTL/optional.h>
#include <EASTL/span.h>

namespace Urho3D
{

namespace
{

unsigned GetScalarUniformSize(Diligent::SHADER_CODE_BASIC_TYPE basicType)
{
    switch (basicType)
    {
    case Diligent::SHADER_CODE_BASIC_TYPE_INT64:
    case Diligent::SHADER_CODE_BASIC_TYPE_UINT64:
    case Diligent::SHADER_CODE_BASIC_TYPE_DOUBLE: return sizeof(double);

    default: return sizeof(float);
    }
}

unsigned GetVectorUniformSize(Diligent::SHADER_CODE_BASIC_TYPE basicType, unsigned numElements)
{
    switch (numElements)
    {
    case 1: return GetScalarUniformSize(basicType);

    case 2: return 2 * GetScalarUniformSize(basicType);

    case 3: return 3 * GetScalarUniformSize(basicType);

    case 4: return 4 * GetScalarUniformSize(basicType);

    default: return 0;
    }
}

unsigned GetVectorArrayUniformSize(Diligent::SHADER_CODE_BASIC_TYPE basicType, unsigned numElements, unsigned arraySize)
{
    const unsigned alignment = 4 * sizeof(float);
    const unsigned elementSize = GetVectorUniformSize(basicType, numElements);
    return arraySize * ((elementSize + alignment - 1) / alignment) * alignment;
}

unsigned GetMatrixUniformSize(Diligent::SHADER_CODE_BASIC_TYPE basicType, unsigned innerSize, unsigned outerSize)
{
    return outerSize * GetVectorUniformSize(basicType, innerSize);
}

unsigned GetUniformSize(const Diligent::ShaderCodeVariableDesc& uniformDesc)
{
    if (uniformDesc.ArraySize > 1)
    {
        switch (uniformDesc.Class)
        {
        case Diligent::SHADER_CODE_VARIABLE_CLASS_SCALAR:
            return GetVectorArrayUniformSize(uniformDesc.BasicType, 1, uniformDesc.ArraySize);

        case Diligent::SHADER_CODE_VARIABLE_CLASS_VECTOR:
            return GetVectorArrayUniformSize(
                uniformDesc.BasicType, ea::max(uniformDesc.NumColumns, uniformDesc.NumRows), uniformDesc.ArraySize);

        case Diligent::SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS:
            return GetVectorArrayUniformSize(
                uniformDesc.BasicType, uniformDesc.NumRows, uniformDesc.ArraySize * uniformDesc.NumColumns);

        case Diligent::SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS:
            return GetVectorArrayUniformSize(
                uniformDesc.BasicType, uniformDesc.NumColumns, uniformDesc.ArraySize * uniformDesc.NumRows);

        default: return 0;
        }
    }
    else
    {
        switch (uniformDesc.Class)
        {
        case Diligent::SHADER_CODE_VARIABLE_CLASS_SCALAR: //
            return GetScalarUniformSize(uniformDesc.BasicType);

        case Diligent::SHADER_CODE_VARIABLE_CLASS_VECTOR:
            return GetVectorUniformSize(uniformDesc.BasicType, ea::max(uniformDesc.NumColumns, uniformDesc.NumRows));

        case Diligent::SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS:
            return GetVectorArrayUniformSize(uniformDesc.BasicType, uniformDesc.NumRows, uniformDesc.NumColumns);

        case Diligent::SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS:
            return GetVectorArrayUniformSize(uniformDesc.BasicType, uniformDesc.NumColumns, uniformDesc.NumRows);

        default: return 0;
        }
    }
}

ea::optional<ea::string_view> SanitateUniformName(ea::string_view name)
{
    const auto pos = name.find('c');
    if (pos == ea::string_view::npos || pos + 1 == name.length())
        return ea::nullopt;

    return name.substr(pos + 1);
}

ea::optional<ea::string_view> SanitateResourceName(ea::string_view name)
{
    if (name.empty() || name[0] != 's')
        return ea::nullopt;

    return name.substr(1);
}

/// TODO(diligent): Get rid of this hack when we remove other backends
class ShaderReflectionImpl : public ShaderProgramLayout
{
public:
    void AddOrCheckUniformBuffer(ShaderParameterGroup group, ea::string_view internalName, unsigned size)
    {
        const UniformBufferReflection* oldBuffer = GetUniformBuffer(group);
        if (oldBuffer)
        {
            if (oldBuffer->size_ != size)
                URHO3D_LOGWARNING("Uniform buffer #{} has inconsistent size in different stages", group);
            if (oldBuffer->internalName_ != internalName)
                URHO3D_LOGWARNING("Uniform buffer #{} has inconsistent name in different stages", group);
            return;
        }

        AddUniformBuffer(group, internalName, size);
    }

    void AddOrCheckUniform(ShaderParameterGroup group, const Diligent::ShaderCodeVariableDesc& desc)
    {
        const auto sanitatedName = SanitateUniformName(desc.Name);
        if (!sanitatedName)
        {
            URHO3D_LOGWARNING("Cannot parse uniform with name '{}'", desc.Name);
            return;
        }

        const unsigned uniformSize = GetUniformSize(desc);
        if (uniformSize == 0)
        {
            URHO3D_LOGWARNING("Cannot deduce the size of the uniform '{}'", *sanitatedName);
            return;
        }

        AddOrCheckUniform(group, *sanitatedName, uniformSize, desc.Offset);
    }

    void AddOrCheckUniform(ShaderParameterGroup group, ea::string_view name, unsigned size, unsigned offset)
    {
        const auto nameHash = StringHash{name};
        const UniformReflection* oldParameter = GetUniform(nameHash);
        if (oldParameter)
        {
            if (oldParameter->size_ != size)
                URHO3D_LOGWARNING("Uniform '{}' has inconsistent size in different stages", name);
            if (oldParameter->offset_ != offset)
                URHO3D_LOGWARNING("Uniform '{}' has inconsistent offset in different stages", name);
            if (oldParameter->group_ != group)
                URHO3D_LOGWARNING("Uniform '{}' has inconsistent owner in different stages", name);
            return;
        }

        AddUniform(nameHash, group, offset, size);
    }

    void AddOrCheckShaderResource(StringHash name, ea::string_view internalName)
    {
        const ShaderResourceReflection* oldResource = GetShaderResource(name);
        if (oldResource)
        {
            URHO3D_LOGWARNING("Shader resource '{}' is referenced by multiple shader stages", internalName);
            return;
        }

        AddShaderResource(name, internalName);
    }
};

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
    destSampler.Desc.AddressU = addressMode[sourceSampler.addressMode_[COORD_U]];
    destSampler.Desc.AddressV = addressMode[sourceSampler.addressMode_[COORD_V]];
    destSampler.Desc.AddressW = addressMode[sourceSampler.addressMode_[COORD_W]];
    destSampler.Desc.MaxAnisotropy = anisotropy;
    destSampler.Desc.ComparisonFunc = COMPARISON_FUNC_LESS_EQUAL;
    destSampler.Desc.MinLOD = -M_INFINITY;
    destSampler.Desc.MaxLOD = M_INFINITY;
    memcpy(&destSampler.Desc.BorderColor, sourceSampler.borderColor_.Data(), 4 * sizeof(float));
}

void InitializeImmutableSamplers(ea::vector<Diligent::ImmutableSamplerDesc>& result, const PipelineStateDesc& desc,
    const ShaderProgramLayout& reflection)
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

Diligent::IShader* ToHandle(ShaderVariation* shader)
{
    // TODO(diligent): Simplify this function, we should have better API.
    return shader ? shader->GetGPUObject().Cast<Diligent::IShader>(Diligent::IID_Shader).RawPtr() : nullptr;
}

ea::optional<ShaderParameterGroup> ParseConstantBufferName(ea::string_view name)
{
    static const ea::string builtinNames[] = {
        "Frame",
        "Camera",
        "Zone",
        "Light",
        "Material",
        "Object",
        "Custom",
    };
    for (unsigned group = 0; group < MAX_SHADER_PARAMETER_GROUPS; ++group)
    {
        if (builtinNames[group] == name)
            return static_cast<ShaderParameterGroup>(group);
    }
    return ea::nullopt;
}

void AppendConstantBufferToReflection(ShaderReflectionImpl& reflection, Diligent::IShader* shader,
    unsigned resourceIndex, const Diligent::ShaderResourceDesc& desc)
{
    const auto bufferGroup = ParseConstantBufferName(desc.Name);
    if (!bufferGroup)
    {
        URHO3D_LOGWARNING("Unknown constant buffer '{}' is ignored", desc.Name);
        return;
    }

    const Diligent::ShaderCodeBufferDesc& bufferDesc = *shader->GetConstantBufferDesc(resourceIndex);
    reflection.AddOrCheckUniformBuffer(*bufferGroup, desc.Name, bufferDesc.Size);

    const unsigned numUniforms = bufferDesc.NumVariables;
    for (unsigned uniformIndex = 0; uniformIndex < numUniforms; ++uniformIndex)
    {
        const Diligent::ShaderCodeVariableDesc& uniformDesc = bufferDesc.pVariables[uniformIndex];
        reflection.AddOrCheckUniform(*bufferGroup, uniformDesc);
    }
}

void AppendShaderReflection(ShaderReflectionImpl& reflection, Diligent::IShader* shader)
{
    const unsigned numResources = shader->GetResourceCount();
    for (unsigned resourceIndex = 0; resourceIndex < numResources; ++resourceIndex)
    {
        Diligent::ShaderResourceDesc desc;
        shader->GetResourceDesc(resourceIndex, desc);

        switch (desc.Type)
        {
        case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
            AppendConstantBufferToReflection(reflection, shader, resourceIndex, desc);
            break;

        case SHADER_RESOURCE_TYPE_TEXTURE_SRV:
            if (const auto sanitatedName = SanitateResourceName(desc.Name))
                reflection.AddOrCheckShaderResource(StringHash{*sanitatedName}, desc.Name);
            break;

        default: break;
        }
    }
}

SharedPtr<ShaderProgramLayout> ReflectShaders(std::initializer_list<Diligent::IShader*> shaders)
{
    auto reflection = MakeShared<ShaderReflectionImpl>();
    for (Diligent::IShader* shader : shaders)
    {
        if (shader)
            AppendShaderReflection(*reflection, shader);
    }
    return reflection;
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

ea::optional<ea::string_view> SanitateGLUniformName(ea::string_view name)
{
    // Remove trailing '[0]' from array names
    const unsigned subscriptIndex = name.find('[');
    if (subscriptIndex != ea::string::npos)
    {
        // If not the first index, skip
        if (name.find("[0]", subscriptIndex) == ea::string::npos)
            return ea::nullopt;

        name = name.substr(0, subscriptIndex);
    }

    // Remove uniform buffer name
    const unsigned dotIndex = name.find('.');
    if (dotIndex != ea::string::npos)
        name = name.substr(dotIndex + 1);

    // Remove trailing 'c', ignore other uniforms
    if (name.empty() || name[0] != 'c')
        return ea::nullopt;

    return name.substr(1);
}

void ConstructUniformDesc(Diligent::ShaderCodeVariableDesc& result, Diligent::SHADER_CODE_VARIABLE_CLASS kind,
    Diligent::SHADER_CODE_BASIC_TYPE basicType, unsigned numColumns, unsigned numRows)
{
    result.Class = kind;
    result.BasicType = basicType;
    result.NumColumns = numColumns;
    result.NumRows = numRows;
}

Diligent::ShaderCodeVariableDesc CreateUniformDesc(GLenum type, GLint elementCount)
{
    using namespace Diligent;

    ShaderCodeVariableDesc result;
    result.ArraySize = elementCount;
    switch (type)
    {
    case GL_BOOL:
        ConstructUniformDesc(result, SHADER_CODE_VARIABLE_CLASS_SCALAR, SHADER_CODE_BASIC_TYPE_BOOL, 1, 1);
        break;

    case GL_INT:
        ConstructUniformDesc(result, SHADER_CODE_VARIABLE_CLASS_SCALAR, SHADER_CODE_BASIC_TYPE_INT, 1, 1);
        break;

    case GL_FLOAT:
        ConstructUniformDesc(result, SHADER_CODE_VARIABLE_CLASS_SCALAR, SHADER_CODE_BASIC_TYPE_FLOAT, 1, 1);
        break;

    case GL_FLOAT_VEC2:
        ConstructUniformDesc(result, SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 2, 1);
        break;

    case GL_FLOAT_VEC3:
        ConstructUniformDesc(result, SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 3, 1);
        break;

    case GL_FLOAT_VEC4:
        ConstructUniformDesc(result, SHADER_CODE_VARIABLE_CLASS_VECTOR, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 1);
        break;

    case GL_FLOAT_MAT3:
        ConstructUniformDesc(result, SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS, SHADER_CODE_BASIC_TYPE_FLOAT, 3, 3);
        break;

    case GL_FLOAT_MAT3x4:
        ConstructUniformDesc(result, SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 3);
        break;

    case GL_FLOAT_MAT4:
        ConstructUniformDesc(result, SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS, SHADER_CODE_BASIC_TYPE_FLOAT, 4, 4);
        break;

    default: break;
    }

    return result;
}

SharedPtr<ShaderProgramLayout> ReflectGLProgram(GLuint programObject)
{
    auto reflection = MakeShared<ShaderReflectionImpl>();

    GLint numUniformBlocks = 0;
    GLint numUniforms = 0;
    glGetProgramiv(programObject, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBlocks);
    glGetProgramiv(programObject, GL_ACTIVE_UNIFORMS, &numUniforms);

    ea::vector<ShaderParameterGroup> indexToGroup;

    static const unsigned maxNameLength = 256;
    char name[maxNameLength]{};

    for (GLuint uniformBlockIndex = 0; uniformBlockIndex < static_cast<GLuint>(numUniformBlocks); ++uniformBlockIndex)
    {
        glGetActiveUniformBlockName(programObject, uniformBlockIndex, maxNameLength, nullptr, name);

        const auto bufferGroup = ParseConstantBufferName(name);
        if (!bufferGroup)
        {
            URHO3D_LOGWARNING("Unknown constant buffer '{}' is ignored", name);
            continue;
        }

        GLint dataSize = 0;
        glGetActiveUniformBlockiv(programObject, uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &dataSize);

        reflection->AddOrCheckUniformBuffer(*bufferGroup, name, dataSize);

        const unsigned blockIndex = glGetUniformBlockIndex(programObject, name);
        if (blockIndex >= indexToGroup.size())
            indexToGroup.resize(blockIndex + 1, MAX_SHADER_PARAMETER_GROUPS);
        indexToGroup[blockIndex] = *bufferGroup;
    }

    for (GLuint uniformIndex = 0; uniformIndex < static_cast<GLuint>(numUniforms); ++uniformIndex)
    {
        GLint elementCount = 0;
        GLenum type = 0;
        glGetActiveUniform(programObject, uniformIndex, maxNameLength, nullptr, &elementCount, &type, name);

        if (const auto sanitatedResourceName = SanitateResourceName(name))
        {
            reflection->AddOrCheckShaderResource(StringHash{*sanitatedResourceName}, name);
            continue;
        }

        const auto sanitatedName = SanitateGLUniformName(name);
        if (!sanitatedName)
            continue;

        GLint blockIndex = 0;
        GLint blockOffset = 0;
        glGetActiveUniformsiv(programObject, 1, &uniformIndex, GL_UNIFORM_BLOCK_INDEX, &blockIndex);
        glGetActiveUniformsiv(programObject, 1, &uniformIndex, GL_UNIFORM_OFFSET, &blockOffset);

        if (blockIndex >= 0 && blockIndex < indexToGroup.size())
        {
            const ShaderParameterGroup group = indexToGroup[blockIndex];
            if (group != MAX_SHADER_PARAMETER_GROUPS)
            {
                const unsigned size = GetUniformSize(CreateUniformDesc(type, elementCount));
                reflection->AddOrCheckUniform(group, *sanitatedName, size, blockOffset);
            }
        }
    }

    return reflection;
}

#endif

}

using namespace Diligent;
bool PipelineState::BuildPipeline(Graphics* graphics)
{
    Diligent::IRenderDevice* renderDevice = graphics->GetImpl()->GetDevice();
    const bool isOpenGL = graphics->GetRenderBackend() == RENDER_GL;
    const bool hasSeparableShaderPrograms = renderDevice->GetDeviceInfo().Features.SeparablePrograms;
    URHO3D_ASSERT(isOpenGL || hasSeparableShaderPrograms);

    // TODO(diligent): This is hack to force shader compilation, we should not need it
    graphics->GetShaderProgramLayout(desc_.vertexShader_, desc_.pixelShader_);

    GraphicsPipelineStateCreateInfo ci;

    ea::vector<LayoutElement> layoutElements;
    InitializeLayoutElements(layoutElements, desc_.GetVertexElements());

    ea::vector<Diligent::ImmutableSamplerDesc> immutableSamplers;

    Diligent::IShader* vertexShader = ToHandle(desc_.vertexShader_);
    Diligent::IShader* pixelShader = ToHandle(desc_.pixelShader_);
    Diligent::IShader* domainShader = ToHandle(desc_.domainShader_);
    Diligent::IShader* hullShader = ToHandle(desc_.hullShader_);
    Diligent::IShader* geometryShader = ToHandle(desc_.geometryShader_);

    // On OpenGL, vertex layout initialization is postponed until the program is linked.
    if (!isOpenGL)
    {
        const VertexShaderAttributeVector& vertexShaderAttributes = desc_.vertexShader_->GetVertexShaderAttributes();
        FillLayoutElementIndices(layoutElements, desc_.GetVertexElements(), vertexShaderAttributes);
        const unsigned numElements = RemoveUnusedElements(layoutElements);
        layoutElements.resize(numElements);
    }

    // On OpenGL, uniform layout initialization may be postponed.
    if (hasSeparableShaderPrograms)
    {
        reflection_ = ReflectShaders({vertexShader, pixelShader, domainShader, hullShader, geometryShader});

        InitializeImmutableSamplers(immutableSamplers, desc_, *reflection_);
        ci.PSODesc.ResourceLayout.NumImmutableSamplers = immutableSamplers.size();
        ci.PSODesc.ResourceLayout.ImmutableSamplers = immutableSamplers.data();
    }

#ifdef URHO3D_DEBUG
    const ea::string debugName = Format("{}#{}", desc_.debugName_, desc_.ToHash());
    ci.PSODesc.Name = debugName.c_str();
#endif

    ci.GraphicsPipeline.PrimitiveTopology = DiligentPrimitiveTopology[desc_.primitiveType_];

    ci.GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
    ci.GraphicsPipeline.InputLayout.LayoutElements = layoutElements.data();

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
    ci.GraphicsPipeline.RasterizerDesc.AntialiasedLineEnable = desc_.lineAntiAlias_;

    ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = desc_.scissorTestEnabled_;

    ci.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

    PipelineStateCache* psoCache = graphics->GetSubsystem<PipelineStateCache>();
    if (psoCache->object_)
        ci.pPSOCache = psoCache->object_.Cast<IPipelineStateCache>(IID_PipelineStateCache);

#if GL_SUPPORTED || GLES_SUPPORTED
    // clang-format off
    auto patchInputLayout = [&](GLuint programObject, Diligent::Uint32& numElements, Diligent::LayoutElement* elements,
        Diligent::PipelineResourceLayoutDesc& resourceLayout)
    {
        const ea::span<Diligent::LayoutElement> elementsSpan{elements, numElements};
        const auto vertexAttributes = GetGLVertexAttributes(programObject);

        FillLayoutElementIndices(elementsSpan, desc_.GetVertexElements(), vertexAttributes);
        numElements = RemoveUnusedElements(elementsSpan);

        if (!hasSeparableShaderPrograms)
        {
            reflection_ = ReflectGLProgram(programObject);

            InitializeImmutableSamplers(immutableSamplers, desc_, *reflection_);
            resourceLayout.NumImmutableSamplers = immutableSamplers.size();
            resourceLayout.ImmutableSamplers = immutableSamplers.data();
        }
    };

    ci.GLPatchVertexLayoutCallbackUserData = &patchInputLayout;
    ci.GLPatchVertexLayoutCallback = [](GLuint programObject,
        Diligent::Uint32* numElements, Diligent::LayoutElement* elements,
        Diligent::PipelineResourceLayoutDesc* resourceLayout, void* userData)
    {
        const auto& callback = *reinterpret_cast<decltype(patchInputLayout)*>(userData);
        callback(programObject, *numElements, elements, *resourceLayout);
    };
    // clang-format on
#endif

    renderDevice->CreateGraphicsPipelineState(ci, &handle_);

    if (!handle_)
    {
        URHO3D_LOGERROR("Error has ocurred at Pipeline Build. ({})", desc_.ToHash());
        return false;
    }

    handle_->CreateShaderResourceBinding(&shaderResourceBinding_, true);
    reflection_->RecalculateLayoutHash();
    reflection_->ConnectToShaderVariables(shaderResourceBinding_);

    URHO3D_LOGDEBUG("Created Graphics Pipeline ({})", desc_.ToHash());
    return true;
}

void PipelineState::ReleasePipeline()
{
    handle_ = nullptr;
}

} // namespace Urho3D
