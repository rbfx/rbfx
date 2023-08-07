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

#include "Urho3D/Precompiled.h"

#include "Urho3D/Graphics/ShaderVariation.h"

#include "Urho3D/Core/ProcessUtils.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Shader.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/VirtualFileSystem.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/Shader/ShaderCompiler.h"
#include "Urho3D/Shader/ShaderOptimizer.h"
#include "Urho3D/Shader/ShaderSourceLogger.h"
#include "Urho3D/Shader/ShaderTranslator.h"

#include <EASTL/span.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

TargetShaderLanguage GetTargetShaderLanguage(RenderBackend renderBackend)
{
    switch (renderBackend)
    {
    case RenderBackend::D3D11:
    case RenderBackend::D3D12: //
        return TargetShaderLanguage::HLSL_5_0;
    case RenderBackend::Vulkan: //
        return TargetShaderLanguage::VULKAN_1_0;
    case RenderBackend::OpenGL:
#if GLES_SUPPORTED
        return TargetShaderLanguage::GLSL_ES_3_0;
#else
        return TargetShaderLanguage::GLSL_4_1;
#endif
    default: //
        URHO3D_ASSERT(0);
        return TargetShaderLanguage::VULKAN_1_0;
    };
}

ea::string GetCompiledShaderMIME(RenderBackend renderBackend)
{
    switch (renderBackend)
    {
    case RenderBackend::D3D11:
    case RenderBackend::D3D12: //
        return "application/hlsl-bin";
    case RenderBackend::Vulkan: //
        return "application/spirv";
    case RenderBackend::OpenGL: return "application/glsl";
    default: //
        URHO3D_ASSERT(0);
        return "";
    };
}

template <class T> ConstByteSpan ToByteSpan(const T& value)
{
    using ElementType = decltype(value[0]);
    const auto sizeInBytes = static_cast<unsigned>(value.size() * sizeof(ElementType));
    const auto dataBytes = reinterpret_cast<const unsigned char*>(value.data());
    return {dataBytes, sizeInBytes};
}

} // namespace

ShaderVariation::ShaderVariation(Shader* owner, ShaderType type, const ea::string& defines)
    : RawShader(owner->GetContext(), type)
    , graphics_(GetSubsystem<Graphics>())
    , owner_(owner)
    , defines_(defines)
{
    SetDebugName(GetShaderVariationName());
    Create();

    owner->OnReloaded.Subscribe(this, &ShaderVariation::OnReloaded);
}

ea::string ShaderVariation::GetShaderName() const
{
    return owner_ ? owner_->GetShaderName() : EMPTY_STRING;
}

ea::string ShaderVariation::GetShaderVariationName() const
{
    return Format("{}({})", GetShaderName(), GetDefines());
}

void ShaderVariation::OnReloaded()
{
    Create();
}

bool ShaderVariation::Create()
{
    Destroy();

    if (!graphics_)
        return false;

    if (!owner_)
    {
        URHO3D_LOGERROR("Owner shader has expired");
        return false;
    }

    const GraphicsSettings& settings = graphics_->GetSettings();
    const FileIdentifier& cacheDir = settings.shaderCacheDir_;
    const FileIdentifier binaryShaderName = cacheDir + GetCachedVariationName("bytecode");

    if (!LoadByteCode(binaryShaderName))
    {
        // Compile shader if don't have valid bytecode
        if (!CompileFromSource())
        {
            // Notify everyone if compilation failed
            CreateFromBinary({GetShaderType()});
            return false;
        }

        // Save the bytecode after successful compile, but not if the source is from a package
        if (settings.cacheShaders_ && owner_->GetTimeStamp())
            SaveByteCode(binaryShaderName);
    }

    return true;
}

bool ShaderVariation::CompileFromSource()
{
    const ea::string& originalShaderCode = owner_->GetSourceCode();
    const ea::string sourceCode = PrepareGLSLShaderCode(originalShaderCode);

    ea::string_view translatedSource;
    const SpirVShader* translatedSpirv{};
    ConstByteSpan translatedBytecode;
    const bool processed = ProcessShaderSource(translatedSource, translatedSpirv, translatedBytecode, sourceCode);

    const FileIdentifier& cacheDir = graphics_->GetSettings().shaderCacheDir_;
    const FileIdentifier loggedSourceShaderName = cacheDir + GetCachedVariationName("glsl");
    LogShaderSource(loggedSourceShaderName, defines_, translatedSource);

    if (!processed)
        return false;

    const RenderBackend renderBackend = graphics_->GetRenderBackend();

    ShaderBytecode bytecode;
    bytecode.type_ = GetShaderType();
    bytecode.mime_ = GetCompiledShaderMIME(renderBackend);
    bytecode.bytecode_.assign(translatedBytecode.begin(), translatedBytecode.end());
    if (translatedSpirv && GetShaderType() == VS)
        bytecode.vertexAttributes_ = GetVertexAttributesFromSpirV(*translatedSpirv);

    CreateFromBinary(bytecode);
    if (!GetHandle())
        return false;

    return true;
}

bool ShaderVariation::LoadByteCode(const FileIdentifier& binaryShaderName)
{
    Context* context = Context::GetInstance();
    auto vfs = context->GetSubsystem<VirtualFileSystem>();
    if (!vfs->Exists(binaryShaderName))
        return false;

    // Check timestamps if available
    const FileTime sourceTimeStamp = owner_->GetTimeStamp();
    if (sourceTimeStamp)
    {
        const FileTime bytecodeTimeStamp = vfs->GetLastModifiedTime(binaryShaderName, false);
        if (bytecodeTimeStamp && bytecodeTimeStamp < sourceTimeStamp)
            return false;
    }

    const AbstractFilePtr file = vfs->OpenFile(binaryShaderName, FILE_READ);
    if (!file)
        return false;

    ShaderBytecode bytecode;
    if (!bytecode.LoadFromFile(*file))
        return false;

    const RenderBackend renderBackend = graphics_->GetRenderBackend();
    if (bytecode.mime_ != GetCompiledShaderMIME(renderBackend))
        return false;

    CreateFromBinary(bytecode);
    if (!GetHandle())
        return false;

    return true;
}

void ShaderVariation::SaveByteCode(const FileIdentifier& binaryShaderName)
{
    // TODO: Enable shader cache in Web. Currently it is disabled because of suboptimal persistent storage support.
    if (GetPlatform() == PlatformId::Web)
        return;

    Context* context = Context::GetInstance();
    auto vfs = context->GetSubsystem<VirtualFileSystem>();

    auto file = vfs->OpenFile(binaryShaderName, FILE_WRITE);
    if (!file)
        return;

    GetBytecode().SaveToFile(*file);
}

ea::string ShaderVariation::GetCachedVariationName(ea::string_view extension) const
{
    const ea::string backendName = ToString(graphics_->GetRenderBackend()).to_lower();
    const ea::string shortName = owner_->GetShaderName();
    const ea::string shaderTypeName = ToString(GetShaderType()).to_lower();
    const StringHash definesHash{defines_};
    return Format("{}_{}_{}_{}.{}", shortName, shaderTypeName, definesHash.ToString(), backendName, extension);
}

bool ShaderVariation::NeedShaderTranslation() const
{
    const ShaderTranslationPolicy policy = graphics_->GetSettings().shaderTranslationPolicy_;
    return policy != ShaderTranslationPolicy::Verbatim;
}

bool ShaderVariation::NeedShaderOptimization() const
{
    const ShaderTranslationPolicy policy = graphics_->GetSettings().shaderTranslationPolicy_;
    return policy == ShaderTranslationPolicy::Optimize;
}

ea::string ShaderVariation::PrepareGLSLShaderCode(const ea::string& originalShaderCode) const
{
    ea::string shaderCode;

    const RenderBackend renderBackend = graphics_->GetRenderBackend();
    const bool skipVersionTag = NeedShaderTranslation();

    // Check if the shader code contains a version define
    const auto versionTag = FindVersionTag(originalShaderCode);
    if (!skipVersionTag)
    {
        if (versionTag)
        {
            // If version define found, insert it first
            const ea::string versionDefine =
                originalShaderCode.substr(versionTag->first, versionTag->second - versionTag->first);
            shaderCode += versionDefine + "\n";
        }
        else
        {
            const bool isOpenGLES = IsOpenGLESBackend(renderBackend);
            const bool isCompute = GetShaderType() == CS;

            static const char* versions[2][2] = {
                {"#version 410\n", "#version 430\n"},
                {"#version 300 es\n", "#version 310 es\n"},
            };

            shaderCode += versions[isOpenGLES][isCompute];
        }
    }

    static const char* shaderTypeDefines[] = {
        "#define COMPILEVS\n", // VS
        "#define COMPILEPS\n", // PS
        "#define COMPILEGS\n", // GS
        "#define COMPILEHS\n", // HS
        "#define COMPILEDS\n", // DS
        "#define COMPILECS\n", // CS
    };
    shaderCode += shaderTypeDefines[GetShaderType()];

    shaderCode += Format("#define URHO3D_{}\n", ToString(renderBackend).to_upper());

    // Prepend the defines to the shader code
    const StringVector defineVec = defines_.split(' ');
    for (const ea::string& define : defineVec)
    {
        const ea::string defineString = "#define " + define.replaced('=', ' ') + " \n";
        shaderCode += defineString;
    }

    // When version define found, do not insert it a second time
    if (!versionTag)
        shaderCode += originalShaderCode;
    else
    {
        shaderCode += originalShaderCode.substr(0, versionTag->first);
        shaderCode += "//";
        shaderCode += originalShaderCode.substr(versionTag->first);
    }

    return shaderCode;
}

bool ShaderVariation::ProcessShaderSource(ea::string_view& translatedSource, const SpirVShader*& translatedSpirv,
    ConstByteSpan& translatedBytecode, ea::string_view originalShaderCode)
{
    translatedSource = originalShaderCode;
    translatedSpirv = nullptr;
    translatedBytecode = ToByteSpan(originalShaderCode);

    const RenderBackend renderBackend = graphics_->GetRenderBackend();
    const TargetShaderLanguage targetShaderLanguage = GetTargetShaderLanguage(renderBackend);
    const bool needShaderTranslation = NeedShaderTranslation();
    const bool needShaderOptimization = NeedShaderOptimization();

#ifdef URHO3D_SHADER_TRANSLATOR
    if (needShaderTranslation)
    {
        static thread_local SpirVShader spirvShader;
        ParseUniversalShader(spirvShader, GetShaderType(), originalShaderCode, {}, targetShaderLanguage);
        if (!spirvShader)
        {
            URHO3D_LOGERROR("Failed to convert shader {} from GLSL to SPIR-V:\n{}{}", GetShaderVariationName(),
                Shader::GetShaderFileList(), spirvShader.compilerOutput_);
            return false;
        }

        translatedSpirv = &spirvShader;

    #ifdef URHO3D_SHADER_OPTIMIZER
        if (needShaderOptimization)
        {
            ea::string optimizerOutput;
            if (!OptimizeSpirVShader(spirvShader, optimizerOutput, targetShaderLanguage))
            {
                URHO3D_LOGERROR("Failed to optimize SPIR-V shader {}:\n{}", GetShaderVariationName(), optimizerOutput);
                return false;
            }
        }
    #endif

        // Vulkan uses SPIRV directly
        if (targetShaderLanguage == TargetShaderLanguage::VULKAN_1_0)
        {
            translatedBytecode = ToByteSpan(spirvShader.bytecode_);
        }
        else
        {
            // Translate to target language
            static thread_local TargetShader targetShader;
            TranslateSpirVShader(targetShader, spirvShader, targetShaderLanguage);
            if (!targetShader)
            {
                URHO3D_LOGERROR("Failed to convert shader {} from SPIR-V to HLSL:\n{}{}", GetShaderVariationName(),
                    Shader::GetShaderFileList(), targetShader.compilerOutput_);
                return false;
            }

            translatedSource = targetShader.sourceCode_;
            if (renderBackend == RenderBackend::D3D11 || renderBackend == RenderBackend::D3D12)
            {
                // On D3D backends, compile the translated source code
                static thread_local ByteVector hlslBytecode;
                ea::string compilerOutput;
                if (!CompileHLSLToBinary(hlslBytecode, compilerOutput, targetShader.sourceCode_, GetShaderType()))
                {
                    URHO3D_LOGERROR("Failed to compile HLSL shader {}:\n{}{}", GetShaderVariationName(),
                        Shader::GetShaderFileList(), compilerOutput);
                    return false;
                }

                translatedBytecode = hlslBytecode;
            }
            else
            {
                // On OpenGL backends, just store the translated source code
                translatedBytecode = ToByteSpan(targetShader.sourceCode_);
            }
        }
    }
#endif

    return true;
}

} // namespace Urho3D
