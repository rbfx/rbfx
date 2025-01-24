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

#include <string>
#include <vector>
#include <unordered_set>

#include "Shader.h"
#include "StringTools.hpp"
#include "ShaderToolsCommon.hpp"
#include "D3DCommonTypeConversions.hpp"
#include "EngineMemory.h"
#include "ParsingTools.hpp"

/// \file
/// D3D shader resource loading

namespace Diligent
{

struct D3DShaderResourceCounters
{
    Uint32 NumCBs          = 0;
    Uint32 NumTexSRVs      = 0;
    Uint32 NumTexUAVs      = 0;
    Uint32 NumBufSRVs      = 0;
    Uint32 NumBufUAVs      = 0;
    Uint32 NumSamplers     = 0;
    Uint32 NumAccelStructs = 0;
};

template <typename TReflectionTraits,
          typename TD3DShaderReflectionType>
void LoadShaderCodeVariableDesc(TD3DShaderReflectionType* pd3dReflecionType, ShaderCodeVariableDescX& TypeDesc)
{
    using D3D_SHADER_TYPE_DESC = typename TReflectionTraits::D3D_SHADER_TYPE_DESC;

    if (pd3dReflecionType == nullptr)
    {
        UNEXPECTED("Reflection type is null");
        return;
    }

    D3D_SHADER_TYPE_DESC d3dTypeDesc = {};
    pd3dReflecionType->GetDesc(&d3dTypeDesc);

    TypeDesc.Class = D3DShaderVariableClassToShaderCodeVaraibleClass(d3dTypeDesc.Class);
    if (d3dTypeDesc.Class != D3D_SVC_STRUCT)
    {
        TypeDesc.BasicType = D3DShaderVariableTypeToShaderCodeBasicType(d3dTypeDesc.Type);
        // Number of rows in a matrix. Otherwise a numeric type returns 1, any other type returns 0.
        TypeDesc.NumRows = StaticCast<decltype(TypeDesc.NumRows)>(d3dTypeDesc.Rows);
        // Number of columns in a matrix. Otherwise a numeric type returns 1, any other type returns 0.
        TypeDesc.NumColumns = StaticCast<decltype(TypeDesc.NumRows)>(d3dTypeDesc.Columns);
    }

    if (d3dTypeDesc.Name != nullptr)
        TypeDesc.SetTypeName(d3dTypeDesc.Name);
    if (TypeDesc.TypeName == nullptr)
        TypeDesc.SetDefaultTypeName(SHADER_SOURCE_LANGUAGE_HLSL);
    if (d3dTypeDesc.Type == D3D_SVT_UINT && SafeStrEqual(TypeDesc.TypeName, "dword"))
        TypeDesc.SetTypeName("uint");

    // Number of elements in an array; otherwise 0.
    TypeDesc.ArraySize = d3dTypeDesc.Elements;
    // Offset, in bytes, between the start of the parent structure and this variable. Can be 0 if not a structure member.
    TypeDesc.Offset += d3dTypeDesc.Offset;

    for (Uint32 m = 0; m < d3dTypeDesc.Members; ++m)
    {
        ShaderCodeVariableDesc MemberDesc;
        MemberDesc.Name = pd3dReflecionType->GetMemberTypeName(m);

        auto idx = TypeDesc.AddMember(MemberDesc);
        VERIFY_EXPR(idx == m);
        auto* pd3dMemberType = pd3dReflecionType->GetMemberTypeByIndex(m);
        VERIFY_EXPR(pd3dMemberType != nullptr);
        LoadShaderCodeVariableDesc<TReflectionTraits>(pd3dMemberType, TypeDesc.GetMember(idx));
    }
}

template <typename TReflectionTraits,
          typename TShaderReflection>
void LoadD3DShaderConstantBufferReflection(TShaderReflection* pBuffReflection, ShaderCodeBufferDescX& BufferDesc, Uint32 NumVariables)
{
    using D3D_SHADER_VARIABLE_DESC = typename TReflectionTraits::D3D_SHADER_VARIABLE_DESC;

    for (Uint32 var = 0; var < NumVariables; ++var)
    {
        if (auto* pVaribable = pBuffReflection->GetVariableByIndex(var))
        {
            D3D_SHADER_VARIABLE_DESC d3dShaderVarDesc = {};
            pVaribable->GetDesc(&d3dShaderVarDesc);

            ShaderCodeVariableDesc VarDesc;
            VarDesc.Name   = d3dShaderVarDesc.Name;        // The variable name.
            VarDesc.Offset = d3dShaderVarDesc.StartOffset; // Offset from the start of the parent structure to the beginning of the variable.

            auto idx = BufferDesc.AddVariable(VarDesc);
            VERIFY_EXPR(idx == var);

            auto* pd3dReflecionType = pVaribable->GetType();
            VERIFY_EXPR(pd3dReflecionType != nullptr);
            LoadShaderCodeVariableDesc<TReflectionTraits>(pd3dReflecionType, BufferDesc.GetVariable(idx));
        }
        else
        {
            UNEXPECTED("Failed to get constant buffer variable reflection information.");
        }
    }
}

template <typename D3D_SHADER_INPUT_BIND_DESC>
Uint32 GetRegisterSpace(const D3D_SHADER_INPUT_BIND_DESC&);

template <typename D3DShaderResourceAttribs,
          typename TReflectionTraits,
          typename TShaderReflection,
          typename THandleShaderDesc,
          typename TOnResourcesCounted,
          typename TOnNewCB,
          typename TOnNewTexUAV,
          typename TOnNewBuffUAV,
          typename TOnNewBuffSRV,
          typename TOnNewSampler,
          typename TOnNewTexSRV,
          typename TOnNewAccelStruct>
void LoadD3DShaderResources(TShaderReflection*  pShaderReflection,
                            bool                LoadConstantBufferReflection,
                            THandleShaderDesc   HandleShaderDesc,
                            TOnResourcesCounted OnResourcesCounted,
                            TOnNewCB            OnNewCB,
                            TOnNewTexUAV        OnNewTexUAV,
                            TOnNewBuffUAV       OnNewBuffUAV,
                            TOnNewBuffSRV       OnNewBuffSRV,
                            TOnNewSampler       OnNewSampler,
                            TOnNewTexSRV        OnNewTexSRV,
                            TOnNewAccelStruct   OnNewAccelStruct)
{
    using D3D_SHADER_DESC            = typename TReflectionTraits::D3D_SHADER_DESC;
    using D3D_SHADER_INPUT_BIND_DESC = typename TReflectionTraits::D3D_SHADER_INPUT_BIND_DESC;
    using D3D_SHADER_BUFFER_DESC     = typename TReflectionTraits::D3D_SHADER_BUFFER_DESC;

    D3D_SHADER_DESC shaderDesc = {};
    pShaderReflection->GetDesc(&shaderDesc);

    HandleShaderDesc(shaderDesc);

    std::vector<D3DShaderResourceAttribs, STDAllocatorRawMem<D3DShaderResourceAttribs>> Resources(STD_ALLOCATOR_RAW_MEM(D3DShaderResourceAttribs, GetRawAllocator(), "Allocator for vector<D3DShaderResourceAttribs>"));
    Resources.reserve(shaderDesc.BoundResources);
    std::unordered_set<std::string> ResourceNamesTmpPool;

    D3DShaderResourceCounters RC;

    size_t ResourceNamesPoolSize = 0;
    // Number of resources to skip (used for array resources)
    UINT SkipCount = 1;
    for (UINT Res = 0; Res < shaderDesc.BoundResources; Res += SkipCount)
    {
        D3D_SHADER_INPUT_BIND_DESC BindingDesc = {};
        pShaderReflection->GetResourceBindingDesc(Res, &BindingDesc);

        std::string Name;
        const auto  ArrayIndex = Parsing::GetArrayIndex(BindingDesc.Name, Name);

        if (BindingDesc.BindPoint == UINT32_MAX)
        {
            BindingDesc.BindPoint = D3DShaderResourceAttribs::InvalidBindPoint;
        }
        else if (ArrayIndex > 0)
        {
            // Adjust bind point for array index
            VERIFY(BindingDesc.BindPoint >= static_cast<UINT>(ArrayIndex), "Resource '", BindingDesc.Name, "' uses bind point point ", BindingDesc.BindPoint,
                   ", which is invalid for its array index ", ArrayIndex);
            BindingDesc.BindPoint -= static_cast<UINT>(ArrayIndex);
        }

        UINT BindCount = BindingDesc.BindCount;
        if (BindCount == UINT_MAX)
        {
            // For some reason
            //      Texture2D g_Textures[]
            // produces BindCount == 0, but
            //      ConstantBuffer<CBData> g_ConstantBuffers[]
            // produces BindCount == UINT_MAX
            BindCount = 0;
        }

        // Handle arrays
        // For shader models 5_0 and before, every resource array element is enumerated individually.
        // For instance, if the following texture array is defined in the shader:
        //
        //     Texture2D<float3> g_tex2DDiffuse[4];
        //
        // The reflection system will enumerate 4 resources with the following names:
        // "g_tex2DDiffuse[0]"
        // "g_tex2DDiffuse[1]"
        // "g_tex2DDiffuse[2]"
        // "g_tex2DDiffuse[3]"
        //
        // Notice that if some array element is not used by the shader, it will not be enumerated

        SkipCount = 1;
        if (ArrayIndex >= 0)
        {
            VERIFY(BindCount == 1, "When array elements are enumerated individually, BindCount is expected to always be 1");

#ifdef DILIGENT_DEBUG
            for (const auto& ExistingRes : Resources)
            {
                VERIFY(Name.compare(ExistingRes.Name) != 0, "Resource with the same name has already been enumerated. All array elements are expected to be enumerated one after another");
            }
#endif
            for (UINT ArrElem = Res + 1; ArrElem < shaderDesc.BoundResources; ++ArrElem)
            {
                D3D_SHADER_INPUT_BIND_DESC NextElemBindingDesc = {};
                pShaderReflection->GetResourceBindingDesc(ArrElem, &NextElemBindingDesc);

                std::string NextElemName;
                const auto  NextElemIndex = Parsing::GetArrayIndex(NextElemBindingDesc.Name, NextElemName);

                // Make sure this case is handled correctly:
                // "g_tex2DDiffuse[.]" != "g_tex2DDiffuse2[.]"
                if (Name == NextElemName)
                {
                    VERIFY_EXPR(NextElemIndex > 0);

                    BindCount = std::max(BindCount, static_cast<UINT>(NextElemIndex) + 1);
                    VERIFY(NextElemBindingDesc.BindPoint == BindingDesc.BindPoint + NextElemIndex,
                           "Array elements are expected to use contiguous bind points.\n",
                           BindingDesc.Name, " uses slot ", BindingDesc.BindPoint, ", so ", NextElemBindingDesc.Name,
                           " is expected to use slot ", BindingDesc.BindPoint + NextElemIndex, " while ", NextElemBindingDesc.BindPoint,
                           " is actually used");

                    // Note that skip count may not necessarily be the same as BindCount.
                    // If some array elements are not used by the shader, the reflection system skips them
                    ++SkipCount;
                }
                else
                {
                    break;
                }
            }
        }

        switch (BindingDesc.Type)
        {
            // clang-format off
            case D3D_SIT_CBUFFER:                       ++RC.NumCBs;                                                                           break;
            case D3D_SIT_TBUFFER:                       UNSUPPORTED( "TBuffers are not supported" );                                           break;
            case D3D_SIT_TEXTURE:                       ++(BindingDesc.Dimension == D3D_SRV_DIMENSION_BUFFER ? RC.NumBufSRVs : RC.NumTexSRVs); break;
            case D3D_SIT_SAMPLER:                       ++RC.NumSamplers;                                                                      break;
            case D3D_SIT_UAV_RWTYPED:                   ++(BindingDesc.Dimension == D3D_SRV_DIMENSION_BUFFER ? RC.NumBufUAVs : RC.NumTexUAVs); break;
            case D3D_SIT_STRUCTURED:                    ++RC.NumBufSRVs;                                                                       break;
            case D3D_SIT_UAV_RWSTRUCTURED:              ++RC.NumBufUAVs;                                                                       break;
            case D3D_SIT_BYTEADDRESS:                   ++RC.NumBufSRVs;                                                                       break;
            case D3D_SIT_UAV_RWBYTEADDRESS:             ++RC.NumBufUAVs;                                                                       break;
            case D3D_SIT_UAV_APPEND_STRUCTURED:         UNSUPPORTED( "Append structured buffers are not supported" );                          break;
            case D3D_SIT_UAV_CONSUME_STRUCTURED:        UNSUPPORTED( "Consume structured buffers are not supported" );                         break;
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER: UNSUPPORTED( "RW structured buffers with counter are not supported" );                 break;
                // clang-format on

#pragma warning(push)
#pragma warning(disable : 4063)
            // D3D_SIT_RTACCELERATIONSTRUCTURE enum value is missing in Win SDK 17763, so we emulate it as #define, which
            // makes the compiler emit the following warning:
            //      warning C4063: case '12' is not a valid value for switch of enum '_D3D_SHADER_INPUT_TYPE'
            case D3D_SIT_RTACCELERATIONSTRUCTURE: ++RC.NumAccelStructs; break;
#pragma warning(pop)

            default: UNEXPECTED("Unexpected resource type");
        }
        ResourceNamesPoolSize += Name.length() + 1;
        auto it = ResourceNamesTmpPool.emplace(std::move(Name));
        Resources.emplace_back(
            it.first->c_str(),
            BindingDesc.BindPoint,
            BindCount,
            GetRegisterSpace(BindingDesc),
            BindingDesc.Type,
            BindingDesc.Dimension,
            D3DShaderResourceAttribs::InvalidSamplerId);
    }

    OnResourcesCounted(RC, ResourceNamesPoolSize);

    std::vector<size_t, STDAllocatorRawMem<size_t>> TexSRVInds(STD_ALLOCATOR_RAW_MEM(size_t, GetRawAllocator(), "Allocator for vector<size_t>"));
    TexSRVInds.reserve(RC.NumTexSRVs);

    for (size_t ResInd = 0; ResInd < Resources.size(); ++ResInd)
    {
        const auto& Res = Resources[ResInd];
        switch (Res.GetInputType())
        {
            case D3D_SIT_CBUFFER:
            {
                ShaderCodeBufferDescX BufferDesc;
                if (LoadConstantBufferReflection)
                {
                    if (auto* pBuffReflection = pShaderReflection->GetConstantBufferByName(Res.Name))
                    {
                        D3D_SHADER_BUFFER_DESC ShaderBuffDesc = {};
                        pBuffReflection->GetDesc(&ShaderBuffDesc);
                        VERIFY_EXPR(SafeStrEqual(Res.Name, ShaderBuffDesc.Name));
                        VERIFY_EXPR(ShaderBuffDesc.Type == D3D_CT_CBUFFER);

                        BufferDesc.Size = ShaderBuffDesc.Size;
                        LoadD3DShaderConstantBufferReflection<TReflectionTraits>(pBuffReflection, BufferDesc, ShaderBuffDesc.Variables);
                    }
                    else
                    {
                        UNEXPECTED("Failed to get constant buffer reflection information.");
                    }
                }

                OnNewCB(Res, std::move(BufferDesc));
                break;
            }

            case D3D_SIT_TBUFFER:
            {
                UNSUPPORTED("TBuffers are not supported");
                break;
            }

            case D3D_SIT_TEXTURE:
            {
                if (Res.GetSRVDimension() == D3D_SRV_DIMENSION_BUFFER)
                {
                    OnNewBuffSRV(Res);
                }
                else
                {
                    // Texture SRVs must be processed all samplers are initialized
                    TexSRVInds.push_back(ResInd);
                }
                break;
            }

            case D3D_SIT_SAMPLER:
            {
                OnNewSampler(Res);
                break;
            }

            case D3D_SIT_UAV_RWTYPED:
            {
                if (Res.GetSRVDimension() == D3D_SRV_DIMENSION_BUFFER)
                {
                    OnNewBuffUAV(Res);
                }
                else
                {
                    OnNewTexUAV(Res);
                }
                break;
            }

            case D3D_SIT_STRUCTURED:
            {
                OnNewBuffSRV(Res);
                break;
            }

            case D3D_SIT_UAV_RWSTRUCTURED:
            {
                OnNewBuffUAV(Res);
                break;
            }

            case D3D_SIT_BYTEADDRESS:
            {
                OnNewBuffSRV(Res);
                break;
            }

            case D3D_SIT_UAV_RWBYTEADDRESS:
            {
                OnNewBuffUAV(Res);
                break;
            }

            case D3D_SIT_UAV_APPEND_STRUCTURED:
            {
                UNSUPPORTED("Append structured buffers are not supported");
                break;
            }

            case D3D_SIT_UAV_CONSUME_STRUCTURED:
            {
                UNSUPPORTED("Consume structured buffers are not supported");
                break;
            }

            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
            {
                UNSUPPORTED("RW structured buffers with counter are not supported");
                break;
            }

#pragma warning(push)
#pragma warning(disable : 4063)
            // D3D_SIT_RTACCELERATIONSTRUCTURE enum value is missing in Win SDK 17763, so we emulate it as #define, which
            // makes the compiler emit the following warning:
            //      warning C4063: case '12' is not a valid value for switch of enum '_D3D_SHADER_INPUT_TYPE'
            case D3D_SIT_RTACCELERATIONSTRUCTURE:
#pragma warning(pop)
            {
                OnNewAccelStruct(Res);
                break;
            }

            default:
            {
                UNEXPECTED("Unexpected resource input type");
            }
        }
    }

    // Process texture SRVs. We need to do this after all samplers are initialized
    for (auto TexSRVInd : TexSRVInds)
    {
        OnNewTexSRV(Resources[TexSRVInd]);
    }
}

} // namespace Diligent
