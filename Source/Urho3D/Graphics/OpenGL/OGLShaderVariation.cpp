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

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Shader.h"
#include "../../Graphics/ShaderProgram.h"
#include "../../Graphics/ShaderVariation.h"
#include "../../IO/Log.h"
#include "../../Shader/ShaderOptimizer.h"
#include "../../Shader/ShaderSourceLogger.h"
#include "../../Shader/ShaderTranslator.h"

#include <cctype>
#include <EASTL/optional.h>

#include "../../DebugNew.h"

namespace Urho3D
{

const char* ShaderVariation::elementSemanticNames[] =
{
    "POS",
    "NORMAL",
    "BINORMAL",
    "TANGENT",
    "TEXCOORD",
    "COLOR",
    "BLENDWEIGHT",
    "BLENDINDICES",
    "OBJECTINDEX"
};

void ShaderVariation::OnDeviceLost()
{
    if (object_.name_ && !graphics_->IsDeviceLost())
        glDeleteShader(object_.name_);

    GPUObject::OnDeviceLost();

    compilerOutput_.clear();
}

void ShaderVariation::Release()
{
    if (object_.name_)
    {
        if (!graphics_)
            return;

        if (!graphics_->IsDeviceLost())
        {
            if (type_ == VS)
            {
                if (graphics_->GetVertexShader() == this)
                    graphics_->SetShaders(nullptr, nullptr);
            }
            else if (type_ == PS)
            {
                if (graphics_->GetPixelShader() == this)
                    graphics_->SetShaders(nullptr, nullptr);
            }

            glDeleteShader(object_.name_);
        }

        object_.name_ = 0;
        graphics_->CleanupShaderPrograms(this);
    }

    compilerOutput_.clear();
}

bool ShaderVariation::Create()
{
    Release();

    if (!owner_)
    {
        compilerOutput_ = "Owner shader has expired";
        return false;
    }

    GLenum shaderStage = 0;
    switch (type_)
    {
    case VS:
        shaderStage = GL_VERTEX_SHADER;
        break;
    case PS:
        shaderStage = GL_FRAGMENT_SHADER;
        break;
#if 0
    case GS:
        shaderStage = GL_GEOMETRY_SHADER;
        break;
    case HS:
        shaderStage = GL_TESS_CONTROL_SHADER;
        break;
    case DS:
        shaderStage = GL_TESS_EVALUATION_SHADER;
        break;
#endif
#ifdef URHO3D_COMPUTE
    case CS:
        shaderStage = GL_COMPUTE_SHADER;
        break;
#endif
    default:
        URHO3D_LOGERROR("ShaderVariation::Create, unsupported shader stage {}", GetShaderType());
        return false;
    }

    object_.name_ = glCreateShader(shaderStage);
    if (!object_.name_)
    {
        compilerOutput_ = "Could not create shader object";
        return false;
    }

    const ea::string& originalShaderCode = owner_->GetSourceCode(type_);
    ea::string shaderCode;
    ea::optional<TargetShaderLanguage> targetShaderLanguage;

    // Check if the shader code contains a version define
    const auto versionTag = FindVersionTag(originalShaderCode);
    if (versionTag)
    {
        // If version define found, insert it first
        const ea::string versionDefine = originalShaderCode.substr(versionTag->first, versionTag->second - versionTag->first);
        shaderCode += versionDefine + "\n";
        targetShaderLanguage = TargetShaderLanguage::GLSL_4_1;
    }
    else if (Graphics::GetGL3Support())
    {
#ifdef MOBILE_GRAPHICS
        shaderCode += "#version 300 es\n";
        targetShaderLanguage = TargetShaderLanguage::GLSL_ES_3_0;
#else
        shaderCode += "#version 150\n";
        targetShaderLanguage = TargetShaderLanguage::GLSL_3_2;
#endif
    }
#if defined(DESKTOP_GRAPHICS)
    shaderCode += "#define DESKTOP_GRAPHICS\n";
#elif defined(MOBILE_GRAPHICS)
    shaderCode += "#define MOBILE_GRAPHICS\n";
#endif

    // Distinguish between VS and PS compile in case the shader code wants to include/omit different things
    static const char* STAGE_DEFS[] = {
        "#define COMPILEVS\n", // VS
        "#define COMPILEPS\n", // PS
        "#define COMPILEGS\n", // GS
        "#define COMPILEHS\n", // HS
        "#define COMPILEDS\n", // DS
        "#define COMPILECS\n", // CS
    };
    shaderCode += STAGE_DEFS[type_];

    // Add define for the maximum number of supported bones
    shaderCode += "#define MAXBONES " + ea::to_string(Graphics::GetMaxBones()) + "\n";

    // Prepend the defines to the shader code
    ea::vector<ea::string> defineVec = defines_.split(' ');
    for (unsigned i = 0; i < defineVec.size(); ++i)
    {
        // Add extra space for the checking code below
        ea::string defineString = "#define " + defineVec[i].replaced('=', ' ') + " \n";
        shaderCode += defineString;

        // In debug mode, check that all defines are referenced by the shader code
#ifdef _DEBUG
        ea::string defineCheck = defineString.substr(8, defineString.find(' ', 8) - 8);
        if (originalShaderCode.find(defineCheck) == ea::string::npos)
            URHO3D_LOGWARNING("Shader " + GetFullName() + " does not use the define " + defineCheck);
#endif
    }

#ifdef RPI
    if (type_ == VS)
        shaderCode += "#define RPI\n";
#endif
#ifdef __EMSCRIPTEN__
    shaderCode += "#define WEBGL\n";
#endif
    if (Graphics::GetGL3Support())
        shaderCode += "#define GL3\n";

    // When version define found, do not insert it a second time
    if (versionTag)
    {
        shaderCode += originalShaderCode.substr(0, versionTag->first);
        shaderCode += "//";
        shaderCode += originalShaderCode.substr(versionTag->first);
    }
    else
        shaderCode += originalShaderCode;

#ifdef URHO3D_SHADER_TRANSLATOR
    const ShaderTranslationPolicy policy = graphics_->GetPolicyGLSL();
    if (targetShaderLanguage && policy != ShaderTranslationPolicy::Verbatim)
    {
        // TODO: Make it optional
        static thread_local SpirVShader spirvShader;
        static thread_local TargetShader glslShader;

        ParseUniversalShader(spirvShader, type_, shaderCode, ShaderDefineArray{});
        if (!spirvShader)
        {
            URHO3D_LOGERROR("Failed to convert shader {} from GLSL to SPIR-V:\n{}{}", GetFullName(),
                Shader::GetShaderFileList(), spirvShader.compilerOutput_);
            return false;
        }

#ifdef URHO3D_SHADER_OPTIMIZER
        if (policy == ShaderTranslationPolicy::Optimize)
            OptimizeSpirVShader(spirvShader, *targetShaderLanguage);
#endif

        TranslateSpirVShader(glslShader, spirvShader, *targetShaderLanguage);
        if (!glslShader)
        {
            URHO3D_LOGERROR("Failed to convert shader {} from SPIR-V to GLSL:\n{}{}", GetFullName(),
                Shader::GetShaderFileList(), glslShader.compilerOutput_);
            return false;
        }

        shaderCode = glslShader.sourceCode_;
    }
#endif

    LogShaderSource(owner_->GetName(), type_, defines_, shaderCode, "glsl");

    const char* shaderCStr = shaderCode.c_str();
    glShaderSource(object_.name_, 1, &shaderCStr, nullptr);
    glCompileShader(object_.name_);

    int compiled, length;
    glGetShaderiv(object_.name_, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        glGetShaderiv(object_.name_, GL_INFO_LOG_LENGTH, &length);
        compilerOutput_.resize((unsigned) length);
        int outLength;
        glGetShaderInfoLog(object_.name_, length, &outLength, &compilerOutput_[0]);
        glDeleteShader(object_.name_);
        object_.name_ = 0;

        compilerOutput_ = Shader::GetShaderFileList() + compilerOutput_;
    }
    else
        compilerOutput_.clear();

    return object_.name_ != 0;
}

void ShaderVariation::SetDefines(const ea::string& defines)
{
    defines_ = defines;
}

// These methods are no-ops for OpenGL
bool ShaderVariation::LoadByteCode(const FileIdentifier& binaryShaderName) { return false; }
bool ShaderVariation::Compile() { return false; }
void ShaderVariation::ParseParameters(unsigned char* bufData, unsigned bufSize) {}
void ShaderVariation::SaveByteCode(const FileIdentifier& binaryShaderName) {}
void ShaderVariation::CalculateConstantBufferSizes() {}

}
