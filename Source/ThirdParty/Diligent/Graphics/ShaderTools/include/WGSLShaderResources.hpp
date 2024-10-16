/*
 *  Copyright 2024 Diligent Graphics LLC
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
/// Declaration of Diligent::WGSLShaderResources class

// WGSLShaderResources class uses continuous chunk of memory to store all resources, as follows:
//
//   m_MemoryBuffer
//    |                                                                                                                                                                    |
//    | Uniform Buffers | Storage Buffers | Textures | Storage Textures | Samplers | Ext Textures |   Resource Names   |

#include <memory>
#include <string>

#include "Shader.h"
#include "PipelineState.h"
#include "PipelineResourceSignature.h"
#include "DebugUtilities.hpp"
#include "STDAllocator.hpp"

namespace tint
{
namespace inspector
{
struct ResourceBinding;
} // namespace inspector
} // namespace tint

namespace Diligent
{

class StringPool;

struct WGSLShaderResourceAttribs
{
    enum ResourceType : Uint8
    {
        UniformBuffer,
        ROStorageBuffer,
        RWStorageBuffer,
        Sampler,
        ComparisonSampler,
        Texture,
        TextureMS,
        DepthTexture,
        DepthTextureMS,
        WOStorageTexture,
        ROStorageTexture,
        RWStorageTexture,
        ExternalTexture,
        NumResourceTypes
    };

    enum TextureSampleType : Uint8
    {
        Unknown,
        Float,
        UInt,
        SInt,
        UnfilterableFloat,
        Depth
    };

    static SHADER_RESOURCE_TYPE    GetShaderResourceType(ResourceType Type);
    static PIPELINE_RESOURCE_FLAGS GetPipelineResourceFlags(ResourceType Type);

    // clang-format off

/*  0  */const char* const      Name;
/*  8  */const Uint16           ArraySize;
/* 10  */const ResourceType     Type;
/* 11  */const Uint8            ResourceDim; // RESOURCE_DIMENSION

/* 12 */const TEXTURE_FORMAT    Format;      // Storage texture format
/* 14 */const uint16_t          BindGroup;
/* 16 */const uint16_t          BindIndex;
/* 18 */const TextureSampleType SampleType;
/* 19 */

/* 20 */const Uint32            BufferStaticSize;
/* 24 */ // End of structure

    // clang-format on

    WGSLShaderResourceAttribs(const char*                             _Name,
                              const tint::inspector::ResourceBinding& TintBinding,
                              Uint32                                  _ArraySize) noexcept;

    WGSLShaderResourceAttribs(const char*        _Name,
                              ResourceType       _Type,
                              Uint16             _ArraySize        = 1,
                              RESOURCE_DIMENSION _ResourceDim      = RESOURCE_DIM_UNDEFINED,
                              TEXTURE_FORMAT     _Format           = TEX_FORMAT_UNKNOWN,
                              TextureSampleType  _SampleType       = TextureSampleType::Unknown,
                              uint16_t           _BindGroup        = 0,
                              uint16_t           _BindIndex        = 0,
                              Uint32             _BufferStaticSize = 0) noexcept;

    ShaderResourceDesc GetResourceDesc() const
    {
        return ShaderResourceDesc{Name, GetShaderResourceType(Type), ArraySize};
    }

    RESOURCE_DIMENSION GetResourceDimension() const
    {
        return static_cast<RESOURCE_DIMENSION>(ResourceDim);
    }

    WebGPUResourceAttribs GetWebGPUAttribs(SHADER_VARIABLE_FLAGS Flags) const;

    bool IsMultisample() const
    {
        return Type == TextureMS || Type == DepthTextureMS;
    }
};
static_assert(sizeof(WGSLShaderResourceAttribs) % sizeof(void*) == 0, "Size of WGSLShaderResourceAttribs struct must be a multiple of sizeof(void*)");


/// Diligent::WGSLShaderResources class
class WGSLShaderResources
{
public:
    WGSLShaderResources(IMemoryAllocator&      Allocator,
                        const std::string&     WGSL,
                        SHADER_SOURCE_LANGUAGE SourceLanguage,
                        const char*            ShaderName,
                        const char*            CombinedSamplerSuffix,
                        const char*            EntryPoint,
                        const char*            EmulatedArrayIndexSuffix,
                        bool                   LoadUniformBufferReflection,
                        IDataBlob**            ppTintOutput) noexcept(false);

    // clang-format off
    WGSLShaderResources             (const WGSLShaderResources&)  = delete;
    WGSLShaderResources             (      WGSLShaderResources&&) = delete;
    WGSLShaderResources& operator = (const WGSLShaderResources&)  = delete;
    WGSLShaderResources& operator = (      WGSLShaderResources&&) = delete;
    // clang-format on

    ~WGSLShaderResources();

    // clang-format off

    Uint32 GetNumUBs         ()const noexcept{ return (m_StorageBufferOffset   - 0);                      }
    Uint32 GetNumSBs         ()const noexcept{ return (m_TextureOffset         - m_StorageBufferOffset);  }
    Uint32 GetNumTextures    ()const noexcept{ return (m_StorageTextureOffset  - m_TextureOffset);        }
    Uint32 GetNumStTextures  ()const noexcept{ return (m_SamplerOffset         - m_StorageTextureOffset); }
    Uint32 GetNumSamplers    ()const noexcept{ return (m_ExternalTextureOffset - m_SamplerOffset);        }
    Uint32 GetNumExtTextures ()const noexcept{ return (m_TotalResources        - m_ExternalTextureOffset);}
    Uint32 GetTotalResources ()const noexcept{ return m_TotalResources; }

    const WGSLShaderResourceAttribs& GetUB        (Uint32 n) const noexcept { return GetResAttribs(n, GetNumUBs(),          0                      ); }
    const WGSLShaderResourceAttribs& GetSB        (Uint32 n) const noexcept { return GetResAttribs(n, GetNumSBs(),          m_StorageBufferOffset  ); }
    const WGSLShaderResourceAttribs& GetTexture   (Uint32 n) const noexcept { return GetResAttribs(n, GetNumTextures(),     m_TextureOffset        ); }
    const WGSLShaderResourceAttribs& GetStTexture (Uint32 n) const noexcept { return GetResAttribs(n, GetNumStTextures(),   m_StorageTextureOffset ); }
    const WGSLShaderResourceAttribs& GetSampler   (Uint32 n) const noexcept { return GetResAttribs(n, GetNumSamplers(),     m_SamplerOffset        ); }
    const WGSLShaderResourceAttribs& GetExtTexture(Uint32 n) const noexcept { return GetResAttribs(n, GetNumExtTextures(),  m_ExternalTextureOffset); }
    const WGSLShaderResourceAttribs& GetResource  (Uint32 n) const noexcept { return GetResAttribs(n, GetTotalResources(),  0                      ); }

    // clang-format on

    const ShaderCodeBufferDesc* GetUniformBufferDesc(Uint32 Index) const
    {
        if (Index >= GetNumUBs())
        {
            UNEXPECTED("Uniform buffer index (", Index, ") is out of range.");
            return nullptr;
        }

        if (!m_UBReflectionBuffer)
        {
            UNEXPECTED("Uniform buffer reflection information is not loaded. Please set the LoadConstantBufferReflection flag when creating the shader.");
            return nullptr;
        }

        return reinterpret_cast<const ShaderCodeBufferDesc*>(m_UBReflectionBuffer.get()) + Index;
    }

    struct ResourceCounters
    {
        Uint32 NumUBs         = 0;
        Uint32 NumSBs         = 0;
        Uint32 NumTextures    = 0;
        Uint32 NumStTextures  = 0;
        Uint32 NumSamplers    = 0;
        Uint32 NumExtTextures = 0;
    };

    SHADER_TYPE GetShaderType() const noexcept { return m_ShaderType; }

    template <typename THandleUB,
              typename THandleSB,
              typename THandleTextures,
              typename THandleStTextures,
              typename THandleSamplers,
              typename THandleExtTextures>
    void ProcessResources(THandleUB          HandleUB,
                          THandleSB          HandleSB,
                          THandleTextures    HandleTexture,
                          THandleStTextures  HandleStTexture,
                          THandleSamplers    HandleSampler,
                          THandleExtTextures HandleExtTexture) const
    {
        for (Uint32 n = 0; n < GetNumUBs(); ++n)
        {
            const WGSLShaderResourceAttribs& UB = GetUB(n);
            HandleUB(UB, n);
        }

        for (Uint32 n = 0; n < GetNumSBs(); ++n)
        {
            const WGSLShaderResourceAttribs& SB = GetSB(n);
            HandleSB(SB, n);
        }

        for (Uint32 n = 0; n < GetNumTextures(); ++n)
        {
            const WGSLShaderResourceAttribs& Tex = GetTexture(n);
            HandleTexture(Tex, n);
        }

        for (Uint32 n = 0; n < GetNumStTextures(); ++n)
        {
            const WGSLShaderResourceAttribs& StTex = GetStTexture(n);
            HandleStTexture(StTex, n);
        }

        for (Uint32 n = 0; n < GetNumSamplers(); ++n)
        {
            const WGSLShaderResourceAttribs& Sam = GetSampler(n);
            HandleSampler(Sam, n);
        }

        for (Uint32 n = 0; n < GetNumExtTextures(); ++n)
        {
            const WGSLShaderResourceAttribs& ExtTex = GetExtTexture(n);
            HandleExtTexture(ExtTex, n);
        }

        static_assert(Uint32{WGSLShaderResourceAttribs::ResourceType::NumResourceTypes} == 13, "Please handle the new resource type here, if needed");
    }

    template <typename THandler>
    void ProcessResources(THandler&& Handler) const
    {
        for (Uint32 n = 0; n < GetTotalResources(); ++n)
        {
            const WGSLShaderResourceAttribs& Res = GetResource(n);
            Handler(Res, n);
        }
    }

    std::string DumpResources();

    // clang-format off

    const char* GetCombinedSamplerSuffix()    const { return m_CombinedSamplerSuffix; }
    const char* GetEmulatedArrayIndexSuffix() const { return m_EmulatedArrayIndexSuffix; }
    const char* GetShaderName()               const { return m_ShaderName; }
    const char* GetEntryPoint()               const { return m_EntryPoint; }
    bool        IsUsingCombinedSamplers()     const { return m_CombinedSamplerSuffix != nullptr; }

    // clang-format on

private:
    void Initialize(IMemoryAllocator&       Allocator,
                    const ResourceCounters& Counters,
                    size_t                  ResourceNamesPoolSize,
                    StringPool&             ResourceNamesPool);

    WGSLShaderResourceAttribs& GetResAttribs(Uint32 n, Uint32 NumResources, Uint32 Offset) noexcept
    {
        VERIFY(n < NumResources, "Resource index (", n, ") is out of range. Total resource count: ", NumResources);
        VERIFY_EXPR(Offset + n < m_TotalResources);
        return reinterpret_cast<WGSLShaderResourceAttribs*>(m_MemoryBuffer.get())[Offset + n];
    }

    const WGSLShaderResourceAttribs& GetResAttribs(Uint32 n, Uint32 NumResources, Uint32 Offset) const noexcept
    {
        VERIFY(n < NumResources, "Resource index (", n, ") is out of range. Total resource count: ", NumResources);
        VERIFY_EXPR(Offset + n < m_TotalResources);
        return reinterpret_cast<WGSLShaderResourceAttribs*>(m_MemoryBuffer.get())[Offset + n];
    }

    // clang-format off

    WGSLShaderResourceAttribs& GetUB        (Uint32 n) noexcept { return GetResAttribs(n, GetNumUBs(),          0                      ); }
    WGSLShaderResourceAttribs& GetSB        (Uint32 n) noexcept { return GetResAttribs(n, GetNumSBs(),          m_StorageBufferOffset  ); }
    WGSLShaderResourceAttribs& GetTexture   (Uint32 n) noexcept { return GetResAttribs(n, GetNumTextures(),     m_TextureOffset        ); }
    WGSLShaderResourceAttribs& GetStTexture (Uint32 n) noexcept { return GetResAttribs(n, GetNumStTextures(),   m_StorageTextureOffset ); }
    WGSLShaderResourceAttribs& GetSampler   (Uint32 n) noexcept { return GetResAttribs(n, GetNumSamplers(),     m_SamplerOffset        ); }
    WGSLShaderResourceAttribs& GetExtTexture(Uint32 n) noexcept { return GetResAttribs(n, GetNumExtTextures(),  m_ExternalTextureOffset); }
    WGSLShaderResourceAttribs& GetResource  (Uint32 n) noexcept { return GetResAttribs(n, GetTotalResources(),  0                      ); }

    // clang-format on

private:
    // Memory buffer that holds all resources as continuous chunk of memory:
    // |  UBs  |  SBs  |  Textures  |  StorageTex  |  Samplers |  ExternalTex | Resource Names |
    std::unique_ptr<void, STDDeleterRawMem<void>> m_MemoryBuffer;
    std::unique_ptr<void, STDDeleterRawMem<void>> m_UBReflectionBuffer;

    const char* m_CombinedSamplerSuffix    = nullptr;
    const char* m_EmulatedArrayIndexSuffix = nullptr;
    const char* m_ShaderName               = nullptr;
    const char* m_EntryPoint               = nullptr;

    using OffsetType                   = Uint16;
    OffsetType m_StorageBufferOffset   = 0;
    OffsetType m_TextureOffset         = 0;
    OffsetType m_StorageTextureOffset  = 0;
    OffsetType m_SamplerOffset         = 0;
    OffsetType m_ExternalTextureOffset = 0;
    OffsetType m_TotalResources        = 0;

    SHADER_TYPE m_ShaderType = SHADER_TYPE_UNKNOWN;
};

} // namespace Diligent
