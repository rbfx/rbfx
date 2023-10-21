// Copyright (c) 2020-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/ShaderProgramReflection.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/GAPIIncludes.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"

#include <Diligent/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>

namespace Urho3D
{

namespace
{

ea::span<const ShaderType> GetShaderTypes(PipelineStateType pipelineType)
{
    static const ShaderType graphicsShaderTypes[] = {VS, PS, GS, HS, DS};
    static const ShaderType computeShaderTypes[] = {CS};
    switch (pipelineType)
    {
    case PipelineStateType::Graphics: return graphicsShaderTypes;
    case PipelineStateType::Compute: return computeShaderTypes;
    default: URHO3D_ASSERT(false); return {};
    }
}

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

ea::optional<ea::string_view> SanitizeUniformName(ea::string_view name)
{
    const auto pos = name.find('c');
    if (pos == ea::string_view::npos || pos + 1 == name.length())
        return ea::nullopt;

    return name.substr(pos + 1);
}

ea::optional<ea::string_view> SanitizeSRVName(ea::string_view name)
{
    if (name.empty() || name[0] != 's')
        return ea::nullopt;

    return name.substr(1);
}

ea::optional<ea::string_view> SanitizeUAVName(ea::string_view name)
{
    if (name.empty() || name[0] != 'u')
        return ea::nullopt;

    return name.substr(1);
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

#if GL_SUPPORTED || GLES_SUPPORTED

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

#endif

} // namespace

ShaderProgramReflection::ShaderProgramReflection(ea::span<Diligent::IShader* const> shaders)
{
    for (Diligent::IShader* shader : shaders)
    {
        if (shader)
            ReflectShader(shader);
    }
    RecalculateUniformHash();
}

ShaderProgramReflection::ShaderProgramReflection(unsigned programObject)
{
#if GL_SUPPORTED || GLES_SUPPORTED
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

        AddUniformBuffer(*bufferGroup, name, dataSize);

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

        if (const auto sanitatedSRVName = SanitizeSRVName(name))
        {
            AddShaderResource(StringHash{*sanitatedSRVName}, name);
            continue;
        }

        if (const auto sanitatedUAVName = SanitizeUAVName(name))
        {
            AddUnorderedAccessView(StringHash{*sanitatedUAVName}, name);
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
                AddUniform(*sanitatedName, group, blockOffset, size);
            }
        }
    }
    RecalculateUniformHash();
#else
    URHO3D_ASSERT(false, "Not implemented");
#endif
}

void ShaderProgramReflection::ReflectShader(Diligent::IShader* shader)
{
    const unsigned numResources = shader->GetResourceCount();
    for (unsigned resourceIndex = 0; resourceIndex < numResources; ++resourceIndex)
    {
        Diligent::ShaderResourceDesc desc;
        shader->GetResourceDesc(resourceIndex, desc);

        switch (desc.Type)
        {
        case Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
        {
            const Diligent::ShaderCodeBufferDesc& bufferDesc = *shader->GetConstantBufferDesc(resourceIndex);
            ReflectUniformBuffer(desc, bufferDesc);
            break;
        }

        case Diligent::SHADER_RESOURCE_TYPE_TEXTURE_SRV:
        {
            if (const auto sanitatedName = SanitizeSRVName(desc.Name))
                AddShaderResource(StringHash{*sanitatedName}, desc.Name);
            break;
        }

        case Diligent::SHADER_RESOURCE_TYPE_TEXTURE_UAV:
        {
            if (const auto sanitatedName = SanitizeUAVName(desc.Name))
                AddUnorderedAccessView(StringHash{*sanitatedName}, desc.Name);
            break;
        }

        default: break;
        }
    }
}

void ShaderProgramReflection::ReflectUniformBuffer(
    const Diligent::ShaderResourceDesc& resourceDesc, const Diligent::ShaderCodeBufferDesc& bufferDesc)
{
    const auto bufferGroup = ParseConstantBufferName(resourceDesc.Name);
    if (!bufferGroup)
    {
        URHO3D_LOGWARNING("Unknown constant buffer '{}' is ignored", resourceDesc.Name);
        return;
    }

    AddUniformBuffer(*bufferGroup, resourceDesc.Name, bufferDesc.Size);

    const unsigned numUniforms = bufferDesc.NumVariables;
    for (unsigned uniformIndex = 0; uniformIndex < numUniforms; ++uniformIndex)
    {
        const Diligent::ShaderCodeVariableDesc& uniformDesc = bufferDesc.pVariables[uniformIndex];
        AddUniform(*bufferGroup, uniformDesc);
    }
}

void ShaderProgramReflection::AddUniformBuffer(ShaderParameterGroup group, ea::string_view internalName, unsigned size)
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

    uniformBuffers_[group] = UniformBufferReflection{size, 0u, ea::string{internalName}};
}

void ShaderProgramReflection::AddUniform(ea::string_view name, ShaderParameterGroup group, unsigned offset, unsigned size)
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

    uniforms_.emplace(name, UniformReflection{group, offset, size});
}

void ShaderProgramReflection::AddUniform(ShaderParameterGroup group, const Diligent::ShaderCodeVariableDesc& desc)
{
    const auto sanitatedName = SanitizeUniformName(desc.Name);
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

    AddUniform(*sanitatedName, group, desc.Offset, uniformSize);
}

void ShaderProgramReflection::AddShaderResource(StringHash name, ea::string_view internalName)
{
    const ShaderResourceReflection* oldResource = GetShaderResource(name);
    if (oldResource)
    {
        URHO3D_LOGWARNING("Shader resource '{}' is referenced by multiple shader stages", internalName);
        return;
    }

    shaderResources_.emplace(name, ShaderResourceReflection{ea::string{internalName}});
}

void ShaderProgramReflection::AddUnorderedAccessView(StringHash name, ea::string_view internalName)
{
    const ShaderResourceReflection* oldResource = GetUnorderedAccessView(name);
    if (oldResource)
    {
        URHO3D_LOGWARNING("Unordered access view '{}' is referenced by multiple shader stages", internalName);
        return;
    }

    unorderedAccessViews_.emplace(name, ShaderResourceReflection{ea::string{internalName}});
}

void ShaderProgramReflection::RecalculateUniformHash()
{
    for (UniformBufferReflection& uniformBuffer : uniformBuffers_)
    {
        uniformBuffer.hash_ = 0;
        if (uniformBuffer.size_ != 0)
            CombineHash(uniformBuffer.hash_, uniformBuffer.size_);
    }

    for (const auto& [nameHash, uniform] : uniforms_)
    {
        UniformBufferReflection& uniformBuffer = uniformBuffers_[uniform.group_];
        CombineHash(uniformBuffer.hash_, nameHash.Value());
        CombineHash(uniformBuffer.hash_, uniform.offset_);
        CombineHash(uniformBuffer.hash_, uniform.size_);

        if (uniformBuffer.hash_ == 0)
            uniformBuffer.hash_ = 1;
    }
}

void ShaderProgramReflection::ConnectToShaderVariables(
    PipelineStateType pipelineType, Diligent::IShaderResourceBinding* binding)
{
    const auto shaderTypes = GetShaderTypes(pipelineType);

    for (UniformBufferReflection& uniformBuffer : uniformBuffers_)
    {
        if (uniformBuffer.size_ == 0)
            continue;

        for (ShaderType shaderType : shaderTypes)
        {
            Diligent::IShaderResourceVariable* shaderVariable =
                binding->GetVariableByName(ToInternalShaderType(shaderType), uniformBuffer.internalName_.c_str());
            if (shaderVariable)
                uniformBuffer.variables_.push_back(shaderVariable);
        }
    }

    for (auto& [nameHash, resource] : shaderResources_)
    {
        for (ShaderType shaderType : shaderTypes)
        {
            Diligent::IShaderResourceVariable* shaderVariable =
                binding->GetVariableByName(ToInternalShaderType(shaderType), resource.internalName_.c_str());
            if (shaderVariable)
            {
                resource.variable_ = shaderVariable;
                break;
            }
        }
    }

    for (auto& [nameHash, resource] : unorderedAccessViews_)
    {
        for (ShaderType shaderType : shaderTypes)
        {
            Diligent::IShaderResourceVariable* shaderVariable =
                binding->GetVariableByName(ToInternalShaderType(shaderType), resource.internalName_.c_str());
            if (shaderVariable)
            {
                resource.variable_ = shaderVariable;
                break;
            }
        }
    }
}

} // namespace Urho3D
