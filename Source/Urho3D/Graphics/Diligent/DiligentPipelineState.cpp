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
    void AddOrCheckConstantBuffer(ShaderParameterGroup group, unsigned size)
    {
        const unsigned oldSize = GetConstantBufferSize(group);
        if (oldSize != 0)
        {
            if (oldSize != size)
                URHO3D_LOGWARNING("Constant buffer #{} has inconsistent size in different stages", group);
            return;
        }

        AddConstantBuffer(group, size);
    }

    void AddOrCheckShaderParameter(ShaderParameterGroup group, const Diligent::ShaderCodeVariableDesc& desc)
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

        AddOrCheckShaderParameter(group, *sanitatedName, uniformSize, desc.Offset);
    }

    void AddOrCheckShaderParameter(ShaderParameterGroup group, ea::string_view name, unsigned size, unsigned offset)
    {
        const auto nameHash = StringHash{name};
        const ShaderParameterReflection* oldParameter = GetConstantBufferParameter(nameHash);
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

        AddConstantBufferParameter(nameHash, group, offset, size);
    }

    void AddOrCheckShaderResource(StringHash name, ea::string_view internalName)
    {
        AddShaderResource(name, internalName);
    }

    void Cook()
    {
        RecalculateLayoutHash();
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
    reflection.AddOrCheckConstantBuffer(*bufferGroup, bufferDesc.Size);

    const unsigned numUniforms = bufferDesc.NumVariables;
    for (unsigned uniformIndex = 0; uniformIndex < numUniforms; ++uniformIndex)
    {
        const Diligent::ShaderCodeVariableDesc& uniformDesc = bufferDesc.pVariables[uniformIndex];
        reflection.AddOrCheckShaderParameter(*bufferGroup, uniformDesc);
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
    reflection->Cook();
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

    ea::string name;
    for (GLuint uniformBlockIndex = 0; uniformBlockIndex < static_cast<GLuint>(numUniformBlocks); ++uniformBlockIndex)
    {
        GLint nameLength = 0;
        glGetActiveUniformBlockiv(programObject, uniformBlockIndex, GL_UNIFORM_BLOCK_NAME_LENGTH, &nameLength);
        name.resize(nameLength, '\0');
        glGetActiveUniformBlockName(programObject, uniformBlockIndex, nameLength, &nameLength, name.data());

        const auto bufferGroup = ParseConstantBufferName(name.c_str());
        if (!bufferGroup)
        {
            URHO3D_LOGWARNING("Unknown constant buffer '{}' is ignored", name);
            continue;
        }

        GLint dataSize = 0;
        glGetActiveUniformBlockiv(programObject, uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &dataSize);

        reflection->AddOrCheckConstantBuffer(*bufferGroup, dataSize);

        const unsigned blockIndex = glGetUniformBlockIndex(programObject, name.c_str());
        if (blockIndex >= indexToGroup.size())
            indexToGroup.resize(blockIndex + 1, MAX_SHADER_PARAMETER_GROUPS);
        indexToGroup[blockIndex] = *bufferGroup;
    }

    for (GLuint uniformIndex = 0; uniformIndex < static_cast<GLuint>(numUniforms); ++uniformIndex)
    {
        GLint nameLength = 0;
        glGetActiveUniformsiv(programObject, 1, &uniformIndex, GL_UNIFORM_NAME_LENGTH, &nameLength);
        name.resize(nameLength, '\0');

        GLint elementCount = 0;
        GLenum type = 0;
        glGetActiveUniform(programObject, uniformIndex, nameLength, nullptr, &elementCount, &type, name.data());

        if (const auto sanitatedResourceName = SanitateResourceName(name.c_str()))
        {
            reflection->AddOrCheckShaderResource(StringHash{*sanitatedResourceName}, name.c_str());
            continue;
        }

        const auto sanitatedName = SanitateGLUniformName(name.c_str());
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
                reflection->AddOrCheckShaderParameter(group, *sanitatedName, size, blockOffset);
            }
        }
    }

    reflection->Cook();
    return reflection;
}

#endif

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
        handle_ = nullptr;
    if (handle_)
        return true;

    Diligent::IRenderDevice* renderDevice = graphics->GetImpl()->GetDevice();
    const bool isOpenGL = graphics->GetRenderBackend() == RENDER_GL;
    const bool hasSeparableShaderPrograms = renderDevice->GetDeviceInfo().Features.SeparablePrograms;
    URHO3D_ASSERT(isOpenGL || hasSeparableShaderPrograms);

    // TODO(diligent): This is hack to force shader compilation, we should not need it
    graphics->GetShaderProgramLayout(desc_.vertexShader_, desc_.pixelShader_);

    ea::vector<LayoutElement> layoutElements;
    InitializeLayoutElements(layoutElements, desc_.GetVertexElements());

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

        if (!hasSeparableShaderPrograms)
            reflection_ = ReflectGLProgram(programObject);
    };

    ci.GLPatchVertexLayoutCallbackUserData = &patchInputLayout;
    ci.GLPatchVertexLayoutCallback = [](
        GLuint programObject, Diligent::Uint32* numElements, Diligent::LayoutElement* elements, void* userData)
    {
        const auto& callback = *reinterpret_cast<decltype(patchInputLayout)*>(userData);
        callback(programObject, *numElements, elements);
    };
#endif

    graphics->GetImpl()->GetDevice()->CreateGraphicsPipelineState(ci, &handle_);

    if (!handle_)
    {
        URHO3D_LOGERROR("Error has ocurred at Pipeline Build. ({})", desc_.ToHash());
        return false;
    }

    URHO3D_LOGDEBUG("Created Graphics Pipeline ({})", desc_.ToHash());
    return true;
}

void PipelineState::ReleasePipeline()
{
    handle_ = nullptr;
}

ShaderResourceBinding* PipelineState::CreateInternalSRB()
{
    assert(handle_);

    IPipelineState* pipeline = static_cast<IPipelineState*>(handle_);
    IShaderResourceBinding* srb = nullptr;
    pipeline->CreateShaderResourceBinding(&srb, false);
    assert(srb);

    return new ShaderResourceBinding(owner_->GetSubsystem<Graphics>(), srb);
}
} // namespace Urho3D
