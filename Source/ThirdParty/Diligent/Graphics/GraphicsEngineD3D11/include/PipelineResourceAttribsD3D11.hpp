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
/// Declaration of Diligent::PipelineResourceAttribsD3D11 struct

#include <array>

#include "BasicTypes.h"
#include "DebugUtilities.hpp"
#include "HashUtils.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

enum D3D11_RESOURCE_RANGE : Uint32
{
    D3D11_RESOURCE_RANGE_CBV = 0,
    D3D11_RESOURCE_RANGE_SRV,
    D3D11_RESOURCE_RANGE_SAMPLER,
    D3D11_RESOURCE_RANGE_UAV,
    D3D11_RESOURCE_RANGE_COUNT,
    D3D11_RESOURCE_RANGE_UNKNOWN = ~0u
};


/// Resource binding points in all shader stages.
// sizeof(D3D11ResourceBindPoints) == 8, x64
struct D3D11ResourceBindPoints
{
    /// The number of different shader types (Vertex, Pixel, Geometry, Hull, Domain, Compute)
    static constexpr Uint32 NumShaderTypes = 6;

    D3D11ResourceBindPoints() noexcept
    {
#ifdef DILIGENT_DEBUG
        for (auto BindPoint : Bindings)
            VERIFY_EXPR(BindPoint == InvalidBindPoint);
#endif
    }

    D3D11ResourceBindPoints(const D3D11ResourceBindPoints&) noexcept = default;
    D3D11ResourceBindPoints& operator=(const D3D11ResourceBindPoints&) = default;

    SHADER_TYPE GetActiveStages() const
    {
        return static_cast<SHADER_TYPE>(ActiveStages);
    }

    bool IsEmpty() const
    {
        return GetActiveStages() == SHADER_TYPE_UNKNOWN;
    }

    bool IsStageActive(Uint32 ShaderInd) const
    {
        bool IsActive = (GetActiveStages() & GetShaderTypeFromIndex(ShaderInd)) != 0;
        VERIFY_EXPR((IsActive && Bindings[ShaderInd] != InvalidBindPoint ||
                     !IsActive && Bindings[ShaderInd] == InvalidBindPoint));
        return IsActive;
    }

    Uint8 operator[](Uint32 ShaderInd) const
    {
        VERIFY(IsStageActive(ShaderInd), "Requesting bind point for inactive shader stage.");
        return Bindings[ShaderInd];
    }

    size_t GetHash() const
    {
        size_t Hash = 0;
        for (auto Binding : Bindings)
            HashCombine(Hash, Binding);
        return Hash;
    }

    bool operator==(const D3D11ResourceBindPoints& rhs) const noexcept
    {
        return Bindings == rhs.Bindings;
    }

    D3D11ResourceBindPoints operator+(Uint32 value) const
    {
        D3D11ResourceBindPoints NewBindPoints{*this};
        for (auto Stages = GetActiveStages(); Stages != SHADER_TYPE_UNKNOWN;)
        {
            auto ShaderInd = ExtractFirstShaderStageIndex(Stages);
            VERIFY_EXPR(Uint32{Bindings[ShaderInd]} + value < InvalidBindPoint);
            NewBindPoints.Bindings[ShaderInd] = Bindings[ShaderInd] + static_cast<Uint8>(value);
        }
        return NewBindPoints;
    }

private:
    struct StageAccessor
    {
        StageAccessor(D3D11ResourceBindPoints& _BindPoints,
                      const Uint32             _ShaderInd) :
            BindPoints{_BindPoints},
            ShaderInd{_ShaderInd}
        {}

        // clang-format off
        StageAccessor           (const StageAccessor&)  = delete;
        StageAccessor           (      StageAccessor&&) = delete;
        StageAccessor& operator=(const StageAccessor&)  = delete;
        StageAccessor& operator=(      StageAccessor&&) = delete;
        // clang-format on

        Uint8 operator=(Uint32 BindPoint)
        {
            return BindPoints.Set(ShaderInd, BindPoint);
        }

        operator Uint8() const
        {
            return static_cast<const D3D11ResourceBindPoints&>(BindPoints)[ShaderInd];
        }

    private:
        D3D11ResourceBindPoints& BindPoints;
        const Uint32             ShaderInd;
    };

public:
    StageAccessor operator[](Uint32 ShaderInd)
    {
        return {*this, ShaderInd};
    }

private:
    Uint8 Set(Uint32 ShaderInd, Uint32 BindPoint)
    {
        VERIFY_EXPR(ShaderInd < NumShaderTypes);
        VERIFY(BindPoint < InvalidBindPoint, "Bind point (", BindPoint, ") is out of range.");

        Bindings[ShaderInd] = static_cast<Uint8>(BindPoint);
        ActiveStages |= Uint32{1} << ShaderInd;
        return static_cast<Uint8>(BindPoint);
    }

    static constexpr Uint8 InvalidBindPoint = 0xFF;

    //     0      1      2      3      4      5
    // |  PS  |  VS  |  GS  |  HS  |  DS  |  CS  |
    std::array<Uint8, NumShaderTypes> Bindings{InvalidBindPoint, InvalidBindPoint, InvalidBindPoint, InvalidBindPoint, InvalidBindPoint, InvalidBindPoint};

    Uint16 ActiveStages = 0;
};
ASSERT_SIZEOF(D3D11ResourceBindPoints, 8, "The struct is used in serialization and must be tightly packed");


/// Shader resource counters for one specific resource range
struct D3D11ResourceRangeCounters
{
    static constexpr Uint32 NumShaderTypes = D3D11ResourceBindPoints::NumShaderTypes;

    Uint8 operator[](Uint32 Stage) const
    {
        VERIFY_EXPR(Stage < NumShaderTypes);
        return (PackedCounters >> (NumBitsPerStage * Stage)) & StageMask;
    }

    D3D11ResourceRangeCounters& operator+=(const D3D11ResourceRangeCounters& rhs)
    {
#ifdef DILIGENT_DEBUG
        for (Uint32 s = 0; s < NumShaderTypes; ++s)
        {
            const Uint32 val0 = static_cast<const D3D11ResourceRangeCounters&>(*this)[s];
            const Uint32 val1 = rhs[s];
            VERIFY(val0 + val1 <= MaxCounter, "The resulting value (", val0 + val1, ") is out of range.");
        }
#endif
        PackedCounters += rhs.PackedCounters;
        return *this;
    }

    bool operator==(const D3D11ResourceRangeCounters& rhs) const noexcept
    {
        return PackedCounters == rhs.PackedCounters;
    }

private:
    struct StageAccessor
    {
        StageAccessor(D3D11ResourceRangeCounters& _Counters,
                      const Uint32                _ShaderInd) :
            Counters{_Counters},
            ShaderInd{_ShaderInd}
        {}

        // clang-format off
        StageAccessor           (const StageAccessor&)  = delete;
        StageAccessor           (      StageAccessor&&) = delete;
        StageAccessor& operator=(const StageAccessor&)  = delete;
        StageAccessor& operator=(      StageAccessor&&) = delete;
        // clang-format on

        Uint8 operator=(Uint32 Counter)
        {
            return Counters.Set(ShaderInd, Counter);
        }

        Uint8 operator+=(Uint32 Val)
        {
            Uint32 CurrValue = static_cast<const D3D11ResourceRangeCounters&>(Counters)[ShaderInd];
            return Counters.Set(ShaderInd, CurrValue + Val);
        }

        operator Uint8() const
        {
            return static_cast<const D3D11ResourceRangeCounters&>(Counters)[ShaderInd];
        }

    private:
        D3D11ResourceRangeCounters& Counters;
        const Uint32                ShaderInd;
    };

public:
    StageAccessor operator[](Uint32 ShaderInd)
    {
        return {*this, ShaderInd};
    }

private:
    Uint8 Set(Uint32 ShaderInd, Uint32 Counter)
    {
        VERIFY_EXPR(Counter <= MaxCounter);
        const Uint64 BitOffset = NumBitsPerStage * ShaderInd;
        PackedCounters &= ~(StageMask << BitOffset);
        PackedCounters |= Uint64{Counter} << BitOffset;
        return static_cast<Uint8>(Counter);
    }

    static constexpr Uint64 NumBitsPerStage = 8;
    static constexpr Uint64 StageMask       = (Uint64{1} << NumBitsPerStage) - 1;
    static constexpr Uint32 MaxCounter      = (Uint32{1} << NumBitsPerStage) - 1;

    // 0      1      2      3      4      5      6      7      8
    // |  VS  |  PS  |  GS  |  HS  |  DS  |  CS  |unused|unused|
    union
    {
        Uint64 PackedCounters = 0;
        struct
        {
            Uint64 VS : NumBitsPerStage;
            Uint64 PS : NumBitsPerStage;
            Uint64 GS : NumBitsPerStage;
            Uint64 HS : NumBitsPerStage;
            Uint64 DS : NumBitsPerStage;
            Uint64 CS : NumBitsPerStage;
        } _Packed;
    };
};

/// Resource counters for all shader stages and all resource types
using D3D11ShaderResourceCounters = std::array<D3D11ResourceRangeCounters, D3D11_RESOURCE_RANGE_COUNT>;


// sizeof(PipelineResourceAttribsD3D11) == 12, x64
struct PipelineResourceAttribsD3D11
{
private:
    static constexpr Uint32 _SamplerIndBits      = 31;
    static constexpr Uint32 _SamplerAssignedBits = 1;

public:
    static constexpr Uint32 InvalidSamplerInd = (1u << _SamplerIndBits) - 1;

    // clang-format off
    const Uint32 SamplerInd           : _SamplerIndBits;       // Index of the assigned sampler in m_Desc.Resources.
    const Uint32 ImtblSamplerAssigned : _SamplerAssignedBits;  // Immutable sampler flag for Texture SRV or Sampler.
    // clang-format on
    const D3D11ResourceBindPoints BindPoints;

    PipelineResourceAttribsD3D11(const D3D11ResourceBindPoints& _BindPoints,
                                 Uint32                         _SamplerInd,
                                 bool                           _ImtblSamplerAssigned) noexcept :
        // clang-format off
        SamplerInd          {_SamplerInd                    },
        ImtblSamplerAssigned{_ImtblSamplerAssigned ? 1u : 0u},
        BindPoints          {_BindPoints                    }
    // clang-format on
    {
        VERIFY(SamplerInd == _SamplerInd, "Sampler index (", _SamplerInd, ") exceeds maximum representable value.");
    }

    // Only for serialization
    PipelineResourceAttribsD3D11() noexcept :
        PipelineResourceAttribsD3D11{{}, 0, false}
    {}

    bool IsSamplerAssigned() const { return SamplerInd != InvalidSamplerInd; }
    bool IsImmutableSamplerAssigned() const { return ImtblSamplerAssigned != 0; }

    bool IsCompatibleWith(const PipelineResourceAttribsD3D11& rhs) const
    {
        // Ignore assigned sampler index.
        // clang-format off
        return IsImmutableSamplerAssigned() == rhs.IsImmutableSamplerAssigned() &&
               BindPoints                   == rhs.BindPoints;
        // clang-format on
    }

    size_t GetHash() const
    {
        return ComputeHash(IsImmutableSamplerAssigned(), BindPoints.GetHash());
    }
};
ASSERT_SIZEOF(PipelineResourceAttribsD3D11, 12, "The struct is used in serialization and must be tightly packed");

} // namespace Diligent
