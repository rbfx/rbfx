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

#pragma once
/// \file
/// Declaration of Diligent::PipelineResourceAttribsWebGPU struct

#include "HashUtils.hpp"
#include "ShaderResourceCacheCommon.hpp"
#include "PrivateConstants.h"

namespace Diligent
{

enum class BindGroupEntryType : Uint8
{
    UniformBuffer,
    UniformBufferDynamic,
    StorageBuffer,
    StorageBufferDynamic,
    StorageBuffer_ReadOnly,
    StorageBufferDynamic_ReadOnly,
    Texture,
    StorageTexture_WriteOnly,
    StorageTexture_ReadOnly,
    StorageTexture_ReadWrite,
    ExternalTexture,
    Sampler,
    Count
};

struct PipelineResourceAttribsWebGPU
{
private:
    static constexpr Uint32 _BindingIndexBits    = 16;
    static constexpr Uint32 _SamplerIndBits      = 16;
    static constexpr Uint32 _ArraySizeBits       = 25;
    static constexpr Uint32 _EntryTypeBits       = 5;
    static constexpr Uint32 _BindGroupBits       = 1;
    static constexpr Uint32 _SamplerAssignedBits = 1;

    static_assert((_BindingIndexBits + _ArraySizeBits + _SamplerIndBits + _EntryTypeBits + _BindGroupBits + _SamplerAssignedBits) % 32 == 0, "Bits are not optimally packed");

    // clang-format off
    static_assert((1u << _EntryTypeBits)    >  static_cast<Uint32>(BindGroupEntryType::Count), "Not enough bits to store EntryType values");
    static_assert((1u << _BindingIndexBits) >= MAX_RESOURCES_IN_SIGNATURE,                     "Not enough bits to store resource binding index");
    static_assert((1u << _SamplerIndBits)   >= MAX_RESOURCES_IN_SIGNATURE,                     "Not enough bits to store sampler resource index");
    // clang-format on

public:
    static constexpr Uint32 MaxBindGroups     = (1u << _BindGroupBits);
    static constexpr Uint32 InvalidSamplerInd = (1u << _SamplerIndBits) - 1;

    // clang-format off
    const Uint32  BindingIndex         : _BindingIndexBits;    // Binding in the descriptor set
    const Uint32  SamplerInd           : _SamplerIndBits;      // Index of the assigned sampler in m_Desc.Resources and m_pPipelineResourceAttribsVk
    const Uint32  ArraySize            : _ArraySizeBits;       // Array size
    const Uint32  EntryType            : _EntryTypeBits;       // Bind group entry type (BindGroupEntryType)
    const Uint32  BindGroup            : _BindGroupBits;       // Bind group (0 or 1)
    const Uint32  ImtblSamplerAssigned : _SamplerAssignedBits; // Immutable sampler flag

    const Uint32  SRBCacheOffset;                              // Offset in the SRB resource cache
    const Uint32  StaticCacheOffset;                           // Offset in the static resource cache
    // clang-format on

    PipelineResourceAttribsWebGPU(Uint32             _BindingIndex,
                                  Uint32             _SamplerInd,
                                  Uint32             _ArraySize,
                                  BindGroupEntryType _EntryType,
                                  Uint32             _BindGroup,
                                  bool               _ImtblSamplerAssigned,
                                  Uint32             _SRBCacheOffset,
                                  Uint32             _StaticCacheOffset) noexcept :
        // clang-format off
        BindingIndex         {_BindingIndex                  },
        SamplerInd           {_SamplerInd                    },
        ArraySize            {_ArraySize                     },
        EntryType            {static_cast<Uint32>(_EntryType)},
        BindGroup            {_BindGroup                      },
        ImtblSamplerAssigned {_ImtblSamplerAssigned ? 1u : 0u},
        SRBCacheOffset       {_SRBCacheOffset                },
        StaticCacheOffset    {_StaticCacheOffset             }
    // clang-format on
    {
        // clang-format off
        VERIFY(BindingIndex            == _BindingIndex, "Binding index (", _BindingIndex, ") exceeds maximum representable value");
        VERIFY(ArraySize               == _ArraySize,    "Array size (", _ArraySize, ") exceeds maximum representable value");
        VERIFY(SamplerInd              == _SamplerInd,   "Sampler index (", _SamplerInd, ") exceeds maximum representable value");
        VERIFY(GetBindGroupEntryType() == _EntryType,    "Bind group entry type (", static_cast<Uint32>(_EntryType), ") exceeds maximum representable value");
        VERIFY(BindGroup               == _BindGroup,    "Bind group (", _BindGroup, ") exceeds maximum representable value");
        // clang-format on
    }

    // Only for serialization
    PipelineResourceAttribsWebGPU() noexcept :
        PipelineResourceAttribsWebGPU{0, 0, 0, BindGroupEntryType::Count, 0, false, 0, 0}
    {}


    Uint32 CacheOffset(ResourceCacheContentType CacheType) const
    {
        return CacheType == ResourceCacheContentType::SRB ? SRBCacheOffset : StaticCacheOffset;
    }

    BindGroupEntryType GetBindGroupEntryType() const
    {
        return static_cast<BindGroupEntryType>(EntryType);
    }

    bool IsImmutableSamplerAssigned() const
    {
        return ImtblSamplerAssigned != 0;
    }

    bool IsCombinedWithSampler() const
    {
        return SamplerInd != InvalidSamplerInd;
    }

    bool IsCompatibleWith(const PipelineResourceAttribsWebGPU& rhs) const
    {
        // Ignore sampler index and cache offsets.
        // clang-format off
        return BindingIndex         == rhs.BindingIndex &&
               ArraySize            == rhs.ArraySize    &&
               EntryType            == rhs.EntryType    &&
               BindGroup            == rhs.BindGroup     &&
               ImtblSamplerAssigned == rhs.ImtblSamplerAssigned;
        // clang-format on
    }

    size_t GetHash() const
    {
        return ComputeHash(BindingIndex, ArraySize, EntryType, BindGroup, ImtblSamplerAssigned);
    }
};
ASSERT_SIZEOF(PipelineResourceAttribsWebGPU, 16, "The struct is used in serialization and must be tightly packed");

} // namespace Diligent
