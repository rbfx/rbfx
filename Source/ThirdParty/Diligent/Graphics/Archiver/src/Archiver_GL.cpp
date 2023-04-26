/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include "../../GraphicsEngineOpenGL/include/pch.h"

#include "ArchiverImpl.hpp"
#include "Archiver_Inc.hpp"

#include "RenderDeviceGLImpl.hpp"
#include "PipelineResourceSignatureGLImpl.hpp"
#include "PipelineStateGLImpl.hpp"
#include "ShaderGLImpl.hpp"
#include "DeviceObjectArchiveGL.hpp"
#include "SerializedPipelineStateImpl.hpp"
#include "ShaderToolsCommon.hpp"

#if !DILIGENT_NO_GLSLANG
#    include "GLSLUtils.hpp"
#    include "GLSLangUtils.hpp"
#endif

namespace Diligent
{

template <>
struct SerializedResourceSignatureImpl::SignatureTraits<PipelineResourceSignatureGLImpl>
{
    static constexpr DeviceType Type = DeviceType::OpenGL;

    template <SerializerMode Mode>
    using PRSSerializerType = PRSSerializerGL<Mode>;
};

namespace
{

struct CompiledShaderGL final : SerializedShaderImpl::CompiledShader
{
    const String           UnrolledSource;
    RefCntAutoPtr<IShader> pShaderGL;

    CompiledShaderGL(IReferenceCounters*             pRefCounters,
                     const ShaderCreateInfo&         ShaderCI,
                     const ShaderGLImpl::CreateInfo& GLShaderCI,
                     IRenderDevice*                  pRenderDeviceGL) :
        UnrolledSource{UnrollSource(ShaderCI)}
    {
        // Use serialization CI to be consistent with what will be saved in the archive.
        const auto SerializationCI = GetSerializationCI(ShaderCI);
        if (pRenderDeviceGL)
        {
            // GL shader must be created through the render device as GL functions
            // are not loaded by the archiver.
            pRenderDeviceGL->CreateShader(SerializationCI, &pShaderGL);
            if (!pShaderGL)
                LOG_ERROR_AND_THROW("Failed to create GL shader '", (ShaderCI.Desc.Name ? ShaderCI.Desc.Name : ""), "'.");
        }
        else
        {
            pShaderGL = NEW_RC_OBJ(GetRawAllocator(), "Shader instance", ShaderGLImpl)(nullptr, SerializationCI, GLShaderCI, true /*bIsDeviceInternal*/);
        }
    }

    ShaderCreateInfo GetSerializationCI(ShaderCreateInfo ShaderCI) const
    {
        ShaderCI.FilePath       = nullptr;
        ShaderCI.ByteCode       = nullptr;
        ShaderCI.Source         = UnrolledSource.c_str();
        ShaderCI.SourceLength   = UnrolledSource.length();
        ShaderCI.ShaderCompiler = SHADER_COMPILER_DEFAULT;
        ShaderCI.Macros         = nullptr; // Macros are inlined into unrolled source

        return ShaderCI;
    }

    virtual SerializedData Serialize(ShaderCreateInfo ShaderCI) const override final
    {
        const auto SerializationCI = GetSerializationCI(ShaderCI);
        return SerializedShaderImpl::SerializeCreateInfo(SerializationCI);
    }

    virtual IShader* GetDeviceShader() override final
    {
        return pShaderGL;
    }

private:
    static String UnrollSource(const ShaderCreateInfo& CI)
    {
        String Source;
        if (CI.Macros != nullptr)
        {
            if (CI.SourceLanguage != SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM)
                AppendShaderMacros(Source, CI.Macros);
            else
                DEV_ERROR("Shader macros are ignored when compiling GLSL verbatim in OpenGL backend");
        }
        Source.append(UnrollShaderIncludes(CI));
        return Source;
    }
};

struct ShaderStageInfoGL
{
    ShaderStageInfoGL() {}

    ShaderStageInfoGL(const SerializedShaderImpl* _pShader) :
        Type{_pShader->GetDesc().ShaderType},
        pShader{_pShader}
    {}

    // Needed only for ray tracing
    void Append(const SerializedShaderImpl*) {}

    Uint32 Count() const { return 1; }

    SHADER_TYPE                 Type    = SHADER_TYPE_UNKNOWN;
    const SerializedShaderImpl* pShader = nullptr;
};

#ifdef DILIGENT_DEBUG
inline SHADER_TYPE GetShaderStageType(const ShaderStageInfoGL& Stage)
{
    return Stage.Type;
}
#endif

} // namespace

template <typename CreateInfoType>
void SerializedPipelineStateImpl::PrepareDefaultSignatureGL(const CreateInfoType& CreateInfo) noexcept(false)
{
    // Add empty device signature - there must be some device-specific data for OpenGL in the archive
    // or there will be an error when unpacking the signature.
    std::vector<ShaderGLImpl*> DummyShadersGL;
    CreateDefaultResourceSignature<PipelineStateGLImpl, PipelineResourceSignatureGLImpl>(DeviceType::OpenGL, CreateInfo.PSODesc, SHADER_TYPE_UNKNOWN, DummyShadersGL);
}

template <typename CreateInfoType>
void SerializedPipelineStateImpl::PatchShadersGL(const CreateInfoType& CreateInfo) noexcept(false)
{
    std::vector<ShaderStageInfoGL> ShaderStages;
    SHADER_TYPE                    ActiveShaderStages = SHADER_TYPE_UNKNOWN;
    PipelineStateGLImpl::ExtractShaders<SerializedShaderImpl>(CreateInfo, ShaderStages, ActiveShaderStages);

    VERIFY_EXPR(m_Data.Shaders[static_cast<size_t>(DeviceType::OpenGL)].empty());
    for (size_t i = 0; i < ShaderStages.size(); ++i)
    {
        const auto& Stage             = ShaderStages[i];
        const auto& CI                = Stage.pShader->GetCreateInfo();
        const auto* pCompiledShaderGL = Stage.pShader->GetShader<CompiledShaderGL>(DeviceObjectArchive::DeviceType::OpenGL);
        const auto  SerCI             = pCompiledShaderGL->GetSerializationCI(CI);

        SerializeShaderCreateInfo(DeviceType::OpenGL, SerCI);
    }
    VERIFY_EXPR(m_Data.Shaders[static_cast<size_t>(DeviceType::OpenGL)].size() == ShaderStages.size());
}

INSTANTIATE_PATCH_SHADER_METHODS(PatchShadersGL)
INSTANTIATE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureGLImpl)

INSTANTIATE_PREPARE_DEF_SIGNATURE_GL(GraphicsPipelineStateCreateInfo);
INSTANTIATE_PREPARE_DEF_SIGNATURE_GL(ComputePipelineStateCreateInfo);
INSTANTIATE_PREPARE_DEF_SIGNATURE_GL(TilePipelineStateCreateInfo);
INSTANTIATE_PREPARE_DEF_SIGNATURE_GL(RayTracingPipelineStateCreateInfo);


void SerializationDeviceImpl::GetPipelineResourceBindingsGL(const PipelineResourceBindingAttribs& Info,
                                                            std::vector<PipelineResourceBinding>& ResourceBindings)
{
    const auto            ShaderStages        = (Info.ShaderStages == SHADER_TYPE_UNKNOWN ? static_cast<SHADER_TYPE>(~0u) : Info.ShaderStages);
    constexpr SHADER_TYPE SupportedStagesMask = (SHADER_TYPE_ALL_GRAPHICS | SHADER_TYPE_COMPUTE);

    SignatureArray<PipelineResourceSignatureGLImpl> Signatures      = {};
    Uint32                                          SignaturesCount = 0;
    SortResourceSignatures(Info.ppResourceSignatures, Info.ResourceSignaturesCount, Signatures, SignaturesCount);

    PipelineResourceSignatureGLImpl::TBindings BaseBindings = {};
    for (Uint32 s = 0; s < SignaturesCount; ++s)
    {
        const auto& pSignature = Signatures[s];
        if (pSignature == nullptr)
            continue;

        for (Uint32 r = 0; r < pSignature->GetTotalResourceCount(); ++r)
        {
            const auto& ResDesc = pSignature->GetResourceDesc(r);
            const auto& ResAttr = pSignature->GetResourceAttribs(r);
            const auto  Range   = PipelineResourceToBindingRange(ResDesc);

            for (auto Stages = ShaderStages & SupportedStagesMask; Stages != 0;)
            {
                const auto ShaderStage = ExtractLSB(Stages);
                if ((ResDesc.ShaderStages & ShaderStage) == 0)
                    continue;

                ResourceBindings.push_back(ResDescToPipelineResBinding(ResDesc, ShaderStage, BaseBindings[Range] + ResAttr.CacheOffset, 0 /*space*/));
            }
        }
        pSignature->ShiftBindings(BaseBindings);
    }
}

void SerializedShaderImpl::CreateShaderGL(IReferenceCounters*     pRefCounters,
                                          const ShaderCreateInfo& ShaderCI,
                                          RENDER_DEVICE_TYPE      DeviceType) noexcept(false)
{
    const ShaderGLImpl::CreateInfo GLShaderCI{
        m_pDevice->GetDeviceInfo(),
        m_pDevice->GetAdapterInfo() //
    };
    CreateShader<CompiledShaderGL>(DeviceType::OpenGL, pRefCounters, ShaderCI, GLShaderCI, m_pDevice->GetRenderDevice(RENDER_DEVICE_TYPE_GL));

#if !DILIGENT_NO_GLSLANG
    const auto* pCompiledShaderGL = GetShader<CompiledShaderGL>(DeviceObjectArchive::DeviceType::OpenGL);
    VERIFY_EXPR(pCompiledShaderGL != nullptr);

    const void* Source    = nullptr;
    Uint64      SourceLen = 0;
    // For OpenGL, GetBytecode returns the full GLSL source
    pCompiledShaderGL->pShaderGL->GetBytecode(&Source, SourceLen);
    VERIFY_EXPR(Source != nullptr && SourceLen != 0);

    GLSLangUtils::GLSLtoSPIRVAttribs Attribs;

    Attribs.ShaderType = ShaderCI.Desc.ShaderType;
    VERIFY_EXPR(DeviceType == RENDER_DEVICE_TYPE_GL || DeviceType == RENDER_DEVICE_TYPE_GLES);
    Attribs.Version = DeviceType == RENDER_DEVICE_TYPE_GL ? GLSLangUtils::SpirvVersion::GL : GLSLangUtils::SpirvVersion::GLES;

    Attribs.ppCompilerOutput = ShaderCI.ppCompilerOutput;
    Attribs.ShaderSource     = static_cast<const char*>(Source);
    Attribs.SourceCodeLen    = static_cast<int>(SourceLen);

    if (GLSLangUtils::GLSLtoSPIRV(Attribs).empty())
        LOG_ERROR_AND_THROW("Failed to compile shader '", ShaderCI.Desc.Name, "'");
#endif
}

} // namespace Diligent
