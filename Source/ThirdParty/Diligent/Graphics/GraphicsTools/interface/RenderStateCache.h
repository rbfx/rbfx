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

#pragma once

/// \file
/// Defines Diligent::IRenderStateCache interface

#include "../../GraphicsEngine/interface/RenderDevice.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// clang-format off

/// Render state cache logging level.
DILIGENT_TYPED_ENUM(RENDER_STATE_CACHE_LOG_LEVEL, Uint8)
{
    /// Logging is disabled.
    RENDER_STATE_CACHE_LOG_LEVEL_DISABLED,

    /// Normal logging level.
    RENDER_STATE_CACHE_LOG_LEVEL_NORMAL,

    /// Verbose logging level.
    RENDER_STATE_CACHE_LOG_LEVEL_VERBOSE
};

// clang-format on

/// Render state cache create information.
struct RenderStateCacheCreateInfo
{
    /// A pointer to the render device, must not be null.
    IRenderDevice* pDevice DEFAULT_INITIALIZER(nullptr);

    /// Logging level, see Diligent::RENDER_STATE_CACHE_LOG_LEVEL.
    RENDER_STATE_CACHE_LOG_LEVEL LogLevel DEFAULT_INITIALIZER(RENDER_STATE_CACHE_LOG_LEVEL_NORMAL);

    /// Whether to enable hot shader and pipeline state reloading.
    ///
    /// \note   Hot reloading introduces some overhead and should
    ///         generally be disabled in production builds.
    bool EnableHotReload DEFAULT_INITIALIZER(false);

    /// Whether to optimize OpenGL shaders.
    ///
    /// \remarks    This option directly controls the value of the
    ///             SerializationDeviceGLInfo::OptimizeShaders member
    ///             of the internal serialization device.
    bool OptimizeGLShaders DEFAULT_INITIALIZER(true);

    /// Optional shader source input stream factory to use when reloading
    /// shaders. If null, original source factory will be used.
    IShaderSourceInputStreamFactory* pReloadSource DEFAULT_INITIALIZER(nullptr);

#if DILIGENT_CPP_INTERFACE
    constexpr RenderStateCacheCreateInfo() noexcept
    {}

    constexpr explicit RenderStateCacheCreateInfo(
        IRenderDevice*                   _pDevice,
        RENDER_STATE_CACHE_LOG_LEVEL     _LogLevel          = RenderStateCacheCreateInfo{}.LogLevel,
        bool                             _EnableHotReload   = RenderStateCacheCreateInfo{}.EnableHotReload,
        bool                             _OptimizeGLShaders = RenderStateCacheCreateInfo{}.OptimizeGLShaders,
        IShaderSourceInputStreamFactory* _pReloadSource     = RenderStateCacheCreateInfo{}.pReloadSource) noexcept :
        pDevice{_pDevice},
        LogLevel{_LogLevel},
        EnableHotReload{_EnableHotReload},
        OptimizeGLShaders{_OptimizeGLShaders},
        pReloadSource{_pReloadSource}
    {}
#endif
};
typedef struct RenderStateCacheCreateInfo RenderStateCacheCreateInfo;

#include "../../../Primitives/interface/DefineRefMacro.h"

/// Type of the callback function called by the IRenderStateCache::Reload method.
typedef void(DILIGENT_CALL_TYPE* ReloadGraphicsPipelineCallbackType)(const char* PipelineName, GraphicsPipelineDesc REF GraphicsDesc, void* pUserData);

#include "../../../Primitives/interface/UndefRefMacro.h"

// clang-format on

// {5B356268-256C-401F-BDE2-B9832157141A}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_RenderStateCache =
    {0x5b356268, 0x256c, 0x401f, {0xbd, 0xe2, 0xb9, 0x83, 0x21, 0x57, 0x14, 0x1a}};

#define DILIGENT_INTERFACE_NAME IRenderStateCache
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IRenderStateCacheInclusiveMethods \
    IObjectInclusiveMethods;              \
    IRenderStateCacheMethods RenderStateCache

// clang-format off

DILIGENT_BEGIN_INTERFACE(IRenderStateCache, IObject)
{
    /// Loads the cache contents.

    /// \param [in] pCacheData     - A pointer to the cache data to load objects from.
    /// \param [in] ContentVersion - The expected version of the content in the cache.
    ///                              If the version of the content in the cache does not
    ///                              match the expected version, the method will fail.
    ///                              If default value is used (~0u aka 0xFFFFFFFF), the version
    ///                              will not be checked.
    /// \param [in] MakeCopy       - Whether to make a copy of the data blob, or use the
    ///                              the original contents.
    /// \return     true if the data were loaded successfully, and false otherwise.
    ///
    /// \note       If the data were not copied, the cache will keep a strong reference
    ///             to the pCacheData data blob. It will be kept alive until the cache object
    ///             is released or the Reset() method is called.
    ///
    /// \warning    If the data were loaded without making a copy, the application
    ///             must not modify it while it is in use by the cache object.
    /// 
    /// \warning    This method is not thread-safe and must not be called simultaneously
    ///             with other methods.
    VIRTUAL bool METHOD(Load)(THIS_
                              const IDataBlob* pCacheData,
                              Uint32           ContentVersion DEFAULT_VALUE(~0u), 
                              bool             MakeCopy       DEFAULT_VALUE(false)) PURE;

    /// Creates a shader object from cached data.

    /// \param [in]  ShaderCI - Shader create info, see Diligent::ShaderCreateInfo for details.
    /// \param [out] ppShader - Address of the memory location where a pointer to the created
    ///                         shader object will be written.
    ///
    /// \return     true if the shader was loaded from the cache, and false otherwise.
    VIRTUAL bool METHOD(CreateShader)(THIS_
                                      const ShaderCreateInfo REF ShaderCI,
                                      IShader**                  ppShader) PURE;

    /// Creates a graphics pipeline state object from cached data.

    /// \param [in]  PSOCreateInfo   - Graphics pipeline state create info, see Diligent::GraphicsPipelineStateCreateInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the created
    ///                                pipeline state object will be written.
    ///
    /// \return     true if the pipeline state was loaded from the cache, and false otherwise.
    VIRTUAL bool METHOD(CreateGraphicsPipelineState)(THIS_
                                                     const GraphicsPipelineStateCreateInfo REF PSOCreateInfo,
                                                     IPipelineState**                          ppPipelineState) PURE;

    /// Creates a compute pipeline state object from cached data.

    /// \param [in]  PSOCreateInfo   - Compute pipeline state create info, see Diligent::ComputePipelineStateCreateInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the created
    ///                                pipeline state object will be written.
    ///
    /// \return     true if the pipeline state was loaded from the cache, and false otherwise.
    VIRTUAL bool METHOD(CreateComputePipelineState)(THIS_
                                                    const ComputePipelineStateCreateInfo REF PSOCreateInfo,
                                                    IPipelineState**                         ppPipelineState) PURE;

    /// Creates a ray tracing pipeline state object from cached data.

    /// \param [in]  PSOCreateInfo   - Ray tracing pipeline state create info, see Diligent::RayTracingPipelineStateCreateInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the created
    ///                                pipeline state object will be written.
    ///
    /// \return     true if the pipeline state was loaded from the cache, and false otherwise.
    VIRTUAL bool METHOD(CreateRayTracingPipelineState)(THIS_
                                                       const RayTracingPipelineStateCreateInfo REF PSOCreateInfo,
                                                       IPipelineState**                            ppPipelineState) PURE;

    /// Creates a tile pipeline state object from cached data.

    /// \param [in]  PSOCreateInfo   - Tile pipeline state create info, see Diligent::TilePipelineStateCreateInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the created
    ///                                pipeline state object will be written.
    ///
    /// \return     true if the pipeline state was loaded from the cache, and false otherwise.
    VIRTUAL bool METHOD(CreateTilePipelineState)(THIS_
                                                 const TilePipelineStateCreateInfo REF PSOCreateInfo,
                                                 IPipelineState**                      ppPipelineState) PURE;

    /// Writes cache contents to a memory blob.

    /// \param [in]   ContentVersion - The version of the content to write.
    /// \param [out]  ppBlob         - Address of the memory location where a pointer to the created
    ///                                data blob will be written.
    ///
    /// \return     true if the data was written successfully, and false otherwise.
    ///
    /// \remarks    If ContentVersion is ~0u (aka 0xFFFFFFFF), the version of the
    ///             previously loaded content will be used, or 0 if none was loaded.
    VIRTUAL Bool METHOD(WriteToBlob)(THIS_
                                     Uint32      ContentVersion, 
                                     IDataBlob** ppBlob) PURE;

    /// Writes cache contents to a file stream.

    /// \param [in]  ContentVersion - The version of the content to write.
    /// \param [in]  pStream        - Pointer to the IFileStream interface to use for writing.
    ///
    /// \return     true if the data was written successfully, and false otherwise.
    ///
    /// \remarks    If ContentVersion is ~0u (aka 0xFFFFFFFF), the version of the
    ///             previously loaded content will be used, or 0 if none was loaded.
    VIRTUAL Bool METHOD(WriteToStream)(THIS_
                                       Uint32       ContentVersion, 
                                       IFileStream* pStream) PURE;


    /// Resets the cache to default state.
    VIRTUAL void METHOD(Reset)(THIS) PURE;

    /// Reloads render states in the cache.

    /// \param [in]  ReloadGraphicsPipeline - An optional callback function that will be called by the render state cache
    ///                                       to let the application modify graphics pipeline state info before creating new
    ///                                       pipeline.
    /// \param [in]  pUserData              - A pointer to the user-specific data to pass to ReloadGraphicsPipeline callback.
    ///
    /// \return     The total number of render states (shaders and pipelines) that were reloaded.
    ///
    /// \remars     Reloading is only enabled if the cache was created with the EnableHotReload member of
    ///             RenderStateCacheCreateInfo member set to true.
    VIRTUAL Uint32 METHOD(Reload)(THIS_
                                  ReloadGraphicsPipelineCallbackType ReloadGraphicsPipeline DEFAULT_VALUE(nullptr), 
                                  void*                              pUserData              DEFAULT_VALUE(nullptr)) PURE;

    /// Returns the content version of the cache data.
    /// If no data has been loaded, returns ~0u (aka 0xFFFFFFFF).
    VIRTUAL Uint32 METHOD(GetContentVersion)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off
#    define IRenderStateCache_Load(This, ...)                          CALL_IFACE_METHOD(RenderStateCache, Load,                         This, __VA_ARGS__)
#    define IRenderStateCache_CreateShader(This, ...)                  CALL_IFACE_METHOD(RenderStateCache, CreateShader,                 This, __VA_ARGS__)
#    define IRenderStateCache_CreateGraphicsPipelineState(This, ...)   CALL_IFACE_METHOD(RenderStateCache, CreateGraphicsPipelineState,  This, __VA_ARGS__)
#    define IRenderStateCache_CreateComputePipelineState(This, ...)    CALL_IFACE_METHOD(RenderStateCache, CreateComputePipelineState,   This, __VA_ARGS__)
#    define IRenderStateCache_CreateRayTracingPipelineState(This, ...) CALL_IFACE_METHOD(RenderStateCache, CreateRayTracingPipelineState,This, __VA_ARGS__)
#    define IRenderStateCache_CreateTilePipelineState(This, ...)       CALL_IFACE_METHOD(RenderStateCache, CreateTilePipelineState,      This, __VA_ARGS__)
#    define IRenderStateCache_WriteToBlob(This, ...)                   CALL_IFACE_METHOD(RenderStateCache, WriteToBlob,                  This, __VA_ARGS__)
#    define IRenderStateCache_WriteToStream(This, ...)                 CALL_IFACE_METHOD(RenderStateCache, WriteToStream,                This, __VA_ARGS__)
#    define IRenderStateCache_Reset(This)                              CALL_IFACE_METHOD(RenderStateCache, Reset,                        This)
#    define IRenderStateCache_Reload(This, ...)                        CALL_IFACE_METHOD(RenderStateCache, Reload,                       This, __VA_ARGS__)
#    define IRenderStateCache_GetContentVersion(This)                  CALL_IFACE_METHOD(RenderStateCache, GetContentVersion,            This)
// clang-format on

#endif

#include "../../../Primitives/interface/DefineGlobalFuncHelperMacros.h"

void DILIGENT_GLOBAL_FUNCTION(CreateRenderStateCache)(const RenderStateCacheCreateInfo REF CreateInfo,
                                                      IRenderStateCache**                  ppCache);

#include "../../../Primitives/interface/UndefGlobalFuncHelperMacros.h"

DILIGENT_END_NAMESPACE // namespace Diligent
