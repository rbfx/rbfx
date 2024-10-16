/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

#include "CommandContext.hpp"

#include "d3dx12_win.h"

#include "RenderDeviceD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "BufferD3D12Impl.hpp"
#include "BottomLevelASD3D12Impl.hpp"
#include "TopLevelASD3D12Impl.hpp"

#include "CommandListManager.hpp"
#include "D3D12TypeConversions.hpp"

#ifdef DILIGENT_USE_PIX

#    if defined(DILIGENT_DEVELOPMENT) && !defined(USE_PIX)
// PIX instrumentation is only enabled if one of the preprocessor symbols
// USE_PIX, DBG, _DEBUG, PROFILE, or PROFILE_BUILD is defined.
#        define USE_PIX
#    endif

#    include "include/WinPixEventRuntime/pix3.h"
#endif

namespace Diligent
{

CommandContext::CommandContext(CommandListManager& CmdListManager) :
    m_PendingResourceBarriers(STD_ALLOCATOR_RAW_MEM(D3D12_RESOURCE_BARRIER, GetRawAllocator(), "Allocator for vector<D3D12_RESOURCE_BARRIER>"))
{
    m_PendingResourceBarriers.reserve(32);
    CmdListManager.CreateNewCommandList(&m_pCommandList, &m_pCurrentAllocator, m_MaxInterfaceVer);
}

CommandContext::~CommandContext(void)
{
    DEV_CHECK_ERR(m_pCurrentAllocator == nullptr, "Command allocator must be released prior to destroying the command context");
}

void CommandContext::Reset(CommandListManager& CmdListManager)
{
    // We only call Reset() on previously freed contexts. The command list persists, but we need to
    // request a new allocator
    VERIFY_EXPR(m_pCommandList != nullptr);
    VERIFY_EXPR(m_pCommandList->GetType() == CmdListManager.GetCommandListType());
    if (!m_pCurrentAllocator)
    {
        CmdListManager.RequestAllocator(&m_pCurrentAllocator);
        // Unlike ID3D12CommandAllocator::Reset, ID3D12GraphicsCommandList::Reset can be called while the
        // command list is still being executed. A typical pattern is to submit a command list and then
        // immediately reset it to reuse the allocated memory for another command list.
        m_pCommandList->Reset(m_pCurrentAllocator, nullptr);
    }

    m_pCurPipelineState         = nullptr;
    m_pCurGraphicsRootSignature = nullptr;
    m_pCurComputeRootSignature  = nullptr;
    m_PendingResourceBarriers.clear();
    m_BoundDescriptorHeaps = ShaderDescriptorHeaps{};

    m_DynamicGPUDescriptorAllocators = nullptr;

    m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
#if 0
    BindDescriptorHeaps();
#endif
}

ID3D12GraphicsCommandList* CommandContext::Close(CComPtr<ID3D12CommandAllocator>& pAllocator)
{
    FlushResourceBarriers();

    //if (m_ID.length() > 0)
    //  EngineProfiling::EndBlock(this);

    VERIFY_EXPR(m_pCurrentAllocator != nullptr);
    auto hr = m_pCommandList->Close();
    DEV_CHECK_ERR(SUCCEEDED(hr), "Failed to close the command list");

    pAllocator = std::move(m_pCurrentAllocator);
    return m_pCommandList;
}

void CommandContext::TransitionResource(TextureD3D12Impl& TexD3D12, RESOURCE_STATE NewState)
{
    VERIFY(TexD3D12.IsInKnownState(), "Texture state can't be unknown");
    TransitionResource(TexD3D12, StateTransitionDesc{&TexD3D12, RESOURCE_STATE_UNKNOWN, NewState, STATE_TRANSITION_FLAG_UPDATE_STATE});
}

void CommandContext::TransitionResource(BufferD3D12Impl& BuffD3D12, RESOURCE_STATE NewState)
{
    VERIFY(BuffD3D12.IsInKnownState(), "Buffer state can't be unknown");
    TransitionResource(BuffD3D12, StateTransitionDesc{&BuffD3D12, RESOURCE_STATE_UNKNOWN, NewState, STATE_TRANSITION_FLAG_UPDATE_STATE});
}

void CommandContext::TransitionResource(BottomLevelASD3D12Impl& BlasD3D12, RESOURCE_STATE NewState)
{
    VERIFY(BlasD3D12.IsInKnownState(), "BLAS state can't be unknown");
    TransitionResource(BlasD3D12, StateTransitionDesc{&BlasD3D12, RESOURCE_STATE_UNKNOWN, NewState, STATE_TRANSITION_FLAG_UPDATE_STATE});
}

void CommandContext::TransitionResource(TopLevelASD3D12Impl& TlasD3D12, RESOURCE_STATE NewState)
{
    VERIFY(TlasD3D12.IsInKnownState(), "TLAS state can't be unknown");
    TransitionResource(TlasD3D12, StateTransitionDesc{&TlasD3D12, RESOURCE_STATE_UNKNOWN, NewState, STATE_TRANSITION_FLAG_UPDATE_STATE});
}

namespace
{

D3D12_RESOURCE_BARRIER_FLAGS TransitionTypeToD3D12ResourceBarrierFlag(STATE_TRANSITION_TYPE TransitionType)
{
    switch (TransitionType)
    {
        case STATE_TRANSITION_TYPE_IMMEDIATE:
            return D3D12_RESOURCE_BARRIER_FLAG_NONE;

        case STATE_TRANSITION_TYPE_BEGIN:
            return D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

        case STATE_TRANSITION_TYPE_END:
            return D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;

        default:
            UNEXPECTED("Unexpected state transition type");
            return D3D12_RESOURCE_BARRIER_FLAG_NONE;
    }
}

class StateTransitionHelper
{
public:
    StateTransitionHelper(const StateTransitionDesc& Barrier,
                          CommandContext&            CmdCtx) :
        m_Barrier{Barrier},
        m_CmdCtx{CmdCtx},
        m_ResStateMask{GetSupportedD3D12ResourceStatesForCommandList(m_CmdCtx.GetCommandListType())}
    {
        DEV_CHECK_ERR(m_Barrier.NewState != RESOURCE_STATE_UNKNOWN, "New resource state can't be unknown");
    }

    template <typename ResourceType>
    void GetD3D12ResourceAndState(ResourceType& Resource);

    template <> void GetD3D12ResourceAndState<BufferD3D12Impl>(BufferD3D12Impl& Buffer);

    void AddD3D12ResourceBarriers(TextureD3D12Impl& Tex, D3D12_RESOURCE_BARRIER& d3d12Barrier);
    void AddD3D12ResourceBarriers(BufferD3D12Impl& Buff, D3D12_RESOURCE_BARRIER& d3d12Barrier);
    void AddD3D12ResourceBarriers(TopLevelASD3D12Impl& TLAS, D3D12_RESOURCE_BARRIER& d3d12Barrier);
    void AddD3D12ResourceBarriers(BottomLevelASD3D12Impl& BLAS, D3D12_RESOURCE_BARRIER& d3d12Barrier);

    template <typename ResourceType>
    void operator()(ResourceType& Resource);

    void DiscardIfAppropriate(const TextureDesc&    TexDesc,
                              D3D12_RESOURCE_STATES d3d12State,
                              Uint32                EndMip   = REMAINING_MIP_LEVELS,
                              Uint32                EndSlice = REMAINING_ARRAY_SLICES) const;

private:
    const StateTransitionDesc& m_Barrier;

    CommandContext& m_CmdCtx;

    RESOURCE_STATE  m_OldState       = RESOURCE_STATE_UNKNOWN;
    ID3D12Resource* m_pd3d12Resource = nullptr;

    bool m_RequireUAVBarrier = false;

    const D3D12_RESOURCE_STATES m_ResStateMask;
};

template <typename ResourceType>
void StateTransitionHelper::GetD3D12ResourceAndState(ResourceType& Resource)
{
    VERIFY_EXPR(m_Barrier.pResource == &Resource);
    m_OldState       = Resource.GetState();
    m_pd3d12Resource = Resource.GetD3D12Resource();
}

template <>
void StateTransitionHelper::GetD3D12ResourceAndState<BufferD3D12Impl>(BufferD3D12Impl& Buffer)
{
    VERIFY_EXPR(m_Barrier.pResource == &Buffer);
#ifdef DILIGENT_DEVELOPMENT
    // Dynamic buffers that have no backing d3d12 resource are suballocated in the upload heap
    // when Map() is called and must always be in D3D12_RESOURCE_STATE_GENERIC_READ state.
    if (Buffer.GetDesc().Usage == USAGE_DYNAMIC && Buffer.GetD3D12Resource() == nullptr)
    {
        DEV_CHECK_ERR(Buffer.GetState() == RESOURCE_STATE_GENERIC_READ, "Dynamic buffers that have no backing d3d12 resource are expected to always be in D3D12_RESOURCE_STATE_GENERIC_READ state");
        VERIFY((m_Barrier.NewState & RESOURCE_STATE_GENERIC_READ) == m_Barrier.NewState, "Dynamic buffers can only transition to one of RESOURCE_STATE_GENERIC_READ states");
    }
#endif

    m_OldState       = Buffer.GetState();
    m_pd3d12Resource = Buffer.GetD3D12Resource();
}

void StateTransitionHelper::DiscardIfAppropriate(const TextureDesc&    TexDesc,
                                                 D3D12_RESOURCE_STATES d3d12State,
                                                 Uint32                EndMip,
                                                 Uint32                EndSlice) const
{
    if ((m_Barrier.Flags & STATE_TRANSITION_FLAG_DISCARD_CONTENT) == 0)
        return;

    const auto d3d12CmdListType = m_CmdCtx.GetCommandListType();

    bool DiscardAllowed = false;

    // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-discardresource
    if (d3d12CmdListType == D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        // For D3D12_COMMAND_LIST_TYPE_DIRECT, the following two rules apply:

        // When a resource has the D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET flag, DiscardResource must be called when the
        // discarded subresource regions are in the D3D12_RESOURCE_STATE_RENDER_TARGET resource barrier state.
        if ((d3d12State & D3D12_RESOURCE_STATE_RENDER_TARGET) != 0)
        {
            VERIFY_EXPR((TexDesc.BindFlags & BIND_RENDER_TARGET) != 0);
            DiscardAllowed = true;
        }

        // When a resource has the D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL flag, DiscardResource must be called when the
        // discarded subresource regions are in the D3D12_RESOURCE_STATE_DEPTH_WRITE.
        if ((d3d12State & D3D12_RESOURCE_STATE_DEPTH_WRITE) != 0)
        {
            VERIFY_EXPR((TexDesc.BindFlags & BIND_DEPTH_STENCIL) != 0);
            DiscardAllowed = true;
        }
    }
    else if (d3d12CmdListType == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        // For D3D12_COMMAND_LIST_TYPE_COMPUTE, the following rule applies:

        // The resource must have the D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS flag, and DiscardResource must be called
        // when the discarded subresource regions are in the D3D12_RESOURCE_STATE_UNORDERED_ACCESS resource barrier state.
        if ((d3d12State & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0)
        {
            VERIFY_EXPR((TexDesc.BindFlags & BIND_UNORDERED_ACCESS) != 0);
            DiscardAllowed = true;
        }
    }

    if (DiscardAllowed)
    {
        m_CmdCtx.FlushResourceBarriers();
        if (m_Barrier.FirstMipLevel == 0 && EndMip == REMAINING_MIP_LEVELS &&
            m_Barrier.FirstArraySlice == 0 && EndSlice == REMAINING_ARRAY_SLICES)
        {
            m_CmdCtx.DiscardResource(m_pd3d12Resource, nullptr);
        }
        else
        {
            D3D12_DISCARD_REGION Region{};
            Region.NumSubresources = EndMip - m_Barrier.FirstMipLevel;
            for (Uint32 slice = m_Barrier.FirstArraySlice; slice < EndSlice; ++slice)
            {
                Region.FirstSubresource = D3D12CalcSubresource(m_Barrier.FirstMipLevel, slice, 0, TexDesc.MipLevels, TexDesc.GetArraySize());
#ifdef DILIGENT_DEBUG
                for (UINT mip = 0; mip < Region.NumSubresources; ++mip)
                    VERIFY_EXPR(D3D12CalcSubresource(m_Barrier.FirstMipLevel + mip, slice, 0, TexDesc.MipLevels, TexDesc.GetArraySize()) == Region.FirstSubresource + mip);
#endif

                m_CmdCtx.DiscardResource(m_pd3d12Resource, &Region);
            }
        }
    }
}

void StateTransitionHelper::AddD3D12ResourceBarriers(TextureD3D12Impl& Tex, D3D12_RESOURCE_BARRIER& d3d12Barrier)
{
    // Note that RESOURCE_STATE_UNDEFINED != RESOURCE_STATE_PRESENT, but
    // D3D12_RESOURCE_STATE_COMMON == D3D12_RESOURCE_STATE_PRESENT
    if (d3d12Barrier.Transition.StateBefore != d3d12Barrier.Transition.StateAfter)
    {
        const auto& TexDesc = Tex.GetDesc();
        VERIFY(m_Barrier.FirstMipLevel < TexDesc.MipLevels, "First mip level is out of range");
        VERIFY(m_Barrier.MipLevelsCount == REMAINING_MIP_LEVELS || m_Barrier.FirstMipLevel + m_Barrier.MipLevelsCount <= TexDesc.MipLevels,
               "Invalid mip level range");
        VERIFY(m_Barrier.FirstArraySlice < TexDesc.GetArraySize(), "First array slice is out of range");
        VERIFY(m_Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES || m_Barrier.FirstArraySlice + m_Barrier.ArraySliceCount <= TexDesc.GetArraySize(),
               "Invalid array slice range");

        if (m_Barrier.FirstMipLevel == 0 && (m_Barrier.MipLevelsCount == REMAINING_MIP_LEVELS || m_Barrier.MipLevelsCount == TexDesc.MipLevels) &&
            m_Barrier.FirstArraySlice == 0 && (m_Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES || m_Barrier.ArraySliceCount == TexDesc.GetArraySize()))
        {
            DiscardIfAppropriate(TexDesc, d3d12Barrier.Transition.StateBefore);
            d3d12Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            m_CmdCtx.ResourceBarrier(d3d12Barrier);
            DiscardIfAppropriate(TexDesc, d3d12Barrier.Transition.StateAfter);
        }
        else
        {
            Uint32 EndMip   = m_Barrier.MipLevelsCount == REMAINING_MIP_LEVELS ? TexDesc.MipLevels : m_Barrier.FirstMipLevel + m_Barrier.MipLevelsCount;
            Uint32 EndSlice = m_Barrier.ArraySliceCount == REMAINING_ARRAY_SLICES ? TexDesc.GetArraySize() : m_Barrier.FirstArraySlice + m_Barrier.ArraySliceCount;
            DiscardIfAppropriate(TexDesc, d3d12Barrier.Transition.StateBefore, EndMip, EndSlice);
            for (Uint32 mip = m_Barrier.FirstMipLevel; mip < EndMip; ++mip)
            {
                for (Uint32 slice = m_Barrier.FirstArraySlice; slice < EndSlice; ++slice)
                {
                    d3d12Barrier.Transition.Subresource = D3D12CalcSubresource(mip, slice, 0, TexDesc.MipLevels, TexDesc.GetArraySize());
                    m_CmdCtx.ResourceBarrier(d3d12Barrier);
                }
            }
            DiscardIfAppropriate(TexDesc, d3d12Barrier.Transition.StateAfter, EndMip, EndSlice);
        }
    }
}

void StateTransitionHelper::AddD3D12ResourceBarriers(BufferD3D12Impl& Buff, D3D12_RESOURCE_BARRIER& d3d12Barrier)
{
    if (d3d12Barrier.Transition.StateBefore != d3d12Barrier.Transition.StateAfter)
    {
        m_CmdCtx.ResourceBarrier(d3d12Barrier);
    }
}

void StateTransitionHelper::AddD3D12ResourceBarriers(TopLevelASD3D12Impl& TLAS, D3D12_RESOURCE_BARRIER& /*d3d12Barrier*/)
{
    // Acceleration structure is always in D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
    // and requires UAV barrier instead of state transition.
    // If either old or new state is a write state, we need to issue a UAV barrier to complete
    // all previous read/write operations.
    if (m_OldState == RESOURCE_STATE_BUILD_AS_WRITE || m_Barrier.NewState == RESOURCE_STATE_BUILD_AS_WRITE)
        m_RequireUAVBarrier = true;

#ifdef DILIGENT_DEVELOPMENT
    if (m_Barrier.NewState & RESOURCE_STATE_RAY_TRACING)
    {
        TLAS.ValidateContent();
    }
#endif
}

void StateTransitionHelper::AddD3D12ResourceBarriers(BottomLevelASD3D12Impl& BLAS, D3D12_RESOURCE_BARRIER& /*d3d12Barrier*/)
{
    // Acceleration structure is always in D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
    // and requires UAV barrier instead of state transition.
    // If either old or new state is a write state, we need to issue a UAV barrier to complete
    // all previous read/write operations.
    if (m_OldState == RESOURCE_STATE_BUILD_AS_WRITE || m_Barrier.NewState == RESOURCE_STATE_BUILD_AS_WRITE)
        m_RequireUAVBarrier = true;
}

template <typename ResourceType>
void StateTransitionHelper::operator()(ResourceType& Resource)
{
    GetD3D12ResourceAndState(Resource);

    if (m_OldState == RESOURCE_STATE_UNKNOWN)
    {
        DEV_CHECK_ERR(m_Barrier.OldState != RESOURCE_STATE_UNKNOWN, "When resource state is unknown (which means it is managed by the app), OldState member must not be RESOURCE_STATE_UNKNOWN");
        m_OldState = m_Barrier.OldState;
    }
    else
    {
        DEV_CHECK_ERR(m_Barrier.OldState == RESOURCE_STATE_UNKNOWN || m_Barrier.OldState == m_OldState,
                      "Resource state is known (", GetResourceStateString(m_OldState), ") and does not match the OldState (",
                      GetResourceStateString(m_Barrier.OldState),
                      ") specified in the resource barrier. Set OldState member to RESOURCE_STATE_UNKNOWN "
                      "to make the engine use current resource state");
    }

    // RESOURCE_STATE_UNORDERED_ACCESS and RESOURCE_STATE_BUILD_AS_WRITE are converted to D3D12_RESOURCE_STATE_UNORDERED_ACCESS.
    // UAV barrier must be inserted between D3D12_RESOURCE_STATE_UNORDERED_ACCESS resource usages.
    m_RequireUAVBarrier =
        (m_OldState == RESOURCE_STATE_UNORDERED_ACCESS || m_OldState == RESOURCE_STATE_BUILD_AS_WRITE) &&
        (m_Barrier.NewState == RESOURCE_STATE_UNORDERED_ACCESS || m_Barrier.NewState == RESOURCE_STATE_BUILD_AS_WRITE);

    // Check if required state is already set
    if ((m_OldState & m_Barrier.NewState) != m_Barrier.NewState)
    {
        auto NewState = m_Barrier.NewState;
        // If both old state and new state are read-only states, combine the two
        if ((m_OldState & RESOURCE_STATE_GENERIC_READ) == m_OldState &&
            (NewState & RESOURCE_STATE_GENERIC_READ) == NewState)
            NewState |= m_OldState;

        D3D12_RESOURCE_BARRIER d3d12Barrier;
        d3d12Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        d3d12Barrier.Flags                  = TransitionTypeToD3D12ResourceBarrierFlag(m_Barrier.TransitionType);
        d3d12Barrier.Transition.pResource   = m_pd3d12Resource;
        d3d12Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        d3d12Barrier.Transition.StateBefore = ResourceStateFlagsToD3D12ResourceStates(m_OldState) & m_ResStateMask;
        d3d12Barrier.Transition.StateAfter  = ResourceStateFlagsToD3D12ResourceStates(NewState) & m_ResStateMask;

        AddD3D12ResourceBarriers(Resource, d3d12Barrier);

        if ((m_Barrier.Flags & STATE_TRANSITION_FLAG_UPDATE_STATE) != 0)
        {
            VERIFY(m_Barrier.TransitionType == STATE_TRANSITION_TYPE_IMMEDIATE || m_Barrier.TransitionType == STATE_TRANSITION_TYPE_END,
                   "Resource state can't be updated in begin-split barrier");
            Resource.SetState(NewState);
        }
    }

    if (m_RequireUAVBarrier)
    {
        // UAV barrier indicates that all UAV accesses (reads or writes) to a particular resource
        // must complete before any future UAV accesses (reads or writes) can begin.

        DEV_CHECK_ERR(m_Barrier.TransitionType == STATE_TRANSITION_TYPE_IMMEDIATE, "UAV barriers must not be split");
        D3D12_RESOURCE_BARRIER d3d12Barrier{D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_BARRIER_FLAG_NONE, {}};
        d3d12Barrier.UAV.pResource = m_pd3d12Resource;
        m_CmdCtx.ResourceBarrier(d3d12Barrier);
    }
}

} // namespace

void CommandContext::TransitionResource(TextureD3D12Impl& Texture, const StateTransitionDesc& Barrier)
{
    StateTransitionHelper Helper{Barrier, *this};
    Helper(Texture);
}

void CommandContext::TransitionResource(BufferD3D12Impl& Buffer, const StateTransitionDesc& Barrier)
{
    StateTransitionHelper Helper{Barrier, *this};
    Helper(Buffer);
}

void CommandContext::TransitionResource(BottomLevelASD3D12Impl& BLAS, const StateTransitionDesc& Barrier)
{
    StateTransitionHelper Helper{Barrier, *this};
    Helper(BLAS);
}

void CommandContext::TransitionResource(TopLevelASD3D12Impl& TLAS, const StateTransitionDesc& Barrier)
{
    StateTransitionHelper Helper{Barrier, *this};
    Helper(TLAS);
}

void CommandContext::InsertAliasBarrier(D3D12ResourceBase& Before, D3D12ResourceBase& After, bool FlushImmediate)
{
    m_PendingResourceBarriers.emplace_back();
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_PendingResourceBarriers.back();

    BarrierDesc.Type                     = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    BarrierDesc.Flags                    = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.Aliasing.pResourceBefore = Before.GetD3D12Resource();
    BarrierDesc.Aliasing.pResourceAfter  = After.GetD3D12Resource();

    if (FlushImmediate)
        FlushResourceBarriers();
}

#ifdef DILIGENT_USE_PIX
inline UINT ConvertColor(const float* pColor)
{
    if (pColor == nullptr)
        return PIX_COLOR(0, 0, 0);

    return PIX_COLOR(static_cast<BYTE>(pColor[0] * 255.f),
                     static_cast<BYTE>(pColor[1] * 255.f),
                     static_cast<BYTE>(pColor[2] * 255.f));
}

void CommandContext::PixBeginEvent(const Char* Name, const float* pColor)
{
    PIXBeginEvent(m_pCommandList.p, ConvertColor(pColor), Name);
}

void CommandContext::PixEndEvent()
{
    PIXEndEvent(m_pCommandList.p);
}

void CommandContext::PixSetMarker(const Char* Label, const float* pColor)
{
    PIXSetMarker(m_pCommandList.p, ConvertColor(pColor), Label);
}
#endif // DILIGENT_USE_PIX

} // namespace Diligent
