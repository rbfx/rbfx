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
/// Implementation of the Diligent::RenderPassBase template class

#include <vector>
#include <utility>

#include "RenderPass.h"
#include "DeviceObjectBase.hpp"
#include "RenderDeviceBase.hpp"
#include "FixedLinearAllocator.hpp"

namespace Diligent
{

void ValidateRenderPassDesc(const RenderPassDesc&      Desc,
                            const RenderDeviceInfo&    DeviceInfo,
                            const GraphicsAdapterInfo& AdapterInfo) noexcept(false);

template <typename RenderDeviceImplType>
void _CorrectAttachmentState(RESOURCE_STATE& State) {}

template <>
inline void _CorrectAttachmentState<class RenderDeviceVkImpl>(RESOURCE_STATE& State)
{
    if (State == RESOURCE_STATE_RESOLVE_DEST)
    {
        // In Vulkan resolve attachments must be in VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL state.
        // It is important to correct the state because outside of render pass RESOURCE_STATE_RESOLVE_DEST maps
        // to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
        State = RESOURCE_STATE_RENDER_TARGET;
    }
}

/// Template class implementing base functionality of the render pass object.

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class RenderPassBase : public DeviceObjectBase<typename EngineImplTraits::RenderPassInterface, typename EngineImplTraits::RenderDeviceImplType, RenderPassDesc>
{
public:
    // Base interface this class inherits (e.g. IRenderPassVk)
    using BaseInterface = typename EngineImplTraits::RenderPassInterface;

    // Render device implementation type (RenderDeviceD3D12Impl, RenderDeviceVkImpl, etc.).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, RenderPassDesc>;

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this render pass.
    /// \param pDevice           - Pointer to the device.
    /// \param Desc              - Render pass description.
    /// \param bIsDeviceInternal - Flag indicating if the RenderPass is an internal device object and
    ///							   must not keep a strong reference to the device.
    RenderPassBase(IReferenceCounters*   pRefCounters,
                   RenderDeviceImplType* pDevice,
                   const RenderPassDesc& Desc,
                   bool                  bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, Desc, bIsDeviceInternal}
    {
        try
        {
            if (pDevice != nullptr)
            {
                ValidateRenderPassDesc(this->m_Desc, pDevice->GetDeviceInfo(), pDevice->GetAdapterInfo());
            }

            auto&                RawAllocator = GetRawAllocator();
            FixedLinearAllocator MemPool{RawAllocator};
            ReserveSpace(this->m_Desc, MemPool);

            MemPool.Reserve();
            // The memory is now owned by RenderPassBase and will be freed by Destruct().
            m_pRawMemory = decltype(m_pRawMemory){MemPool.ReleaseOwnership(), STDDeleterRawMem<void>{RawAllocator}};

            CopyDesc(this->m_Desc, m_AttachmentStates, m_AttachmentFirstLastUse, MemPool);
        }
        catch (...)
        {
            Destruct();
            throw;
        }
    }

    ~RenderPassBase()
    {
        Destruct();
    }

    void Destruct()
    {
        VERIFY(!m_IsDestructed, "This object has already been destructed");

        m_pRawMemory.reset();

        m_AttachmentStates       = nullptr;
        m_AttachmentFirstLastUse = nullptr;

#if DILIGENT_DEBUG
        m_IsDestructed = true;
#endif
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_RenderPass, TDeviceObjectBase)

    RESOURCE_STATE GetAttachmentState(Uint32 Subpass, Uint32 Attachment) const
    {
        VERIFY_EXPR(Attachment < this->m_Desc.AttachmentCount);
        VERIFY_EXPR(Subpass < this->m_Desc.SubpassCount);
        return m_AttachmentStates[this->m_Desc.AttachmentCount * Subpass + Attachment];
    }

    std::pair<Uint32, Uint32> GetAttachmentFirstLastUse(Uint32 Attachment) const
    {
        return m_AttachmentFirstLastUse[Attachment];
    }

    const SubpassDesc& GetSubpass(Uint32 SubpassIndex) const
    {
        VERIFY_EXPR(SubpassIndex < this->m_Desc.SubpassCount);
        return this->m_Desc.pSubpasses[SubpassIndex];
    }

private:
    void ReserveSpace(const RenderPassDesc& Desc, FixedLinearAllocator& MemPool) const
    {
        MemPool.AddSpace<RESOURCE_STATE>(size_t{Desc.AttachmentCount} * size_t{Desc.SubpassCount}); // m_AttachmentStates
        MemPool.AddSpace<std::pair<Uint32, Uint32>>(Desc.AttachmentCount);                          // m_AttachmentFirstLastUse

        MemPool.AddSpace<RenderPassAttachmentDesc>(Desc.AttachmentCount); // Desc.pAttachments
        MemPool.AddSpace<SubpassDesc>(Desc.SubpassCount);                 // Desc.pSubpasses

        for (Uint32 subpass = 0; subpass < Desc.SubpassCount; ++subpass)
        {
            const auto& SrcSubpass = Desc.pSubpasses[subpass];

            MemPool.AddSpace<AttachmentReference>(SrcSubpass.InputAttachmentCount);        // Subpass.pInputAttachments
            MemPool.AddSpace<AttachmentReference>(SrcSubpass.RenderTargetAttachmentCount); // Subpass.pRenderTargetAttachments

            if (SrcSubpass.pResolveAttachments != nullptr)
                MemPool.AddSpace<AttachmentReference>(SrcSubpass.RenderTargetAttachmentCount); // Subpass.pResolveAttachments

            if (SrcSubpass.pDepthStencilAttachment != nullptr)
                MemPool.AddSpace<AttachmentReference>(1); // Subpass.pDepthStencilAttachment

            MemPool.AddSpace<Uint32>(SrcSubpass.PreserveAttachmentCount); // Subpass.pPreserveAttachments

            if (SrcSubpass.pShadingRateAttachment != nullptr)
                MemPool.AddSpace<ShadingRateAttachment>(1); // Subpass.pShadingRateAttachment
        }

        MemPool.AddSpace<SubpassDependencyDesc>(Desc.DependencyCount); // Desc.pDependencies
    }

    void CopyDesc(RenderPassDesc& Desc, RESOURCE_STATE*& AttachmentStates, const std::pair<Uint32, Uint32>*& outAttachmentFirstLastUse, FixedLinearAllocator& MemPool) const
    {
        AttachmentStates             = MemPool.ConstructArray<RESOURCE_STATE>(size_t{Desc.AttachmentCount} * size_t{Desc.SubpassCount}, RESOURCE_STATE_UNKNOWN);
        auto* AttachmentFirstLastUse = MemPool.ConstructArray<std::pair<Uint32, Uint32>>(Desc.AttachmentCount, std::pair<Uint32, Uint32>{ATTACHMENT_UNUSED, 0});
        outAttachmentFirstLastUse    = AttachmentFirstLastUse;

        if (Desc.AttachmentCount != 0)
        {
            const auto* SrcAttachments = Desc.pAttachments;
            auto*       DstAttachments = MemPool.Allocate<RenderPassAttachmentDesc>(Desc.AttachmentCount);
            Desc.pAttachments          = DstAttachments;
            for (Uint32 i = 0; i < Desc.AttachmentCount; ++i)
            {
                DstAttachments[i] = SrcAttachments[i];
                _CorrectAttachmentState<RenderDeviceImplType>(DstAttachments[i].FinalState);
            }
        }

        VERIFY(Desc.SubpassCount != 0, "Render pass must have at least one subpass");
        const auto* SrcSubpasses = Desc.pSubpasses;
        auto*       DstSubpasses = MemPool.Allocate<SubpassDesc>(Desc.SubpassCount);
        Desc.pSubpasses          = DstSubpasses;

        const auto SetAttachmentState = [AttachmentStates, &Desc](Uint32 Subpass, Uint32 Attachment, RESOURCE_STATE State) //
        {
            VERIFY_EXPR(Attachment < Desc.AttachmentCount);
            VERIFY_EXPR(Subpass < Desc.SubpassCount);
            VERIFY_EXPR((Desc.AttachmentCount * Subpass + Attachment) < (Desc.AttachmentCount * Desc.SubpassCount));
            AttachmentStates[Desc.AttachmentCount * Subpass + Attachment] = State;
        };

        for (Uint32 subpass = 0; subpass < Desc.SubpassCount; ++subpass)
        {
            for (Uint32 att = 0; att < Desc.AttachmentCount; ++att)
            {
                SetAttachmentState(subpass, att, subpass > 0 ? GetAttachmentState(subpass - 1, att) : Desc.pAttachments[att].InitialState);
            }

            const auto& SrcSubpass = SrcSubpasses[subpass];
            auto&       DstSubpass = DstSubpasses[subpass];

            auto UpdateAttachmentStateAndFirstUseSubpass = [&SetAttachmentState, AttachmentFirstLastUse, subpass](const AttachmentReference& AttRef) //
            {
                if (AttRef.AttachmentIndex != ATTACHMENT_UNUSED)
                {
                    SetAttachmentState(subpass, AttRef.AttachmentIndex, AttRef.State);
                    auto& FirstLastUse = AttachmentFirstLastUse[AttRef.AttachmentIndex];
                    if (FirstLastUse.first == ATTACHMENT_UNUSED)
                        FirstLastUse.first = subpass;
                    FirstLastUse.second = subpass;
                }
            };

            DstSubpass = SrcSubpass;
            if (SrcSubpass.InputAttachmentCount != 0)
            {
                auto* DstInputAttachments    = MemPool.Allocate<AttachmentReference>(SrcSubpass.InputAttachmentCount);
                DstSubpass.pInputAttachments = DstInputAttachments;
                for (Uint32 i = 0; i < SrcSubpass.InputAttachmentCount; ++i)
                {
                    DstInputAttachments[i] = SrcSubpass.pInputAttachments[i];
                    UpdateAttachmentStateAndFirstUseSubpass(DstInputAttachments[i]);
                }
            }
            else
                DstSubpass.pInputAttachments = nullptr;

            if (SrcSubpass.RenderTargetAttachmentCount != 0)
            {
                auto* DstRenderTargetAttachments    = MemPool.Allocate<AttachmentReference>(SrcSubpass.RenderTargetAttachmentCount);
                DstSubpass.pRenderTargetAttachments = DstRenderTargetAttachments;
                for (Uint32 i = 0; i < SrcSubpass.RenderTargetAttachmentCount; ++i)
                {
                    DstRenderTargetAttachments[i] = SrcSubpass.pRenderTargetAttachments[i];
                    UpdateAttachmentStateAndFirstUseSubpass(DstRenderTargetAttachments[i]);
                }

                if (DstSubpass.pResolveAttachments != nullptr)
                {
                    auto* DstResolveAttachments    = MemPool.Allocate<AttachmentReference>(SrcSubpass.RenderTargetAttachmentCount);
                    DstSubpass.pResolveAttachments = DstResolveAttachments;
                    for (Uint32 i = 0; i < SrcSubpass.RenderTargetAttachmentCount; ++i)
                    {
                        DstResolveAttachments[i] = SrcSubpass.pResolveAttachments[i];
                        _CorrectAttachmentState<RenderDeviceImplType>(DstResolveAttachments[i].State);
                        UpdateAttachmentStateAndFirstUseSubpass(DstResolveAttachments[i]);
                    }
                }
            }
            else
            {
                DstSubpass.pRenderTargetAttachments = nullptr;
                DstSubpass.pResolveAttachments      = nullptr;
            }

            if (SrcSubpass.pDepthStencilAttachment != nullptr)
            {
                DstSubpass.pDepthStencilAttachment = MemPool.Copy(*SrcSubpass.pDepthStencilAttachment);
                UpdateAttachmentStateAndFirstUseSubpass(*SrcSubpass.pDepthStencilAttachment);
            }

            if (SrcSubpass.PreserveAttachmentCount != 0)
                DstSubpass.pPreserveAttachments = MemPool.CopyArray<Uint32>(SrcSubpass.pPreserveAttachments, SrcSubpass.PreserveAttachmentCount);
            else
                DstSubpass.pPreserveAttachments = nullptr;

            if (SrcSubpass.pShadingRateAttachment != nullptr)
            {
                DstSubpass.pShadingRateAttachment = MemPool.Copy(*SrcSubpass.pShadingRateAttachment);
                UpdateAttachmentStateAndFirstUseSubpass(SrcSubpass.pShadingRateAttachment->Attachment);
            }
            else
                DstSubpass.pShadingRateAttachment = nullptr;
        }

        if (Desc.DependencyCount != 0)
            Desc.pDependencies = MemPool.CopyArray(Desc.pDependencies, Desc.DependencyCount);
    }

private:
    std::unique_ptr<void, STDDeleterRawMem<void>> m_pRawMemory;

    // Attachment states during each subpass
    RESOURCE_STATE* m_AttachmentStates = nullptr; // [m_Desc.AttachmentCount * m_Desc.SubpassCount]

    // The index of the subpass where the attachment is first used
    const std::pair<Uint32, Uint32>* m_AttachmentFirstLastUse = nullptr; // [m_Desc.AttachmentCount]

#ifdef DILIGENT_DEBUG
    bool m_IsDestructed = false;
#endif
};

} // namespace Diligent
