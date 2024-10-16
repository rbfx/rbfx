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
/// Implementation of the Diligent::RenderDeviceBase template class and related structures

#include <atomic>
#include <thread>

#include "RenderDevice.h"
#include "DeviceObjectBase.hpp"
#include "Defines.h"
#include "ResourceMappingImpl.hpp"
#include "ObjectsRegistry.hpp"
#include "HashUtils.hpp"
#include "ObjectBase.hpp"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "GraphicsAccessories.hpp"
#include "FixedBlockMemoryAllocator.hpp"
#include "EngineMemory.h"
#include "STDAllocator.hpp"
#include "IndexWrapper.hpp"
#include "ThreadPool.hpp"

namespace Diligent
{

/// Returns enabled device features based on the supported features and requested features,
/// and throws an exception in case requested features are missing:
///
/// | SupportedFeature  |  RequestedFeature  |     Result    |
/// |-------------------|--------------------|---------------|
/// |    DISABLED       |     DISABLED       |   DISABLED    |
/// |    OPTIONAL       |     DISABLED       |   DISABLED    |
/// |    ENABLED        |     DISABLED       |   ENABLED     |
/// |                   |                    |               |
/// |    DISABLED       |     OPTIONAL       |   DISABLED    |
/// |    OPTIONAL       |     OPTIONAL       |   ENABLED     |
/// |    ENABLED        |     OPTIONAL       |   ENABLED     |
/// |                   |                    |               |
/// |    DISABLED       |     ENABLED        |   EXCEPTION   |
/// |    OPTIONAL       |     ENABLED        |   ENABLED     |
/// |    ENABLED        |     ENABLED        |   ENABLED     |
///
DeviceFeatures EnableDeviceFeatures(const DeviceFeatures& SupportedFeatures,
                                    const DeviceFeatures& RequestedFeatures) noexcept(false);

/// Checks sparse texture format support and returns the component type
COMPONENT_TYPE CheckSparseTextureFormatSupport(TEXTURE_FORMAT                  TexFormat,
                                               RESOURCE_DIMENSION              Dimension,
                                               Uint32                          SampleCount,
                                               const SparseResourceProperties& SparseRes) noexcept;
/// Base implementation of a render device

/// \tparam EngineImplTraits - Engine implementation type traits.
///
/// \warning    Render device must *NOT* hold strong references to any object it creates
///             to avoid cyclic dependencies. Device context, swap chain and all object
///             the device creates keep strong reference to the device.
///             Device only holds weak reference to the immediate context.
template <typename EngineImplTraits>
class RenderDeviceBase : public ObjectBase<typename EngineImplTraits::RenderDeviceInterface>
{
public:
    using BaseInterface = typename EngineImplTraits::RenderDeviceInterface;
    using TObjectBase   = ObjectBase<BaseInterface>;

    using RenderDeviceImplType              = typename EngineImplTraits::RenderDeviceImplType;
    using DeviceContextImplType             = typename EngineImplTraits::DeviceContextImplType;
    using PipelineStateImplType             = typename EngineImplTraits::PipelineStateImplType;
    using ShaderResourceBindingImplType     = typename EngineImplTraits::ShaderResourceBindingImplType;
    using BufferImplType                    = typename EngineImplTraits::BufferImplType;
    using BufferViewImplType                = typename EngineImplTraits::BufferViewImplType;
    using TextureImplType                   = typename EngineImplTraits::TextureImplType;
    using TextureViewImplType               = typename EngineImplTraits::TextureViewImplType;
    using ShaderImplType                    = typename EngineImplTraits::ShaderImplType;
    using SamplerImplType                   = typename EngineImplTraits::SamplerImplType;
    using FenceImplType                     = typename EngineImplTraits::FenceImplType;
    using QueryImplType                     = typename EngineImplTraits::QueryImplType;
    using RenderPassImplType                = typename EngineImplTraits::RenderPassImplType;
    using FramebufferImplType               = typename EngineImplTraits::FramebufferImplType;
    using BottomLevelASImplType             = typename EngineImplTraits::BottomLevelASImplType;
    using TopLevelASImplType                = typename EngineImplTraits::TopLevelASImplType;
    using ShaderBindingTableImplType        = typename EngineImplTraits::ShaderBindingTableImplType;
    using PipelineResourceSignatureImplType = typename EngineImplTraits::PipelineResourceSignatureImplType;
    using DeviceMemoryImplType              = typename EngineImplTraits::DeviceMemoryImplType;
    using PipelineStateCacheImplType        = typename EngineImplTraits::PipelineStateCacheImplType;

    /// \param pRefCounters    - Reference counters object that controls the lifetime of this render device
    /// \param RawMemAllocator - Allocator that will be used to allocate memory for all device objects (including render device itself)
    /// \param pEngineFactory  - Engine factory that was used to create this device
    /// \param EngineCI        - Engine create info struct, see Diligent::EngineCreateInfo.
    /// \param AdapterInfo     - Graphics adapter info, see Diligent::AdapterInfo.
    ///
    /// \remarks Render device uses fixed block allocators (see FixedBlockMemoryAllocator) to allocate memory for
    ///          device objects. The object sizes from EngineImplTraits are used to initialize the allocators.
    RenderDeviceBase(IReferenceCounters*        pRefCounters,
                     IMemoryAllocator&          RawMemAllocator,
                     IEngineFactory*            pEngineFactory,
                     const EngineCreateInfo&    EngineCI,
                     const GraphicsAdapterInfo& AdapterInfo) :
        // clang-format off
        TObjectBase           {pRefCounters},
        m_pEngineFactory      {pEngineFactory},
        m_ValidationFlags     {EngineCI.ValidationFlags},
        m_AdapterInfo         {AdapterInfo},
        m_TextureFormatsInfo  (TEX_FORMAT_NUM_FORMATS, TextureFormatInfoExt(), STD_ALLOCATOR_RAW_MEM(TextureFormatInfoExt, RawMemAllocator, "Allocator for vector<TextureFormatInfoExt>")),
        m_TexFmtInfoInitFlags (TEX_FORMAT_NUM_FORMATS, false, STD_ALLOCATOR_RAW_MEM(bool, RawMemAllocator, "Allocator for vector<bool>")),
        m_wpImmediateContexts (std::max(1u, EngineCI.NumImmediateContexts), RefCntWeakPtr<DeviceContextImplType>(), STD_ALLOCATOR_RAW_MEM(RefCntWeakPtr<DeviceContextImplType>, RawMemAllocator, "Allocator for vector<RefCntWeakPtr<DeviceContextImplType>>")),
        m_wpDeferredContexts  (EngineCI.NumDeferredContexts, RefCntWeakPtr<DeviceContextImplType>(), STD_ALLOCATOR_RAW_MEM(RefCntWeakPtr<DeviceContextImplType>, RawMemAllocator, "Allocator for vector<RefCntWeakPtr<DeviceContextImplType>>")),
        m_RawMemAllocator     {RawMemAllocator},
        m_TexObjAllocator     {RawMemAllocator, sizeof(TextureImplType),                   16},
        m_TexViewObjAllocator {RawMemAllocator, sizeof(TextureViewImplType),               32},
        m_BufObjAllocator     {RawMemAllocator, sizeof(BufferImplType),                    16},
        m_BuffViewObjAllocator{RawMemAllocator, sizeof(BufferViewImplType),                32},
        m_ShaderObjAllocator  {RawMemAllocator, sizeof(ShaderImplType),                    16},
        m_SamplerObjAllocator {RawMemAllocator, sizeof(SamplerImplType),                   32},
        m_PSOAllocator        {RawMemAllocator, sizeof(PipelineStateImplType),             16},
        m_SRBAllocator        {RawMemAllocator, sizeof(ShaderResourceBindingImplType),     64},
        m_ResMappingAllocator {RawMemAllocator, sizeof(ResourceMappingImpl),                8},
        m_FenceAllocator      {RawMemAllocator, sizeof(FenceImplType),                     16},
        m_QueryAllocator      {RawMemAllocator, sizeof(QueryImplType),                     16},
        m_RenderPassAllocator {RawMemAllocator, sizeof(RenderPassImplType),                16},
        m_FramebufferAllocator{RawMemAllocator, sizeof(FramebufferImplType),               16},
        m_BLASAllocator       {RawMemAllocator, sizeof(BottomLevelASImplType),              8},
        m_TLASAllocator       {RawMemAllocator, sizeof(TopLevelASImplType),                 8},
        m_SBTAllocator        {RawMemAllocator, sizeof(ShaderBindingTableImplType),         8},
        m_PipeResSignAllocator{RawMemAllocator, sizeof(PipelineResourceSignatureImplType), 16},
        m_MemObjAllocator     {RawMemAllocator, sizeof(DeviceMemoryImplType),              16},
        m_PSOCacheAllocator   {RawMemAllocator, sizeof(PipelineStateCacheImplType),         4}
    // clang-format on
    {
        // Initialize texture format info
        for (Uint32 Fmt = TEX_FORMAT_UNKNOWN; Fmt < TEX_FORMAT_NUM_FORMATS; ++Fmt)
            static_cast<TextureFormatAttribs&>(m_TextureFormatsInfo[Fmt]) = GetTextureFormatAttribs(static_cast<TEXTURE_FORMAT>(Fmt));

        // https://msdn.microsoft.com/en-us/library/windows/desktop/ff471325(v=vs.85).aspx
        TEXTURE_FORMAT FilterableFormats[] =
            {
                TEX_FORMAT_RGBA32_FLOAT, // OpenGL ES3.1 does not require this format to be filterable
                TEX_FORMAT_RGBA16_FLOAT,
                TEX_FORMAT_RGBA16_UNORM,
                TEX_FORMAT_RGBA16_SNORM,
                TEX_FORMAT_RG32_FLOAT, // OpenGL ES3.1 does not require this format to be filterable
                TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS,
                //TEX_FORMAT_R10G10B10A2_UNORM,
                TEX_FORMAT_R11G11B10_FLOAT,
                TEX_FORMAT_RGBA8_UNORM,
                TEX_FORMAT_RGBA8_UNORM_SRGB,
                TEX_FORMAT_RGBA8_SNORM,
                TEX_FORMAT_RG16_FLOAT,
                TEX_FORMAT_RG16_UNORM,
                TEX_FORMAT_RG16_SNORM,
                TEX_FORMAT_R32_FLOAT, // OpenGL ES3.1 does not require this format to be filterable
                TEX_FORMAT_R24_UNORM_X8_TYPELESS,
                TEX_FORMAT_RG8_UNORM,
                TEX_FORMAT_RG8_SNORM,
                TEX_FORMAT_R16_FLOAT,
                TEX_FORMAT_R16_UNORM,
                TEX_FORMAT_R16_SNORM,
                TEX_FORMAT_R8_UNORM,
                TEX_FORMAT_R8_SNORM,
                TEX_FORMAT_A8_UNORM,
                TEX_FORMAT_RGB9E5_SHAREDEXP,
                TEX_FORMAT_RG8_B8G8_UNORM,
                TEX_FORMAT_G8R8_G8B8_UNORM,
                TEX_FORMAT_BC1_UNORM,
                TEX_FORMAT_BC1_UNORM_SRGB,
                TEX_FORMAT_BC2_UNORM,
                TEX_FORMAT_BC2_UNORM_SRGB,
                TEX_FORMAT_BC3_UNORM,
                TEX_FORMAT_BC3_UNORM_SRGB,
                TEX_FORMAT_BC4_UNORM,
                TEX_FORMAT_BC4_SNORM,
                TEX_FORMAT_BC5_UNORM,
                TEX_FORMAT_BC5_SNORM,
                TEX_FORMAT_B5G6R5_UNORM};
        for (Uint32 fmt = 0; fmt < _countof(FilterableFormats); ++fmt)
            m_TextureFormatsInfo[FilterableFormats[fmt]].Filterable = true;
    }

    ~RenderDeviceBase()
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_RenderDevice, ObjectBase<BaseInterface>)

    // It is important to have final implementation of Release() method to avoid
    // virtual calls
    inline virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() override final
    {
        return TObjectBase::Release();
    }

    /// Implementation of IRenderDevice::CreateResourceMapping().
    virtual void DILIGENT_CALL_TYPE CreateResourceMapping(const ResourceMappingCreateInfo& ResMappingCI, IResourceMapping** ppMapping) override final
    {
        DEV_CHECK_ERR(ppMapping != nullptr, "Null pointer provided");
        if (ppMapping == nullptr)
            return;
        DEV_CHECK_ERR(*ppMapping == nullptr, "Overwriting reference to existing object may cause memory leaks");
        DEV_CHECK_ERR(ResMappingCI.pEntries == nullptr || ResMappingCI.NumEntries != 0, "Starting with API253010, the number of entries is defined through the NumEntries member.");

        auto* pResourceMapping{NEW_RC_OBJ(m_ResMappingAllocator, "ResourceMappingImpl instance", ResourceMappingImpl)(GetRawAllocator())};
        pResourceMapping->QueryInterface(IID_ResourceMapping, reinterpret_cast<IObject**>(ppMapping));
        if (ResMappingCI.pEntries != nullptr)
        {
            for (Uint32 i = 0; i < ResMappingCI.NumEntries; ++i)
            {
                const auto& Entry = ResMappingCI.pEntries[i];
                if (Entry.Name != nullptr && Entry.pObject != nullptr)
                    (*ppMapping)->AddResourceArray(Entry.Name, Entry.ArrayIndex, &Entry.pObject, 1, true);
                else
                    DEV_ERROR("Name and pObject must not be null. Note that starting with API253010, the number of entries is defined through the NumEntries member.");
            }
        }
    }


    /// Implementation of IRenderDevice::GetDeviceInfo().
    virtual const RenderDeviceInfo& DILIGENT_CALL_TYPE GetDeviceInfo() const override final
    {
        return m_DeviceInfo;
    }

    /// Implementation of IRenderDevice::GetAdapterInfo().
    virtual const GraphicsAdapterInfo& DILIGENT_CALL_TYPE GetAdapterInfo() const override final
    {
        return m_AdapterInfo;
    }

    /// Implementation of IRenderDevice::GetTextureFormatInfo().
    virtual const TextureFormatInfo& DILIGENT_CALL_TYPE GetTextureFormatInfo(TEXTURE_FORMAT TexFormat) const override final
    {
        VERIFY(TexFormat >= TEX_FORMAT_UNKNOWN && TexFormat < TEX_FORMAT_NUM_FORMATS, "Texture format out of range");
        const auto& TexFmtInfo = m_TextureFormatsInfo[TexFormat];
        VERIFY(TexFmtInfo.Format == TexFormat, "Sanity check failed");
        return TexFmtInfo;
    }

    /// Implementation of IRenderDevice::GetTextureFormatInfoExt().
    virtual const TextureFormatInfoExt& DILIGENT_CALL_TYPE GetTextureFormatInfoExt(TEXTURE_FORMAT TexFormat) override final
    {
        VERIFY(TexFormat >= TEX_FORMAT_UNKNOWN && TexFormat < TEX_FORMAT_NUM_FORMATS, "Texture format out of range");
        const auto& TexFmtInfo = m_TextureFormatsInfo[TexFormat];
        VERIFY(TexFmtInfo.Format == TexFormat, "Sanity check failed");
        if (!m_TexFmtInfoInitFlags[TexFormat])
        {
            if (TexFmtInfo.Supported)
                TestTextureFormat(TexFormat);
            m_TexFmtInfoInitFlags[TexFormat] = true;
        }
        return TexFmtInfo;
    }

    virtual IEngineFactory* DILIGENT_CALL_TYPE GetEngineFactory() const override final
    {
        return m_pEngineFactory;
    }

    /// Base implementation of IRenderDevice::CreateTilePipelineState().
    virtual void DILIGENT_CALL_TYPE CreateTilePipelineState(const TilePipelineStateCreateInfo& PSOCreateInfo,
                                                            IPipelineState**                   ppPipelineState) override
    {
        UNSUPPORTED("Tile pipeline is not supported by this device. Please check DeviceFeatures.TileShaders feature.");
    }

    /// Set weak reference to the immediate context
    void SetImmediateContext(size_t Ctx, DeviceContextImplType* pImmediateContext)
    {
        VERIFY(m_wpImmediateContexts[Ctx].Lock() == nullptr, "Immediate context has already been set");
        m_wpImmediateContexts[Ctx] = pImmediateContext;
    }

    /// Set weak reference to the deferred context
    void SetDeferredContext(size_t Ctx, DeviceContextImplType* pDeferredCtx)
    {
        VERIFY(m_wpDeferredContexts[Ctx].Lock() == nullptr, "Deferred context has already been set");
        m_wpDeferredContexts[Ctx] = pDeferredCtx;
    }

    /// Returns the number of immediate contexts
    size_t GetNumImmediateContexts() const
    {
        return m_wpImmediateContexts.size();
    }

    /// Returns number of deferred contexts
    size_t GetNumDeferredContexts() const
    {
        return m_wpDeferredContexts.size();
    }

    RefCntAutoPtr<DeviceContextImplType> GetImmediateContext(size_t Ctx) { return m_wpImmediateContexts[Ctx].Lock(); }
    RefCntAutoPtr<DeviceContextImplType> GetDeferredContext(size_t Ctx) { return m_wpDeferredContexts[Ctx].Lock(); }

    FixedBlockMemoryAllocator& GetTexViewObjAllocator() { return m_TexViewObjAllocator; }
    FixedBlockMemoryAllocator& GetBuffViewObjAllocator() { return m_BuffViewObjAllocator; }
    FixedBlockMemoryAllocator& GetSRBAllocator() { return m_SRBAllocator; }

    VALIDATION_FLAGS GetValidationFlags() const { return m_ValidationFlags; }

    // Convenience function
    const DeviceFeatures& GetFeatures() const
    {
        return m_DeviceInfo.Features;
    }

    UniqueIdentifier GenerateUniqueId()
    {
        return m_UniqueId.fetch_add(1) + 1;
    }

    virtual IThreadPool* DILIGENT_CALL_TYPE GetShaderCompilationThreadPool() const override final
    {
        return m_pShaderCompilationThreadPool;
    }

protected:
    virtual void TestTextureFormat(TEXTURE_FORMAT TexFormat) = 0;

    void InitShaderCompilationThreadPool(IThreadPool* pShaderCompilationThreadPool, Uint32 NumThreads)
    {
        if (!m_DeviceInfo.Features.AsyncShaderCompilation)
            return;

        if (pShaderCompilationThreadPool != nullptr)
        {
            m_pShaderCompilationThreadPool = pShaderCompilationThreadPool;
        }
        else if (NumThreads != 0)
        {
            const Uint32 NumCores = std::max(std::thread::hardware_concurrency(), 1u);

            ThreadPoolCreateInfo ThreadPoolCI;
            if (NumThreads == ~0u)
            {
                // Leave one core for the main thread
                ThreadPoolCI.NumThreads = std::max(NumCores, 2u) - 1u;
            }
            else
            {
                ThreadPoolCI.NumThreads = std::min(NumThreads, std::max(NumCores * 4, 128u));
            }
            m_pShaderCompilationThreadPool = CreateThreadPool(ThreadPoolCI);
        }
    }
    /// Helper template function to facilitate device object creation

    /// \tparam ObjectType            - The type of the object being created (IBuffer, ITexture, etc.).
    /// \tparam ObjectDescType        - The type of the object description structure (BufferDesc, TextureDesc, etc.).
    /// \tparam ObjectConstructorType - The type of the function that constructs the object.
    ///
    /// \param ObjectTypeName      - String name of the object type ("buffer", "texture", etc.).
    /// \param Desc                - Object description.
    /// \param ppObject            - Memory address where the pointer to the created object will be stored.
    /// \param ConstructObject     - Function that constructs the object.
    template <typename ObjectType, typename ObjectDescType, typename ObjectConstructorType>
    void CreateDeviceObject(const Char*           ObjectTypeName,
                            const ObjectDescType& Desc,
                            ObjectType**          ppObject,
                            ObjectConstructorType ConstructObject)
    {
        DEV_CHECK_ERR(ppObject != nullptr, "Null pointer provided");
        if (!ppObject)
            return;

        DEV_CHECK_ERR(*ppObject == nullptr, "Overwriting reference to existing object may cause memory leaks");
        // Do not release *ppObject here!
        // Should this happen, RefCntAutoPtr<> will take care of this!
        //if( *ppObject )
        //{
        //    (*ppObject)->Release();
        //    *ppObject = nullptr;
        //}

        *ppObject = nullptr;

        try
        {
            ConstructObject();
        }
        catch (...)
        {
            VERIFY(*ppObject == nullptr, "Object was created despite error");
            if (*ppObject)
            {
                (*ppObject)->Release();
                *ppObject = nullptr;
            }
            const auto ObjectDescString = GetObjectDescString(Desc);
            if (!ObjectDescString.empty())
            {
                LOG_ERROR("Failed to create ", ObjectTypeName, " object '", (Desc.Name ? Desc.Name : ""), "'\n", ObjectDescString);
            }
            else
            {
                LOG_ERROR("Failed to create ", ObjectTypeName, " object '", (Desc.Name ? Desc.Name : ""), "'");
            }
        }
    }

    template <typename PSOCreateInfoType, typename... ExtraArgsType>
    void CreatePipelineStateImpl(IPipelineState** ppPipelineState, const PSOCreateInfoType& PSOCreateInfo, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("Pipeline State", PSOCreateInfo.PSODesc, ppPipelineState,
                           [&]() //
                           {
                               auto* pPipelineStateImpl = NEW_RC_OBJ(m_PSOAllocator, "Pipeline State instance", PipelineStateImplType)(static_cast<RenderDeviceImplType*>(this), PSOCreateInfo, ExtraArgs...);
                               pPipelineStateImpl->QueryInterface(IID_PipelineState, reinterpret_cast<IObject**>(ppPipelineState));
                           });
    }

    template <typename... ExtraArgsType>
    void CreateBufferImpl(IBuffer** ppBuffer, const BufferDesc& BuffDesc, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("Buffer", BuffDesc, ppBuffer,
                           [&]() //
                           {
                               auto* pBufferImpl = NEW_RC_OBJ(m_BufObjAllocator, "Buffer instance", BufferImplType)(m_BuffViewObjAllocator, static_cast<RenderDeviceImplType*>(this), BuffDesc, ExtraArgs...);
                               pBufferImpl->QueryInterface(IID_Buffer, reinterpret_cast<IObject**>(ppBuffer));
                               pBufferImpl->CreateDefaultViews();
                           });
    }

    template <typename... ExtraArgsType>
    void CreateTextureImpl(ITexture** ppTexture, const TextureDesc& TexDesc, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("Texture", TexDesc, ppTexture,
                           [&]() //
                           {
                               auto* pTextureImpl = NEW_RC_OBJ(m_TexObjAllocator, "Texture instance", TextureImplType)(m_TexViewObjAllocator, static_cast<RenderDeviceImplType*>(this), TexDesc, ExtraArgs...);
                               pTextureImpl->QueryInterface(IID_Texture, reinterpret_cast<IObject**>(ppTexture));
                               pTextureImpl->CreateDefaultViews();
                           });
    }

    template <typename... ExtraArgsType>
    void CreateShaderImpl(IShader** ppShader, const ShaderCreateInfo& ShaderCI, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("Shader", ShaderCI.Desc, ppShader,
                           [&]() //
                           {
                               auto* pShaderImpl = NEW_RC_OBJ(m_ShaderObjAllocator, "Shader instance", ShaderImplType)(static_cast<RenderDeviceImplType*>(this), ShaderCI, ExtraArgs...);
                               pShaderImpl->QueryInterface(IID_Shader, reinterpret_cast<IObject**>(ppShader));
                           });
    }

    template <typename... ExtraArgsType>
    void CreateSamplerImpl(ISampler** ppSampler, const SamplerDesc& SamplerDesc, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("Sampler", SamplerDesc, ppSampler,
                           [&]() //
                           {
                               auto pSampler = m_SamplersRegistry.Get(
                                   SamplerDesc,
                                   [&]() {
                                       return RefCntAutoPtr<ISampler>{NEW_RC_OBJ(m_SamplerObjAllocator, "Sampler instance", SamplerImplType)(static_cast<RenderDeviceImplType*>(this), SamplerDesc, ExtraArgs...)};
                                   });

                               *ppSampler = pSampler.Detach();
                           });
    }

    template <typename... ExtraArgsType>
    void CreateFenceImpl(IFence** ppFence, const FenceDesc& Desc, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("Fence", Desc, ppFence,
                           [&]() //
                           {
                               auto* pFenceImpl = NEW_RC_OBJ(m_FenceAllocator, "Fence instance", FenceImplType)(static_cast<RenderDeviceImplType*>(this), Desc, ExtraArgs...);
                               pFenceImpl->QueryInterface(IID_Fence, reinterpret_cast<IObject**>(ppFence));
                           });
    }

    void CreateQueryImpl(IQuery** ppQuery, const QueryDesc& Desc)
    {
        CreateDeviceObject("Query", Desc, ppQuery,
                           [&]() //
                           {
                               auto* pQueryImpl = NEW_RC_OBJ(m_QueryAllocator, "Query instance", QueryImplType)(static_cast<RenderDeviceImplType*>(this), Desc);
                               pQueryImpl->QueryInterface(IID_Query, reinterpret_cast<IObject**>(ppQuery));
                           });
    }

    template <typename... ExtraArgsType>
    void CreateRenderPassImpl(IRenderPass** ppRenderPass, const RenderPassDesc& Desc, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("RenderPass", Desc, ppRenderPass,
                           [&]() //
                           {
                               auto* pRenderPassImpl = NEW_RC_OBJ(m_RenderPassAllocator, "Render instance", RenderPassImplType)(static_cast<RenderDeviceImplType*>(this), Desc, ExtraArgs...);
                               pRenderPassImpl->QueryInterface(IID_RenderPass, reinterpret_cast<IObject**>(ppRenderPass));
                           });
    }

    template <typename... ExtraArgsType>
    void CreateFramebufferImpl(IFramebuffer** ppFramebuffer, const FramebufferDesc& Desc, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("Framebuffer", Desc, ppFramebuffer,
                           [&]() //
                           {
                               auto* pFramebufferImpl = NEW_RC_OBJ(m_FramebufferAllocator, "Framebuffer instance", FramebufferImplType)(static_cast<RenderDeviceImplType*>(this), Desc, ExtraArgs...);
                               pFramebufferImpl->QueryInterface(IID_Framebuffer, reinterpret_cast<IObject**>(ppFramebuffer));
                           });
    }

    template <typename... ExtraArgsType>
    void CreateBLASImpl(IBottomLevelAS** ppBLAS, const BottomLevelASDesc& Desc, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("BottomLevelAS", Desc, ppBLAS,
                           [&]() //
                           {
                               auto* pBottomLevelASImpl = NEW_RC_OBJ(m_BLASAllocator, "BottomLevelAS instance", BottomLevelASImplType)(static_cast<RenderDeviceImplType*>(this), Desc, ExtraArgs...);
                               pBottomLevelASImpl->QueryInterface(IID_BottomLevelAS, reinterpret_cast<IObject**>(ppBLAS));
                           });
    }

    template <typename... ExtraArgsType>
    void CreateTLASImpl(ITopLevelAS** ppTLAS, const TopLevelASDesc& Desc, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("TopLevelAS", Desc, ppTLAS,
                           [&]() //
                           {
                               auto* pTopLevelASImpl = NEW_RC_OBJ(m_TLASAllocator, "TopLevelAS instance", TopLevelASImplType)(static_cast<RenderDeviceImplType*>(this), Desc, ExtraArgs...);
                               pTopLevelASImpl->QueryInterface(IID_TopLevelAS, reinterpret_cast<IObject**>(ppTLAS));
                           });
    }

    void CreateSBTImpl(IShaderBindingTable** ppSBT, const ShaderBindingTableDesc& Desc)
    {
        CreateDeviceObject("ShaderBindingTable", Desc, ppSBT,
                           [&]() //
                           {
                               auto* pSBTImpl = NEW_RC_OBJ(m_SBTAllocator, "ShaderBindingTable instance", ShaderBindingTableImplType)(static_cast<RenderDeviceImplType*>(this), Desc);
                               pSBTImpl->QueryInterface(IID_ShaderBindingTable, reinterpret_cast<IObject**>(ppSBT));
                           });
    }

    template <typename... ExtraArgsType>
    void CreatePipelineResourceSignatureImpl(IPipelineResourceSignature** ppSignature, const PipelineResourceSignatureDesc& Desc, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("PipelineResourceSignature", Desc, ppSignature,
                           [&]() //
                           {
                               auto* pPRSImpl = NEW_RC_OBJ(m_PipeResSignAllocator, "PipelineResourceSignature instance", PipelineResourceSignatureImplType)(static_cast<RenderDeviceImplType*>(this), Desc, ExtraArgs...);
                               pPRSImpl->QueryInterface(IID_PipelineResourceSignature, reinterpret_cast<IObject**>(ppSignature));
                           });
    }

    template <typename... ExtraArgsType>
    void CreateDeviceMemoryImpl(IDeviceMemory** ppMemory, const DeviceMemoryCreateInfo& MemCI, const ExtraArgsType&... ExtraArgs)
    {
        CreateDeviceObject("DeviceMemory", MemCI.Desc, ppMemory,
                           [&]() //
                           {
                               auto* pDevMemImpl = NEW_RC_OBJ(m_MemObjAllocator, "DeviceMemory instance", DeviceMemoryImplType)(static_cast<RenderDeviceImplType*>(this), MemCI, ExtraArgs...);
                               pDevMemImpl->QueryInterface(IID_DeviceMemory, reinterpret_cast<IObject**>(ppMemory));
                           });
    }

    void CreatePipelineStateCacheImpl(IPipelineStateCache** ppCache, const PipelineStateCacheCreateInfo& PSOCacheCI)
    {
        CreateDeviceObject("PSOCache", PSOCacheCI.Desc, ppCache,
                           [&]() //
                           {
                               auto* pPSOCacheImpl = NEW_RC_OBJ(m_PSOCacheAllocator, "PSOCache instance", PipelineStateCacheImplType)(static_cast<RenderDeviceImplType*>(this), PSOCacheCI);
                               pPSOCacheImpl->QueryInterface(IID_PipelineStateCache, reinterpret_cast<IObject**>(ppCache));
                           });
    }

protected:
    RefCntAutoPtr<IEngineFactory> m_pEngineFactory;

    const VALIDATION_FLAGS m_ValidationFlags;
    GraphicsAdapterInfo    m_AdapterInfo;
    RenderDeviceInfo       m_DeviceInfo;

    // All state object registries hold raw pointers.
    // This is safe because every object unregisters itself
    // when it is deleted.
    ObjectsRegistry<SamplerDesc, RefCntAutoPtr<ISampler>>                       m_SamplersRegistry; ///< Sampler state registry
    std::vector<TextureFormatInfoExt, STDAllocatorRawMem<TextureFormatInfoExt>> m_TextureFormatsInfo;
    std::vector<bool, STDAllocatorRawMem<bool>>                                 m_TexFmtInfoInitFlags;

    /// Weak references to immediate contexts. Immediate contexts hold strong reference
    /// to the device, so we must use weak references to avoid circular dependencies.
    std::vector<RefCntWeakPtr<DeviceContextImplType>, STDAllocatorRawMem<RefCntWeakPtr<DeviceContextImplType>>> m_wpImmediateContexts;

    /// Weak references to deferred contexts.
    std::vector<RefCntWeakPtr<DeviceContextImplType>, STDAllocatorRawMem<RefCntWeakPtr<DeviceContextImplType>>> m_wpDeferredContexts;

    IMemoryAllocator&         m_RawMemAllocator;      ///< Raw memory allocator
    FixedBlockMemoryAllocator m_TexObjAllocator;      ///< Allocator for texture objects
    FixedBlockMemoryAllocator m_TexViewObjAllocator;  ///< Allocator for texture view objects
    FixedBlockMemoryAllocator m_BufObjAllocator;      ///< Allocator for buffer objects
    FixedBlockMemoryAllocator m_BuffViewObjAllocator; ///< Allocator for buffer view objects
    FixedBlockMemoryAllocator m_ShaderObjAllocator;   ///< Allocator for shader objects
    FixedBlockMemoryAllocator m_SamplerObjAllocator;  ///< Allocator for sampler objects
    FixedBlockMemoryAllocator m_PSOAllocator;         ///< Allocator for pipeline state objects
    FixedBlockMemoryAllocator m_SRBAllocator;         ///< Allocator for shader resource binding objects
    FixedBlockMemoryAllocator m_ResMappingAllocator;  ///< Allocator for resource mapping objects
    FixedBlockMemoryAllocator m_FenceAllocator;       ///< Allocator for fence objects
    FixedBlockMemoryAllocator m_QueryAllocator;       ///< Allocator for query objects
    FixedBlockMemoryAllocator m_RenderPassAllocator;  ///< Allocator for render pass objects
    FixedBlockMemoryAllocator m_FramebufferAllocator; ///< Allocator for framebuffer objects
    FixedBlockMemoryAllocator m_BLASAllocator;        ///< Allocator for bottom-level acceleration structure objects
    FixedBlockMemoryAllocator m_TLASAllocator;        ///< Allocator for top-level acceleration structure objects
    FixedBlockMemoryAllocator m_SBTAllocator;         ///< Allocator for shader binding table objects
    FixedBlockMemoryAllocator m_PipeResSignAllocator; ///< Allocator for pipeline resource signature objects
    FixedBlockMemoryAllocator m_MemObjAllocator;      ///< Allocator for device memory objects
    FixedBlockMemoryAllocator m_PSOCacheAllocator;    ///< Allocator for pipeline state cache objects

    RefCntAutoPtr<IThreadPool> m_pShaderCompilationThreadPool;

    std::atomic<UniqueIdentifier> m_UniqueId{0};
};

} // namespace Diligent
