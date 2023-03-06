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
/// Implementation of the Diligent::ShaderBindingTableBase template class

#include <unordered_map>
#include <cstring>

#include "ShaderBindingTable.h"
#include "TopLevelASBase.hpp"
#include "DeviceObjectBase.hpp"
#include "RenderDeviceBase.hpp"
#include "StringPool.hpp"
#include "HashUtils.hpp"

namespace Diligent
{

/// Validates SBT description and throws an exception in case of an error.
void ValidateShaderBindingTableDesc(const ShaderBindingTableDesc& Desc, Uint32 ShaderGroupHandleSize, Uint32 MaxShaderRecordStride) noexcept(false);

/// Template class implementing base functionality of the shader binding table object.

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class ShaderBindingTableBase : public DeviceObjectBase<typename EngineImplTraits::ShaderBindingTableInterface, typename EngineImplTraits::RenderDeviceImplType, ShaderBindingTableDesc>
{
public:
    // Base interface this class inherits (IShaderBindingTableD3D12, IShaderBindingTableVk, etc.)
    using BaseInterface = typename EngineImplTraits::ShaderBindingTableInterface;

    // Render device implementation type (RenderDeviceD3D12Impl, RenderDeviceVkImpl, etc.).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    // Pipeline state implementation type (PipelineStateD3D12Impl, PipelineStateVkImpl, etc.).
    using PipelineStateImplType = typename EngineImplTraits::PipelineStateImplType;

    // Top-level AS implementation type (TopLevelASD3D12Impl, TopLevelASVkImpl, etc.).
    using TopLevelASImplType = typename EngineImplTraits::TopLevelASImplType;

    // Buffer implementation type (BufferVkImpl, BufferD3D12Impl, etc.).
    using BufferImplType = typename EngineImplTraits::BufferImplType;

    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, ShaderBindingTableDesc>;

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this SBT.
    /// \param pDevice           - Pointer to the device.
    /// \param Desc              - SBT description.
    /// \param bIsDeviceInternal - Flag indicating if the BLAS is an internal device object and
    ///							   must not keep a strong reference to the device.
    ShaderBindingTableBase(IReferenceCounters*           pRefCounters,
                           RenderDeviceImplType*         pDevice,
                           const ShaderBindingTableDesc& Desc,
                           bool                          bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, Desc, bIsDeviceInternal}
    {
        const auto& RTProps = this->GetDevice()->GetAdapterInfo().RayTracing;
        if (!this->GetDevice()->GetFeatures().RayTracing)
            LOG_ERROR_AND_THROW("Ray tracing is not supported by device");

        ValidateShaderBindingTableDesc(this->m_Desc, RTProps.ShaderGroupHandleSize, RTProps.MaxShaderRecordStride);

        this->m_pPSO               = ClassPtrCast<PipelineStateImplType>(this->m_Desc.pPSO);
        this->m_ShaderRecordSize   = this->m_pPSO->GetRayTracingPipelineDesc().ShaderRecordSize;
        this->m_ShaderRecordStride = this->m_ShaderRecordSize + RTProps.ShaderGroupHandleSize;
    }

    ~ShaderBindingTableBase()
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_ShaderBindingTable, TDeviceObjectBase)


    void DILIGENT_CALL_TYPE Reset(IPipelineState* pPSO) override final
    {
#ifdef DILIGENT_DEVELOPMENT
        this->m_DbgHitGroupBindings.clear();
#endif
        this->m_RayGenShaderRecord.clear();
        this->m_MissShadersRecord.clear();
        this->m_CallableShadersRecord.clear();
        this->m_HitGroupsRecord.clear();
        this->m_Changed = true;
        this->m_pPSO    = nullptr;

        this->m_Desc.pPSO = pPSO;

        const auto& RayTracingProps = this->GetDevice()->GetAdapterInfo().RayTracing;
        try
        {
            ValidateShaderBindingTableDesc(this->m_Desc, RayTracingProps.ShaderGroupHandleSize, RayTracingProps.MaxShaderRecordStride);
        }
        catch (const std::runtime_error&)
        {
            return;
        }

        this->m_pPSO               = ClassPtrCast<PipelineStateImplType>(this->m_Desc.pPSO);
        this->m_ShaderRecordSize   = this->m_pPSO->GetRayTracingPipelineDesc().ShaderRecordSize;
        this->m_ShaderRecordStride = this->m_ShaderRecordSize + RayTracingProps.ShaderGroupHandleSize;
    }


    void DILIGENT_CALL_TYPE ResetHitGroups() override final
    {
#ifdef DILIGENT_DEVELOPMENT
        this->m_DbgHitGroupBindings.clear();
#endif
        this->m_HitGroupsRecord.clear();
        this->m_Changed = true;
    }


    void DILIGENT_CALL_TYPE BindRayGenShader(const char* pShaderGroupName, const void* pData, Uint32 DataSize) override final
    {
        VERIFY_EXPR((pData == nullptr) == (DataSize == 0));
        VERIFY_EXPR((pData == nullptr) || (DataSize == this->m_ShaderRecordSize));

        this->m_RayGenShaderRecord.resize(this->m_ShaderRecordStride, Uint8{EmptyElem});
        this->m_pPSO->CopyShaderHandle(pShaderGroupName, this->m_RayGenShaderRecord.data(), this->m_ShaderRecordStride);

        const Uint32 GroupSize = this->GetDevice()->GetAdapterInfo().RayTracing.ShaderGroupHandleSize;
        std::memcpy(this->m_RayGenShaderRecord.data() + GroupSize, pData, DataSize);
        this->m_Changed = true;
    }


    void DILIGENT_CALL_TYPE BindMissShader(const char* pShaderGroupName, Uint32 MissIndex, const void* pData, Uint32 DataSize) override final
    {
        VERIFY_EXPR((pData == nullptr) == (DataSize == 0));
        VERIFY_EXPR((pData == nullptr) || (DataSize == this->m_ShaderRecordSize));

        const Uint32 GroupSize = this->GetDevice()->GetAdapterInfo().RayTracing.ShaderGroupHandleSize;
        const size_t Stride    = this->m_ShaderRecordStride;
        const size_t Offset    = MissIndex * Stride;
        this->m_MissShadersRecord.resize(std::max(this->m_MissShadersRecord.size(), Offset + Stride), Uint8{EmptyElem});

        this->m_pPSO->CopyShaderHandle(pShaderGroupName, this->m_MissShadersRecord.data() + Offset, Stride);
        std::memcpy(this->m_MissShadersRecord.data() + Offset + GroupSize, pData, DataSize);
        this->m_Changed = true;
    }


    void DILIGENT_CALL_TYPE BindHitGroupByIndex(Uint32      BindingIndex,
                                                const char* pShaderGroupName,
                                                const void* pData,
                                                Uint32      DataSize) override final
    {
        VERIFY_EXPR((pData == nullptr) == (DataSize == 0));
        VERIFY_EXPR((pData == nullptr) || (DataSize == this->m_ShaderRecordSize));

        const size_t Stride    = this->m_ShaderRecordStride;
        const Uint32 GroupSize = this->GetDevice()->GetAdapterInfo().RayTracing.ShaderGroupHandleSize;
        const size_t Offset    = BindingIndex * Stride;

        this->m_HitGroupsRecord.resize(std::max(this->m_HitGroupsRecord.size(), Offset + Stride), Uint8{EmptyElem});

        this->m_pPSO->CopyShaderHandle(pShaderGroupName, this->m_HitGroupsRecord.data() + Offset, Stride);
        std::memcpy(this->m_HitGroupsRecord.data() + Offset + GroupSize, pData, DataSize);
        this->m_Changed = true;

#ifdef DILIGENT_DEVELOPMENT
        OnBindHitGroup(nullptr, BindingIndex);
#endif
    }


    void DILIGENT_CALL_TYPE BindHitGroupForGeometry(ITopLevelAS* pTLAS,
                                                    const char*  pInstanceName,
                                                    const char*  pGeometryName,
                                                    Uint32       RayOffsetInHitGroupIndex,
                                                    const char*  pShaderGroupName,
                                                    const void*  pData,
                                                    Uint32       DataSize) override final
    {
        VERIFY_EXPR((pData == nullptr) == (DataSize == 0));
        VERIFY_EXPR((pData == nullptr) || (DataSize == this->m_ShaderRecordSize));
        VERIFY_EXPR(pTLAS != nullptr);

        auto* const pTLASImpl = ClassPtrCast<TopLevelASImplType>(pTLAS);
        const auto  Info      = pTLASImpl->GetBuildInfo();
        const auto  Desc      = pTLASImpl->GetInstanceDesc(pInstanceName);

        VERIFY_EXPR(Info.BindingMode == HIT_GROUP_BINDING_MODE_PER_GEOMETRY);
        VERIFY_EXPR(RayOffsetInHitGroupIndex < Info.HitGroupStride);
        VERIFY_EXPR(Desc.ContributionToHitGroupIndex != ~0u);
        VERIFY_EXPR(Desc.pBLAS != nullptr);

        const Uint32 InstanceOffset = Desc.ContributionToHitGroupIndex;
        const Uint32 GeometryIndex  = Desc.pBLAS->GetGeometryIndex(pGeometryName);
        VERIFY_EXPR(GeometryIndex != INVALID_INDEX);

        const Uint32 Index     = InstanceOffset + GeometryIndex * Info.HitGroupStride + RayOffsetInHitGroupIndex;
        const size_t Stride    = this->m_ShaderRecordStride;
        const Uint32 GroupSize = this->GetDevice()->GetAdapterInfo().RayTracing.ShaderGroupHandleSize;
        const size_t Offset    = Index * Stride;

        this->m_HitGroupsRecord.resize(std::max(this->m_HitGroupsRecord.size(), Offset + Stride), Uint8{EmptyElem});

        this->m_pPSO->CopyShaderHandle(pShaderGroupName, this->m_HitGroupsRecord.data() + Offset, Stride);
        std::memcpy(this->m_HitGroupsRecord.data() + Offset + GroupSize, pData, DataSize);
        this->m_Changed = true;

#ifdef DILIGENT_DEVELOPMENT
        VERIFY_EXPR(Index >= Info.FirstContributionToHitGroupIndex && Index <= Info.LastContributionToHitGroupIndex);
        OnBindHitGroup(pTLASImpl, Index);
#endif
    }


    void DILIGENT_CALL_TYPE BindHitGroupForInstance(ITopLevelAS* pTLAS,
                                                    const char*  pInstanceName,
                                                    Uint32       RayOffsetInHitGroupIndex,
                                                    const char*  pShaderGroupName,
                                                    const void*  pData,
                                                    Uint32       DataSize) override final
    {
        VERIFY_EXPR((pData == nullptr) == (DataSize == 0));
        VERIFY_EXPR((pData == nullptr) || (DataSize == this->m_ShaderRecordSize));
        VERIFY_EXPR(pTLAS != nullptr);

        auto* const pTLASImpl = ClassPtrCast<TopLevelASImplType>(pTLAS);
        const auto  Info      = pTLASImpl->GetBuildInfo();
        const auto  Desc      = pTLASImpl->GetInstanceDesc(pInstanceName);

        VERIFY_EXPR(Info.BindingMode == HIT_GROUP_BINDING_MODE_PER_GEOMETRY ||
                    Info.BindingMode == HIT_GROUP_BINDING_MODE_PER_INSTANCE);
        VERIFY_EXPR(RayOffsetInHitGroupIndex < Info.HitGroupStride);
        VERIFY_EXPR(Desc.ContributionToHitGroupIndex != INVALID_INDEX);
        VERIFY_EXPR(Desc.pBLAS != nullptr);

        const Uint32 InstanceOffset = Desc.ContributionToHitGroupIndex;
        Uint32       GeometryCount  = 0;

        switch (Info.BindingMode)
        {
            // clang-format off
            case HIT_GROUP_BINDING_MODE_PER_GEOMETRY:     GeometryCount = Desc.pBLAS->GetActualGeometryCount(); break;
            case HIT_GROUP_BINDING_MODE_PER_INSTANCE:     GeometryCount = 1;                                    break;
            default:                                      UNEXPECTED("unknown binding mode");
                // clang-format on
        }

        const Uint32 BeginIndex = InstanceOffset;
        const size_t EndIndex   = InstanceOffset + size_t{GeometryCount} * size_t{Info.HitGroupStride};
        const Uint32 GroupSize  = this->GetDevice()->GetAdapterInfo().RayTracing.ShaderGroupHandleSize;
        const size_t Stride     = this->m_ShaderRecordStride;

        this->m_HitGroupsRecord.resize(std::max(this->m_HitGroupsRecord.size(), EndIndex * Stride), Uint8{EmptyElem});
        this->m_Changed = true;

        for (Uint32 i = 0; i < GeometryCount; ++i)
        {
            Uint32 Index  = BeginIndex + i * Info.HitGroupStride + RayOffsetInHitGroupIndex;
            size_t Offset = Index * Stride;
            this->m_pPSO->CopyShaderHandle(pShaderGroupName, this->m_HitGroupsRecord.data() + Offset, Stride);

            std::memcpy(this->m_HitGroupsRecord.data() + Offset + GroupSize, pData, DataSize);

#ifdef DILIGENT_DEVELOPMENT
            VERIFY_EXPR(Index >= Info.FirstContributionToHitGroupIndex && Index <= Info.LastContributionToHitGroupIndex);
            OnBindHitGroup(pTLASImpl, Index);
#endif
        }
    }


    void DILIGENT_CALL_TYPE BindHitGroupForTLAS(ITopLevelAS* pTLAS,
                                                Uint32       RayOffsetInHitGroupIndex,
                                                const char*  pShaderGroupName,
                                                const void*  pData,
                                                Uint32       DataSize) override final
    {
        VERIFY_EXPR((pData == nullptr) == (DataSize == 0));
        VERIFY_EXPR((pData == nullptr) || (DataSize == this->m_ShaderRecordSize));
        VERIFY_EXPR(pTLAS != nullptr);

        auto*      pTLASImpl = ClassPtrCast<TopLevelASImplType>(pTLAS);
        const auto Info      = pTLASImpl->GetBuildInfo();
        VERIFY_EXPR(Info.BindingMode == HIT_GROUP_BINDING_MODE_PER_GEOMETRY ||
                    Info.BindingMode == HIT_GROUP_BINDING_MODE_PER_INSTANCE ||
                    Info.BindingMode == HIT_GROUP_BINDING_MODE_PER_TLAS);
        VERIFY_EXPR(RayOffsetInHitGroupIndex < Info.HitGroupStride);

        const Uint32 GroupSize = this->GetDevice()->GetAdapterInfo().RayTracing.ShaderGroupHandleSize;
        const size_t Stride    = this->m_ShaderRecordStride;
        this->m_HitGroupsRecord.resize(std::max(this->m_HitGroupsRecord.size(), (size_t{Info.LastContributionToHitGroupIndex} + 1) * Stride), Uint8{EmptyElem});
        this->m_Changed = true;

        for (Uint32 Index = RayOffsetInHitGroupIndex + Info.FirstContributionToHitGroupIndex;
             Index <= Info.LastContributionToHitGroupIndex;
             Index += Info.HitGroupStride)
        {
            const size_t Offset = Index * Stride;
            this->m_pPSO->CopyShaderHandle(pShaderGroupName, this->m_HitGroupsRecord.data() + Offset, Stride);
            std::memcpy(this->m_HitGroupsRecord.data() + Offset + GroupSize, pData, DataSize);

#ifdef DILIGENT_DEVELOPMENT
            OnBindHitGroup(pTLASImpl, Index);
#endif
        }
    }


    void DILIGENT_CALL_TYPE BindCallableShader(const char* pShaderGroupName,
                                               Uint32      CallableIndex,
                                               const void* pData,
                                               Uint32      DataSize) override final
    {
        VERIFY_EXPR((pData == nullptr) == (DataSize == 0));
        VERIFY_EXPR((pData == nullptr) || (DataSize == this->m_ShaderRecordSize));

        const Uint32 GroupSize = this->GetDevice()->GetAdapterInfo().RayTracing.ShaderGroupHandleSize;
        const size_t Offset    = size_t{CallableIndex} * size_t{this->m_ShaderRecordStride};
        this->m_CallableShadersRecord.resize(std::max(this->m_CallableShadersRecord.size(), Offset + this->m_ShaderRecordStride), Uint8{EmptyElem});

        this->m_pPSO->CopyShaderHandle(pShaderGroupName, this->m_CallableShadersRecord.data() + Offset, this->m_ShaderRecordStride);
        std::memcpy(this->m_CallableShadersRecord.data() + Offset + GroupSize, pData, DataSize);
        this->m_Changed = true;
    }


    Bool DILIGENT_CALL_TYPE Verify(VERIFY_SBT_FLAGS Flags) const override final
    {
#ifdef DILIGENT_DEVELOPMENT
        static_assert(EmptyElem != 0, "must not be zero");

        const auto Stride      = this->m_ShaderRecordStride;
        const auto ShSize      = this->GetDevice()->GetAdapterInfo().RayTracing.ShaderGroupHandleSize;
        const auto FindPattern = [&](const std::vector<Uint8>& Data, const char* GroupName) -> bool //
        {
            for (size_t i = 0; i < Data.size(); i += Stride)
            {
                if (Flags & VERIFY_SBT_FLAG_SHADER_ONLY)
                {
                    Uint32 Count = 0;
                    for (size_t j = 0; j < ShSize; ++j)
                        Count += (Data[i + j] == EmptyElem);

                    if (Count == ShSize)
                    {
                        LOG_INFO_MESSAGE("Shader binding table '", this->m_Desc.Name, "' is not valid: shader in '", GroupName, "'(", i / Stride, ") is not bound.");
                        return false;
                    }
                }

                if ((Flags & VERIFY_SBT_FLAG_SHADER_RECORD) && this->m_ShaderRecordSize > 0)
                {
                    Uint32 Count = 0;
                    for (size_t j = ShSize; j < Stride; ++j)
                        Count += (Data[i + j] == EmptyElem);

                    // shader record data may not be used in the shader
                    if (Count == Stride - ShSize)
                    {
                        LOG_INFO_MESSAGE("Shader binding table '", this->m_Desc.Name, "' is not valid: shader record data in '", GroupName, "' (", i / Stride, ") is not initialized.");
                        return false;
                    }
                }
            }
            return true;
        };

        if (m_RayGenShaderRecord.empty())
        {
            LOG_INFO_MESSAGE("Shader binding table '", this->m_Desc.Name, "' is not valid: ray generation shader is not bound.");
            return false;
        }

        if (Flags & VERIFY_SBT_FLAG_TLAS)
        {
            for (size_t i = 0; i < m_DbgHitGroupBindings.size(); ++i)
            {
                auto& Binding = m_DbgHitGroupBindings[i];
                auto  pTLAS   = Binding.pTLAS.Lock();
                if (!Binding.IsBound)
                {
                    LOG_INFO_MESSAGE("Shader binding table '", this->m_Desc.Name, "' is not valid: hit group at index (", i, ") is not bound.");
                    return false;
                }
                if (!pTLAS)
                {
                    LOG_INFO_MESSAGE("Shader binding table '", this->m_Desc.Name, "' is not valid: TLAS that was used to bind hit group at index (", i, ") was deleted.");
                    return false;
                }
                if (pTLAS->GetVersion() != Binding.Version)
                {
                    LOG_INFO_MESSAGE("Shader binding table '", this->m_Desc.Name, "' is not valid: TLAS that was used to bind hit group at index '(", i,
                                     ") with name '", pTLAS->GetDesc().Name, " was changed and no longer compatible with SBT.");
                    return false;
                }
            }
        }

        bool valid = true;
        valid      = valid && FindPattern(m_RayGenShaderRecord, "ray generation");
        valid      = valid && FindPattern(m_MissShadersRecord, "miss");
        valid      = valid && FindPattern(m_CallableShadersRecord, "callable");
        valid      = valid && FindPattern(m_HitGroupsRecord, "hit groups");
        return valid;
#else
        return true;

#endif // DILIGENT_DEVELOPMENT
    }

    bool                  HasPendingData() const { return this->m_Changed; }
    const BufferImplType* GetInternalBuffer() const { return this->m_pBuffer; }

protected:
    struct BindingTable
    {
        const void* pData  = nullptr;
        Uint32      Size   = 0;
        Uint32      Offset = 0;
        Uint32      Stride = 0;
    };
    void GetData(BufferImplType*& pSBTBuffer,
                 BindingTable&    RaygenShaderBindingTable,
                 BindingTable&    MissShaderBindingTable,
                 BindingTable&    HitShaderBindingTable,
                 BindingTable&    CallableShaderBindingTable)
    {
        const auto ShaderGroupBaseAlignment = this->GetDevice()->GetAdapterInfo().RayTracing.ShaderGroupBaseAlignment;

        const auto AlignToLarger = [ShaderGroupBaseAlignment](size_t offset) -> Uint32 {
            return AlignUp(static_cast<Uint32>(offset), ShaderGroupBaseAlignment);
        };

        const Uint32 RayGenOffset          = 0;
        const Uint32 MissShaderOffset      = AlignToLarger(m_RayGenShaderRecord.size());
        const Uint32 HitGroupOffset        = AlignToLarger(MissShaderOffset + m_MissShadersRecord.size());
        const Uint32 CallableShadersOffset = AlignToLarger(HitGroupOffset + m_HitGroupsRecord.size());
        const Uint32 BufSize               = AlignToLarger(CallableShadersOffset + m_CallableShadersRecord.size());

        // Recreate buffer
        if (m_pBuffer == nullptr || m_pBuffer->GetDesc().Size < BufSize)
        {
            m_pBuffer = nullptr;

            String     BuffName = String{this->m_Desc.Name} + " - internal buffer";
            BufferDesc BuffDesc;
            BuffDesc.Name      = BuffName.c_str();
            BuffDesc.Usage     = USAGE_DEFAULT;
            BuffDesc.BindFlags = BIND_RAY_TRACING;
            BuffDesc.Size      = BufSize;

            this->GetDevice()->CreateBuffer(BuffDesc, nullptr, m_pBuffer.template DblPtr<IBuffer>());
            VERIFY_EXPR(m_pBuffer != nullptr);
        }

        if (m_pBuffer == nullptr)
            return; // Something went wrong

        pSBTBuffer = m_pBuffer;

        if (!m_RayGenShaderRecord.empty())
        {
            RaygenShaderBindingTable.pData  = m_Changed ? m_RayGenShaderRecord.data() : nullptr;
            RaygenShaderBindingTable.Offset = RayGenOffset;
            RaygenShaderBindingTable.Size   = static_cast<Uint32>(m_RayGenShaderRecord.size());
            RaygenShaderBindingTable.Stride = this->m_ShaderRecordStride;
        }

        if (!m_MissShadersRecord.empty())
        {
            MissShaderBindingTable.pData  = m_Changed ? m_MissShadersRecord.data() : nullptr;
            MissShaderBindingTable.Offset = MissShaderOffset;
            MissShaderBindingTable.Size   = static_cast<Uint32>(m_MissShadersRecord.size());
            MissShaderBindingTable.Stride = this->m_ShaderRecordStride;
        }

        if (!m_HitGroupsRecord.empty())
        {
            HitShaderBindingTable.pData  = m_Changed ? m_HitGroupsRecord.data() : nullptr;
            HitShaderBindingTable.Offset = HitGroupOffset;
            HitShaderBindingTable.Size   = static_cast<Uint32>(m_HitGroupsRecord.size());
            HitShaderBindingTable.Stride = this->m_ShaderRecordStride;
        }

        if (!m_CallableShadersRecord.empty())
        {
            CallableShaderBindingTable.pData  = m_Changed ? m_CallableShadersRecord.data() : nullptr;
            CallableShaderBindingTable.Offset = CallableShadersOffset;
            CallableShaderBindingTable.Size   = static_cast<Uint32>(m_CallableShadersRecord.size());
            CallableShaderBindingTable.Stride = this->m_ShaderRecordStride;
        }

        m_Changed = false;
    }

protected:
    std::vector<Uint8> m_RayGenShaderRecord;
    std::vector<Uint8> m_MissShadersRecord;
    std::vector<Uint8> m_CallableShadersRecord;
    std::vector<Uint8> m_HitGroupsRecord;

    RefCntAutoPtr<PipelineStateImplType> m_pPSO;
    RefCntAutoPtr<BufferImplType>        m_pBuffer;

    Uint32 m_ShaderRecordSize   = 0;
    Uint32 m_ShaderRecordStride = 0;
    bool   m_Changed            = true;

#ifdef DILIGENT_DEVELOPMENT
    static constexpr Uint8 EmptyElem = 0xA7;
#else
    // In release mode clear uninitialized data by zeros.
    // This makes shader inactive, which hides errors but prevents crashes.
    static constexpr Uint8 EmptyElem = 0;
#endif

private:
#ifdef DILIGENT_DEVELOPMENT
    struct HitGroupBinding
    {
        RefCntWeakPtr<TopLevelASImplType> pTLAS;
        Uint32                            Version = ~0u;
        bool                              IsBound = false;
    };
    mutable std::vector<HitGroupBinding> m_DbgHitGroupBindings;

    void OnBindHitGroup(TopLevelASImplType* pTLAS, size_t Index)
    {
        this->m_DbgHitGroupBindings.resize(std::max(this->m_DbgHitGroupBindings.size(), Index + 1));

        auto& Binding   = this->m_DbgHitGroupBindings[Index];
        Binding.pTLAS   = pTLAS;
        Binding.Version = pTLAS ? pTLAS->GetVersion() : ~0u;
        Binding.IsBound = true;
    }
#endif
};

} // namespace Diligent
