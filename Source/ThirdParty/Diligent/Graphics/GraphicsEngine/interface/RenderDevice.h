/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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
/// Definition of the Diligent::IRenderDevice interface and related data structures

#include "../../../Primitives/interface/Object.h"
#include "EngineFactory.h"
#include "GraphicsTypes.h"
#include "Constants.h"
#include "Buffer.h"
#include "InputLayout.h"
#include "Shader.h"
#include "Texture.h"
#include "Sampler.h"
#include "ResourceMapping.h"
#include "TextureView.h"
#include "BufferView.h"
#include "PipelineState.h"
#include "PipelineStateCache.h"
#include "Fence.h"
#include "Query.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "BottomLevelAS.h"
#include "TopLevelAS.h"
#include "ShaderBindingTable.h"
#include "PipelineResourceSignature.h"
#include "DeviceMemory.h"

#include "DepthStencilState.h"
#include "RasterizerState.h"
#include "BlendState.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {F0E9B607-AE33-4B2B-B1AF-A8B2C3104022}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_RenderDevice =
    {0xf0e9b607, 0xae33, 0x4b2b, {0xb1, 0xaf, 0xa8, 0xb2, 0xc3, 0x10, 0x40, 0x22}};

#define DILIGENT_INTERFACE_NAME IRenderDevice
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IRenderDeviceInclusiveMethods \
    IObjectInclusiveMethods;          \
    IRenderDeviceMethods RenderDevice

// clang-format off

/// Render device interface
DILIGENT_BEGIN_INTERFACE(IRenderDevice, IObject)
{
    /// Creates a new buffer object

    /// \param [in] BuffDesc   - Buffer description, see Diligent::BufferDesc for details.
    /// \param [in] pBuffData  - Pointer to Diligent::BufferData structure that describes
    ///                          initial buffer data or nullptr if no data is provided.
    ///                          Immutable buffers (USAGE_IMMUTABLE) must be initialized at creation time.
    /// \param [out] ppBuffer  - Address of the memory location where a pointer to the
    ///                          buffer interface will be written. The function calls AddRef(),
    ///                          so that the new buffer will have one reference and must be
    ///                          released by a call to Release().
    ///
    /// \remarks
    /// Size of a uniform buffer (BIND_UNIFORM_BUFFER) must be multiple of 16.\n
    /// Stride of a formatted buffer will be computed automatically from the format if
    /// ElementByteStride member of buffer description is set to default value (0).
    VIRTUAL void METHOD(CreateBuffer)(THIS_
                                      const BufferDesc REF BuffDesc,
                                      const BufferData*    pBuffData,
                                      IBuffer**            ppBuffer) PURE;

    /// Creates a new shader object

    /// \param [in]  ShaderCI - Shader create info, see Diligent::ShaderCreateInfo for details.
    /// \param [out] ppShader - Address of the memory location where a pointer to the
    ///                         shader interface will be written.
    ///                         The function calls AddRef(), so that the new object will have
    ///                         one reference.
    /// \param [out] ppCompilerOutput - Address of the memory location where a pointer to the
    ///                                 the compiler output data blob will be written.
    ///                                 If null, the compiler output will be ignored.
    ///
    /// \remarks    The buffer returned in ppCompilerOutput contains two null-terminated strings.
    ///             The first one is the compiler output message. The second one is the full
    ///             shader source code including definitions added by the engine. The data blob
    ///             object must be released by the client.
    VIRTUAL void METHOD(CreateShader)(THIS_
                                      const ShaderCreateInfo REF ShaderCI,
                                      IShader**                  ppShader,
                                      IDataBlob**                ppCompilerOutput DEFAULT_VALUE(nullptr)) PURE;

    /// Creates a new texture object

    /// \param [in] TexDesc - Texture description, see Diligent::TextureDesc for details.
    /// \param [in] pData   - Pointer to Diligent::TextureData structure that describes
    ///                       initial texture data or nullptr if no data is provided.
    ///                       Immutable textures (USAGE_IMMUTABLE) must be initialized at creation time.
    ///
    /// \param [out] ppTexture - Address of the memory location where a pointer to the
    ///                          texture interface will be written.
    ///                          The function calls AddRef(), so that the new object will have
    ///                          one reference.
    /// \remarks
    /// To create all mip levels, set the TexDesc.MipLevels to zero.\n
    /// Multisampled resources cannot be initialized with data when they are created. \n
    /// If initial data is provided, number of subresources must exactly match the number
    /// of subresources in the texture (which is the number of mip levels times the number of array slices.
    /// For a 3D texture, this is just the number of mip levels).
    /// For example, for a 15 x 6 x 2 2D texture array, the following array of subresources should be
    /// provided: \n
    /// 15x6, 7x3, 3x1, 1x1, 15x6, 7x3, 3x1, 1x1.\n
    /// For a 15 x 6 x 4 3D texture, the following array of subresources should be provided:\n
    /// 15x6x4, 7x3x2, 3x1x1, 1x1x1
    VIRTUAL void METHOD(CreateTexture)(THIS_
                                       const TextureDesc REF TexDesc,
                                       const TextureData*    pData,
                                       ITexture**            ppTexture) PURE;

    /// Creates a new sampler object

    /// \param [in]  SamDesc   - Sampler description, see Diligent::SamplerDesc for details.
    /// \param [out] ppSampler - Address of the memory location where a pointer to the
    ///                          sampler interface will be written.
    ///                          The function calls AddRef(), so that the new object will have
    ///                          one reference.
    /// \remark If an application attempts to create a sampler interface with the same attributes
    ///         as an existing interface, the same interface will be returned.
    /// \note   In D3D11, 4096 unique sampler state objects can be created on a device at a time.
    VIRTUAL void METHOD(CreateSampler)(THIS_
                                       const SamplerDesc REF SamDesc,
                                       ISampler**            ppSampler) PURE;

    /// Creates a new resource mapping

    /// \param [in]  ResMappingCI - Resource mapping create info, see Diligent::ResourceMappingCreateInfo for details.
    /// \param [out] ppMapping    - Address of the memory location where a pointer to the
    ///                             resource mapping interface will be written.
    ///                             The function calls AddRef(), so that the new object will have
    ///                             one reference.
    VIRTUAL void METHOD(CreateResourceMapping)(THIS_
                                               const ResourceMappingCreateInfo REF ResMappingCI,
                                               IResourceMapping**                  ppMapping) PURE;

    /// Creates a new graphics pipeline state object

    /// \param [in]  PSOCreateInfo   - Graphics pipeline state create info, see Diligent::GraphicsPipelineStateCreateInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the
    ///                                pipeline state interface will be written.
    ///                                The function calls AddRef(), so that the new object will have
    ///                                one reference.
    VIRTUAL void METHOD(CreateGraphicsPipelineState)(THIS_
                                                     const GraphicsPipelineStateCreateInfo REF PSOCreateInfo,
                                                     IPipelineState**                          ppPipelineState) PURE;

    /// Creates a new compute pipeline state object

    /// \param [in]  PSOCreateInfo   - Compute pipeline state create info, see Diligent::ComputePipelineStateCreateInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the
    ///                                pipeline state interface will be written.
    ///                                The function calls AddRef(), so that the new object will have
    ///                                one reference.
    VIRTUAL void METHOD(CreateComputePipelineState)(THIS_
                                                    const ComputePipelineStateCreateInfo REF PSOCreateInfo,
                                                    IPipelineState**                         ppPipelineState) PURE;

    /// Creates a new ray tracing pipeline state object

    /// \param [in]  PSOCreateInfo   - Ray tracing pipeline state create info, see Diligent::RayTracingPipelineStateCreateInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the
    ///                                pipeline state interface will be written.
    ///                                The function calls AddRef(), so that the new object will have
    ///                                one reference.
    VIRTUAL void METHOD(CreateRayTracingPipelineState)(THIS_
                                                       const RayTracingPipelineStateCreateInfo REF PSOCreateInfo,
                                                       IPipelineState**                            ppPipelineState) PURE;

    /// Creates a new tile pipeline state object

    /// \param [in]  PSOCreateInfo   - Tile pipeline state create info, see Diligent::TilePipelineStateCreateInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the
    ///                                pipeline state interface will be written.
    ///                                The function calls AddRef(), so that the new object will have
    ///                                one reference.
    VIRTUAL void METHOD(CreateTilePipelineState)(THIS_
                                                 const TilePipelineStateCreateInfo REF PSOCreateInfo,
                                                 IPipelineState**                      ppPipelineState) PURE;

    /// Creates a new fence object

    /// \param [in]  Desc    - Fence description, see Diligent::FenceDesc for details.
    /// \param [out] ppFence - Address of the memory location where a pointer to the
    ///                        fence interface will be written.
    ///                        The function calls AddRef(), so that the new object will have
    ///                        one reference.
    VIRTUAL void METHOD(CreateFence)(THIS_
                                     const FenceDesc REF Desc,
                                     IFence**            ppFence) PURE;


    /// Creates a new query object

    /// \param [in]  Desc    - Query description, see Diligent::QueryDesc for details.
    /// \param [out] ppQuery - Address of the memory location where a pointer to the
    ///                        query interface will be written.
    ///                        The function calls AddRef(), so that the new object will have
    ///                        one reference.
    VIRTUAL void METHOD(CreateQuery)(THIS_
                                     const QueryDesc REF Desc,
                                     IQuery**            ppQuery) PURE;


    /// Creates a render pass object

    /// \param [in]  Desc         - Render pass description, see Diligent::RenderPassDesc for details.
    /// \param [out] ppRenderPass - Address of the memory location where a pointer to the
    ///                             render pass interface will be written.
    ///                             The function calls AddRef(), so that the new object will have
    ///                             one reference.
    VIRTUAL void METHOD(CreateRenderPass)(THIS_
                                          const RenderPassDesc REF Desc,
                                          IRenderPass**            ppRenderPass) PURE;



    /// Creates a framebuffer object

    /// \param [in]  Desc          - Framebuffer description, see Diligent::FramebufferDesc for details.
    /// \param [out] ppFramebuffer - Address of the memory location where a pointer to the
    ///                              framebuffer interface will be written.
    ///                              The function calls AddRef(), so that the new object will have
    ///                              one reference.
    VIRTUAL void METHOD(CreateFramebuffer)(THIS_
                                           const FramebufferDesc REF Desc,
                                           IFramebuffer**            ppFramebuffer) PURE;


    /// Creates a bottom-level acceleration structure object (BLAS).

    /// \param [in]  Desc    - BLAS description, see Diligent::BottomLevelASDesc for details.
    /// \param [out] ppBLAS  - Address of the memory location where a pointer to the
    ///                        BLAS interface will be written.
    ///                        The function calls AddRef(), so that the new object will have
    ///                        one reference.
    VIRTUAL void METHOD(CreateBLAS)(THIS_
                                    const BottomLevelASDesc REF Desc,
                                    IBottomLevelAS**            ppBLAS) PURE;


    /// Creates a top-level acceleration structure object (TLAS).

    /// \param [in]  Desc    - TLAS description, see Diligent::TopLevelASDesc for details.
    /// \param [out] ppTLAS  - Address of the memory location where a pointer to the
    ///                        TLAS interface will be written.
    ///                        The function calls AddRef(), so that the new object will have
    ///                        one reference.
    VIRTUAL void METHOD(CreateTLAS)(THIS_
                                    const TopLevelASDesc REF Desc,
                                    ITopLevelAS**            ppTLAS) PURE;


    /// Creates a shader resource binding table object (SBT).

    /// \param [in]  Desc    - SBT description, see Diligent::ShaderBindingTableDesc for details.
    /// \param [out] ppSBT   - Address of the memory location where a pointer to the
    ///                        SBT interface will be written.
    ///                        The function calls AddRef(), so that the new object will have
    ///                        one reference.
    VIRTUAL void METHOD(CreateSBT)(THIS_
                                   const ShaderBindingTableDesc REF Desc,
                                   IShaderBindingTable**            ppSBT) PURE;

    /// Creates a pipeline resource signature object.

    /// \param [in]  Desc         - Resource signature description, see Diligent::PipelineResourceSignatureDesc for details.
    /// \param [out] ppSignature  - Address of the memory location where a pointer to the
    ///                             pipeline resource signature interface will be written.
    ///                             The function calls AddRef(), so that the new object will have
    ///                             one reference.
    VIRTUAL void METHOD(CreatePipelineResourceSignature)(THIS_
                                                         const PipelineResourceSignatureDesc REF Desc,
                                                         IPipelineResourceSignature**            ppSignature) PURE;


    /// Creates a device memory object.

    /// \param [in]  CreateInfo - Device memory create info, see Diligent::DeviceMemoryCreateInfo for details.
    /// \param [out] ppMemory   - Address of the memory location where a pointer to the
    ///                           device memory interface will be written.
    ///                           The function calls AddRef(), so that the new object will have
    ///                           one reference.
    VIRTUAL void METHOD(CreateDeviceMemory)(THIS_
                                            const DeviceMemoryCreateInfo REF CreateInfo,
                                            IDeviceMemory**                  ppMemory) PURE;


    /// Creates a pipeline state cache object.

    /// \param [in]  CreateInfo - Pipeline state cache create info, see Diligent::PiplineStateCacheCreateInfo for details.
    /// \param [out] ppPSOCache - Address of the memory location where a pointer to the
    ///                           pipeline state cache interface will be written.
    ///                           The function calls AddRef(), so that the new object will have
    ///                           one reference.
    ///
    /// \remarks    On devices that don't support pipeline state caches (e.g. Direct3D11, OpenGL),
    ///             the method will silently do nothing.
    VIRTUAL void METHOD(CreatePipelineStateCache)(THIS_
                                                  const PipelineStateCacheCreateInfo REF CreateInfo,
                                                  IPipelineStateCache**                  ppPSOCache) PURE;


    /// Returns the device information, see Diligent::RenderDeviceInfo for details.
    VIRTUAL const RenderDeviceInfo REF METHOD(GetDeviceInfo)(THIS) CONST PURE;

    /// Returns the graphics adapter information, see Diligent::GraphicsAdapterInfo for details.
    VIRTUAL const GraphicsAdapterInfo REF METHOD(GetAdapterInfo)(THIS) CONST PURE;

    /// Returns the basic texture format information.

    /// See Diligent::TextureFormatInfo for details on the provided information.
    /// \param [in] TexFormat - Texture format for which to provide the information
    /// \return Const reference to the TextureFormatInfo structure containing the
    ///         texture format description.
    ///
    /// \remarks This method must be externally synchronized.
    VIRTUAL const TextureFormatInfo REF METHOD(GetTextureFormatInfo)(THIS_
                                                                     TEXTURE_FORMAT TexFormat) CONST PURE;


    /// Returns the extended texture format information.

    /// See Diligent::TextureFormatInfoExt for details on the provided information.
    /// \param [in] TexFormat - Texture format for which to provide the information
    /// \return Const reference to the TextureFormatInfoExt structure containing the
    ///         extended texture format description.
    /// \remark The first time this method is called for a particular format, it may be
    ///         considerably slower than GetTextureFormatInfo(). If you do not require
    ///         extended information, call GetTextureFormatInfo() instead.
    ///
    /// \remarks This method must be externally synchronized.
    VIRTUAL const TextureFormatInfoExt REF METHOD(GetTextureFormatInfoExt)(THIS_
                                                                           TEXTURE_FORMAT TexFormat) PURE;


    /// Returns the sparse texture format info for the given texture format, resource dimension and sample count.
    VIRTUAL SparseTextureFormatInfo METHOD(GetSparseTextureFormatInfo)(THIS_
                                                                       TEXTURE_FORMAT     TexFormat,
                                                                       RESOURCE_DIMENSION Dimension,
                                                                       Uint32             SampleCount) CONST PURE;

    /// Purges device release queues and releases all stale resources.
    /// This method is automatically called by ISwapChain::Present() of the primary swap chain.
    /// \param [in]  ForceRelease - Forces release of all objects. Use this option with
    ///                             great care only if you are sure the resources are not
    ///                             in use by the GPU (such as when the device has just been idled).
    VIRTUAL void METHOD(ReleaseStaleResources)(THIS_
                                               Bool ForceRelease DEFAULT_VALUE(false)) PURE;


    /// Waits until all outstanding operations on the GPU are complete.

    /// \note The method blocks the execution of the calling thread until the GPU is idle.
    ///
    /// \remarks The method does not flush immediate contexts, so it will only wait for commands that
    ///          have been previously submitted for execution. An application should explicitly flush
    ///          the contexts using IDeviceContext::Flush() if it needs to make sure all recorded commands
    ///          are complete when the method returns.
    VIRTUAL void METHOD(IdleGPU)(THIS) PURE;


    /// Returns engine factory this device was created from.
    /// \remarks This method does not increment the reference counter of the returned interface,
    ///          so an application should not call Release().
    VIRTUAL IEngineFactory* METHOD(GetEngineFactory)(THIS) CONST PURE;


    /// Returns a pointer to the shader compilation thread pool.
    /// \remarks This method does not increment the reference counter of the returned interface,
    ///          so an application should not call Release().
    VIRTUAL IThreadPool* METHOD(GetShaderCompilationThreadPool)(THIS) CONST PURE;

#if DILIGENT_CPP_INTERFACE
    /// Overloaded alias for CreateGraphicsPipelineState.
    void CreatePipelineState(const GraphicsPipelineStateCreateInfo& CI, IPipelineState** ppPipelineState)
    {
        CreateGraphicsPipelineState(CI, ppPipelineState);
    }
    /// Overloaded alias for CreateComputePipelineState.
    void CreatePipelineState(const ComputePipelineStateCreateInfo& CI, IPipelineState** ppPipelineState)
    {
        CreateComputePipelineState(CI, ppPipelineState);
    }
    /// Overloaded alias for CreateRayTracingPipelineState.
    void CreatePipelineState(const RayTracingPipelineStateCreateInfo& CI, IPipelineState** ppPipelineState)
    {
        CreateRayTracingPipelineState(CI, ppPipelineState);
    }
    /// Overloaded alias for CreateTilePipelineState.
    void CreatePipelineState(const TilePipelineStateCreateInfo& CI, IPipelineState** ppPipelineState)
    {
        CreateTilePipelineState(CI, ppPipelineState);
    }
#endif
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off
#    define IRenderDevice_CreateBuffer(This, ...)                    CALL_IFACE_METHOD(RenderDevice, CreateBuffer,                    This, __VA_ARGS__)
#    define IRenderDevice_CreateShader(This, ...)                    CALL_IFACE_METHOD(RenderDevice, CreateShader,                    This, __VA_ARGS__)
#    define IRenderDevice_CreateTexture(This, ...)                   CALL_IFACE_METHOD(RenderDevice, CreateTexture,                   This, __VA_ARGS__)
#    define IRenderDevice_CreateSampler(This, ...)                   CALL_IFACE_METHOD(RenderDevice, CreateSampler,                   This, __VA_ARGS__)
#    define IRenderDevice_CreateResourceMapping(This, ...)           CALL_IFACE_METHOD(RenderDevice, CreateResourceMapping,           This, __VA_ARGS__)
#    define IRenderDevice_CreateGraphicsPipelineState(This, ...)     CALL_IFACE_METHOD(RenderDevice, CreateGraphicsPipelineState,     This, __VA_ARGS__)
#    define IRenderDevice_CreateComputePipelineState(This, ...)      CALL_IFACE_METHOD(RenderDevice, CreateComputePipelineState,      This, __VA_ARGS__)
#    define IRenderDevice_CreateRayTracingPipelineState(This, ...)   CALL_IFACE_METHOD(RenderDevice, CreateRayTracingPipelineState,   This, __VA_ARGS__)
#    define IRenderDevice_CreateFence(This, ...)                     CALL_IFACE_METHOD(RenderDevice, CreateFence,                     This, __VA_ARGS__)
#    define IRenderDevice_CreateQuery(This, ...)                     CALL_IFACE_METHOD(RenderDevice, CreateQuery,                     This, __VA_ARGS__)
#    define IRenderDevice_CreateRenderPass(This, ...)                CALL_IFACE_METHOD(RenderDevice, CreateRenderPass,                This, __VA_ARGS__)
#    define IRenderDevice_CreateFramebuffer(This, ...)               CALL_IFACE_METHOD(RenderDevice, CreateFramebuffer,               This, __VA_ARGS__)
#    define IRenderDevice_CreateBLAS(This, ...)                      CALL_IFACE_METHOD(RenderDevice, CreateBLAS,                      This, __VA_ARGS__)
#    define IRenderDevice_CreateTLAS(This, ...)                      CALL_IFACE_METHOD(RenderDevice, CreateTLAS,                      This, __VA_ARGS__)
#    define IRenderDevice_CreateSBT(This, ...)                       CALL_IFACE_METHOD(RenderDevice, CreateSBT,                       This, __VA_ARGS__)
#    define IRenderDevice_CreatePipelineResourceSignature(This, ...) CALL_IFACE_METHOD(RenderDevice, CreatePipelineResourceSignature, This, __VA_ARGS__)
#    define IRenderDevice_CreateDeviceMemory(This, ...)              CALL_IFACE_METHOD(RenderDevice, CreateDeviceMemory,              This, __VA_ARGS__)
#    define IRenderDevice_GetAdapterInfo(This)                       CALL_IFACE_METHOD(RenderDevice, GetAdapterInfo,                  This)
#    define IRenderDevice_GetDeviceInfo(This)                        CALL_IFACE_METHOD(RenderDevice, GetDeviceInfo,                   This)
#    define IRenderDevice_GetTextureFormatInfo(This, ...)            CALL_IFACE_METHOD(RenderDevice, GetTextureFormatInfo,            This, __VA_ARGS__)
#    define IRenderDevice_GetTextureFormatInfoExt(This, ...)         CALL_IFACE_METHOD(RenderDevice, GetTextureFormatInfoExt,         This, __VA_ARGS__)
#    define IRenderDevice_GetSparseTextureFormatInfo(This, ...)      CALL_IFACE_METHOD(RenderDevice, GetSparseTextureFormatInfo,      This, __VA_ARGS__)
#    define IRenderDevice_ReleaseStaleResources(This, ...)           CALL_IFACE_METHOD(RenderDevice, ReleaseStaleResources,           This, __VA_ARGS__)
#    define IRenderDevice_IdleGPU(This)                              CALL_IFACE_METHOD(RenderDevice, IdleGPU,                         This)
#    define IRenderDevice_GetEngineFactory(This)                     CALL_IFACE_METHOD(RenderDevice, GetEngineFactory,                This)
#    define IRenderDevice_GetShaderCompilationThreadPool(This)       CALL_IFACE_METHOD(RenderDevice, GetShaderCompilationThreadPool,  This)
// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
