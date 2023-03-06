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

/// \file
/// Declaration of Diligent::SPIRVShaderResources class

// SPIRVShaderResources class uses continuous chunk of memory to store all resources, as follows:
//
//   m_MemoryBuffer                                                                                                              m_TotalResources
//    |                                                                                                                             |                                       |
//    | Uniform Buffers | Storage Buffers | Storage Images | Sampled Images | Atomic Counters | Separate Samplers | Separate Images |   Stage Inputs   |   Resource Names   |

#include <memory>
#include <vector>
#include <sstream>
#include <array>

#include "Shader.h"
#include "PipelineResourceSignature.h"
#include "STDAllocator.hpp"
#include "RefCntAutoPtr.hpp"
#include "StringPool.hpp"

#ifdef DILIGENT_SPIRV_CROSS_NAMESPACE
#    define diligent_spirv_cross DILIGENT_SPIRV_CROSS_NAMESPACE
#else
#    define diligent_spirv_cross spirv_cross
#endif

namespace diligent_spirv_cross
{
class Compiler;
struct Resource;
} // namespace diligent_spirv_cross

namespace Diligent
{

// sizeof(SPIRVShaderResourceAttribs) == 32, msvc x64
struct SPIRVShaderResourceAttribs
{
    enum ResourceType : Uint8
    {
        UniformBuffer = 0,
        ROStorageBuffer,
        RWStorageBuffer,
        UniformTexelBuffer,
        StorageTexelBuffer,
        StorageImage,
        SampledImage,
        AtomicCounter,
        SeparateImage,
        SeparateSampler,
        InputAttachment,
        AccelerationStructure,
        NumResourceTypes
    };

    static SHADER_RESOURCE_TYPE    GetShaderResourceType(ResourceType Type);
    static PIPELINE_RESOURCE_FLAGS GetPipelineResourceFlags(ResourceType Type);

    // clang-format off

/*  0  */const char* const      Name;
/*  8  */const Uint16           ArraySize;
/* 10  */const ResourceType     Type;
/* 11.0*/const Uint8            ResourceDim   : 7;
/* 11.7*/const Uint8            IsMS          : 1;

      // Offset in SPIRV words (uint32_t) of binding & descriptor set decorations in SPIRV binary
/* 12 */const uint32_t          BindingDecorationOffset;
/* 16 */const uint32_t          DescriptorSetDecorationOffset;

/* 20 */const Uint32            BufferStaticSize;
/* 24 */const Uint32            BufferStride;
/* 28 */
/* 32 */ // End of structure

    // clang-format on

    SPIRVShaderResourceAttribs(const diligent_spirv_cross::Compiler& Compiler,
                               const diligent_spirv_cross::Resource& Res,
                               const char*                           _Name,
                               ResourceType                          _Type,
                               Uint32                                _BufferStaticSize = 0,
                               Uint32                                _BufferStride     = 0) noexcept;

    ShaderResourceDesc GetResourceDesc() const
    {
        return ShaderResourceDesc{Name, GetShaderResourceType(Type), ArraySize};
    }

    RESOURCE_DIMENSION GetResourceDimension() const
    {
        return static_cast<RESOURCE_DIMENSION>(ResourceDim);
    }

    bool IsMultisample() const
    {
        return IsMS != 0;
    }
};
static_assert(sizeof(SPIRVShaderResourceAttribs) % sizeof(void*) == 0, "Size of SPIRVShaderResourceAttribs struct must be multiple of sizeof(void*)");

// sizeof(SPIRVShaderResourceAttribs) == 16, msvc x64
struct SPIRVShaderStageInputAttribs
{
    // clang-format off
    SPIRVShaderStageInputAttribs(const char* _Semantic, uint32_t _LocationDecorationOffset) :
        Semantic                 {_Semantic},
        LocationDecorationOffset {_LocationDecorationOffset}
    {}
    // clang-format on

    const char* const Semantic;
    const uint32_t    LocationDecorationOffset;
};
static_assert(sizeof(SPIRVShaderStageInputAttribs) % sizeof(void*) == 0, "Size of SPIRVShaderStageInputAttribs struct must be multiple of sizeof(void*)");

/// Diligent::SPIRVShaderResources class
class SPIRVShaderResources
{
public:
    SPIRVShaderResources(IMemoryAllocator&     Allocator,
                         std::vector<uint32_t> spirv_binary,
                         const ShaderDesc&     shaderDesc,
                         const char*           CombinedSamplerSuffix,
                         bool                  LoadShaderStageInputs,
                         bool                  LoadUniformBufferReflection,
                         std::string&          EntryPoint);

    // clang-format off
    SPIRVShaderResources             (const SPIRVShaderResources&)  = delete;
    SPIRVShaderResources             (      SPIRVShaderResources&&) = delete;
    SPIRVShaderResources& operator = (const SPIRVShaderResources&)  = delete;
    SPIRVShaderResources& operator = (      SPIRVShaderResources&&) = delete;
    // clang-format on

    ~SPIRVShaderResources();

    // clang-format off

    Uint32 GetNumUBs         ()const noexcept{ return (m_StorageBufferOffset   - 0);                      }
    Uint32 GetNumSBs         ()const noexcept{ return (m_StorageImageOffset    - m_StorageBufferOffset);  }
    Uint32 GetNumImgs        ()const noexcept{ return (m_SampledImageOffset    - m_StorageImageOffset);   }
    Uint32 GetNumSmpldImgs   ()const noexcept{ return (m_AtomicCounterOffset   - m_SampledImageOffset);   }
    Uint32 GetNumACs         ()const noexcept{ return (m_SeparateSamplerOffset - m_AtomicCounterOffset);  }
    Uint32 GetNumSepSmplrs   ()const noexcept{ return (m_SeparateImageOffset   - m_SeparateSamplerOffset);}
    Uint32 GetNumSepImgs     ()const noexcept{ return (m_InputAttachmentOffset - m_SeparateImageOffset);  }
    Uint32 GetNumInptAtts    ()const noexcept{ return (m_AccelStructOffset     - m_InputAttachmentOffset);}
    Uint32 GetNumAccelStructs()const noexcept{ return (m_TotalResources        - m_AccelStructOffset);    }
    Uint32 GetTotalResources ()    const noexcept { return m_TotalResources; }
    Uint32 GetNumShaderStageInputs()const noexcept { return m_NumShaderStageInputs; }

    const SPIRVShaderResourceAttribs& GetUB         (Uint32 n)const noexcept{ return GetResAttribs(n, GetNumUBs(),          0                      ); }
    const SPIRVShaderResourceAttribs& GetSB         (Uint32 n)const noexcept{ return GetResAttribs(n, GetNumSBs(),          m_StorageBufferOffset  ); }
    const SPIRVShaderResourceAttribs& GetImg        (Uint32 n)const noexcept{ return GetResAttribs(n, GetNumImgs(),         m_StorageImageOffset   ); }
    const SPIRVShaderResourceAttribs& GetSmpldImg   (Uint32 n)const noexcept{ return GetResAttribs(n, GetNumSmpldImgs(),    m_SampledImageOffset   ); }
    const SPIRVShaderResourceAttribs& GetAC         (Uint32 n)const noexcept{ return GetResAttribs(n, GetNumACs(),          m_AtomicCounterOffset  ); }
    const SPIRVShaderResourceAttribs& GetSepSmplr   (Uint32 n)const noexcept{ return GetResAttribs(n, GetNumSepSmplrs(),    m_SeparateSamplerOffset); }
    const SPIRVShaderResourceAttribs& GetSepImg     (Uint32 n)const noexcept{ return GetResAttribs(n, GetNumSepImgs(),      m_SeparateImageOffset  ); }
    const SPIRVShaderResourceAttribs& GetInptAtt    (Uint32 n)const noexcept{ return GetResAttribs(n, GetNumInptAtts(),     m_InputAttachmentOffset); }
    const SPIRVShaderResourceAttribs& GetAccelStruct(Uint32 n)const noexcept{ return GetResAttribs(n, GetNumAccelStructs(), m_AccelStructOffset    ); }
    const SPIRVShaderResourceAttribs& GetResource   (Uint32 n)const noexcept{ return GetResAttribs(n, GetTotalResources(),  0                      ); }

    // clang-format on

    const SPIRVShaderStageInputAttribs& GetShaderStageInputAttribs(Uint32 n) const noexcept
    {
        VERIFY(n < m_NumShaderStageInputs, "Shader stage input index (", n, ") is out of range. Total input count: ", m_NumShaderStageInputs);
        auto* ResourceMemoryEnd = reinterpret_cast<const SPIRVShaderResourceAttribs*>(m_MemoryBuffer.get()) + m_TotalResources;
        return reinterpret_cast<const SPIRVShaderStageInputAttribs*>(ResourceMemoryEnd)[n];
    }

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
        Uint32 NumUBs          = 0;
        Uint32 NumSBs          = 0;
        Uint32 NumImgs         = 0;
        Uint32 NumSmpldImgs    = 0;
        Uint32 NumACs          = 0;
        Uint32 NumSepSmplrs    = 0;
        Uint32 NumSepImgs      = 0;
        Uint32 NumInptAtts     = 0;
        Uint32 NumAccelStructs = 0;
    };

    SHADER_TYPE GetShaderType() const noexcept { return m_ShaderType; }

    const std::array<Uint32, 3>& GetComputeGroupSize() const
    {
        return m_ComputeGroupSize;
    }

    // Process only resources listed in AllowedVarTypes
    template <typename THandleUB,
              typename THandleSB,
              typename THandleImg,
              typename THandleSmplImg,
              typename THandleAC,
              typename THandleSepSmpl,
              typename THandleSepImg,
              typename THandleInptAtt,
              typename THandleAccelStruct>
    void ProcessResources(THandleUB          HandleUB,
                          THandleSB          HandleSB,
                          THandleImg         HandleImg,
                          THandleSmplImg     HandleSmplImg,
                          THandleAC          HandleAC,
                          THandleSepSmpl     HandleSepSmpl,
                          THandleSepImg      HandleSepImg,
                          THandleInptAtt     HandleInptAtt,
                          THandleAccelStruct HandleAccelStruct) const
    {
        for (Uint32 n = 0; n < GetNumUBs(); ++n)
        {
            const auto& UB = GetUB(n);
            HandleUB(UB, n);
        }

        for (Uint32 n = 0; n < GetNumSBs(); ++n)
        {
            const auto& SB = GetSB(n);
            HandleSB(SB, n);
        }

        for (Uint32 n = 0; n < GetNumImgs(); ++n)
        {
            const auto& Img = GetImg(n);
            HandleImg(Img, n);
        }

        for (Uint32 n = 0; n < GetNumSmpldImgs(); ++n)
        {
            const auto& SmplImg = GetSmpldImg(n);
            HandleSmplImg(SmplImg, n);
        }

        for (Uint32 n = 0; n < GetNumACs(); ++n)
        {
            const auto& AC = GetAC(n);
            HandleAC(AC, n);
        }

        for (Uint32 n = 0; n < GetNumSepSmplrs(); ++n)
        {
            const auto& SepSmpl = GetSepSmplr(n);
            HandleSepSmpl(SepSmpl, n);
        }

        for (Uint32 n = 0; n < GetNumSepImgs(); ++n)
        {
            const auto& SepImg = GetSepImg(n);
            HandleSepImg(SepImg, n);
        }

        for (Uint32 n = 0; n < GetNumInptAtts(); ++n)
        {
            const auto& InptAtt = GetInptAtt(n);
            HandleInptAtt(InptAtt, n);
        }

        for (Uint32 n = 0; n < GetNumAccelStructs(); ++n)
        {
            const auto& AccelStruct = GetAccelStruct(n);
            HandleAccelStruct(AccelStruct, n);
        }

        static_assert(Uint32{SPIRVShaderResourceAttribs::ResourceType::NumResourceTypes} == 12, "Please handle the new resource type here, if needed");
    }

    template <typename THandler>
    void ProcessResources(THandler&& Handler) const
    {
        for (Uint32 n = 0; n < GetTotalResources(); ++n)
        {
            const auto& Res = GetResource(n);
            Handler(Res, n);
        }
    }

    std::string DumpResources();

    // clang-format off

    const char* GetCombinedSamplerSuffix() const { return m_CombinedSamplerSuffix; }
    const char* GetShaderName()            const { return m_ShaderName; }
    bool        IsUsingCombinedSamplers()  const { return m_CombinedSamplerSuffix != nullptr; }

    // clang-format on

    bool IsHLSLSource() const { return m_IsHLSLSource; }

private:
    void Initialize(IMemoryAllocator&       Allocator,
                    const ResourceCounters& Counters,
                    Uint32                  NumShaderStageInputs,
                    size_t                  ResourceNamesPoolSize,
                    StringPool&             ResourceNamesPool);

    SPIRVShaderResourceAttribs& GetResAttribs(Uint32 n, Uint32 NumResources, Uint32 Offset) noexcept
    {
        VERIFY(n < NumResources, "Resource index (", n, ") is out of range. Total resource count: ", NumResources);
        VERIFY_EXPR(Offset + n < m_TotalResources);
        return reinterpret_cast<SPIRVShaderResourceAttribs*>(m_MemoryBuffer.get())[Offset + n];
    }

    const SPIRVShaderResourceAttribs& GetResAttribs(Uint32 n, Uint32 NumResources, Uint32 Offset) const noexcept
    {
        VERIFY(n < NumResources, "Resource index (", n, ") is out of range. Total resource count: ", NumResources);
        VERIFY_EXPR(Offset + n < m_TotalResources);
        return reinterpret_cast<SPIRVShaderResourceAttribs*>(m_MemoryBuffer.get())[Offset + n];
    }

    // clang-format off

    SPIRVShaderResourceAttribs& GetUB         (Uint32 n)noexcept{ return GetResAttribs(n, GetNumUBs(),          0                      ); }
    SPIRVShaderResourceAttribs& GetSB         (Uint32 n)noexcept{ return GetResAttribs(n, GetNumSBs(),          m_StorageBufferOffset  ); }
    SPIRVShaderResourceAttribs& GetImg        (Uint32 n)noexcept{ return GetResAttribs(n, GetNumImgs(),         m_StorageImageOffset   ); }
    SPIRVShaderResourceAttribs& GetSmpldImg   (Uint32 n)noexcept{ return GetResAttribs(n, GetNumSmpldImgs(),    m_SampledImageOffset   ); }
    SPIRVShaderResourceAttribs& GetAC         (Uint32 n)noexcept{ return GetResAttribs(n, GetNumACs(),          m_AtomicCounterOffset  ); }
    SPIRVShaderResourceAttribs& GetSepSmplr   (Uint32 n)noexcept{ return GetResAttribs(n, GetNumSepSmplrs(),    m_SeparateSamplerOffset); }
    SPIRVShaderResourceAttribs& GetSepImg     (Uint32 n)noexcept{ return GetResAttribs(n, GetNumSepImgs(),      m_SeparateImageOffset  ); }
    SPIRVShaderResourceAttribs& GetInptAtt    (Uint32 n)noexcept{ return GetResAttribs(n, GetNumInptAtts(),     m_InputAttachmentOffset); }
    SPIRVShaderResourceAttribs& GetAccelStruct(Uint32 n)noexcept{ return GetResAttribs(n, GetNumAccelStructs(), m_AccelStructOffset    ); }
    SPIRVShaderResourceAttribs& GetResource   (Uint32 n)noexcept{ return GetResAttribs(n, GetTotalResources(),  0                      ); }

    // clang-format on

    SPIRVShaderStageInputAttribs& GetShaderStageInputAttribs(Uint32 n) noexcept
    {
        return const_cast<SPIRVShaderStageInputAttribs&>(const_cast<const SPIRVShaderResources*>(this)->GetShaderStageInputAttribs(n));
    }

    // Memory buffer that holds all resources as continuous chunk of memory:
    // |  UBs  |  SBs  |  StrgImgs  |  SmplImgs  |  ACs  |  SepSamplers  |  SepImgs  | Stage Inputs | Resource Names |
    std::unique_ptr<void, STDDeleterRawMem<void>> m_MemoryBuffer;
    std::unique_ptr<void, STDDeleterRawMem<void>> m_UBReflectionBuffer;

    const char* m_CombinedSamplerSuffix = nullptr;
    const char* m_ShaderName            = nullptr;

    using OffsetType                   = Uint16;
    OffsetType m_StorageBufferOffset   = 0;
    OffsetType m_StorageImageOffset    = 0;
    OffsetType m_SampledImageOffset    = 0;
    OffsetType m_AtomicCounterOffset   = 0;
    OffsetType m_SeparateSamplerOffset = 0;
    OffsetType m_SeparateImageOffset   = 0;
    OffsetType m_InputAttachmentOffset = 0;
    OffsetType m_AccelStructOffset     = 0;
    OffsetType m_TotalResources        = 0;
    OffsetType m_NumShaderStageInputs  = 0;

    SHADER_TYPE m_ShaderType = SHADER_TYPE_UNKNOWN;

    std::array<Uint32, 3> m_ComputeGroupSize = {};

    // Indicates if the shader was compiled from HLSL source.
    bool m_IsHLSLSource = false;
};

} // namespace Diligent
