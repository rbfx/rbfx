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

#include "pch.h"

#include "PipelineStateD3D12Impl.hpp"
#include "PipelineStateCacheD3D12Impl.hpp"

#include <array>
#include <sstream>
#include <unordered_map>

#include "WinHPreface.h"
#include <d3dcompiler.h>
#include "WinHPostface.h"

#include "RenderDeviceD3D12Impl.hpp"
#include "ShaderD3D12Impl.hpp"
#include "ShaderResourceBindingD3D12Impl.hpp"
#include "RenderPassD3D12Impl.hpp"

#include "D3D12TypeConversions.hpp"
#include "DXGITypeConversions.hpp"
#include "CommandContext.hpp"
#include "EngineMemory.h"
#include "StringTools.hpp"
#include "DynamicLinearAllocator.hpp"
#include "D3DShaderResourceValidation.hpp"

#include "DXBCUtils.hpp"
#include "DXCompiler.hpp"
#include "dxc/dxcapi.h"

namespace Diligent
{

constexpr INTERFACE_ID PipelineStateD3D12Impl::IID_InternalImpl;

namespace
{
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4324) //  warning C4324: structure was padded due to alignment specifier
#endif

template <typename InnerStructType, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE SubObjType>
struct alignas(void*) PSS_SubObject
{
    const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type{SubObjType};
    InnerStructType                           Obj{};

    PSS_SubObject() noexcept {}

    PSS_SubObject& operator=(const InnerStructType& obj)
    {
        Obj = obj;
        return *this;
    }

    InnerStructType* operator->() { return &Obj; }
    InnerStructType* operator&() { return &Obj; }
    InnerStructType& operator*() { return Obj; }
};

#ifdef _MSC_VER
#    pragma warning(pop)
#endif


class PrimitiveTopology_To_D3D12_PRIMITIVE_TOPOLOGY_TYPE
{
public:
    PrimitiveTopology_To_D3D12_PRIMITIVE_TOPOLOGY_TYPE()
    {
        // clang-format off
        m_Map[PRIMITIVE_TOPOLOGY_UNDEFINED]      = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        m_Map[PRIMITIVE_TOPOLOGY_TRIANGLE_LIST]  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        m_Map[PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP] = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        m_Map[PRIMITIVE_TOPOLOGY_POINT_LIST]     = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        m_Map[PRIMITIVE_TOPOLOGY_LINE_LIST]      = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        m_Map[PRIMITIVE_TOPOLOGY_LINE_STRIP]     = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        // clang-format on
        for (int t = static_cast<int>(PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST); t < static_cast<int>(PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES); ++t)
            m_Map[t] = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE operator[](PRIMITIVE_TOPOLOGY Topology) const
    {
        return m_Map[static_cast<int>(Topology)];
    }

private:
    std::array<D3D12_PRIMITIVE_TOPOLOGY_TYPE, PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES> m_Map;
};

template <typename TShaderStages>
void BuildRTPipelineDescription(const RayTracingPipelineStateCreateInfo& CreateInfo,
                                std::vector<D3D12_STATE_SUBOBJECT>&      Subobjects,
                                DynamicLinearAllocator&                  TempPool,
                                TShaderStages&                           ShaderStages) noexcept(false)
{
#define LOG_PSO_ERROR_AND_THROW(...) LOG_ERROR_AND_THROW("Description of ray tracing PSO '", (CreateInfo.PSODesc.Name ? CreateInfo.PSODesc.Name : ""), "' is invalid: ", ##__VA_ARGS__)

    Uint32 UnnamedExportIndex = 0;

    std::unordered_map<IShader*, LPCWSTR> UniqueShaders;

    std::array<typename TShaderStages::value_type*, MAX_SHADERS_IN_PIPELINE> StagesPtr     = {};
    std::array<Uint32, MAX_SHADERS_IN_PIPELINE>                              ShaderIndices = {};

    for (auto& Stage : ShaderStages)
    {
        const auto Idx = GetShaderTypePipelineIndex(Stage.Type, PIPELINE_TYPE_RAY_TRACING);
        VERIFY_EXPR(StagesPtr[Idx] == nullptr);
        StagesPtr[Idx] = &Stage;
    }

    const auto AddDxilLib = [&](IShader* pShader, const char* Name) -> LPCWSTR {
        if (pShader == nullptr)
            return nullptr;

        auto it_inserted = UniqueShaders.emplace(pShader, nullptr);
        if (it_inserted.second)
        {
            const auto  StageIdx    = GetShaderTypePipelineIndex(pShader->GetDesc().ShaderType, PIPELINE_TYPE_RAY_TRACING);
            const auto& Stage       = *StagesPtr[StageIdx];
            auto&       ShaderIndex = ShaderIndices[StageIdx];

            // shaders must be in same order as in ExtractShaders()
            RefCntAutoPtr<ShaderD3D12Impl> pShaderD3D12{pShader, ShaderD3D12Impl::IID_InternalImpl};
            VERIFY(pShaderD3D12, "Unexpected shader object implementation");
            VERIFY_EXPR(Stage.Shaders[ShaderIndex] == pShaderD3D12);

            auto&       LibDesc    = *TempPool.Construct<D3D12_DXIL_LIBRARY_DESC>();
            auto&       ExportDesc = *TempPool.Construct<D3D12_EXPORT_DESC>();
            const auto& pBlob      = Stage.ByteCodes[ShaderIndex];
            ++ShaderIndex;

            LibDesc.DXILLibrary.BytecodeLength  = pBlob->GetBufferSize();
            LibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
            LibDesc.NumExports                  = 1;
            LibDesc.pExports                    = &ExportDesc;

            ExportDesc.Flags          = D3D12_EXPORT_FLAG_NONE;
            ExportDesc.ExportToRename = TempPool.CopyWString(pShaderD3D12->GetEntryPoint());

            if (Name != nullptr)
                ExportDesc.Name = TempPool.CopyWString(Name);
            else
            {
                std::stringstream ss;
                ss << "__Shader_" << std::setfill('0') << std::setw(4) << UnnamedExportIndex++;
                ExportDesc.Name = TempPool.CopyWString(ss.str());
            }

            Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &LibDesc});

            it_inserted.first->second = ExportDesc.Name;
            return ExportDesc.Name;
        }
        else
            return it_inserted.first->second;
    };

    for (Uint32 i = 0; i < CreateInfo.GeneralShaderCount; ++i)
    {
        const auto& GeneralShader = CreateInfo.pGeneralShaders[i];
        AddDxilLib(GeneralShader.pShader, GeneralShader.Name);
    }

    for (Uint32 i = 0; i < CreateInfo.TriangleHitShaderCount; ++i)
    {
        const auto& TriHitShader = CreateInfo.pTriangleHitShaders[i];

        auto& HitGroupDesc                    = *TempPool.Construct<D3D12_HIT_GROUP_DESC>();
        HitGroupDesc.HitGroupExport           = TempPool.CopyWString(TriHitShader.Name);
        HitGroupDesc.Type                     = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        HitGroupDesc.ClosestHitShaderImport   = AddDxilLib(TriHitShader.pClosestHitShader, nullptr);
        HitGroupDesc.AnyHitShaderImport       = AddDxilLib(TriHitShader.pAnyHitShader, nullptr);
        HitGroupDesc.IntersectionShaderImport = nullptr;

        Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &HitGroupDesc});
    }

    for (Uint32 i = 0; i < CreateInfo.ProceduralHitShaderCount; ++i)
    {
        const auto& ProcHitShader = CreateInfo.pProceduralHitShaders[i];

        auto& HitGroupDesc                    = *TempPool.Construct<D3D12_HIT_GROUP_DESC>();
        HitGroupDesc.HitGroupExport           = TempPool.CopyWString(ProcHitShader.Name);
        HitGroupDesc.Type                     = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
        HitGroupDesc.ClosestHitShaderImport   = AddDxilLib(ProcHitShader.pClosestHitShader, nullptr);
        HitGroupDesc.AnyHitShaderImport       = AddDxilLib(ProcHitShader.pAnyHitShader, nullptr);
        HitGroupDesc.IntersectionShaderImport = AddDxilLib(ProcHitShader.pIntersectionShader, nullptr);

        Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &HitGroupDesc});
    }

    constexpr Uint32 DefaultPayloadSize = sizeof(float) * 8;

    auto& PipelineConfig = *TempPool.Construct<D3D12_RAYTRACING_PIPELINE_CONFIG>();

    PipelineConfig.MaxTraceRecursionDepth = CreateInfo.RayTracingPipeline.MaxRecursionDepth;
    Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &PipelineConfig});

    auto& ShaderConfig                   = *TempPool.Construct<D3D12_RAYTRACING_SHADER_CONFIG>();
    ShaderConfig.MaxAttributeSizeInBytes = CreateInfo.MaxAttributeSize == 0 ? D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES : CreateInfo.MaxAttributeSize;
    ShaderConfig.MaxPayloadSizeInBytes   = CreateInfo.MaxPayloadSize == 0 ? DefaultPayloadSize : CreateInfo.MaxPayloadSize;
    Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &ShaderConfig});
#undef LOG_PSO_ERROR_AND_THROW
}

template <typename TNameToGroupIndexMap>
void GetShaderIdentifiers(ID3D12DeviceChild*                       pSO,
                          const RayTracingPipelineStateCreateInfo& CreateInfo,
                          const TNameToGroupIndexMap&              NameToGroupIndex,
                          Uint8*                                   ShaderData,
                          Uint32                                   ShaderIdentifierSize)
{
    CComPtr<ID3D12StateObjectProperties> pStateObjectProperties;

    auto hr = pSO->QueryInterface(IID_PPV_ARGS(&pStateObjectProperties));
    if (FAILED(hr))
        LOG_ERROR_AND_THROW("Failed to get state object properties");

    for (Uint32 i = 0; i < CreateInfo.GeneralShaderCount; ++i)
    {
        const auto& GeneralShader = CreateInfo.pGeneralShaders[i];

        auto iter = NameToGroupIndex.find(GeneralShader.Name);
        VERIFY(iter != NameToGroupIndex.end(),
               "Can't find general shader '", GeneralShader.Name,
               "'. This looks to be a bug as NameToGroupIndex is initialized by "
               "CopyRTShaderGroupNames() that processes the same general shaders.");

        const auto* ShaderID = pStateObjectProperties->GetShaderIdentifier(WidenString(GeneralShader.Name).c_str());
        if (ShaderID == nullptr)
            LOG_ERROR_AND_THROW("Failed to get shader identifier for general shader group '", GeneralShader.Name, "'");

        std::memcpy(&ShaderData[ShaderIdentifierSize * iter->second], ShaderID, ShaderIdentifierSize);
    }

    for (Uint32 i = 0; i < CreateInfo.TriangleHitShaderCount; ++i)
    {
        const auto& TriHitShader = CreateInfo.pTriangleHitShaders[i];

        auto iter = NameToGroupIndex.find(TriHitShader.Name);
        VERIFY(iter != NameToGroupIndex.end(),
               "Can't find triangle hit group '", TriHitShader.Name,
               "'. This looks to be a bug as NameToGroupIndex is initialized by "
               "CopyRTShaderGroupNames() that processes the same hit groups.");

        const auto* ShaderID = pStateObjectProperties->GetShaderIdentifier(WidenString(TriHitShader.Name).c_str());
        if (ShaderID == nullptr)
            LOG_ERROR_AND_THROW("Failed to get shader identifier for triangle hit group '", TriHitShader.Name, "'");

        std::memcpy(&ShaderData[ShaderIdentifierSize * iter->second], ShaderID, ShaderIdentifierSize);
    }

    for (Uint32 i = 0; i < CreateInfo.ProceduralHitShaderCount; ++i)
    {
        const auto& ProcHitShader = CreateInfo.pProceduralHitShaders[i];

        auto iter = NameToGroupIndex.find(ProcHitShader.Name);
        VERIFY(iter != NameToGroupIndex.end(),
               "Can't find procedural hit group '", ProcHitShader.Name,
               "'. This looks to be a bug as NameToGroupIndex is initialized by "
               "CopyRTShaderGroupNames() that processes the same hit groups.");

        const auto* ShaderID = pStateObjectProperties->GetShaderIdentifier(WidenString(ProcHitShader.Name).c_str());
        if (ShaderID == nullptr)
            LOG_ERROR_AND_THROW("Failed to get shader identifier for procedural hit shader group '", ProcHitShader.Name, "'");

        std::memcpy(&ShaderData[ShaderIdentifierSize * iter->second], ShaderID, ShaderIdentifierSize);
    }
}

} // namespace


PipelineStateD3D12Impl::ShaderStageInfo::ShaderStageInfo(const ShaderD3D12Impl* _pShader) :
    Type{_pShader->GetDesc().ShaderType},
    Shaders{_pShader},
    ByteCodes{_pShader->GetD3DBytecode()}
{
}

void PipelineStateD3D12Impl::ShaderStageInfo::Append(const ShaderD3D12Impl* pShader)
{
    VERIFY_EXPR(pShader != nullptr);
    VERIFY(std::find(Shaders.begin(), Shaders.end(), pShader) == Shaders.end(),
           "Shader '", pShader->GetDesc().Name, "' already exists in the stage. Shaders must be deduplicated.");

    const auto NewShaderType = pShader->GetDesc().ShaderType;
    if (Type == SHADER_TYPE_UNKNOWN)
    {
        VERIFY_EXPR(Shaders.empty());
        Type = NewShaderType;
    }
    else
    {
        VERIFY(Type == NewShaderType, "The type (", GetShaderTypeLiteralName(NewShaderType),
               ") of shader '", pShader->GetDesc().Name, "' being added to the stage is inconsistent with the stage type (",
               GetShaderTypeLiteralName(Type), ").");
    }

    Shaders.push_back(pShader);
    ByteCodes.push_back(pShader->GetD3DBytecode());
}

size_t PipelineStateD3D12Impl::ShaderStageInfo::Count() const
{
    VERIFY_EXPR(Shaders.size() == ByteCodes.size());
    return Shaders.size();
}


PipelineResourceSignatureDescWrapper PipelineStateD3D12Impl::GetDefaultResourceSignatureDesc(
    const TShaderStages&              ShaderStages,
    const char*                       PSOName,
    const PipelineResourceLayoutDesc& ResourceLayout,
    Uint32                            SRBAllocationGranularity,
    const LocalRootSignatureD3D12*    pLocalRootSig) noexcept(false)
{
    PipelineResourceSignatureDescWrapper SignDesc{PSOName, ResourceLayout, SRBAllocationGranularity};

    std::unordered_map<ShaderResourceHashKey, const D3DShaderResourceAttribs&, ShaderResourceHashKey::Hasher> UniqueResources;
    for (auto& Stage : ShaderStages)
    {
        for (auto* pShader : Stage.Shaders)
        {
            const auto& ShaderResources = *pShader->GetShaderResources();

            ShaderResources.ProcessResources(
                [&](const D3DShaderResourceAttribs& Attribs, Uint32) //
                {
                    if (pLocalRootSig != nullptr && pLocalRootSig->IsShaderRecord(Attribs))
                        return;

                    const char* const SamplerSuffix =
                        (ShaderResources.IsUsingCombinedTextureSamplers() && Attribs.GetInputType() == D3D_SIT_SAMPLER) ?
                        ShaderResources.GetCombinedSamplerSuffix() :
                        nullptr;

                    const auto VarDesc = FindPipelineResourceLayoutVariable(ResourceLayout, Attribs.Name, Stage.Type, SamplerSuffix);
                    // Note that Attribs.Name != VarDesc.Name for combined samplers
                    const auto it_assigned = UniqueResources.emplace(ShaderResourceHashKey{VarDesc.ShaderStages, Attribs.Name}, Attribs);
                    if (it_assigned.second)
                    {
                        if (Attribs.BindCount == 0)
                        {
                            LOG_ERROR_AND_THROW("Resource '", Attribs.Name, "' in shader '", pShader->GetDesc().Name, "' is a runtime-sized array. ",
                                                "Use explicit resource signature to specify the array size.");
                        }

                        const auto ResType  = Attribs.GetShaderResourceType();
                        const auto ResFlags = Attribs.GetPipelineResourceFlags() | ShaderVariableFlagsToPipelineResourceFlags(VarDesc.Flags);
                        SignDesc.AddResource(VarDesc.ShaderStages, Attribs.Name, Attribs.BindCount, ResType, VarDesc.Type, ResFlags);
                    }
                    else
                    {
                        VerifyD3DResourceMerge(PSOName, it_assigned.first->second, Attribs);
                    }
                } //
            );

            // merge combined sampler suffixes
            if (ShaderResources.IsUsingCombinedTextureSamplers() && ShaderResources.GetNumSamplers() > 0)
            {
                SignDesc.SetCombinedSamplerSuffix(ShaderResources.GetCombinedSamplerSuffix());
            }
        }
    }

    return SignDesc;
}

void PipelineStateD3D12Impl::RemapOrVerifyShaderResources(TShaderStages&                                           ShaderStages,
                                                          const RefCntAutoPtr<PipelineResourceSignatureD3D12Impl>* pSignatures,
                                                          Uint32                                                   SignatureCount,
                                                          const RootSignatureD3D12&                                RootSig,
                                                          IDXCompiler*                                             pDxCompiler,
                                                          LocalRootSignatureD3D12*                                 pLocalRootSig,
                                                          const TValidateShaderResourcesFn&                        ValidateShaderResourcesFn,
                                                          const TValidateShaderBindingsFn&                         VlidateBindingsFn) noexcept(false)
{
    for (size_t s = 0; s < ShaderStages.size(); ++s)
    {
        const auto& Shaders    = ShaderStages[s].Shaders;
        auto&       ByteCodes  = ShaderStages[s].ByteCodes;
        const auto  ShaderType = ShaderStages[s].Type;

        bool                  HasImtblSamArray = false;
        ResourceBinding::TMap ResourceMap;
        // Note that we must use signatures from m_ResourceSignatures for resource binding map,
        // because signatures from RootSig may have resources with different names.
        for (Uint32 sign = 0; sign < SignatureCount; ++sign)
        {
            const PipelineResourceSignatureD3D12Impl* const pSignature = pSignatures[sign];
            if (pSignature == nullptr)
                continue;

            VERIFY_EXPR(pSignature->GetDesc().BindingIndex == sign);
            pSignature->UpdateShaderResourceBindingMap(ResourceMap, ShaderType, RootSig.GetBaseRegisterSpace(sign));

            if (pSignature->HasImmutableSamplerArray(ShaderType))
                HasImtblSamArray = true;
        }

        if (pLocalRootSig != nullptr && pLocalRootSig->IsDefined())
        {
            ResourceBinding::BindInfo BindInfo //
                {
                    pLocalRootSig->GetShaderRegister(),
                    pLocalRootSig->GetRegisterSpace(),
                    1,
                    SHADER_RESOURCE_TYPE_CONSTANT_BUFFER //
                };
            bool IsUnique = ResourceMap.emplace(HashMapStringKey{pLocalRootSig->GetName()}, BindInfo).second;
            if (!IsUnique)
                LOG_ERROR_AND_THROW("Shader record constant buffer already exists in the resource signature");
        }

        for (size_t i = 0; i < Shaders.size(); ++i)
        {
            const auto* const pShader = Shaders[i];

            Uint32 VerMajor, VerMinor;
            pShader->GetShaderResources()->GetShaderModel(VerMajor, VerMinor);
            const bool IsSM51orAbove = ((VerMajor == 5 && VerMinor >= 1) || VerMajor >= 6);

            if (HasImtblSamArray && IsSM51orAbove)
            {
                LOG_ERROR_AND_THROW("One of resource signatures uses immutable sampler array that is not allowed in shader model 5.1 and above.");
            }

            if (RootSig.GetTotalSpaces() > 1 && !IsSM51orAbove)
            {
                LOG_ERROR_AND_THROW("Shader '", pShader->GetDesc().Name,
                                    "' is compiled using SM5.0 or below that only supports single register space. "
                                    "Compile the shader using SM5.1+ or change the resource layout to use only one space.");
            }

            // Validate resources before remapping
            if (ValidateShaderResourcesFn)
                ValidateShaderResourcesFn(pShader, pLocalRootSig);

            if (VlidateBindingsFn)
            {
                VlidateBindingsFn(pShader, ResourceMap);
            }
            else
            {
                auto& pBytecode = ByteCodes[i];

                CComPtr<ID3DBlob> pBlob;
                if (IsDXILBytecode(pBytecode->GetBufferPointer(), pBytecode->GetBufferSize()))
                {
                    if (!pDxCompiler)
                        LOG_ERROR_AND_THROW("DXC compiler does not exists, can not remap resource bindings");

                    if (!pDxCompiler->RemapResourceBindings(ResourceMap, reinterpret_cast<IDxcBlob*>(pBytecode.p), reinterpret_cast<IDxcBlob**>(&pBlob)))
                        LOG_ERROR_AND_THROW("Failed to remap resource bindings in shader '", pShader->GetDesc().Name, "'.");
                }
                else
                {
                    D3DCreateBlob(pBytecode->GetBufferSize(), &pBlob);
                    memcpy(pBlob->GetBufferPointer(), pBytecode->GetBufferPointer(), pBytecode->GetBufferSize());

                    if (!DXBCUtils::RemapResourceBindings(ResourceMap, pBlob->GetBufferPointer(), pBlob->GetBufferSize()))
                        LOG_ERROR_AND_THROW("Failed to remap resource bindings in shader '", pShader->GetDesc().Name, "'.");
                }
                pBytecode = pBlob;
            }
        }
    }
}

void PipelineStateD3D12Impl::InitRootSignature(const PipelineStateCreateInfo& CreateInfo,
                                               TShaderStages&                 ShaderStages,
                                               LocalRootSignatureD3D12*       pLocalRootSig) noexcept(false)
{
    const auto InternalFlags = GetInternalCreateFlags(CreateInfo);
    if (m_UsingImplicitSignature && (InternalFlags & PSO_CREATE_INTERNAL_FLAG_IMPLICIT_SIGNATURE0) == 0)
    {
        const auto SignDesc = GetDefaultResourceSignatureDesc(ShaderStages, m_Desc.Name, m_Desc.ResourceLayout, m_Desc.SRBAllocationGranularity, pLocalRootSig);

        // Always initialize default resource signature as internal device object.
        // This is necessary to avoid cyclic references from GenerateMips.
        // This may never be a problem as the PSO keeps the reference to the device if necessary.
        constexpr bool bIsDeviceInternal = true;
        InitDefaultSignature(SignDesc, GetActiveShaderStages(), bIsDeviceInternal);
        VERIFY_EXPR(m_Signatures[0]);
    }

    m_RootSig = GetDevice()->GetRootSignatureCache().GetRootSig(m_Signatures, m_SignatureCount);
    if (!m_RootSig)
        LOG_ERROR_AND_THROW("Failed to create root signature for pipeline '", m_Desc.Name, "'.");

    if (pLocalRootSig != nullptr && pLocalRootSig->IsDefined())
    {
        if (!pLocalRootSig->Create(GetDevice()->GetD3D12Device(), m_RootSig->GetTotalSpaces()))
            LOG_ERROR_AND_THROW("Failed to create local root signature for pipeline '", m_Desc.Name, "'.");
    }

    const auto RemapResources = (CreateInfo.Flags & PSO_CREATE_FLAG_DONT_REMAP_SHADER_RESOURCES) == 0;

    const auto ValidateBindings = [this](const ShaderD3D12Impl* pShader, const ResourceBinding::TMap& BindingsMap) //
    {
        ValidateShaderResourceBindings(m_Desc.Name, *pShader->GetShaderResources(), BindingsMap);
    };
    const auto ValidateBindingsFn = !RemapResources && (InternalFlags & PSO_CREATE_INTERNAL_FLAG_NO_SHADER_REFLECTION) == 0 ?
        TValidateShaderBindingsFn{ValidateBindings} :
        TValidateShaderBindingsFn{nullptr};

    // Verify that pipeline layout is compatible with shader resources and remap resource bindings.
    if (RemapResources || ValidateBindingsFn)
    {
        RemapOrVerifyShaderResources(
            ShaderStages,
            m_Signatures,
            m_SignatureCount,
            *m_RootSig,
            GetDevice()->GetDxCompiler(),
            pLocalRootSig,
            [this](const ShaderD3D12Impl* pShader, const LocalRootSignatureD3D12* pLocalRootSig) {
                ValidateShaderResources(pShader, pLocalRootSig);
            },
            ValidateBindingsFn);
    }
}

void PipelineStateD3D12Impl::ValidateShaderResources(const ShaderD3D12Impl* pShader, const LocalRootSignatureD3D12* pLocalRootSig)
{
    const auto& pShaderResources = pShader->GetShaderResources();
    const auto  ShaderType       = pShader->GetDesc().ShaderType;

#ifdef DILIGENT_DEVELOPMENT
    m_ShaderResources.emplace_back(pShaderResources);
#endif

    // Check compatibility between shader resources and resource signature.
    pShaderResources->ProcessResources(
        [&](const D3DShaderResourceAttribs& Attribs, Uint32) //
        {
#ifdef DILIGENT_DEVELOPMENT
            m_ResourceAttibutions.emplace_back();
            auto& ResAttribution = m_ResourceAttibutions.back();
#else
            ResourceAttribution ResAttribution;
#endif

            if (pLocalRootSig != nullptr && pLocalRootSig->IsShaderRecord(Attribs))
                return;

            const auto IsSampler = Attribs.GetInputType() == D3D_SIT_SAMPLER;
            if (IsSampler && pShaderResources->IsUsingCombinedTextureSamplers())
                return;

            ResAttribution = GetResourceAttribution(Attribs.Name, ShaderType);
            if (!ResAttribution)
            {
                LOG_ERROR_AND_THROW("Shader '", pShader->GetDesc().Name, "' contains resource '", Attribs.Name,
                                    "' that is not present in any pipeline resource signature used to create pipeline state '",
                                    m_Desc.Name, "'.");
            }

            const auto ResType  = Attribs.GetShaderResourceType();
            const auto ResFlags = Attribs.GetPipelineResourceFlags();

            const auto* const pSignature = ResAttribution.pSignature;
            VERIFY_EXPR(pSignature != nullptr);

            if (ResAttribution.ResourceIndex != ResourceAttribution::InvalidResourceIndex)
            {
                auto ResDesc = pSignature->GetResourceDesc(ResAttribution.ResourceIndex);
                if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT)
                    ResDesc.ResourceType = SHADER_RESOURCE_TYPE_TEXTURE_SRV;
                ValidatePipelineResourceCompatibility(ResDesc, ResType, ResFlags, Attribs.BindCount,
                                                      pShader->GetDesc().Name, pSignature->GetDesc().Name);
            }
            else if (ResAttribution.ImmutableSamplerIndex != ResourceAttribution::InvalidResourceIndex)
            {
                if (ResType != SHADER_RESOURCE_TYPE_SAMPLER)
                {
                    LOG_ERROR_AND_THROW("Shader '", pShader->GetDesc().Name, "' contains resource with name '", Attribs.Name,
                                        "' and type '", GetShaderResourceTypeLiteralName(ResType),
                                        "' that is not compatible with immutable sampler defined in pipeline resource signature '",
                                        pSignature->GetDesc().Name, "'.");
                }
            }
            else
            {
                UNEXPECTED("Either immutable sampler or resource index should be valid");
            }
        } //
    );
}

#ifdef DILIGENT_DEVELOPMENT
void PipelineStateD3D12Impl::DvpVerifySRBResources(const DeviceContextD3D12Impl*       pDeviceCtx,
                                                   const ShaderResourceCacheArrayType& ResourceCaches) const
{
    auto attrib_it = m_ResourceAttibutions.begin();
    for (const auto& pResources : m_ShaderResources)
    {
        pResources->ProcessResources(
            [&](const D3DShaderResourceAttribs& Attribs, Uint32) //
            {
                if (*attrib_it && !attrib_it->IsImmutableSampler())
                {
                    VERIFY_EXPR(attrib_it->pSignature != nullptr);
                    VERIFY_EXPR(attrib_it->pSignature->GetDesc().BindingIndex == attrib_it->SignatureIndex);
                    const auto* pResourceCache = ResourceCaches[attrib_it->SignatureIndex];
                    DEV_CHECK_ERR(pResourceCache != nullptr, "Resource cache at index ", attrib_it->SignatureIndex, " is null.");
                    attrib_it->pSignature->DvpValidateCommittedResource(pDeviceCtx, Attribs, attrib_it->ResourceIndex, *pResourceCache,
                                                                        pResources->GetShaderName(), m_Desc.Name);
                }
                ++attrib_it;
            } //
        );
    }
    VERIFY_EXPR(attrib_it == m_ResourceAttibutions.end());
}

#endif // DILIGENT_DEVELOPMENT


template <typename PSOCreateInfoType>
void PipelineStateD3D12Impl::InitInternalObjects(const PSOCreateInfoType& CreateInfo,
                                                 TShaderStages&           ShaderStages,
                                                 LocalRootSignatureD3D12* pLocalRootSig) noexcept(false)
{
    ExtractShaders<ShaderD3D12Impl>(CreateInfo, ShaderStages);

    FixedLinearAllocator MemPool{GetRawAllocator()};

    ReserveSpaceForPipelineDesc(CreateInfo, MemPool);

    MemPool.Reserve();

    InitializePipelineDesc(CreateInfo, MemPool);

    // It is important to construct all objects before initializing them because if an exception is thrown,
    // destructors will be called for all objects

    InitRootSignature(CreateInfo, ShaderStages, pLocalRootSig);
}


PipelineStateD3D12Impl::PipelineStateD3D12Impl(IReferenceCounters*                    pRefCounters,
                                               RenderDeviceD3D12Impl*                 pDeviceD3D12,
                                               const GraphicsPipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pDeviceD3D12, CreateInfo}
{
    try
    {
        const auto WName = WidenString(m_Desc.Name);

        TShaderStages ShaderStages;
        InitInternalObjects(CreateInfo, ShaderStages);

        auto* pd3d12Device = pDeviceD3D12->GetD3D12Device();
        if (m_Desc.PipelineType == PIPELINE_TYPE_GRAPHICS)
        {
            const auto& GraphicsPipeline = GetGraphicsPipelineDesc();

            D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12PSODesc = {};

            for (const auto& Stage : ShaderStages)
            {
                VERIFY_EXPR(Stage.Count() == 1);
                const auto& pByteCode = Stage.ByteCodes[0];

                D3D12_SHADER_BYTECODE* pd3d12ShaderBytecode = nullptr;
                switch (Stage.Type)
                {
                    // clang-format off
                    case SHADER_TYPE_VERTEX:   pd3d12ShaderBytecode = &d3d12PSODesc.VS; break;
                    case SHADER_TYPE_PIXEL:    pd3d12ShaderBytecode = &d3d12PSODesc.PS; break;
                    case SHADER_TYPE_GEOMETRY: pd3d12ShaderBytecode = &d3d12PSODesc.GS; break;
                    case SHADER_TYPE_HULL:     pd3d12ShaderBytecode = &d3d12PSODesc.HS; break;
                    case SHADER_TYPE_DOMAIN:   pd3d12ShaderBytecode = &d3d12PSODesc.DS; break;
                    // clang-format on
                    default: UNEXPECTED("Unexpected shader type");
                }

                pd3d12ShaderBytecode->pShaderBytecode = pByteCode->GetBufferPointer();
                pd3d12ShaderBytecode->BytecodeLength  = pByteCode->GetBufferSize();
            }

            d3d12PSODesc.pRootSignature = m_RootSig->GetD3D12RootSignature();

            memset(&d3d12PSODesc.StreamOutput, 0, sizeof(d3d12PSODesc.StreamOutput));

            BlendStateDesc_To_D3D12_BLEND_DESC(GraphicsPipeline.BlendDesc, d3d12PSODesc.BlendState);
            // The sample mask for the blend state.
            d3d12PSODesc.SampleMask = GraphicsPipeline.SampleMask;

            RasterizerStateDesc_To_D3D12_RASTERIZER_DESC(GraphicsPipeline.RasterizerDesc, d3d12PSODesc.RasterizerState);
            DepthStencilStateDesc_To_D3D12_DEPTH_STENCIL_DESC(GraphicsPipeline.DepthStencilDesc, d3d12PSODesc.DepthStencilState);

            std::vector<D3D12_INPUT_ELEMENT_DESC, STDAllocatorRawMem<D3D12_INPUT_ELEMENT_DESC>> d312InputElements(STD_ALLOCATOR_RAW_MEM(D3D12_INPUT_ELEMENT_DESC, GetRawAllocator(), "Allocator for vector<D3D12_INPUT_ELEMENT_DESC>"));

            const auto& InputLayout = GetGraphicsPipelineDesc().InputLayout;
            if (InputLayout.NumElements > 0)
            {
                LayoutElements_To_D3D12_INPUT_ELEMENT_DESCs(InputLayout, d312InputElements);
                d3d12PSODesc.InputLayout.NumElements        = static_cast<UINT>(d312InputElements.size());
                d3d12PSODesc.InputLayout.pInputElementDescs = d312InputElements.data();
            }
            else
            {
                d3d12PSODesc.InputLayout.NumElements        = 0;
                d3d12PSODesc.InputLayout.pInputElementDescs = nullptr;
            }

            d3d12PSODesc.IBStripCutValue = (GraphicsPipeline.PrimitiveTopology == PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP ||
                                            GraphicsPipeline.PrimitiveTopology == PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_ADJ ||
                                            GraphicsPipeline.PrimitiveTopology == PRIMITIVE_TOPOLOGY_LINE_STRIP ||
                                            GraphicsPipeline.PrimitiveTopology == PRIMITIVE_TOPOLOGY_LINE_STRIP_ADJ) ?
                D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF :
                D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
            static const PrimitiveTopology_To_D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimTopologyToD3D12TopologyType;
            d3d12PSODesc.PrimitiveTopologyType = PrimTopologyToD3D12TopologyType[GraphicsPipeline.PrimitiveTopology];

            d3d12PSODesc.NumRenderTargets = GraphicsPipeline.NumRenderTargets;
            for (Uint32 rt = 0; rt < GraphicsPipeline.NumRenderTargets; ++rt)
                d3d12PSODesc.RTVFormats[rt] = TexFormatToDXGI_Format(GraphicsPipeline.RTVFormats[rt]);
            for (Uint32 rt = GraphicsPipeline.NumRenderTargets; rt < _countof(d3d12PSODesc.RTVFormats); ++rt)
                d3d12PSODesc.RTVFormats[rt] = DXGI_FORMAT_UNKNOWN;
            d3d12PSODesc.DSVFormat = TexFormatToDXGI_Format(GraphicsPipeline.DSVFormat);

            d3d12PSODesc.SampleDesc.Count   = GraphicsPipeline.SmplDesc.Count;
            d3d12PSODesc.SampleDesc.Quality = GraphicsPipeline.SmplDesc.Quality;

            // For single GPU operation, set this to zero. If there are multiple GPU nodes,
            // set bits to identify the nodes (the device's physical adapters) for which the
            // graphics pipeline state is to apply. Each bit in the mask corresponds to a single node.
            d3d12PSODesc.NodeMask = 0;

            d3d12PSODesc.CachedPSO.pCachedBlob           = nullptr;
            d3d12PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;

            // The only valid bit is D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG, which can only be set on WARP devices.
            d3d12PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

            // Try to load from the cache
            auto* const pPSOCacheD3D12 = ClassPtrCast<PipelineStateCacheD3D12Impl>(CreateInfo.pPSOCache);
            if (pPSOCacheD3D12 != nullptr && !WName.empty())
                m_pd3d12PSO = pPSOCacheD3D12->LoadGraphicsPipeline(WName.c_str(), d3d12PSODesc);
            if (!m_pd3d12PSO)
            {
                // Note: renderdoc frame capture fails if any interface but IID_ID3D12PipelineState is requested
                HRESULT hr = pd3d12Device->CreateGraphicsPipelineState(&d3d12PSODesc, __uuidof(ID3D12PipelineState), IID_PPV_ARGS_Helper(&m_pd3d12PSO));
                if (FAILED(hr))
                    LOG_ERROR_AND_THROW("Failed to create pipeline state");

                // Add to the cache
                if (pPSOCacheD3D12 != nullptr && !WName.empty())
                    pPSOCacheD3D12->StorePipeline(WName.c_str(), m_pd3d12PSO);
            }
        }
#ifdef D3D12_H_HAS_MESH_SHADER
        else if (m_Desc.PipelineType == PIPELINE_TYPE_MESH)
        {
            const auto& GraphicsPipeline = GetGraphicsPipelineDesc();

            struct MESH_SHADER_PIPELINE_STATE_DESC
            {
                PSS_SubObject<D3D12_PIPELINE_STATE_FLAGS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS>            Flags;
                PSS_SubObject<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK>                              NodeMask;
                PSS_SubObject<ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE>         pRootSignature;
                PSS_SubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>                    PS;
                PSS_SubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS>                    AS;
                PSS_SubObject<D3D12_SHADER_BYTECODE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS>                    MS;
                PSS_SubObject<D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND>                      BlendState;
                PSS_SubObject<D3D12_DEPTH_STENCIL_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL>      DepthStencilState;
                PSS_SubObject<D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER>            RasterizerState;
                PSS_SubObject<DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC>                SampleDesc;
                PSS_SubObject<UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK>                            SampleMask;
                PSS_SubObject<DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT>            DSVFormat;
                PSS_SubObject<D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS> RTVFormatArray;
                PSS_SubObject<D3D12_CACHED_PIPELINE_STATE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO>      CachedPSO;
            };
            MESH_SHADER_PIPELINE_STATE_DESC d3d12PSODesc = {};

            for (const auto& Stage : ShaderStages)
            {
                VERIFY_EXPR(Stage.Count() == 1);
                const auto& pByteCode = Stage.ByteCodes[0];

                D3D12_SHADER_BYTECODE* pd3d12ShaderBytecode = nullptr;
                switch (Stage.Type)
                {
                    // clang-format off
                    case SHADER_TYPE_AMPLIFICATION: pd3d12ShaderBytecode = &d3d12PSODesc.AS; break;
                    case SHADER_TYPE_MESH:          pd3d12ShaderBytecode = &d3d12PSODesc.MS; break;
                    case SHADER_TYPE_PIXEL:         pd3d12ShaderBytecode = &d3d12PSODesc.PS; break;
                    // clang-format on
                    default: UNEXPECTED("Unexpected shader type");
                }

                pd3d12ShaderBytecode->pShaderBytecode = pByteCode->GetBufferPointer();
                pd3d12ShaderBytecode->BytecodeLength  = pByteCode->GetBufferSize();
            }

            d3d12PSODesc.pRootSignature = m_RootSig->GetD3D12RootSignature();

            BlendStateDesc_To_D3D12_BLEND_DESC(GraphicsPipeline.BlendDesc, *d3d12PSODesc.BlendState);
            d3d12PSODesc.SampleMask = GraphicsPipeline.SampleMask;

            RasterizerStateDesc_To_D3D12_RASTERIZER_DESC(GraphicsPipeline.RasterizerDesc, *d3d12PSODesc.RasterizerState);
            DepthStencilStateDesc_To_D3D12_DEPTH_STENCIL_DESC(GraphicsPipeline.DepthStencilDesc, *d3d12PSODesc.DepthStencilState);

            d3d12PSODesc.RTVFormatArray->NumRenderTargets = GraphicsPipeline.NumRenderTargets;
            for (Uint32 rt = 0; rt < GraphicsPipeline.NumRenderTargets; ++rt)
                d3d12PSODesc.RTVFormatArray->RTFormats[rt] = TexFormatToDXGI_Format(GraphicsPipeline.RTVFormats[rt]);
            for (Uint32 rt = GraphicsPipeline.NumRenderTargets; rt < _countof(d3d12PSODesc.RTVFormatArray->RTFormats); ++rt)
                d3d12PSODesc.RTVFormatArray->RTFormats[rt] = DXGI_FORMAT_UNKNOWN;
            d3d12PSODesc.DSVFormat = TexFormatToDXGI_Format(GraphicsPipeline.DSVFormat);

            d3d12PSODesc.SampleDesc->Count   = GraphicsPipeline.SmplDesc.Count;
            d3d12PSODesc.SampleDesc->Quality = GraphicsPipeline.SmplDesc.Quality;

            // For single GPU operation, set this to zero. If there are multiple GPU nodes,
            // set bits to identify the nodes (the device's physical adapters) for which the
            // graphics pipeline state is to apply. Each bit in the mask corresponds to a single node.
            d3d12PSODesc.NodeMask = 0;

            d3d12PSODesc.CachedPSO->pCachedBlob           = nullptr;
            d3d12PSODesc.CachedPSO->CachedBlobSizeInBytes = 0;

            // The only valid bit is D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG, which can only be set on WARP devices.
            d3d12PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

            D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
            streamDesc.SizeInBytes                   = sizeof(d3d12PSODesc);
            streamDesc.pPipelineStateSubobjectStream = &d3d12PSODesc;

            auto* pd3d12Device2 = pDeviceD3D12->GetD3D12Device2();
            // Note: renderdoc frame capture fails if any interface but IID_ID3D12PipelineState is requested
            HRESULT hr = pd3d12Device2->CreatePipelineState(&streamDesc, __uuidof(ID3D12PipelineState), IID_PPV_ARGS_Helper(&m_pd3d12PSO));
            if (FAILED(hr))
                LOG_ERROR_AND_THROW("Failed to create pipeline state");
        }
#endif // D3D12_H_HAS_MESH_SHADER
        else
        {
            LOG_ERROR_AND_THROW("Unsupported pipeline type");
        }

        if (!WName.empty())
        {
            m_pd3d12PSO->SetName(WName.c_str());
        }
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateD3D12Impl::PipelineStateD3D12Impl(IReferenceCounters*                   pRefCounters,
                                               RenderDeviceD3D12Impl*                pDeviceD3D12,
                                               const ComputePipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pDeviceD3D12, CreateInfo}
{
    try
    {
        TShaderStages ShaderStages;
        InitInternalObjects(CreateInfo, ShaderStages);

        auto* pd3d12Device = pDeviceD3D12->GetD3D12Device();

        D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12PSODesc = {};

        VERIFY_EXPR(ShaderStages[0].Type == SHADER_TYPE_COMPUTE);
        VERIFY_EXPR(ShaderStages[0].Count() == 1);
        const auto& pByteCode           = ShaderStages[0].ByteCodes[0];
        d3d12PSODesc.CS.pShaderBytecode = pByteCode->GetBufferPointer();
        d3d12PSODesc.CS.BytecodeLength  = pByteCode->GetBufferSize();

        // For single GPU operation, set this to zero. If there are multiple GPU nodes,
        // set bits to identify the nodes (the device's physical adapters) for which the
        // graphics pipeline state is to apply. Each bit in the mask corresponds to a single node.
        d3d12PSODesc.NodeMask = 0;

        d3d12PSODesc.CachedPSO.pCachedBlob           = nullptr;
        d3d12PSODesc.CachedPSO.CachedBlobSizeInBytes = 0;

        // The only valid bit is D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG, which can only be set on WARP devices.
        d3d12PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        d3d12PSODesc.pRootSignature = m_RootSig->GetD3D12RootSignature();

        // Try to load from the cache
        const auto  WName          = WidenString(m_Desc.Name);
        auto* const pPSOCacheD3D12 = ClassPtrCast<PipelineStateCacheD3D12Impl>(CreateInfo.pPSOCache);
        if (pPSOCacheD3D12 != nullptr && !WName.empty())
            m_pd3d12PSO = pPSOCacheD3D12->LoadComputePipeline(WName.c_str(), d3d12PSODesc);
        if (!m_pd3d12PSO)
        {
            // Note: renderdoc frame capture fails if any interface but IID_ID3D12PipelineState is requested
            HRESULT hr = pd3d12Device->CreateComputePipelineState(&d3d12PSODesc, __uuidof(ID3D12PipelineState), IID_PPV_ARGS_Helper(&m_pd3d12PSO));
            if (FAILED(hr))
                LOG_ERROR_AND_THROW("Failed to create pipeline state");

            // Add to the cache
            if (pPSOCacheD3D12 != nullptr && !WName.empty())
                pPSOCacheD3D12->StorePipeline(WName.c_str(), m_pd3d12PSO);
        }

        if (!WName.empty())
        {
            m_pd3d12PSO->SetName(WName.c_str());
        }
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateD3D12Impl::PipelineStateD3D12Impl(IReferenceCounters*                      pRefCounters,
                                               RenderDeviceD3D12Impl*                   pDeviceD3D12,
                                               const RayTracingPipelineStateCreateInfo& CreateInfo) :
    TPipelineStateBase{pRefCounters, pDeviceD3D12, CreateInfo}
{
    try
    {
        LocalRootSignatureD3D12 LocalRootSig{CreateInfo.pShaderRecordName, CreateInfo.RayTracingPipeline.ShaderRecordSize};
        TShaderStages           ShaderStages;
        InitInternalObjects(CreateInfo, ShaderStages, &LocalRootSig);

        auto* pd3d12Device = pDeviceD3D12->GetD3D12Device5();

        DynamicLinearAllocator             TempPool{GetRawAllocator(), 4 << 10};
        std::vector<D3D12_STATE_SUBOBJECT> Subobjects;
        BuildRTPipelineDescription(CreateInfo, Subobjects, TempPool, ShaderStages);

        D3D12_GLOBAL_ROOT_SIGNATURE GlobalRoot = {m_RootSig->GetD3D12RootSignature()};
        Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &GlobalRoot});

        D3D12_LOCAL_ROOT_SIGNATURE LocalRoot = {LocalRootSig.GetD3D12RootSignature()};
        if (LocalRoot.pLocalRootSignature)
            Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, &LocalRoot});

        D3D12_STATE_OBJECT_DESC RTPipelineDesc = {};
        RTPipelineDesc.Type                    = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        RTPipelineDesc.NumSubobjects           = static_cast<UINT>(Subobjects.size());
        RTPipelineDesc.pSubobjects             = Subobjects.data();

        HRESULT hr = pd3d12Device->CreateStateObject(&RTPipelineDesc, __uuidof(ID3D12StateObject), IID_PPV_ARGS_Helper(&m_pd3d12PSO));
        if (FAILED(hr))
            LOG_ERROR_AND_THROW("Failed to create ray tracing state object");

        // Extract shader identifiers from ray tracing pipeline and store them in ShaderHandles
        GetShaderIdentifiers(m_pd3d12PSO, CreateInfo, m_pRayTracingPipelineData->NameToGroupIndex,
                             m_pRayTracingPipelineData->ShaderHandles, m_pRayTracingPipelineData->ShaderHandleSize);

        if (*m_Desc.Name != 0)
        {
            m_pd3d12PSO->SetName(WidenString(m_Desc.Name).c_str());
        }
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateD3D12Impl::~PipelineStateD3D12Impl()
{
    Destruct();
}

void PipelineStateD3D12Impl::Destruct()
{
    m_RootSig.Release();

    if (m_pd3d12PSO)
    {
        // D3D12 object can only be destroyed when it is no longer used by the GPU
        m_pDevice->SafeReleaseDeviceObject(std::move(m_pd3d12PSO), m_Desc.ImmediateContextMask);
    }

    TPipelineStateBase::Destruct();
}

bool PipelineStateD3D12Impl::IsCompatibleWith(const IPipelineState* pPSO) const
{
    DEV_CHECK_ERR(pPSO != nullptr, "pPSO must not be null");

    if (pPSO == this)
        return true;

    RefCntAutoPtr<PipelineStateD3D12Impl> pPSOImpl{const_cast<IPipelineState*>(pPSO), PipelineStateImplType::IID_InternalImpl};
    VERIFY(pPSOImpl, "Unknown PSO implementation type");

    bool IsCompatible = (m_RootSig == pPSOImpl->m_RootSig);
    VERIFY_EXPR(IsCompatible == TPipelineStateBase::IsCompatibleWith(pPSO));
    return IsCompatible;
}

} // namespace Diligent
