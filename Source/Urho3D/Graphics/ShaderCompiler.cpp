#include "./ShaderCompiler.h"
#include "../IO/Log.h"
#include "../Graphics/Diligent/DiligentLookupSettings.h"
#ifdef WIN32
#include <d3dcompiler.h>
#endif
#include <Diligent/Graphics/GraphicsEngine/interface/Shader.h>
#include <Diligent/Graphics/ShaderTools/include/GLSLangUtils.hpp>

#include "../Graphics/ShaderConverter.h"
namespace Urho3D
{
    ShaderCompiler::ShaderCompiler(ShaderCompilerDesc desc) : desc_(desc)
    {
    }
    bool ShaderCompiler::Compile()
    {
        byteCode_.clear();
        compilerOutput_.clear();

        if (desc_.output_ == ShaderCompilerOutput::DXC) {
#ifdef WIN32
            return CompileHLSL();
#else
            URHO3D_LOGERROR("Failed to compile {}. DXC bytecode is only available on Windows Platform.", desc_.name_);
            return false;
#endif
        }
        else if (desc_.output_ == ShaderCompilerOutput::SPIRV)
            return CompileSPIRV();
        else if (desc_.output_ == ShaderCompilerOutput::GLSL)
            return CompileGLSL();
        return false;
    }

#ifdef WIN32
    bool ShaderCompiler::CompileHLSL()
    {
        const char* profile = nullptr;
        unsigned flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef URHO3D_DEBUG
        // Debug flag will help developer to inspect shader code through renderdoc.
        flags |= D3DCOMPILE_DEBUG;
#endif

        switch (desc_.type_)
        {
        case VS:
            profile = "vs_4_0";
            break;
        case PS:
            profile = "ps_5_0";
            flags |= D3DCOMPILE_PREFER_FLOW_CONTROL;
            break;
        case GS:
            profile = "gs_5_0";
            break;
        case HS:
            profile = "hs_5_0";
            break;
        case DS:
            profile = "ds_5_0";
            break;
        case CS:
            profile = "cs_5_0";
            break;
        default:
            URHO3D_LOGERROR("Unsupported ShaderType {}", desc_.type_);
            return false;
        }

        ID3DBlob* byteCode = nullptr;
        ID3DBlob* errorMsg = nullptr;

        HRESULT hr = D3DCompile(
            desc_.sourceCode_.data(),
            desc_.sourceCode_.length(),
            desc_.name_.c_str(),
            nullptr,
            nullptr,
            desc_.entryPoint_.c_str(),
            profile,
            flags,
            0,
            &byteCode,
            &errorMsg
        );

        if (errorMsg) {
            ea::string output(
                (const char*)errorMsg->GetBufferPointer(),
                (const char*)errorMsg->GetBufferPointer() + errorMsg->GetBufferSize()
            );
            if (!output.empty())
                compilerOutput_.append(output);
            errorMsg->Release();
        }

        if (FAILED(hr)) {
            if (byteCode)
                byteCode->Release();
            return false;
        }

        uint8_t* byteCodeBin = (uint8_t*)byteCode->GetBufferPointer();
        size_t byteCodeLength = byteCode->GetBufferSize();

        byteCode_.resize(byteCodeLength);
        memcpy_s(byteCode_.data(), byteCodeLength, byteCodeBin, byteCodeLength);

        byteCode->Release();
        return true;
    }
#endif

    bool ShaderCompiler::CompileSPIRV()
    {
        using namespace Diligent;
        ShaderCreateInfo ci;
        ci.Desc.Name = desc_.name_.c_str();
        ci.Desc.ShaderType = DiligentShaderType[desc_.type_];
        ci.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ci.Source = desc_.sourceCode_.data();
        ci.EntryPoint = desc_.entryPoint_.data();
        const char* extraDefine = R"(
#ifndef VULKAN
#   define VULKAN 1
#endif
        )";
        auto byteCode = GLSLangUtils::HLSLtoSPIRV(
            ci,
            GLSLangUtils::SpirvVersion::Vk100,
            extraDefine,
            nullptr
        );

        size_t byteCodeLength = byteCode.size() * sizeof(unsigned);
        byteCode_.resize(byteCodeLength);

        memcpy(byteCode_.data(), byteCode.data(), byteCodeLength);
        return byteCode_.size() > 0;
    }

    bool ShaderCompiler::CompileGLSL()
    {
        // Build SPIRV Bytecode
        ea::string glVersion = "#version 450";
        size_t glVersionIdx = desc_.sourceCode_.find("#version 450");
        if (glVersionIdx != ea::string::npos)
            desc_.sourceCode_ = desc_.sourceCode_.substr(glVersionIdx + glVersion.size());
        if (!CompileSPIRV())
            return false;
        ea::vector<unsigned> byteCode(byteCode_.size() / sizeof(unsigned));
        memcpy(byteCode.data(), byteCode_.data(), byteCode_.size());

        ea::string outputShader;
        if (!ConvertSPIRVToGLSL(byteCode, outputShader, compilerOutput_))
            return false;

        byteCode_.clear();
        byteCode_.resize(outputShader.size());
        memcpy(byteCode_.data(), outputShader.data(), outputShader.size());
        return true;
    }

}
