// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/GraphicsImpl.h"
#include "Urho3D/Graphics/Shader.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/VirtualFileSystem.h"
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
    case RENDER_D3D11:
    case RENDER_D3D12: //
        return TargetShaderLanguage::HLSL_5_0;
    case RENDER_VULKAN: //
        return TargetShaderLanguage::VULKAN_1_0;
    case RENDER_GL:
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
    case RENDER_D3D11:
    case RENDER_D3D12: //
        return "application/hlsl-bin";
    case RENDER_VULKAN: //
        return "application/spirv";
    case RENDER_GL:
        return "application/glsl";
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

void ShaderVariation::OnDeviceLost()
{
    // No-op on Direct3D11
}

bool ShaderVariation::Create()
{
    if (!graphics_)
        return false;

    if (!owner_)
    {
        URHO3D_LOGERROR("Owner shader has expired");
        return false;
    }

    const FileIdentifier& cacheDir = graphics_->GetShaderCacheDir();
    const FileIdentifier binaryShaderName = cacheDir + GetCachedVariationName("bin");

    if (!LoadByteCode(binaryShaderName))
    {
        // Compile shader if don't have valid bytecode
        if (!Compile())
            return false;

        // Save the bytecode after successful compile, but not if the source is from a package
        if (owner_->GetTimeStamp())
            SaveByteCode(binaryShaderName);
    }

    return true;
}

void ShaderVariation::Release()
{
}

void ShaderVariation::SetDefines(const ea::string& defines)
{
    defines_ = defines;
}

bool ShaderVariation::Compile()
{
    const ea::string& originalShaderCode = owner_->GetSourceCode(type_);
    const ea::string sourceCode = PrepareGLSLShaderCode(originalShaderCode);

    ea::string_view translatedSource;
    const SpirVShader* translatedSpirv{};
    ConstByteSpan translatedBytecode;
    const bool processed = ProcessShaderSource(translatedSource, translatedSpirv, translatedBytecode, sourceCode);

    const FileIdentifier& cacheDir = graphics_->GetShaderCacheDir();
    const FileIdentifier loggedSourceShaderName = cacheDir + GetCachedVariationName("txt");
    LogShaderSource(loggedSourceShaderName, defines_, translatedSource);

    if (!processed)
        return false;

    const RenderBackend renderBackend = graphics_->GetRenderBackend();

    compiled_.type_ = type_;
    compiled_.mime_ = GetCompiledShaderMIME(renderBackend);
    compiled_.bytecode_.assign(translatedBytecode.begin(), translatedBytecode.end());
    if (translatedSpirv && type_ == VS)
        compiled_.vertexAttributes_ = GetVertexAttributesFromSpirV(*translatedSpirv);

    Diligent::IShader* shader = CreateShader(compiled_);
    if (!shader)
    {
        URHO3D_LOGERROR("Failed to create shader {}", GetFullName());
        return false;
    }

    object_ = shader;
    return true;
}

bool ShaderVariation::LoadByteCode(const FileIdentifier& binaryShaderName)
{
    auto vfs = owner_->GetSubsystem<VirtualFileSystem>();
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

    if (!compiled_.LoadFromFile(*file))
        return false;

    const RenderBackend renderBackend = graphics_->GetRenderBackend();
    if (compiled_.mime_ != GetCompiledShaderMIME(renderBackend))
        return false;

    Diligent::IShader* shader = CreateShader(compiled_);
    if (!shader)
    {
        URHO3D_LOGERROR("Failed to load shader {} from cache", GetFullName());
        return false;
    }

    object_ = shader;
    return true;
}

void ShaderVariation::SaveByteCode(const FileIdentifier& binaryShaderName)
{
    auto vfs = owner_->GetSubsystem<VirtualFileSystem>();

    auto file = vfs->OpenFile(binaryShaderName, FILE_WRITE);
    if (!file)
        return;

    compiled_.SaveToFile(*file);
}

ea::string ShaderVariation::GetCachedVariationName(ea::string_view extension) const
{
    // TODO(diligent): Extract both typeSuffix and backendSuffix to common place
    static const ea::string typeSuffix[] = {
        "vertex",
        "pixel",
        "geometry",
        "hull",
        "domain",
        "compute",
    };

    static const ea::string backendSuffix[] = {
        "d3d11",
        "d3d12",
        "opengl",
        "vulkan",
        "metal",
    };

    const RenderBackend backend = graphics_->GetRenderBackend();
    const ea::string shortName = GetFileName(owner_->GetName());
    const StringHash definesHash{defines_};
    return Format(
        "{}_{}_{}_{}.{}", shortName, typeSuffix[type_], definesHash.ToString(), backendSuffix[backend], extension);
}

bool ShaderVariation::NeedShaderTranslation() const
{
    const RenderBackend backend = graphics_->GetRenderBackend();
    if (backend == RENDER_GL)
    {
        const ShaderTranslationPolicy policy = graphics_->GetPolicyGLSL();
        return policy != ShaderTranslationPolicy::Verbatim;
    }

    return true;
}

bool ShaderVariation::NeedShaderOptimization() const
{
    const RenderBackend backend = graphics_->GetRenderBackend();
    switch (backend)
    {
    case RENDER_VULKAN:
        // TODO(diligent): Revisit this. Can we not optimize SPIRV? Does glslang provide legalized SPIRV?
        return true;

    case RENDER_GL: //
        return graphics_->GetPolicyGLSL() == ShaderTranslationPolicy::Optimize;

    case RENDER_D3D11:
    case RENDER_D3D12:
        return graphics_->GetPolicyHLSL() == ShaderTranslationPolicy::Optimize;

    default:
        return false;
    }
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
#if GLES_SUPPORTED
            shaderCode += "#version 300 es\n";
#else
            shaderCode += "#version 410\n";
#endif
        }
    }

    // TODO(diligent): Remove these defines
    shaderCode += "#define DESKTOP_GRAPHICS\n";
    shaderCode += "#define GL3\n";

    static const char* shaderTypeDefines[] = {
        "#define COMPILEVS\n", // VS
        "#define COMPILEPS\n", // PS
        "#define COMPILEGS\n", // GS
        "#define COMPILEHS\n", // HS
        "#define COMPILEDS\n", // DS
        "#define COMPILECS\n", // CS
    };
    shaderCode += shaderTypeDefines[type_];

    shaderCode += "#define MAXBONES " + ea::to_string(Graphics::GetMaxBones()) + "\n";

    // Prepend the defines to the shader code
    const StringVector defineVec = defines_.split(' ');
    for (const ea::string& define : defineVec)
    {
        const ea::string defineString = "#define " + define.replaced('=', ' ') + " \n";
        shaderCode += defineString;
    }

#if URHO3D_PLATFORM_WEB
    shaderCode += "#define WEBGL\n";
#endif

    // TODO(diligent): Revisit this define
    if (renderBackend == RENDER_D3D11 || renderBackend == RENDER_D3D12 || renderBackend == RENDER_VULKAN)
        shaderCode += "#define D3D11\n";

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
        ParseUniversalShader(spirvShader, type_, originalShaderCode, {}, targetShaderLanguage);
        if (!spirvShader)
        {
            URHO3D_LOGERROR("Failed to convert shader {} from GLSL to SPIR-V:\n{}{}", GetFullName(),
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
                URHO3D_LOGERROR("Failed to optimize SPIR-V shader {}:\n{}", GetFullName(), optimizerOutput);
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
                URHO3D_LOGERROR("Failed to convert shader {} from SPIR-V to HLSL:\n{}{}", GetFullName(),
                    Shader::GetShaderFileList(), targetShader.compilerOutput_);
                return false;
            }

            translatedSource = targetShader.sourceCode_;
            if (renderBackend == RENDER_D3D11 || renderBackend == RENDER_D3D12)
            {
                // On D3D backends, compile the translated source code
                static thread_local ByteVector hlslBytecode;
                ea::string compilerOutput;
                if (!CompileHLSLToBinary(hlslBytecode, compilerOutput, targetShader.sourceCode_, type_))
                {
                    URHO3D_LOGERROR("Failed to compile HLSL shader {}:\n{}{}", GetFullName(),
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

Diligent::IShader* ShaderVariation::CreateShader(const CompiledShaderVariation& compiledShader) const
{
    static Diligent::SHADER_TYPE shaderTypes[] = {
        Diligent::SHADER_TYPE_VERTEX,
        Diligent::SHADER_TYPE_PIXEL,
        Diligent::SHADER_TYPE_GEOMETRY,
        Diligent::SHADER_TYPE_HULL,
        Diligent::SHADER_TYPE_DOMAIN,
        Diligent::SHADER_TYPE_COMPUTE,
    };

    Diligent::ShaderCreateInfo createInfo;
#ifdef URHO3D_DEBUG
    createInfo.Desc.Name = name_.c_str();
#endif
    createInfo.Desc.ShaderType = shaderTypes[type_];
    createInfo.Desc.UseCombinedTextureSamplers = true;
    createInfo.EntryPoint = "main";
    createInfo.LoadConstantBufferReflection = true;

    switch (graphics_->GetRenderBackend())
    {
    case RENDER_D3D11:
    case RENDER_D3D12:
    {
        createInfo.ByteCode = compiledShader.bytecode_.data();
        createInfo.ByteCodeSize = compiledShader.bytecode_.size();
        break;
    }
    case RENDER_GL:
    {
        createInfo.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM;
        createInfo.Source = reinterpret_cast<const Diligent::Char*>(compiledShader.bytecode_.data());
        createInfo.SourceLength = compiledShader.bytecode_.size();
        break;
    }
    case RENDER_VULKAN:
    {
        createInfo.ByteCode = compiledShader.bytecode_.data();
        createInfo.ByteCodeSize = compiledShader.bytecode_.size();
        break;
    }
    case RENDER_METAL:
    default:
    {
        URHO3D_ASSERT(0, "Not implemented");
        break;
    }
    }

    Diligent::IRenderDevice* renderDevice = graphics_->GetImpl()->GetDevice();
    Diligent::RefCntAutoPtr<Diligent::IShader> shader;
    renderDevice->CreateShader(createInfo, &shader);

    return shader.Detach();
}

}
