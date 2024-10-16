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
/// Declaration of Diligent::PipelineResourceAttribsD3D12 struct

#include "BasicTypes.h"
#include "PrivateConstants.h"
#include "ShaderResourceCacheCommon.hpp"
#include "DebugUtilities.hpp"
#include "HashUtils.hpp"

namespace Diligent
{

// sizeof(ResourceAttribs) == 16, x64
struct PipelineResourceAttribsD3D12
{
private:
    static constexpr Uint32 _RegisterBits        = 16;
    static constexpr Uint32 _SRBRootIndexBits    = 16;
    static constexpr Uint32 _SamplerIndBits      = 16;
    static constexpr Uint32 _SpaceBits           = 8;
    static constexpr Uint32 _SigRootIndexBits    = 3;
    static constexpr Uint32 _SamplerAssignedBits = 1;
    static constexpr Uint32 _RootParamTypeBits   = 4;

    // clang-format off
    static_assert((1u << _RegisterBits)      >= MAX_RESOURCES_IN_SIGNATURE,        "Not enough bits to store shader register");
    static_assert((1u << _SamplerIndBits)    >= MAX_RESOURCES_IN_SIGNATURE,        "Not enough bits to store sampler resource index");
    static_assert((1u << _RootParamTypeBits) >  D3D12_ROOT_PARAMETER_TYPE_UAV + 1, "Not enough bits to store D3D12_ROOT_PARAMETER_TYPE");
    // clang-format on

public:
    static constexpr Uint32 InvalidSamplerInd   = (1u << _SamplerIndBits) - 1;
    static constexpr Uint32 InvalidSRBRootIndex = (1u << _SRBRootIndexBits) - 1;
    static constexpr Uint32 InvalidSigRootIndex = (1u << _SigRootIndexBits) - 1;
    static constexpr Uint32 InvalidRegister     = (1u << _RegisterBits) - 1;
    static constexpr Uint32 InvalidOffset       = ~0u;

    // clang-format off
/* 0  */const Uint32  Register             : _RegisterBits;        // Shader register
/* 2  */const Uint32  SRBRootIndex         : _SRBRootIndexBits;    // Root view/table index in the SRB
/* 4  */const Uint32  SamplerInd           : _SamplerIndBits;      // Assigned sampler index in m_Desc.Resources and m_pResourceAttribs
/* 6  */const Uint32  Space                : _SpaceBits;           // Shader register space
/* 7.0*/const Uint32  SigRootIndex         : _SigRootIndexBits;    // Root table index for signature (static resources only)
/* 7.3*/const Uint32  ImtblSamplerAssigned : _SamplerAssignedBits; // Immutable sampler flag for Texture SRVs and Samplers
/* 7.4*/const Uint32  RootParamType        : _RootParamTypeBits;   // Root parameter type (D3D12_ROOT_PARAMETER_TYPE)
/* 8  */const Uint32  SigOffsetFromTableStart;                     // Offset in the root table for signature (static only)
/* 12 */const Uint32  SRBOffsetFromTableStart;                     // Offset in the root table for SRB
/* 16 */
    // clang-format on

    PipelineResourceAttribsD3D12(Uint32                    _Register,
                                 Uint32                    _Space,
                                 Uint32                    _SamplerInd,
                                 Uint32                    _SRBRootIndex,
                                 Uint32                    _SRBOffsetFromTableStart,
                                 Uint32                    _SigRootIndex,
                                 Uint32                    _SigOffsetFromTableStart,
                                 bool                      _ImtblSamplerAssigned,
                                 D3D12_ROOT_PARAMETER_TYPE _RootParamType) noexcept :
        // clang-format off
        Register               {_Register                          },
        SRBRootIndex           {_SRBRootIndex                      },
        SamplerInd             {_SamplerInd                        },
        Space                  {_Space                             },
        SigRootIndex           {_SigRootIndex                      },
        ImtblSamplerAssigned   {_ImtblSamplerAssigned ? 1u : 0u    },
        RootParamType          {static_cast<Uint32>(_RootParamType)},
        SigOffsetFromTableStart{_SigOffsetFromTableStart           },
        SRBOffsetFromTableStart{_SRBOffsetFromTableStart           }
    // clang-format on
    {
        VERIFY(Register == _Register, "Shader register (", _Register, ") exceeds maximum representable value");
        VERIFY(SRBRootIndex == _SRBRootIndex, "SRB Root index (", _SRBRootIndex, ") exceeds maximum representable value");
        VERIFY(SigRootIndex == _SigRootIndex, "Signature Root index (", _SigRootIndex, ") exceeds maximum representable value");
        VERIFY(SamplerInd == _SamplerInd, "Sampler index (", _SamplerInd, ") exceeds maximum representable value");
        VERIFY(Space == _Space, "Space (", _Space, ") exceeds maximum representable value");
        VERIFY(GetD3D12RootParamType() == _RootParamType, "Not enough bits to represent root parameter type");
    }

    // Only for serialization
    PipelineResourceAttribsD3D12() noexcept :
        PipelineResourceAttribsD3D12{0, 0, 0, 0, 0, 0, 0, false, D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE}
    {}

    bool IsImmutableSamplerAssigned() const
    {
        return ImtblSamplerAssigned != 0;
    }
    bool IsCombinedWithSampler() const
    {
        return SamplerInd != InvalidSamplerInd;
    }

    Uint32 RootIndex(ResourceCacheContentType Type) const
    {
        return Type == ResourceCacheContentType::SRB ? SRBRootIndex : SigRootIndex;
    }
    Uint32 OffsetFromTableStart(ResourceCacheContentType Type) const
    {
        return Type == ResourceCacheContentType::SRB ? SRBOffsetFromTableStart : SigOffsetFromTableStart;
    }

    D3D12_ROOT_PARAMETER_TYPE GetD3D12RootParamType() const { return static_cast<D3D12_ROOT_PARAMETER_TYPE>(RootParamType); }

    bool IsRootView() const
    {
        return (GetD3D12RootParamType() == D3D12_ROOT_PARAMETER_TYPE_CBV ||
                GetD3D12RootParamType() == D3D12_ROOT_PARAMETER_TYPE_SRV ||
                GetD3D12RootParamType() == D3D12_ROOT_PARAMETER_TYPE_UAV);
    }

    bool IsCompatibleWith(const PipelineResourceAttribsD3D12& rhs) const
    {
        // Ignore sampler index, signature root index & offset.
        // clang-format off
        return Register                == rhs.Register                &&
               Space                   == rhs.Space                   &&
               SRBRootIndex            == rhs.SRBRootIndex            &&
               SRBOffsetFromTableStart == rhs.SRBOffsetFromTableStart &&
               ImtblSamplerAssigned    == rhs.ImtblSamplerAssigned    &&
               RootParamType           == rhs.RootParamType;
        // clang-format on
    }

    size_t GetHash() const
    {
        return ComputeHash(Register, Space, SRBRootIndex, SRBOffsetFromTableStart, ImtblSamplerAssigned, RootParamType);
    }
};
ASSERT_SIZEOF(PipelineResourceAttribsD3D12, 16, "The struct is used in serialization and must be tightly packed");

} // namespace Diligent
