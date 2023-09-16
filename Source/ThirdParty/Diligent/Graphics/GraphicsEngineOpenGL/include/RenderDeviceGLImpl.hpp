/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include <memory>

#include "EngineGLImplTraits.hpp"
#include "RenderDeviceBase.hpp"
#include "GLContext.hpp"
#include "VAOCache.hpp"
#include "BaseInterfacesGL.h"
#include "FBOCache.hpp"
#include "TexRegionRender.hpp"

namespace Diligent
{

/// Render device implementation in OpenGL backend.
// RenderDeviceGLESImpl is inherited from RenderDeviceGLImpl
class RenderDeviceGLImpl : public RenderDeviceBase<EngineGLImplTraits>
{
public:
    using TRenderDeviceBase = RenderDeviceBase<EngineGLImplTraits>;

    RenderDeviceGLImpl(IReferenceCounters*       pRefCounters,
                       IMemoryAllocator&         RawMemAllocator,
                       IEngineFactory*           pEngineFactory,
                       const EngineGLCreateInfo& InitAttribs,
                       const SwapChainDesc*      pSCDesc = nullptr) noexcept(false);
    ~RenderDeviceGLImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override;

    void CreateBuffer(const BufferDesc& BuffDesc,
                      const BufferData* pBuffData,
                      IBuffer**         ppBuffer,
                      bool              bIsDeviceInternal);

    /// Implementation of IRenderDevice::CreateBuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateBuffer(const BufferDesc& BuffDesc,
                                                 const BufferData* BuffData,
                                                 IBuffer**         ppBuffer) override final;

    // Special version used to create internal shaders (e.g. used by TexRegionRender)
    void CreateShader(const ShaderCreateInfo& ShaderCreateInfo,
                      IShader**               ppShader,
                      bool                    bIsDeviceInternal);

    /// Implementation of IRenderDevice::CreateShader() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateShader(const ShaderCreateInfo& ShaderCreateInfo,
                                                 IShader**               ppShader) override final;

    void CreateTexture(const TextureDesc& TexDesc,
                       const TextureData* pData,
                       ITexture**         ppTexture,
                       bool               bIsDeviceInternal);

    /// Implementation of IRenderDevice::CreateTexture() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateTexture(const TextureDesc& TexDesc,
                                                  const TextureData* Data,
                                                  ITexture**         ppTexture) override final;

    void CreateSampler(const SamplerDesc& SamplerDesc,
                       ISampler**         ppSampler,
                       bool               bIsDeviceInternal);

    /// Implementation of IRenderDevice::CreateSampler() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateSampler(const SamplerDesc& SamplerDesc,
                                                  ISampler**         ppSampler) override final;

    /// Implementation of IRenderDevice::CreateGraphicsPipelineState() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo,
                                                                IPipelineState**                       ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateComputePipelineState() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo,
                                                               IPipelineState**                      ppPipelineState) override final;

    /// Implementation of IRenderDevice::CreateRayTracingPipelineState() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateRayTracingPipelineState(const RayTracingPipelineStateCreateInfo& PSOCreateInfo,
                                                                  IPipelineState**                         ppPipelineState) override final;

    void CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& PSOCreateInfo,
                                     IPipelineState**                       ppPipelineState,
                                     bool                                   bIsDeviceInternal);
    void CreateComputePipelineState(const ComputePipelineStateCreateInfo& PSOCreateInfo,
                                    IPipelineState**                      ppPipelineState,
                                    bool                                  bIsDeviceInternal);

    /// Implementation of IRenderDevice::CreateFence() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateFence(const FenceDesc& Desc, IFence** ppFence) override final;

    /// Implementation of IRenderDevice::CreateQuery() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateQuery(const QueryDesc& Desc, IQuery** ppQuery) override final;

    /// Implementation of IRenderDevice::CreateRenderPass() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateRenderPass(const RenderPassDesc& Desc,
                                                     IRenderPass**         ppRenderPass) override final;

    /// Implementation of IRenderDevice::CreateFramebuffer() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateFramebuffer(const FramebufferDesc& Desc,
                                                      IFramebuffer**         ppFramebuffer) override final;

    /// Implementation of IRenderDevice::CreateBLAS() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateBLAS(const BottomLevelASDesc& Desc,
                                               IBottomLevelAS**         ppBLAS) override final;

    /// Implementation of IRenderDevice::CreateTLAS() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateTLAS(const TopLevelASDesc& Desc,
                                               ITopLevelAS**         ppTLAS) override final;

    /// Implementation of IRenderDevice::CreateSBT() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateSBT(const ShaderBindingTableDesc& Desc,
                                              IShaderBindingTable**         ppSBT) override final;

    /// Implementation of IRenderDevice::CreatePipelineResourceSignature() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                                                    IPipelineResourceSignature**         ppSignature) override final;

    void CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc,
                                         IPipelineResourceSignature**         ppSignature,
                                         SHADER_TYPE                          ShaderStages,
                                         bool                                 IsDeviceInternal);

    void CreatePipelineResourceSignature(const PipelineResourceSignatureDesc&           Desc,
                                         const PipelineResourceSignatureInternalDataGL& InternalData,
                                         IPipelineResourceSignature**                   ppSignature);

    /// Implementation of IRenderDeviceGL::CreateTextureFromGLHandle().
    virtual void DILIGENT_CALL_TYPE CreateTextureFromGLHandle(Uint32             GLHandle,
                                                              Uint32             GLBindTarget,
                                                              const TextureDesc& TexDesc,
                                                              RESOURCE_STATE     InitialState,
                                                              ITexture**         ppTexture) override final;

    /// Implementation of IRenderDeviceGL::CreateBufferFromGLHandle().
    virtual void DILIGENT_CALL_TYPE CreateBufferFromGLHandle(Uint32            GLHandle,
                                                             const BufferDesc& BuffDesc,
                                                             RESOURCE_STATE    InitialState,
                                                             IBuffer**         ppBuffer) override final;

    /// Implementation of IRenderDeviceGL::CreateDummyTexture().
    virtual void DILIGENT_CALL_TYPE CreateDummyTexture(const TextureDesc& TexDesc,
                                                       RESOURCE_STATE     InitialState,
                                                       ITexture**         ppTexture) override final;

    /// Implementation of IRenderDevice::ReleaseStaleResources() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE ReleaseStaleResources(bool ForceRelease = false) override final {}

    /// Implementation of IRenderDevice::IdleGPU() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE IdleGPU() override final;

    /// Implementation of IRenderDevice::CreateDeviceMemory() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreateDeviceMemory(const DeviceMemoryCreateInfo& CreateInfo,
                                                       IDeviceMemory**               ppMemory) override final;

    /// Implementation of IRenderDevice::CreatePipelineStateCache() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE CreatePipelineStateCache(const PipelineStateCacheCreateInfo& CreateInfo,
                                                             IPipelineStateCache**               ppPSOCache) override final;

    /// Implementation of IRenderDevice::GetSparseTextureFormatInfo() in OpenGL backend.
    virtual SparseTextureFormatInfo DILIGENT_CALL_TYPE GetSparseTextureFormatInfo(TEXTURE_FORMAT     TexFormat,
                                                                                  RESOURCE_DIMENSION Dimension,
                                                                                  Uint32             SampleCount) const override final;

    FBOCache& GetFBOCache(GLContext::NativeGLContextType Context);
    void      OnReleaseTexture(ITexture* pTexture);

    VAOCache& GetVAOCache(GLContext::NativeGLContextType Context);
    void      OnDestroyPSO(PipelineStateGLImpl& PSO);
    void      OnDestroyBuffer(BufferGLImpl& Buffer);

    size_t GetCommandQueueCount() const { return 1; }
    Uint64 GetCommandQueueMask() const { return Uint64{1}; }

    void InitTexRegionRender();

    struct GLDeviceLimits
    {
        GLint MaxUniformBlocks;
        GLint MaxTextureUnits;
        GLint MaxStorageBlock;
        GLint MaxImagesUnits;
    };
    const GLDeviceLimits& GetDeviceLimits() const { return m_DeviceLimits; }

protected:
    friend class DeviceContextGLImpl;
    friend class TextureBaseGL;
    friend class PipelineStateGLImpl;
    friend class ShaderGLImpl;
    friend class BufferGLImpl;
    friend class TextureViewGLImpl;
    friend class SwapChainGLImpl;
    friend class GLContextState;

    // Must be the first member because its constructor initializes OpenGL
    GLContext m_GLContext;

    std::unordered_set<String> m_ExtensionStrings;

    Threading::SpinLock                                          m_VAOCacheLock;
    std::unordered_map<GLContext::NativeGLContextType, VAOCache> m_VAOCache;

    Threading::SpinLock                                          m_FBOCacheLock;
    std::unordered_map<GLContext::NativeGLContextType, FBOCache> m_FBOCache;

    std::unique_ptr<TexRegionRender> m_pTexRegionRender;

private:
    virtual void TestTextureFormat(TEXTURE_FORMAT TexFormat) override final;
    bool         CheckExtension(const Char* ExtensionString) const;
    void         FlagSupportedTexFormats();
    void         InitAdapterInfo();

    int m_ShowDebugGLOutput = 1;

    GLDeviceLimits m_DeviceLimits = {};
};

} // namespace Diligent
