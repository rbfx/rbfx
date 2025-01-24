/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include "ArchiverImpl.hpp"
#include "SerializationDeviceImpl.hpp"
#include "GLSLangUtils.hpp"
#include "SerializedShaderImpl.hpp"
#include "SerializedRenderPassImpl.hpp"
#include "SerializedResourceSignatureImpl.hpp"
#include "SerializedPipelineStateImpl.hpp"
#include "EngineMemory.h"

namespace Diligent
{

static constexpr ARCHIVE_DEVICE_DATA_FLAGS GetSupportedDeviceFlags()
{
    ARCHIVE_DEVICE_DATA_FLAGS Flags = ARCHIVE_DEVICE_DATA_FLAG_NONE;
#if GL_SUPPORTED
    Flags = Flags | ARCHIVE_DEVICE_DATA_FLAG_GL;
    Flags = Flags | ARCHIVE_DEVICE_DATA_FLAG_GLES;
#endif
#if GLES_SUPPORTED
    Flags = Flags | ARCHIVE_DEVICE_DATA_FLAG_GLES;
#endif
#if D3D11_SUPPORTED
    Flags = Flags | ARCHIVE_DEVICE_DATA_FLAG_D3D11;
#endif
#if D3D12_SUPPORTED
    Flags = Flags | ARCHIVE_DEVICE_DATA_FLAG_D3D12;
#endif
#if VULKAN_SUPPORTED
    Flags = Flags | ARCHIVE_DEVICE_DATA_FLAG_VULKAN;
#endif
#if METAL_SUPPORTED
    Flags = Flags | ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS;
    Flags = Flags | ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS;
#endif
#if WEBGPU_SUPPORTED
    Flags = Flags | ARCHIVE_DEVICE_DATA_FLAG_WEBGPU;
#endif
    return Flags;
}

SerializationDeviceImpl::SerializationDeviceImpl(IReferenceCounters* pRefCounters, const SerializationDeviceCreateInfo& CreateInfo) :
    TBase{pRefCounters, GetRawAllocator(), nullptr, EngineCreateInfo{}, CreateInfo.AdapterInfo},
    m_ValidDeviceFlags{Diligent::GetSupportedDeviceFlags()}
{
    m_DeviceInfo = CreateInfo.DeviceInfo;

#if !DILIGENT_NO_GLSLANG
    GLSLangUtils::InitializeGlslang();
#endif

    if (m_ValidDeviceFlags & ARCHIVE_DEVICE_DATA_FLAG_D3D11)
    {
        m_D3D11Props.FeatureLevel = (CreateInfo.D3D11.FeatureLevel.Major << 12u) | (CreateInfo.D3D11.FeatureLevel.Minor << 8u);
    }

    if (m_ValidDeviceFlags & ARCHIVE_DEVICE_DATA_FLAG_D3D12)
    {
        m_pDxCompiler              = CreateDXCompiler(DXCompilerTarget::Direct3D12, 0, CreateInfo.D3D12.DxCompilerPath);
        m_D3D12Props.pDxCompiler   = m_pDxCompiler.get();
        m_D3D12Props.ShaderVersion = CreateInfo.D3D12.ShaderVersion;
    }

    if (m_ValidDeviceFlags & (ARCHIVE_DEVICE_DATA_FLAG_GL | ARCHIVE_DEVICE_DATA_FLAG_GLES))
    {
        m_GLProps.OptimizeShaders = CreateInfo.GL.OptimizeShaders;
        m_GLProps.ZeroToOneClipZ  = CreateInfo.GL.ZeroToOneClipZ;
    }

    if (m_ValidDeviceFlags & ARCHIVE_DEVICE_DATA_FLAG_VULKAN)
    {
        const auto& ApiVersion    = CreateInfo.Vulkan.ApiVersion;
        m_VkProps.VkVersion       = (ApiVersion.Major << 22u) | (ApiVersion.Minor << 12u);
        m_pVkDxCompiler           = CreateDXCompiler(DXCompilerTarget::Vulkan, m_VkProps.VkVersion, CreateInfo.Vulkan.DxCompilerPath);
        m_VkProps.pDxCompiler     = m_pVkDxCompiler.get();
        m_VkProps.SupportsSpirv14 = ApiVersion >= Version{1, 2} || CreateInfo.Vulkan.SupportsSpirv14;
    }

    if (m_ValidDeviceFlags & ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS)
    {
        const auto* CompileOptionsMacOS = CreateInfo.Metal.CompileOptionsMacOS;
        if (CompileOptionsMacOS != nullptr && CompileOptionsMacOS[0] != '\0')
        {
            m_MtlProps.CompileOptionsMacOS = CompileOptionsMacOS;
        }
        else
        {
            LOG_WARNING_MESSAGE("CreateInfo.Metal.CompileOptionsMacOS is null or empty. Compilation for MacOS will be disabled.");
            m_ValidDeviceFlags &= ~ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS;
        }
    }

    if (m_ValidDeviceFlags & ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS)
    {
        const auto* CompileOptionsiOS = CreateInfo.Metal.CompileOptionsiOS;
        if (CompileOptionsiOS != nullptr && CompileOptionsiOS[0] != '\0')
        {
            m_MtlProps.CompileOptionsIOS = CompileOptionsiOS;
        }
        else
        {
            LOG_WARNING_MESSAGE("CreateInfo.Metal.CompileOptionsiOS is null or empty. Compilation for iOS will be disabled.");
            m_ValidDeviceFlags &= ~ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS;
        }
    }

    if (m_ValidDeviceFlags & (ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS | ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS))
    {
        const auto* MslPreprocessorCmd = CreateInfo.Metal.MslPreprocessorCmd;
        if (MslPreprocessorCmd != nullptr && MslPreprocessorCmd[0] != '\0')
        {
            m_MtlProps.MslPreprocessorCmd = MslPreprocessorCmd;
        }

        const auto* DumpFolder = CreateInfo.Metal.DumpDirectory;
        if (DumpFolder != nullptr && DumpFolder[0] != '\0')
        {
            m_MtlProps.DumpFolder = DumpFolder;
        }
    }

    InitShaderCompilationThreadPool(CreateInfo.pAsyncShaderCompilationThreadPool, CreateInfo.NumAsyncShaderCompilationThreads);
}

SerializationDeviceImpl::~SerializationDeviceImpl()
{
#if !DILIGENT_NO_GLSLANG
    GLSLangUtils::FinalizeGlslang();
#endif
}

void SerializationDeviceImpl::CreateShader(const ShaderCreateInfo&  ShaderCI,
                                           const ShaderArchiveInfo& ArchiveInfo,
                                           IShader**                ppShader,
                                           IDataBlob**              ppCompilerOutput)
{
    CreateShaderImpl(ppShader, ShaderCI, ArchiveInfo, ppCompilerOutput);
}

void SerializationDeviceImpl::CreateRenderPass(const RenderPassDesc& Desc, IRenderPass** ppRenderPass)
{
    CreateRenderPassImpl(ppRenderPass, Desc);
}

void SerializationDeviceImpl::CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                              const ResourceSignatureArchiveInfo&  ArchiveInfo,
                                                              IPipelineResourceSignature**         ppSignature)
{
    CreatePipelineResourceSignatureImpl(ppSignature, Desc, ArchiveInfo, SHADER_TYPE_UNKNOWN);
}

void SerializationDeviceImpl::CreateSerializedResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                                const ResourceSignatureArchiveInfo&  ArchiveInfo,
                                                                SHADER_TYPE                          ShaderStages,
                                                                SerializedResourceSignatureImpl**    ppSignature)
{
    CreatePipelineResourceSignatureImpl(reinterpret_cast<IPipelineResourceSignature**>(ppSignature), Desc, ArchiveInfo, ShaderStages);
}

void SerializationDeviceImpl::CreateSerializedResourceSignature(SerializedResourceSignatureImpl** ppSignature, const char* Name)
{
    auto& RawMemAllocator = GetRawAllocator();
    auto* pSignatureImpl  = NEW_RC_OBJ(RawMemAllocator, "Pipeline resource signature instance", SerializedResourceSignatureImpl)(Name);
    pSignatureImpl->QueryInterface(IID_PipelineResourceSignature, reinterpret_cast<IObject**>(ppSignature));
}

void SerializationDeviceImpl::CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo,
                                                          const PipelineStateArchiveInfo&        ArchiveInfo,
                                                          IPipelineState**                       ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo, ArchiveInfo);
}

void SerializationDeviceImpl::CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo,
                                                         const PipelineStateArchiveInfo&       ArchiveInfo,
                                                         IPipelineState**                      ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo, ArchiveInfo);
}

void SerializationDeviceImpl::CreateRayTracingPipelineState(const RayTracingPipelineStateCreateInfo& PSOCreateInfo,
                                                            const PipelineStateArchiveInfo&          ArchiveInfo,
                                                            IPipelineState**                         ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo, ArchiveInfo);
}

void SerializationDeviceImpl::CreateTilePipelineState(const TilePipelineStateCreateInfo& PSOCreateInfo,
                                                      const PipelineStateArchiveInfo&    ArchiveInfo,
                                                      IPipelineState**                   ppPipelineState)
{
    CreatePipelineStateImpl(ppPipelineState, PSOCreateInfo, ArchiveInfo);
}

void SerializationDeviceImpl::GetPipelineResourceBindings(const PipelineResourceBindingAttribs& Info,
                                                          Uint32&                               NumBindings,
                                                          const PipelineResourceBinding*&       pBindings)
{
    NumBindings = 0;
    pBindings   = nullptr;
    m_ResourceBindings.clear();

    switch (Info.DeviceType)
    {
#if D3D11_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D11:
            GetPipelineResourceBindingsD3D11(Info, m_ResourceBindings);
            break;
#endif
#if D3D12_SUPPORTED
        case RENDER_DEVICE_TYPE_D3D12:
            GetPipelineResourceBindingsD3D12(Info, m_ResourceBindings);
            break;
#endif
#if GL_SUPPORTED || GLES_SUPPORTED
        case RENDER_DEVICE_TYPE_GL:
        case RENDER_DEVICE_TYPE_GLES:
            GetPipelineResourceBindingsGL(Info, m_ResourceBindings);
            break;
#endif
#if VULKAN_SUPPORTED
        case RENDER_DEVICE_TYPE_VULKAN:
            GetPipelineResourceBindingsVk(Info, m_ResourceBindings);
            break;
#endif
#if METAL_SUPPORTED
        case RENDER_DEVICE_TYPE_METAL:
            GetPipelineResourceBindingsMtl(Info, m_ResourceBindings, m_MtlProps.MaxBufferFunctionArgumets);
            break;
#endif
        case RENDER_DEVICE_TYPE_UNDEFINED:
        case RENDER_DEVICE_TYPE_COUNT:
        default:
            return;
    }

    NumBindings = static_cast<Uint32>(m_ResourceBindings.size());
    pBindings   = m_ResourceBindings.data();
}

PipelineResourceBinding SerializationDeviceImpl::ResDescToPipelineResBinding(const PipelineResourceDesc& ResDesc,
                                                                             SHADER_TYPE                 Stages,
                                                                             Uint32                      Register,
                                                                             Uint32                      Space)
{
    PipelineResourceBinding BindigDesc;
    BindigDesc.Name         = ResDesc.Name;
    BindigDesc.ResourceType = ResDesc.ResourceType;
    BindigDesc.Register     = Register;
    BindigDesc.Space        = StaticCast<Uint16>(Space);
    BindigDesc.ArraySize    = (ResDesc.Flags & PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY) == 0 ? ResDesc.ArraySize : 0;
    BindigDesc.ShaderStages = Stages;
    return BindigDesc;
}

void SerializationDeviceImpl::AddRenderDevice(IRenderDevice* pDevice)
{
    if (pDevice == nullptr)
    {
        DEV_ERROR("pDevice must not be null");
        return;
    }

    const auto Type = pDevice->GetDeviceInfo().Type;
    if (m_RenderDevices[Type])
        LOG_WARNING_MESSAGE(GetRenderDeviceTypeString(Type), " device has already been added.");

    m_RenderDevices[Type] = pDevice;
}

} // namespace Diligent
