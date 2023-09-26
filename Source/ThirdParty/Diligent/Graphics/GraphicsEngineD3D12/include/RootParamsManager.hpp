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
/// Declaration of Diligent::RootParamsManager class and related data structures

#include <memory>
#include <vector>
#include <array>

#include "Shader.h"
#include "ShaderResourceVariable.h"

namespace Diligent
{

enum ROOT_PARAMETER_GROUP : Uint32
{
    ROOT_PARAMETER_GROUP_STATIC_MUTABLE = 0,
    ROOT_PARAMETER_GROUP_DYNAMIC        = 1,
    ROOT_PARAMETER_GROUP_COUNT
};

inline ROOT_PARAMETER_GROUP VariableTypeToRootParameterGroup(SHADER_RESOURCE_VARIABLE_TYPE VarType)
{
    return VarType == SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC ? ROOT_PARAMETER_GROUP_DYNAMIC : ROOT_PARAMETER_GROUP_STATIC_MUTABLE;
}

struct RootParameter
{
private:
    static constexpr Uint32 ParameterGroupBits = 1;
    static constexpr Uint32 RootIndexBits      = 32 - ParameterGroupBits;
    static_assert((1 << ParameterGroupBits) >= ROOT_PARAMETER_GROUP_COUNT, "Not enough bits to represent ROOT_PARAMETER_GROUP");

public:
    const Uint32 RootIndex : RootIndexBits;

    const ROOT_PARAMETER_GROUP Group : ParameterGroupBits;

    // Each descriptor table is suballocated from one of the four descriptor heap allocations:
    // {CBV_SRV_UAV, SAMPLER} x {STATIC_MUTABLE, DYNAMIC}.
    // TableOffsetInGroupAllocation indicates starting offset from the beginning of the
    // corresponding allocation.
    const Uint32 TableOffsetInGroupAllocation;

    static constexpr Uint32 InvalidTableOffsetInGroupAllocation = ~0u;

    const D3D12_ROOT_PARAMETER d3d12RootParam;

    RootParameter(Uint32                      _RootIndex,
                  ROOT_PARAMETER_GROUP        _Group,
                  const D3D12_ROOT_PARAMETER& _d3d12RootParam,
                  Uint32                      _TableOffsetInGroupAllocation = InvalidTableOffsetInGroupAllocation) noexcept;

    // clang-format off
    RootParameter           (const RootParameter&)  = delete;
    RootParameter& operator=(const RootParameter&)  = delete;
    RootParameter           (RootParameter&&)       = default;
    RootParameter& operator=(RootParameter&&)       = delete;
    // clang-format on

    Uint32 GetDescriptorTableSize() const
    {
        VERIFY(d3d12RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
               "Incorrect parameter type: descriptor table is expected");

        // All descriptors in the table are tightly packed, so the table size is given
        // by the end of the last range
        const auto& d3d12Tbl = d3d12RootParam.DescriptorTable;
        VERIFY(d3d12Tbl.NumDescriptorRanges > 0, "Descriptor table must contain at least one range");
        const auto& d3d12LastRange = d3d12Tbl.pDescriptorRanges[d3d12Tbl.NumDescriptorRanges - 1];
        VERIFY(d3d12LastRange.NumDescriptors > 0, "The range must not be empty");
        return d3d12LastRange.OffsetInDescriptorsFromTableStart + d3d12LastRange.NumDescriptors;
    }

    bool operator==(const RootParameter& rhs) const noexcept;
    bool operator!=(const RootParameter& rhs) const noexcept { return !(*this == rhs); }

    size_t GetHash() const;
};
static_assert(sizeof(RootParameter) == sizeof(D3D12_ROOT_PARAMETER) + sizeof(Uint32) * 2, "Unexpected sizeof(RootParameter) - did you pack the members properly?");


/// Container for root parameters

/// RootParamsManager keeps root parameters of a single pipeline resource signature.
/// When resource signatures are combined into a single d3d12 root signature,
/// root indices and shader spaces are biased based on earlier signatures.

// Note that root index is NOT the same as the index of
// the root table or index of the root view, e.g.
//
//   Root Index |  Root Table Index | Root View Index
//       0      |         0         |
//       1      |                   |        0
//       2      |         1         |
//       3      |         2         |
//       4      |                   |        1
//
class RootParamsManager
{
public:
    RootParamsManager() noexcept {}
    ~RootParamsManager();

    // clang-format off
    RootParamsManager           (const RootParamsManager&) = delete;
    RootParamsManager& operator=(const RootParamsManager&) = delete;
    RootParamsManager           (RootParamsManager&&)      = delete;
    RootParamsManager& operator=(RootParamsManager&&)      = delete;
    // clang-format on

    Uint32 GetNumRootTables() const { return m_NumRootTables; }
    Uint32 GetNumRootViews() const { return m_NumRootViews; }

    const RootParameter& GetRootTable(Uint32 TableInd) const
    {
        VERIFY_EXPR(TableInd < m_NumRootTables);
        return m_pRootTables[TableInd];
    }

    const RootParameter& GetRootView(Uint32 ViewInd) const
    {
        VERIFY_EXPR(ViewInd < m_NumRootViews);
        return m_pRootViews[ViewInd];
    }

    // Returns the total number of resources in a given parameter group and descriptor heap type
    Uint32 GetParameterGroupSize(D3D12_DESCRIPTOR_HEAP_TYPE d3d12HeapType, ROOT_PARAMETER_GROUP Group) const
    {
        return m_ParameterGroupSizes[d3d12HeapType][Group];
    }

    bool operator==(const RootParamsManager& RootParams) const noexcept;

#ifdef DILIGENT_DEBUG
    void Validate() const;
#endif

private:
    friend class RootParamsBuilder;

    std::unique_ptr<void, STDDeleter<void, IMemoryAllocator>> m_pMemory;

    Uint32 m_NumRootTables = 0;
    Uint32 m_NumRootViews  = 0;

    const RootParameter* m_pRootTables = nullptr;
    const RootParameter* m_pRootViews  = nullptr;

    // The total number of resources placed in descriptor tables for each heap type and parameter group type
    std::array<std::array<Uint32, ROOT_PARAMETER_GROUP_COUNT>, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER + 1> m_ParameterGroupSizes{};
};

class RootParamsBuilder
{
public:
    RootParamsBuilder();

    // Allocates root parameter slot for the given resource attributes.
    void AllocateResourceSlot(SHADER_TYPE                   ShaderStages,
                              SHADER_RESOURCE_VARIABLE_TYPE VariableType,
                              D3D12_ROOT_PARAMETER_TYPE     RootParameterType,
                              D3D12_DESCRIPTOR_RANGE_TYPE   RangeType,
                              Uint32                        ArraySize,
                              Uint32                        Register,
                              Uint32                        Space,
                              Uint32&                       RootIndex,
                              Uint32&                       OffsetFromTableStart);

    void InitializeMgr(IMemoryAllocator& MemAllocator, RootParamsManager& ParamsMgr);

private:
    // Adds a new root view parameter and returns the reference to it.
    RootParameter& AddRootView(D3D12_ROOT_PARAMETER_TYPE ParameterType,
                               Uint32                    RootIndex,
                               UINT                      Register,
                               UINT                      RegisterSpace,
                               D3D12_SHADER_VISIBILITY   Visibility,
                               ROOT_PARAMETER_GROUP      RootType);

    struct RootTableData;
    // Adds a new root table parameter and returns the reference to it.
    RootTableData& AddRootTable(Uint32                  RootIndex,
                                D3D12_SHADER_VISIBILITY Visibility,
                                ROOT_PARAMETER_GROUP    RootType,
                                Uint32                  NumRangesInNewTable = 1);


private:
    struct RootTableData
    {
        RootTableData(Uint32                  _RootIndex,
                      D3D12_SHADER_VISIBILITY _Visibility,
                      ROOT_PARAMETER_GROUP    _Group,
                      Uint32                  _NumRanges);
        void Extend(Uint32 NumExtraRanges);

        const Uint32               RootIndex;
        const ROOT_PARAMETER_GROUP Group;
        D3D12_ROOT_PARAMETER       d3d12RootParam{};

        std::vector<D3D12_DESCRIPTOR_RANGE> Ranges;
    };
    std::vector<RootTableData> m_RootTables;
    std::vector<RootParameter> m_RootViews;

    static constexpr int InvalidRootTableIndex = -1;

    // The array below contains the index of a CBV/SRV/UAV root table in m_RootTables
    // (NOT the Root Index!), for every root parameter group (static/mutable, dynamic)
    // and every shader visibility, or -1, if the table is not yet assigned to the combination.
    // Note: max(D3D12_SHADER_VISIBILITY) == D3D12_SHADER_VISIBILITY_MESH == 7
    std::array<std::array<int, 8>, ROOT_PARAMETER_GROUP_COUNT> m_SrvCbvUavRootTablesMap = {};

    // This array contains the same data for Sampler root table
    std::array<std::array<int, 8>, ROOT_PARAMETER_GROUP_COUNT> m_SamplerRootTablesMap = {};
};

} // namespace Diligent
