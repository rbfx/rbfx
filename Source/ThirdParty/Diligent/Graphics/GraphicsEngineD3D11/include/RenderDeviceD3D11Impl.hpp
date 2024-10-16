/*
 *  Copyright 2019-2023 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
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

#pragma once

/// \file
/// Declaration of Diligent::RenderDeviceD3D11Impl class

#include "EngineD3D11ImplTraits.hpp"
#include "RenderDeviceD3DBase.hpp"

namespace Diligent
{

/// Render device implementation in Direct3D11 backend.
class RenderDeviceD3D11Impl final : public RenderDeviceD3DBase<EngineD3D11ImplTraits>
{
public:
    using TRenderDeviceBase = RenderDeviceD3DBase<EngineD3D11ImplTraits>;

    RenderDeviceD3D11Impl(IReferenceCounters*          pRefCounters,
                          IMemoryAllocator&            RawMemAllocator,
                          IEngineFactory*              pEngineFactory,
                          const EngineD3D11CreateInfo& EngineAttribs,
                          const GraphicsAdapterInfo&   AdapterInfo,
                          ID3D11Device*                pd3d11Device) noexcept(false);
    ~RenderDeviceD3D11Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IRenderDevice::CreateBuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateBuffer(const BufferDesc& BuffDesc,
                                                 const BufferData* pBuffData,
                                                 IBuffer**         ppBuffer) override final;

    /// Implementation of IRenderDevice::CreateShader() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateShader(const ShaderCreateInfo& ShaderCI,
                                                 IShader**               ppShader,
                                                 IDataBlob**             ppCompilerOutput) override final;

    /// Implementation of IRenderDevice::CreateTexture() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateTexture(const TextureDesc& TexDesc,
                                                  const TextureData* pData,
                                                  ITexture**         ppTexture) override final;

    /// Implementation of IRenderDevice::CreateSampler() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateSampler(const SamplerDesc& SamplerDesc,
                                                  ISampler**         ppSampler) override final;

    /// Implementation of IRenderDevice::CreateGraphicsPipelineState() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo,
                                                                IPipelineState**                       ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateComputePipelineState() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo,
                                                               IPipelineState**                      ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateRayTracingPipelineState() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateRayTracingPipelineState(const RayTracingPipelineStateCreateInfo& PSOCreateInfo,
                                                                  IPipelineState**                         ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateFence() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateFence(const FenceDesc& Desc,
                                                IFence**         ppFence) override final;

    /// Implementation of IRenderDevice::CreateQuery() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateQuery(const QueryDesc& Desc,
                                                IQuery**         ppQuery) override final;

    /// Implementation of IRenderDevice::CreateRenderPass() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateRenderPass(const RenderPassDesc& Desc,
                                                     IRenderPass**         ppRenderPass) override final;

    /// Implementation of IRenderDevice::CreateFramebuffer() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateFramebuffer(const FramebufferDesc& Desc,
                                                      IFramebuffer**         ppFramebuffer) override final;

    /// Implementation of IRenderDevice::CreateBLAS() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateBLAS(const BottomLevelASDesc& Desc,
                                               IBottomLevelAS**         ppBLAS) override final;

    /// Implementation of IRenderDevice::CreateTLAS() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateTLAS(const TopLevelASDesc& Desc,
                                               ITopLevelAS**         ppTLAS) override final;

    /// Implementation of IRenderDevice::CreateSBT() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateSBT(const ShaderBindingTableDesc& Desc,
                                              IShaderBindingTable**         ppSBT) override final;

    /// Implementation of IRenderDevice::CreatePipelineResourceSignature() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                                    IPipelineResourceSignature**         ppSignature) override final;

    void CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                         IPipelineResourceSignature**         ppSignature,
                                         SHADER_TYPE                          ShaderStages,
                                         bool                                 IsDeviceInternal);

    void CreatePipelineResourceSignature(const PipelineResourceSignatureDesc&              Desc,
                                         const PipelineResourceSignatureInternalDataD3D11& InternalData,
                                         IPipelineResourceSignature**                      ppSignature);

    /// Implementation of IRenderDevice::CreateDeviceMemory() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateDeviceMemory(const DeviceMemoryCreateInfo& CreateInfo,
                                                       IDeviceMemory**               ppMemory) override final;

    /// Implementation of IRenderDevice::CreatePipelineStateCache() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreatePipelineStateCache(const PipelineStateCacheCreateInfo& CreateInfo,
                                                             IPipelineStateCache**               ppPSOCache) override final;

    /// Implementation of IRenderDeviceD3D11::GetD3D11Device() in Direct3D11 backend.
    ID3D11Device* DILIGENT_CALL_TYPE GetD3D11Device() override final { return m_pd3d11Device; }

    /// Implementation of IRenderDeviceD3D11::CreateBufferFromD3DResource() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateBufferFromD3DResource(ID3D11Buffer* pd3d11Buffer, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer) override final;

    /// Implementation of IRenderDeviceD3D11::CreateTextureFromD3DResource() for 1D textures in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateTexture1DFromD3DResource(ID3D11Texture1D* pd3d11Texture,
                                                                   RESOURCE_STATE   InitialState,
                                                                   ITexture**       ppTexture) override final;

    /// Implementation of IRenderDeviceD3D11::CreateTextureFromD3DResource() for 2D textures in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateTexture2DFromD3DResource(ID3D11Texture2D* pd3d11Texture,
                                                                   RESOURCE_STATE   InitialState,
                                                                   ITexture**       ppTexture) override final;

    /// Implementation of IRenderDeviceD3D11::CreateTextureFromD3DResource() for 3D textures in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE CreateTexture3DFromD3DResource(ID3D11Texture3D* pd3d11Texture,
                                                                   RESOURCE_STATE   InitialState,
                                                                   ITexture**       ppTexture) override final;

    /// Implementation of IRenderDevice::ReleaseStaleResources() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE ReleaseStaleResources(bool ForceRelease = false) override final {}

    /// Implementation of IRenderDevice::IdleGPU() in Direct3D11 backend.
    virtual void DILIGENT_CALL_TYPE IdleGPU() override final;

    /// Implementation of IRenderDevice::GetSparseTextureFormatInfo() in Direct3D11 backend.
    virtual SparseTextureFormatInfo DILIGENT_CALL_TYPE GetSparseTextureFormatInfo(TEXTURE_FORMAT     TexFormat,
                                                                                  RESOURCE_DIMENSION Dimension,
                                                                                  Uint32             SampleCount) const override final;

    size_t GetCommandQueueCount() const { return 1; }
    Uint64 GetCommandQueueMask() const { return Uint64{1}; }


#define GET_D3D11_DEVICE(Version)                                                  \
    ID3D11Device##Version* GetD3D11Device##Version()                               \
    {                                                                              \
        DEV_CHECK_ERR(m_MaxD3D11DeviceVersion >= Version, "ID3D11Device", Version, \
                      " is not supported. Maximum supported version: ",            \
                      m_MaxD3D11DeviceVersion);                                    \
        return static_cast<ID3D11Device##Version*>(m_pd3d11Device.p);              \
    }
#if D3D11_VERSION >= 1
    GET_D3D11_DEVICE(1)
#endif
#if D3D11_VERSION >= 2
    GET_D3D11_DEVICE(2)
#endif
#if D3D11_VERSION >= 3
    GET_D3D11_DEVICE(3)
#endif
#if D3D11_VERSION >= 4
    GET_D3D11_DEVICE(4)
#endif
#undef GET_D3D11_DEVICE

private:
    virtual void TestTextureFormat(TEXTURE_FORMAT TexFormat) override final;

    /// D3D11 device
    CComPtr<ID3D11Device> m_pd3d11Device;

#ifdef DILIGENT_DEVELOPMENT
    Uint32 m_MaxD3D11DeviceVersion = 0;
#endif
};

} // namespace Diligent
