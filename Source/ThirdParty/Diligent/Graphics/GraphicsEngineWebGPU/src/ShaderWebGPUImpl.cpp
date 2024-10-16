/*
 *  Copyright 2023-2024 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include "pch.h"

#include "ShaderWebGPUImpl.hpp"
#include "RenderDeviceWebGPUImpl.hpp"
#include "GLSLUtils.hpp"
#include "ShaderToolsCommon.hpp"
#include "WGSLUtils.hpp"
#include "HLSLUtils.hpp"
#include "HLSLParsingTools.hpp"
#include "SPIRVUtils.hpp"

#if !DILIGENT_NO_GLSLANG
#    include "GLSLangUtils.hpp"
#    include "SPIRVShaderResources.hpp"
#endif

#if !DILIGENT_NO_HLSL
#    include "SPIRVTools.hpp"
#endif

namespace Diligent
{

constexpr INTERFACE_ID ShaderWebGPUImpl::IID_InternalImpl;

namespace
{

constexpr char WebGPUDefine[] =
    "#ifndef WEBGPU\n"
    "#   define WEBGPU 1\n"
    "#endif\n";

std::vector<uint32_t> CompileShaderToSPIRV(const ShaderCreateInfo&             ShaderCI,
                                           const ShaderWebGPUImpl::CreateInfo& WebGPUShaderCI)
{
    std::vector<uint32_t> SPIRV;

#if DILIGENT_NO_GLSLANG
    LOG_ERROR_AND_THROW("Diligent engine was not linked with glslang, use DXC or precompiled SPIRV bytecode.");
#else
    if (ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_HLSL)
    {
        SPIRV = GLSLangUtils::HLSLtoSPIRV(ShaderCI, GLSLangUtils::SpirvVersion::Vk100, WebGPUDefine, WebGPUShaderCI.ppCompilerOutput);

        std::string EntryPoint;

        SPIRVShaderResources Resources{
            GetRawAllocator(),
            SPIRV,
            ShaderCI.Desc,
            ShaderCI.Desc.UseCombinedTextureSamplers ? ShaderCI.Desc.CombinedSamplerSuffix : nullptr,
            ShaderCI.Desc.ShaderType == SHADER_TYPE_VERTEX, // LoadShaderStageInputs
            false,                                          // LoadUniformBufferReflection
            EntryPoint,
        };

        if (ShaderCI.Desc.ShaderType == SHADER_TYPE_VERTEX)
        {
            Resources.MapHLSLVertexShaderInputs(SPIRV);
        }

        if (Resources.GetNumImgs() > 0)
        {
            // Image formats are lost during HLSL->SPIRV conversion, so we need to patch them manually
            const std::string HLSLSource = BuildHLSLSourceString(ShaderCI);
            if (!HLSLSource.empty())
            {
                // Extract image formats from special comments in HLSL code:
                //    Texture2D<float4 /*format=rgba32f*/> g_RWTexture;
                const auto ImageFormats = Parsing::ExtractGLSLImageFormatsFromHLSL(HLSLSource);
                if (!ImageFormats.empty())
                {
                    SPIRV = PatchImageFormats(SPIRV, ImageFormats);
                }
            }
        }
    }
    else
    {
        std::string          GLSLSourceString;
        ShaderSourceFileData SourceData;

        ShaderMacroArray Macros;
        if (ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM)
        {
            // Read the source file directly and use it as is
            SourceData = ReadShaderSourceFile(ShaderCI);

            // Add user macros.
            // BuildGLSLSourceString adds the macros to the source string, so we don't need to do this for SHADER_SOURCE_LANGUAGE_GLSL
            Macros = ShaderCI.Macros;
        }
        else
        {
            // Build the full source code string that will contain GLSL version declaration,
            // platform definitions, user-provided shader macros, etc.
            GLSLSourceString = BuildGLSLSourceString(
                {
                    ShaderCI,
                    WebGPUShaderCI.AdapterInfo,
                    WebGPUShaderCI.DeviceInfo.Features,
                    WebGPUShaderCI.DeviceInfo.Type,
                    WebGPUShaderCI.DeviceInfo.MaxShaderVersion,
                    TargetGLSLCompiler::glslang,
                    true, // ZeroToOneClipZ
                    WebGPUDefine,
                });
            SourceData.Source       = GLSLSourceString.c_str();
            SourceData.SourceLength = StaticCast<Uint32>(GLSLSourceString.length());
        }

        GLSLangUtils::GLSLtoSPIRVAttribs Attribs;
        Attribs.ShaderType                 = ShaderCI.Desc.ShaderType;
        Attribs.ShaderSource               = SourceData.Source;
        Attribs.SourceCodeLen              = static_cast<int>(SourceData.SourceLength);
        Attribs.Version                    = GLSLangUtils::SpirvVersion::Vk100;
        Attribs.Macros                     = Macros;
        Attribs.AssignBindings             = true;
        Attribs.UseRowMajorMatrices        = (ShaderCI.CompileFlags & SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR) != 0;
        Attribs.pShaderSourceStreamFactory = ShaderCI.pShaderSourceStreamFactory;
        Attribs.ppCompilerOutput           = WebGPUShaderCI.ppCompilerOutput;

        SPIRV = GLSLangUtils::GLSLtoSPIRV(Attribs);
    }
#endif

    return SPIRV;
}

} // namespace

void ShaderWebGPUImpl::Initialize(const ShaderCreateInfo& ShaderCI,
                                  const CreateInfo&       WebGPUShaderCI) noexcept(false)
{
    SHADER_SOURCE_LANGUAGE ParsedSourceLanguage = SHADER_SOURCE_LANGUAGE_DEFAULT;
    SHADER_SOURCE_LANGUAGE SourceLanguage       = ShaderCI.SourceLanguage;
    if (ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_DEFAULT ||
        ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_WGSL)
    {
        if (ShaderCI.Macros)
        {
            LOG_WARNING_MESSAGE("Shader macros are not supported for WGSL shaders and will be ignored.");
        }
        // Read the source file directly and use it as is
        ShaderSourceFileData SourceData = ReadShaderSourceFile(ShaderCI);
        m_WGSL.assign(SourceData.Source, SourceData.SourceLength);

        // Shaders packed into archive are WGSL, but we need to recover the original source language
        ParsedSourceLanguage = ParseShaderSourceLanguageDefinition(m_WGSL);
        if (ParsedSourceLanguage != SHADER_SOURCE_LANGUAGE_DEFAULT)
            SourceLanguage = ParsedSourceLanguage;
    }
    else if (ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_HLSL ||
             ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_GLSL ||
             ShaderCI.SourceLanguage == SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM)
    {
        std::vector<uint32_t> SPIRV;
        if (ShaderCI.Source != nullptr || ShaderCI.FilePath != nullptr)
        {
            DEV_CHECK_ERR(ShaderCI.ByteCode == nullptr, "'ByteCode' must be null when shader is created from source code or a file");
            SPIRV = CompileShaderToSPIRV(ShaderCI, WebGPUShaderCI);
            if (SPIRV.empty())
            {
                LOG_ERROR_AND_THROW("Failed to compile shader '", m_Desc.Name, '\'');
            }
        }
        else if (ShaderCI.ByteCode != nullptr)
        {
            DEV_CHECK_ERR(ShaderCI.ByteCodeSize != 0, "ByteCodeSize must not be 0");
            DEV_CHECK_ERR(ShaderCI.ByteCodeSize % 4 == 0, "Byte code size (", ShaderCI.ByteCodeSize, ") is not multiple of 4");
            SPIRV.resize(ShaderCI.ByteCodeSize / 4);
            memcpy(SPIRV.data(), ShaderCI.ByteCode, ShaderCI.ByteCodeSize);
        }
        else
        {
            LOG_ERROR_AND_THROW("Shader source must be provided through one of the 'Source', 'FilePath' or 'ByteCode' members");
        }

        m_WGSL = ConvertSPIRVtoWGSL(SPIRV);
        if (m_WGSL.empty())
        {
            LOG_ERROR_AND_THROW("Failed to convert SPIRV to WGSL for shader '", m_Desc.Name, '\'');
        }
    }
    else
    {
        LOG_ERROR_AND_THROW("Unsupported shader source language");
    }

    if (ParsedSourceLanguage == SHADER_SOURCE_LANGUAGE_DEFAULT && SourceLanguage != SHADER_SOURCE_LANGUAGE_DEFAULT)
    {
        // Add source language definition. It will be needed if the shader source is requested through the GetBytecode method
        // (e.g. by render state cache).
        AppendShaderSourceLanguageDefinition(m_WGSL, SourceLanguage);
    }
    // Note that once we add the source language definition, it will always be kept in the WGSL source as
    // RamapWGSLResourceBindings preserves it.

    // We cannot create shader module here because resource bindings are assigned when
    // pipeline state is created. Besides, WebGPU does not support multithreading.

    // Load shader resources
    if ((ShaderCI.CompileFlags & SHADER_COMPILE_FLAG_SKIP_REFLECTION) == 0)
    {
        auto& Allocator = GetRawAllocator();

        std::unique_ptr<void, STDDeleterRawMem<void>> pRawMem{
            ALLOCATE(Allocator, "Memory for WGSLShaderResources", WGSLShaderResources, 1),
            STDDeleterRawMem<void>(Allocator),
        };
        new (pRawMem.get()) WGSLShaderResources // May throw
            {
                Allocator,
                m_WGSL,
                SourceLanguage,
                m_Desc.Name,
                m_Desc.UseCombinedTextureSamplers ? m_Desc.CombinedSamplerSuffix : nullptr,
                SourceLanguage == SHADER_SOURCE_LANGUAGE_WGSL ? ShaderCI.EntryPoint : nullptr,
                ShaderCI.WebGPUEmulatedArrayIndexSuffix,
                ShaderCI.LoadConstantBufferReflection,
                WebGPUShaderCI.ppCompilerOutput,
            };
        m_pShaderResources.reset(static_cast<WGSLShaderResources*>(pRawMem.release()), STDDeleterRawMem<WGSLShaderResources>(Allocator));
        m_EntryPoint = m_pShaderResources->GetEntryPoint();
    }

    m_Status.store(SHADER_STATUS_READY);
}

ShaderWebGPUImpl::ShaderWebGPUImpl(IReferenceCounters*     pRefCounters,
                                   RenderDeviceWebGPUImpl* pDeviceWebGPU,
                                   const ShaderCreateInfo& ShaderCI,
                                   const CreateInfo&       WebGPUShaderCI,
                                   bool                    IsDeviceInternal) :
    // clang-format off
    TShaderBase
    {
        pRefCounters,
        pDeviceWebGPU,
        ShaderCI.Desc,
        WebGPUShaderCI.DeviceInfo,
        WebGPUShaderCI.AdapterInfo,
        IsDeviceInternal
    },
    m_EntryPoint{ShaderCI.EntryPoint != nullptr ? ShaderCI.EntryPoint : ""}
// clang-format on
{
    m_Status.store(SHADER_STATUS_COMPILING);
    if (WebGPUShaderCI.pCompilationThreadPool == nullptr || (ShaderCI.CompileFlags & SHADER_COMPILE_FLAG_ASYNCHRONOUS) == 0)
    {
        Initialize(ShaderCI, WebGPUShaderCI);
    }
    else
    {
        this->m_AsyncInitializer = AsyncInitializer::Start(
            WebGPUShaderCI.pCompilationThreadPool,
            [this,
             ShaderCI         = ShaderCreateInfoWrapper{ShaderCI, GetRawAllocator()},
             DeviceInfo       = WebGPUShaderCI.DeviceInfo,
             AdapterInfo      = WebGPUShaderCI.AdapterInfo,
             ppCompilerOutput = WebGPUShaderCI.ppCompilerOutput](Uint32 ThreadId) mutable //
            {
                try
                {
                    const CreateInfo WebGPUShaderCI{
                        DeviceInfo,
                        AdapterInfo,
                        ppCompilerOutput,
                        nullptr, // pCompilationThreadPool
                    };
                    Initialize(ShaderCI, WebGPUShaderCI);
                }
                catch (...)
                {
                    m_Status.store(SHADER_STATUS_FAILED);
                }
                ShaderCI = ShaderCreateInfoWrapper{};
            });
    }
}

ShaderWebGPUImpl::~ShaderWebGPUImpl()
{
    GetStatus(/*WaitForCompletion = */ true);
}

Uint32 ShaderWebGPUImpl::GetResourceCount() const
{
    DEV_CHECK_ERR(!IsCompiling(), "Shader resources are not available until the shader is compiled. Use GetStatus() to check the shader status.");
    return m_pShaderResources ? m_pShaderResources->GetTotalResources() : 0;
}

void ShaderWebGPUImpl::GetResourceDesc(Uint32 Index, ShaderResourceDesc& ResourceDesc) const
{
    DEV_CHECK_ERR(!IsCompiling(), "Shader resources are not available until the shader is compiled. Use GetStatus() to check the shader status.");

    Uint32 ResCount = GetResourceCount();
    DEV_CHECK_ERR(Index < ResCount, "Resource index (", Index, ") is out of range");
    if (Index < ResCount)
    {
        const WGSLShaderResourceAttribs& WGSLResource = m_pShaderResources->GetResource(Index);
        ResourceDesc                                  = WGSLResource.GetResourceDesc();
    }
}

const ShaderCodeBufferDesc* ShaderWebGPUImpl::GetConstantBufferDesc(Uint32 Index) const
{
    DEV_CHECK_ERR(!IsCompiling(), "Shader resources are not available until the shader is compiled. Use GetStatus() to check the shader status.");

    Uint32 ResCount = GetResourceCount();
    if (Index >= ResCount)
    {
        UNEXPECTED("Resource index (", Index, ") is out of range");
        return nullptr;
    }

    // Uniform buffers always go first in the list of resources
    return m_pShaderResources->GetUniformBufferDesc(Index);
}

void ShaderWebGPUImpl::GetBytecode(const void** ppBytecode, Uint64& Size) const
{
    DEV_CHECK_ERR(!IsCompiling(), "WGSL is not available until the shader is compiled. Use GetStatus() to check the shader status.");
    *ppBytecode = m_WGSL.data();
    Size        = m_WGSL.size();
}

const std::string& ShaderWebGPUImpl::GetWGSL() const
{
    DEV_CHECK_ERR(!IsCompiling(), "WGSL is not available until the shader is compiled. Use GetStatus() to check the shader status.");
    return m_WGSL;
}

const char* ShaderWebGPUImpl::GetEntryPoint() const
{
    DEV_CHECK_ERR(!IsCompiling(), "Shader resources are not available until the shader is compiled. Use GetStatus() to check the shader status.");
    return m_EntryPoint.c_str();
}

} // namespace Diligent
