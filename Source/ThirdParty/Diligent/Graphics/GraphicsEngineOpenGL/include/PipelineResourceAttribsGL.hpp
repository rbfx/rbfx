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
/// Declaration of Diligent::PipelineResourceAttribsGL struct

#include "BasicTypes.h"
#include "DebugUtilities.hpp"
#include "HashUtils.hpp"

namespace Diligent
{

// sizeof(PipelineResourceAttribsGL) == 8, x64
struct PipelineResourceAttribsGL
{
private:
    static constexpr Uint32 _SamplerIndBits      = 31;
    static constexpr Uint32 _SamplerAssignedBits = 1;

public:
    static constexpr Uint32 InvalidCacheOffset = ~0u;
    static constexpr Uint32 InvalidSamplerInd  = (1u << _SamplerIndBits) - 1;

    // clang-format off
    const Uint32  CacheOffset;                                 // SRB and Signature use the same cache offsets for static resources.
                                                               // (thanks to sorting variables by type, where all static vars go first).
                                                               // Binding == BaseBinding[Range] + CacheOffset
    const Uint32  SamplerInd           : _SamplerIndBits;      // ImtblSamplerAssigned == true:  index of the immutable sampler in m_ImmutableSamplers.
                                                               // ImtblSamplerAssigned == false: index of the assigned sampler in m_Desc.Resources.
    const Uint32  ImtblSamplerAssigned : _SamplerAssignedBits; // Immutable sampler flag
    // clang-format on

    PipelineResourceAttribsGL(Uint32 _CacheOffset,
                              Uint32 _SamplerInd,
                              bool   _ImtblSamplerAssigned) noexcept :
        // clang-format off
        CacheOffset         {_CacheOffset                   },
        SamplerInd          {_SamplerInd                    },
        ImtblSamplerAssigned{_ImtblSamplerAssigned ? 1u : 0u}
    // clang-format on
    {
        VERIFY(SamplerInd == _SamplerInd, "Sampler index (", _SamplerInd, ") exceeds maximum representable value");
        VERIFY(!_ImtblSamplerAssigned || SamplerInd != InvalidSamplerInd, "Immutable sampler is assigned, but sampler index is not valid");
    }

    // Only for serialization
    PipelineResourceAttribsGL() noexcept :
        PipelineResourceAttribsGL{0, 0, false}
    {}

    bool IsSamplerAssigned() const
    {
        return SamplerInd != InvalidSamplerInd;
    }

    bool IsImmutableSamplerAssigned() const
    {
        return ImtblSamplerAssigned != 0;
    }

    bool IsCompatibleWith(const PipelineResourceAttribsGL& rhs) const
    {
        // Ignore sampler index.
        // clang-format off
        return CacheOffset          == rhs.CacheOffset &&
               ImtblSamplerAssigned == rhs.ImtblSamplerAssigned;
        // clang-format on
    }

    size_t GetHash() const
    {
        return ComputeHash(CacheOffset, ImtblSamplerAssigned);
    }
};

} // namespace Diligent
