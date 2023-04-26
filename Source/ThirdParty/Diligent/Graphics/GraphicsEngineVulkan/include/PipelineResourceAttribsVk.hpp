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
/// Declaration of Diligent::PipelineResourceSignatureVkImpl struct

#include "BasicTypes.h"
#include "ShaderResourceCacheCommon.hpp"
#include "PrivateConstants.h"
#include "DebugUtilities.hpp"
#include "HashUtils.hpp"

namespace Diligent
{

enum class DescriptorType : Uint8
{
    Sampler,
    CombinedImageSampler,
    SeparateImage,
    StorageImage,
    UniformTexelBuffer,
    StorageTexelBuffer,
    StorageTexelBuffer_ReadOnly,
    UniformBuffer,
    UniformBufferDynamic,
    StorageBuffer,
    StorageBuffer_ReadOnly,
    StorageBufferDynamic,
    StorageBufferDynamic_ReadOnly,
    InputAttachment,
    InputAttachment_General,
    AccelerationStructure,
    Count,
    Unknown = 31,
};


// sizeof(PipelineResourceAttribsVk) == 16, x64
struct PipelineResourceAttribsVk
{
private:
    static constexpr Uint32 _BindingIndexBits    = 16;
    static constexpr Uint32 _SamplerIndBits      = 16;
    static constexpr Uint32 _ArraySizeBits       = 25;
    static constexpr Uint32 _DescrTypeBits       = 5;
    static constexpr Uint32 _DescrSetBits        = 1;
    static constexpr Uint32 _SamplerAssignedBits = 1;

    static_assert((_BindingIndexBits + _ArraySizeBits + _SamplerIndBits + _DescrTypeBits + _DescrSetBits + _SamplerAssignedBits) % 32 == 0, "Bits are not optimally packed");

    // clang-format off
    static_assert((1u << _DescrTypeBits)    >  static_cast<Uint32>(DescriptorType::Count), "Not enough bits to store DescriptorType values");
    static_assert((1u << _BindingIndexBits) >= MAX_RESOURCES_IN_SIGNATURE,                 "Not enough bits to store resource binding index");
    static_assert((1u << _SamplerIndBits)   >= MAX_RESOURCES_IN_SIGNATURE,                 "Not enough bits to store sampler resource index");
    // clang-format on

public:
    static constexpr Uint32 MaxDescriptorSets = (1u << _DescrSetBits);
    static constexpr Uint32 InvalidSamplerInd = (1u << _SamplerIndBits) - 1;

    // clang-format off
    const Uint32  BindingIndex         : _BindingIndexBits;    // Binding in the descriptor set
    const Uint32  SamplerInd           : _SamplerIndBits;      // Index of the assigned sampler in m_Desc.Resources and m_pPipelineResourceAttribsVk
    const Uint32  ArraySize            : _ArraySizeBits;       // Array size
    const Uint32  DescrType            : _DescrTypeBits;       // Descriptor type (DescriptorType)
    const Uint32  DescrSet             : _DescrSetBits;        // Descriptor set (0 or 1)
    const Uint32  ImtblSamplerAssigned : _SamplerAssignedBits; // Immutable sampler flag

    const Uint32  SRBCacheOffset;                              // Offset in the SRB resource cache
    const Uint32  StaticCacheOffset;                           // Offset in the static resource cache
    // clang-format on

    PipelineResourceAttribsVk(Uint32         _BindingIndex,
                              Uint32         _SamplerInd,
                              Uint32         _ArraySize,
                              DescriptorType _DescrType,
                              Uint32         _DescrSet,
                              bool           _ImtblSamplerAssigned,
                              Uint32         _SRBCacheOffset,
                              Uint32         _StaticCacheOffset) noexcept :
        // clang-format off
        BindingIndex         {_BindingIndex                  },
        SamplerInd           {_SamplerInd                    },
        ArraySize            {_ArraySize                     },
        DescrType            {static_cast<Uint32>(_DescrType)},
        DescrSet             {_DescrSet                      },
        ImtblSamplerAssigned {_ImtblSamplerAssigned ? 1u : 0u},
        SRBCacheOffset       {_SRBCacheOffset                },
        StaticCacheOffset    {_StaticCacheOffset             }
    // clang-format on
    {
        // clang-format off
        VERIFY(BindingIndex        == _BindingIndex, "Binding index (", _BindingIndex, ") exceeds maximum representable value");
        VERIFY(ArraySize           == _ArraySize,    "Array size (", _ArraySize, ") exceeds maximum representable value");
        VERIFY(SamplerInd          == _SamplerInd,   "Sampler index (", _SamplerInd, ") exceeds maximum representable value");
        VERIFY(GetDescriptorType() == _DescrType,    "Descriptor type (", static_cast<Uint32>(_DescrType), ") exceeds maximum representable value");
        VERIFY(DescrSet            == _DescrSet,     "Descriptor set (", _DescrSet, ") exceeds maximum representable value");
        // clang-format on
    }

    // Only for serialization
    PipelineResourceAttribsVk() noexcept :
        PipelineResourceAttribsVk{0, 0, 0, DescriptorType::Unknown, 0, false, 0, 0}
    {}


    Uint32 CacheOffset(ResourceCacheContentType CacheType) const
    {
        return CacheType == ResourceCacheContentType::SRB ? SRBCacheOffset : StaticCacheOffset;
    }

    DescriptorType GetDescriptorType() const
    {
        return static_cast<DescriptorType>(DescrType);
    }

    bool IsImmutableSamplerAssigned() const
    {
        return ImtblSamplerAssigned != 0;
    }

    bool IsCombinedWithSampler() const
    {
        return SamplerInd != InvalidSamplerInd;
    }

    bool IsCompatibleWith(const PipelineResourceAttribsVk& rhs) const
    {
        // Ignore sampler index and cache offsets.
        // clang-format off
        return BindingIndex         == rhs.BindingIndex &&
               ArraySize            == rhs.ArraySize    &&
               DescrType            == rhs.DescrType    &&
               DescrSet             == rhs.DescrSet     &&
               ImtblSamplerAssigned == rhs.ImtblSamplerAssigned;
        // clang-format on
    }

    size_t GetHash() const
    {
        return ComputeHash(BindingIndex, ArraySize, DescrType, DescrSet, ImtblSamplerAssigned);
    }
};
ASSERT_SIZEOF(PipelineResourceAttribsVk, 16, "The struct is used in serialization and must be tightly packed");

inline VkDescriptorType DescriptorTypeToVkDescriptorType(DescriptorType Type)
{
    static_assert(static_cast<Uint32>(DescriptorType::Count) == 16, "Please update the switch below to handle the new descriptor type");
    switch (Type)
    {
        // clang-format off
        case DescriptorType::Sampler:                       return VK_DESCRIPTOR_TYPE_SAMPLER;
        case DescriptorType::CombinedImageSampler:          return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DescriptorType::SeparateImage:                 return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case DescriptorType::StorageImage:                  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case DescriptorType::UniformTexelBuffer:            return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case DescriptorType::StorageTexelBuffer:            return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case DescriptorType::StorageTexelBuffer_ReadOnly:   return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case DescriptorType::UniformBuffer:                 return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescriptorType::UniformBufferDynamic:          return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case DescriptorType::StorageBuffer:                 return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescriptorType::StorageBuffer_ReadOnly:        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescriptorType::StorageBufferDynamic:          return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case DescriptorType::StorageBufferDynamic_ReadOnly: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case DescriptorType::InputAttachment:               return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case DescriptorType::InputAttachment_General:       return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case DescriptorType::AccelerationStructure:         return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        // clang-format on
        default:
            UNEXPECTED("Unknown descriptor type");
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}


} // namespace Diligent
