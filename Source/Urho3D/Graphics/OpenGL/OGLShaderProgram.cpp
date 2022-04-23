//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../../Precompiled.h"

#include "../../Graphics/ConstantBuffer.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/ShaderProgram.h"
#include "../../Graphics/ShaderVariation.h"
#include "../../IO/Log.h"

#include "../../DebugNew.h"

namespace Urho3D
{

static const char* shaderParameterGroups[] = {
    "frame",
    "camera",
    "zone",
    "light",
    "material",
    "object",
    "custom"
};

static unsigned NumberPostfix(const ea::string& str)
{
    for (unsigned i = 0; i < str.length(); ++i)
    {
        if (IsDigit(str[i]))
            return ToUInt(str.c_str() + i);
    }

    return M_MAX_UNSIGNED;
}

static unsigned GetUniformElementSize(int glType)
{
    switch (glType)
    {
    case GL_BOOL:
    case GL_INT:
    case GL_FLOAT:
        return sizeof(float);
    case GL_FLOAT_VEC2:
        return 2 * sizeof(float);
    case GL_FLOAT_VEC3:
        return 3 * sizeof(float);
    case GL_FLOAT_VEC4:
        return 4 * sizeof(float);
    case GL_FLOAT_MAT3:
#ifndef GL_ES_VERSION_2_0
    case GL_FLOAT_MAT3x4:
#endif
        return 12 * sizeof(float);
    case GL_FLOAT_MAT4:
        return 16 * sizeof(float);
    default:
        return 0;
    }
}

static bool IsIntegerType(int glType)
{
    switch (glType)
    {
    case GL_INT:
    case GL_INT_VEC2:
    case GL_INT_VEC3:
    case GL_INT_VEC4:
    case GL_UNSIGNED_INT:
#ifndef GL_ES_VERSION_2_0
    case GL_UNSIGNED_INT_VEC2:
    case GL_UNSIGNED_INT_VEC3:
    case GL_UNSIGNED_INT_VEC4:
#endif
        return true;
    default:
        return false;
    };
}

static unsigned GetUniformSize(int glType, int arraySize)
{
    const unsigned size = GetUniformElementSize(glType);
    const unsigned minStride = 4 * sizeof(float);
    return size < minStride && arraySize > 1 ? M_MAX_UNSIGNED : size * arraySize - (glType == GL_FLOAT_MAT3 ? sizeof(float) : 0);
}

unsigned ShaderProgram::globalFrameNumber = 0;

ShaderProgram::ShaderProgram(Graphics* graphics, ShaderVariation* vertexShader, ShaderVariation* pixelShader) :
    GPUObject(graphics),
    vertexShader_(vertexShader),
    pixelShader_(pixelShader)
{
    for (auto& parameterSource : parameterSources_)
        parameterSource = (const void*)(uintptr_t)M_MAX_UNSIGNED;
}

ShaderProgram::ShaderProgram(Graphics* graphics, ShaderVariation* computeShader) :
    GPUObject(graphics),
    computeShader_(computeShader)
{
#ifdef URHO3D_COMPUTE
    if (computeShader == nullptr)
    {
        URHO3D_LOGERROR("Provided null compute shader to ShaderProgram constructor");
    }

    if (computeShader->GetShaderType() != CS)
    {
        switch (computeShader->GetShaderType())
        {
        case VS:
            URHO3D_LOGERROR("Provided vertex shader to ShaderProgram compute-shader constructor");
            break;
        case PS:
            URHO3D_LOGERROR("Provided pixel shader to ShaderProgram compute-shader constructor");
            break;
        case GS:
            URHO3D_LOGERROR("Provided geometry shader to ShaderProgram compute-shader constructor");
            break;
        case HS:
            URHO3D_LOGERROR("Provided hull shader to ShaderProgram compute-shader constructor");
            break;
        case DS:
            URHO3D_LOGERROR("Provided domain shader to ShaderProgram compute-shader constructor");
            break;
        }
    }

    for (auto& parameterSource : parameterSources_)
        parameterSource = (const void*)(uintptr_t)M_MAX_UNSIGNED;
#else
    URHO3D_LOGERROR("ComputeShader is not supported");
#endif
}

ShaderProgram::~ShaderProgram()
{
    Release();
}

void ShaderProgram::OnDeviceLost()
{
    if (object_.name_ && !graphics_->IsDeviceLost())
        glDeleteProgram(object_.name_);

    GPUObject::OnDeviceLost();

    if (graphics_ && graphics_->GetShaderProgram() == this)
        graphics_->SetShaders(nullptr, nullptr);

    linkerOutput_.clear();
}

void ShaderProgram::Release()
{
    if (object_.name_)
    {
        if (!graphics_)
            return;

        if (!graphics_->IsDeviceLost())
        {
            if (graphics_->GetShaderProgram() == this)
                graphics_->SetShaders(nullptr, nullptr);

            glDeleteProgram(object_.name_);
        }

        object_.name_ = 0;
        linkerOutput_.clear();
        shaderParameters_.clear();
        vertexAttributes_.clear();
        usedVertexAttributes_ = 0;

        for (bool& useTextureUnit : useTextureUnits_)
            useTextureUnit = false;
    }
}

bool ShaderProgram::Link()
{
    Release();

    // Compute shader has a short path at present.
#if URHO3D_COMPUTE
    if (computeShader_)
    {
        object_.name_ = glCreateProgram();
        if (!object_.name_)
        {
            linkerOutput_ = "Could not create shader program";
            return false;
        }

        glAttachShader(object_.name_, computeShader_->GetGPUObjectName());
        glLinkProgram(object_.name_);

        int linked, length;
        glGetProgramiv(object_.name_, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            glGetProgramiv(object_.name_, GL_INFO_LOG_LENGTH, &length);
            linkerOutput_.resize((unsigned)length);
            int outLength;
            glGetProgramInfoLog(object_.name_, length, &outLength, &linkerOutput_[0]);
            glDeleteProgram(object_.name_);
            object_.name_ = 0;
            return false;
        }

        linkerOutput_.clear();
        return true;
    }
#endif

    if (!vertexShader_ || !pixelShader_ || !vertexShader_->GetGPUObjectName() || !pixelShader_->GetGPUObjectName())
        return false;

    object_.name_ = glCreateProgram();
    if (!object_.name_)
    {
        linkerOutput_ = "Could not create shader program";
        return false;
    }

    glAttachShader(object_.name_, vertexShader_->GetGPUObjectName());
    glAttachShader(object_.name_, pixelShader_->GetGPUObjectName());
    glLinkProgram(object_.name_);

    int linked, length;
    glGetProgramiv(object_.name_, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        glGetProgramiv(object_.name_, GL_INFO_LOG_LENGTH, &length);
        linkerOutput_.resize((unsigned) length);
        int outLength;
        glGetProgramInfoLog(object_.name_, length, &outLength, &linkerOutput_[0]);
        glDeleteProgram(object_.name_);
        object_.name_ = 0;
    }
    else
        linkerOutput_.clear();

    if (!object_.name_)
        return false;

    const int MAX_NAME_LENGTH = 256;
    char nameBuffer[MAX_NAME_LENGTH];
    int attributeCount, uniformCount, elementCount, nameLength;
    GLenum type;

    glUseProgram(object_.name_);

    // Check for vertex attributes
    glGetProgramiv(object_.name_, GL_ACTIVE_ATTRIBUTES, &attributeCount);
    for (int i = 0; i < attributeCount; ++i)
    {
        glGetActiveAttrib(object_.name_, i, (GLsizei)MAX_NAME_LENGTH, &nameLength, &elementCount, &type, nameBuffer);

        ea::string name = ea::string(nameBuffer, nameLength);
        VertexElementSemantic semantic = MAX_VERTEX_ELEMENT_SEMANTICS;
        unsigned char semanticIndex = 0;

        // Go in reverse order so that "binormal" is detected before "normal"
        for (unsigned j = MAX_VERTEX_ELEMENT_SEMANTICS - 1; j < MAX_VERTEX_ELEMENT_SEMANTICS; --j)
        {
            if (name.contains(ShaderVariation::elementSemanticNames[j], false))
            {
                semantic = (VertexElementSemantic)j;
                unsigned index = NumberPostfix(name);
                if (index != M_MAX_UNSIGNED)
                    semanticIndex = (unsigned char)index;
                break;
            }
        }

        if (semantic == MAX_VERTEX_ELEMENT_SEMANTICS)
        {
            URHO3D_LOGWARNING("Found vertex attribute " + name + " with no known semantic in shader program " +
                vertexShader_->GetFullName() + " " + pixelShader_->GetFullName());
            continue;
        }

        const int location = glGetAttribLocation(object_.name_, name.c_str());
        const bool isInteger = IsIntegerType(type);
        vertexAttributes_[ea::make_pair((unsigned char)semantic, semanticIndex)] = { location, isInteger };
        usedVertexAttributes_ |= (1u << location);
    }

    // Check for constant buffers
#ifndef GL_ES_VERSION_2_0
    ea::unordered_map<unsigned, unsigned> blockToBinding;

    if (Graphics::GetGL3Support())
    {
        int numUniformBlocks = 0;

        glGetProgramiv(object_.name_, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBlocks);
        for (int i = 0; i < numUniformBlocks; ++i)
        {
            glGetActiveUniformBlockName(object_.name_, (GLuint)i, MAX_NAME_LENGTH, &nameLength, nameBuffer);

            ea::string name(nameBuffer, (unsigned)nameLength);

            unsigned blockIndex = glGetUniformBlockIndex(object_.name_, name.c_str());
            unsigned group = M_MAX_UNSIGNED;

            // Try to recognize the use of the buffer from its name
            for (unsigned j = 0; j < MAX_SHADER_PARAMETER_GROUPS; ++j)
            {
                if (name.contains(shaderParameterGroups[j], false))
                {
                    group = j;
                    break;
                }
            }

            // If name is not recognized, search for a digit in the name and use that as the group index
            if (group == M_MAX_UNSIGNED)
                group = NumberPostfix(name);

            if (group >= MAX_SHADER_PARAMETER_GROUPS)
            {
                URHO3D_LOGWARNING("Skipping unrecognized uniform block " + name + " in shader program " + vertexShader_->GetFullName() +
                           " " + pixelShader_->GetFullName());
                continue;
            }

            // Find total constant buffer data size
            int dataSize;
            glGetActiveUniformBlockiv(object_.name_, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &dataSize);
            if (!dataSize)
                continue;

            // Register in layout
            AddConstantBuffer(static_cast<ShaderParameterGroup>(group), static_cast<unsigned>(dataSize));

            const unsigned bindingIndex = group;
            glUniformBlockBinding(object_.name_, blockIndex, bindingIndex);
            blockToBinding[blockIndex] = bindingIndex;
        }
    }
#endif

    // Check for shader parameters and texture units
    glGetProgramiv(object_.name_, GL_ACTIVE_UNIFORMS, &uniformCount);
    for (int i = 0; i < uniformCount; ++i)
    {
        glGetActiveUniform(object_.name_, (GLuint)i, MAX_NAME_LENGTH, nullptr, &elementCount, &type, nameBuffer);
        int location = glGetUniformLocation(object_.name_, nameBuffer);

        // Check for array index included in the name and strip it
        ea::string name(nameBuffer);
        unsigned index = name.find('[');
        if (index != ea::string::npos)
        {
            // If not the first index, skip
            if (name.find("[0]", index) == ea::string::npos)
                continue;

            name = name.substr(0, index);
        }

        if (name[0] == 'c')
        {
            // Store constant uniform
            ea::string paramName = name.substr(1);
            ShaderParameter parameter{paramName, type, location};
            if (location >= 0)
                shaderParameters_[StringHash(paramName)] = parameter;

#ifndef GL_ES_VERSION_2_0
            // If running OpenGL 3, the uniform may be inside a constant buffer
            if (parameter.location_ < 0 && Graphics::GetGL3Support())
            {
                int blockIndex, blockOffset;
                glGetActiveUniformsiv(object_.name_, 1, (const GLuint*)&i, GL_UNIFORM_BLOCK_INDEX, &blockIndex);
                glGetActiveUniformsiv(object_.name_, 1, (const GLuint*)&i, GL_UNIFORM_OFFSET, &blockOffset);
                if (blockIndex >= 0)
                {
                    // Register in layout
                    const unsigned parameterGroup = blockToBinding[blockIndex] % MAX_SHADER_PARAMETER_GROUPS;
                    const unsigned size = GetUniformSize(type, elementCount);
                    if (size == M_MAX_UNSIGNED)
                    {
                        URHO3D_LOGERROR("Invalid shader parameter '{}': "
                            "only vec4, mat3x4 and mat4 arrays are supported", paramName);
                        continue;
                    }
                    AddConstantBufferParameter(paramName,
                        static_cast<ShaderParameterGroup>(parameterGroup), blockOffset, size);
                }
            }
#endif
        }
        else if (location >= 0 && name[0] == 's')
        {
            // Set the samplers here so that they do not have to be set later
            unsigned unit = graphics_->GetTextureUnit(name.substr(1));
            if (unit >= MAX_TEXTURE_UNITS)
                unit = NumberPostfix(name);

            if (unit < MAX_TEXTURE_UNITS)
            {
                useTextureUnits_[unit] = true;
                glUniform1iv(location, 1, reinterpret_cast<int*>(&unit));
            }
        }
    }

    // Rehash the parameter & vertex attributes maps to ensure minimal load factor
    vertexAttributes_.rehash(Max(2, NextPowerOfTwo(vertexAttributes_.size())));
    shaderParameters_.rehash(Max(2, NextPowerOfTwo(shaderParameters_.size())));

    RecalculateLayoutHash();
    return true;
}

ShaderVariation* ShaderProgram::GetVertexShader() const
{
    return vertexShader_;
}

ShaderVariation* ShaderProgram::GetPixelShader() const
{
    return pixelShader_;
}

ShaderVariation* ShaderProgram::GetComputeShader() const
{
    return computeShader_;
}

bool ShaderProgram::HasParameter(StringHash param) const
{
    return shaderParameters_.find(param) != shaderParameters_.end();
}

const ShaderParameter* ShaderProgram::GetParameter(StringHash param) const
{
    auto i = shaderParameters_.find(param);
    if (i != shaderParameters_.end())
        return &i->second;
    else
        return nullptr;
}

bool ShaderProgram::NeedParameterUpdate(ShaderParameterGroup group, const void* source)
{
    // If global framenumber has changed, invalidate all per-program parameter sources now
    if (globalFrameNumber != frameNumber_)
    {
        for (auto& parameterSource : parameterSources_)
            parameterSource = (const void*)(uintptr_t)M_MAX_UNSIGNED;
        frameNumber_ = globalFrameNumber;
    }

    if (parameterSources_[group] != source)
    {
        parameterSources_[group] = source;
        return true;
    }
    else
        return false;
}

void ShaderProgram::ClearParameterSource(ShaderParameterGroup group)
{
    parameterSources_[group] = (const void*)(uintptr_t)M_MAX_UNSIGNED;
}

void ShaderProgram::ClearParameterSources()
{
    ++globalFrameNumber;
    if (!globalFrameNumber)
        ++globalFrameNumber;
}

void ShaderProgram::ClearGlobalParameterSource(ShaderParameterGroup group)
{
}

}
