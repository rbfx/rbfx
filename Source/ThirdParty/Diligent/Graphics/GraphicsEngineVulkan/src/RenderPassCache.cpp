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

#include "RenderPassCache.hpp"

#include <sstream>

#include "RenderDeviceVkImpl.hpp"
#include "PipelineStateVkImpl.hpp"
#include "RenderPassVkImpl.hpp"

namespace Diligent
{

RenderPassCache::RenderPassCache(RenderDeviceVkImpl& DeviceVk) noexcept :
    m_DeviceVkImpl{DeviceVk}
{}


RenderPassCache::~RenderPassCache()
{
    // Render pass cache is part of the render device, so we can't release
    // render pass objects from here as their destructors will attempt to
    // call SafeReleaseDeviceObject.
    VERIFY(m_Cache.empty(), "Render pass cache is not empty. Did you call Destroy?");
}

void RenderPassCache::Destroy()
{
    auto& FBCache = m_DeviceVkImpl.GetFramebufferCache();
    for (auto it = m_Cache.begin(); it != m_Cache.end(); ++it)
    {
        FBCache.OnDestroyRenderPass(it->second->GetVkRenderPass());
    }
    m_Cache.clear();
}

static RenderPassDesc GetImplicitRenderPassDesc(
    Uint32                                                        NumRenderTargets,
    const TEXTURE_FORMAT                                          RTVFormats[],
    TEXTURE_FORMAT                                                DSVFormat,
    bool                                                          ReadOnlyDepth,
    Uint8                                                         SampleCount,
    TEXTURE_FORMAT                                                ShadingRateTexFormat,
    uint2                                                         ShadingRateTileSize,
    std::array<RenderPassAttachmentDesc, MAX_RENDER_TARGETS + 2>& Attachments,
    std::array<AttachmentReference, MAX_RENDER_TARGETS + 2>&      AttachmentReferences,
    SubpassDesc&                                                  SubpassDesc,
    ShadingRateAttachment&                                        ShadingRateAttachment)
{
    VERIFY_EXPR(NumRenderTargets <= MAX_RENDER_TARGETS);

    RenderPassDesc RPDesc;

    auto& AttachmentInd{RPDesc.AttachmentCount};

    AttachmentReference* pDepthAttachmentReference = nullptr;
    if (DSVFormat != TEX_FORMAT_UNKNOWN)
    {
        const RESOURCE_STATE DepthAttachmentState = ReadOnlyDepth ? RESOURCE_STATE_DEPTH_READ : RESOURCE_STATE_DEPTH_WRITE;

        auto& DepthAttachment = Attachments[AttachmentInd];

        DepthAttachment.Format      = DSVFormat;
        DepthAttachment.SampleCount = SampleCount;
        DepthAttachment.LoadOp      = ATTACHMENT_LOAD_OP_LOAD; // previous contents of the image within the render area
                                                               // will be preserved. For attachments with a depth/stencil format,
                                                               // this uses the access type VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT.
        DepthAttachment.StoreOp = ATTACHMENT_STORE_OP_STORE;   // the contents generated during the render pass and within the render
                                                               // area are written to memory. For attachments with a depth/stencil format,
                                                               // this uses the access type VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT.
        DepthAttachment.StencilLoadOp  = ATTACHMENT_LOAD_OP_LOAD;
        DepthAttachment.StencilStoreOp = ATTACHMENT_STORE_OP_STORE;
        DepthAttachment.InitialState   = DepthAttachmentState;
        DepthAttachment.FinalState     = DepthAttachmentState;

        pDepthAttachmentReference                  = &AttachmentReferences[AttachmentInd];
        pDepthAttachmentReference->AttachmentIndex = AttachmentInd;
        pDepthAttachmentReference->State           = DepthAttachmentState;

        ++AttachmentInd;
    }

    AttachmentReference* pColorAttachmentsReference = NumRenderTargets > 0 ? &AttachmentReferences[AttachmentInd] : nullptr;
    for (Uint32 rt = 0; rt < NumRenderTargets; ++rt)
    {
        auto& ColorAttachmentRef = pColorAttachmentsReference[rt];

        if (RTVFormats[rt] == TEX_FORMAT_UNKNOWN)
        {
            ColorAttachmentRef.AttachmentIndex = ATTACHMENT_UNUSED;
            continue;
        }

        auto& ColorAttachment = Attachments[AttachmentInd];

        ColorAttachment.Format      = RTVFormats[rt];
        ColorAttachment.SampleCount = SampleCount;
        ColorAttachment.LoadOp      = ATTACHMENT_LOAD_OP_LOAD; // previous contents of the image within the render area
                                                               // will be preserved. For attachments with a depth/stencil format,
                                                               // this uses the access type VK_ACCESS_COLOR_ATTACHMENT_READ_BIT.
        ColorAttachment.StoreOp = ATTACHMENT_STORE_OP_STORE;   // the contents generated during the render pass and within the render
                                                               // area are written to memory. For attachments with a color format,
                                                               // this uses the access type VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT.
        ColorAttachment.StencilLoadOp  = ATTACHMENT_LOAD_OP_DISCARD;
        ColorAttachment.StencilStoreOp = ATTACHMENT_STORE_OP_DISCARD;
        ColorAttachment.InitialState   = RESOURCE_STATE_RENDER_TARGET;
        ColorAttachment.FinalState     = RESOURCE_STATE_RENDER_TARGET;

        ColorAttachmentRef.AttachmentIndex = AttachmentInd;
        ColorAttachmentRef.State           = RESOURCE_STATE_RENDER_TARGET;

        ++AttachmentInd;
    }

    if (ShadingRateTexFormat != TEX_FORMAT_UNKNOWN)
    {
        auto& SRAttachment = Attachments[AttachmentInd];

        SRAttachment.Format         = ShadingRateTexFormat;
        SRAttachment.SampleCount    = 1;
        SRAttachment.LoadOp         = ATTACHMENT_LOAD_OP_LOAD;
        SRAttachment.StoreOp        = ATTACHMENT_STORE_OP_DISCARD;
        SRAttachment.StencilLoadOp  = ATTACHMENT_LOAD_OP_DISCARD;
        SRAttachment.StencilStoreOp = ATTACHMENT_STORE_OP_DISCARD;
        SRAttachment.InitialState   = RESOURCE_STATE_SHADING_RATE;
        SRAttachment.FinalState     = RESOURCE_STATE_SHADING_RATE;

        ShadingRateAttachment.Attachment.AttachmentIndex = AttachmentInd;
        ShadingRateAttachment.Attachment.State           = RESOURCE_STATE_SHADING_RATE;

        ShadingRateAttachment.TileSize[0] = ShadingRateTileSize.x;
        ShadingRateAttachment.TileSize[1] = ShadingRateTileSize.y;

        SubpassDesc.pShadingRateAttachment = &ShadingRateAttachment;

        ++AttachmentInd;
    }

    RPDesc.pAttachments    = Attachments.data();
    RPDesc.SubpassCount    = 1;
    RPDesc.pSubpasses      = &SubpassDesc;
    RPDesc.DependencyCount = 0;       // the number of dependencies between pairs of subpasses, or zero indicating no dependencies.
    RPDesc.pDependencies   = nullptr; // an array of dependencyCount number of VkSubpassDependency structures describing
                                      // dependencies between pairs of subpasses, or NULL if dependencyCount is zero.


    SubpassDesc.InputAttachmentCount        = 0;
    SubpassDesc.pInputAttachments           = nullptr;
    SubpassDesc.RenderTargetAttachmentCount = NumRenderTargets;
    SubpassDesc.pRenderTargetAttachments    = pColorAttachmentsReference;
    SubpassDesc.pResolveAttachments         = nullptr;
    SubpassDesc.pDepthStencilAttachment     = pDepthAttachmentReference;
    SubpassDesc.PreserveAttachmentCount     = 0;
    SubpassDesc.pPreserveAttachments        = nullptr;

    return RPDesc;
}

RenderPassVkImpl* RenderPassCache::GetRenderPass(const RenderPassCacheKey& Key)
{
    std::lock_guard<std::mutex> Lock{m_Mutex};
    auto                        it = m_Cache.find(Key);
    if (it == m_Cache.end())
    {
        // Do not zero-initialize arrays
        std::array<RenderPassAttachmentDesc, MAX_RENDER_TARGETS + 2> Attachments;
        std::array<AttachmentReference, MAX_RENDER_TARGETS + 2>      AttachmentReferences;

        TEXTURE_FORMAT SRFormat = TEX_FORMAT_UNKNOWN;
        uint2          SRTileSize;
        if (Key.EnableVRS)
        {
            const auto& SRProps = m_DeviceVkImpl.GetAdapterInfo().ShadingRate;
            switch (SRProps.Format)
            {
                case SHADING_RATE_FORMAT_PALETTE: SRFormat = TEX_FORMAT_R8_UINT; break;
                case SHADING_RATE_FORMAT_UNORM8: SRFormat = TEX_FORMAT_RG8_UNORM; break;
                default:
                    UNEXPECTED("Unexpected shading rate format");
            }
            SRTileSize = {SRProps.MaxTileSize[0], SRProps.MaxTileSize[1]};
        }

        SubpassDesc           Subpass;
        ShadingRateAttachment ShadingRate;

        auto RPDesc = GetImplicitRenderPassDesc(Key.NumRenderTargets, Key.RTVFormats, Key.DSVFormat, Key.ReadOnlyDSV, Key.SampleCount, SRFormat, SRTileSize,
                                                Attachments, AttachmentReferences, Subpass, ShadingRate);

        std::stringstream PassNameSS;
        PassNameSS << "Implicit render pass: RT count: " << Uint32{Key.NumRenderTargets} << "; sample count: " << Uint32{Key.SampleCount}
                   << "; DSV Format: " << GetTextureFormatAttribs(Key.DSVFormat).Name;
        if (Key.NumRenderTargets > 0)
        {
            PassNameSS << (Key.NumRenderTargets > 1 ? "; RTV Formats: " : "; RTV Format: ");
            for (Uint32 rt = 0; rt < Key.NumRenderTargets; ++rt)
            {
                PassNameSS << (rt > 0 ? ", " : "") << GetTextureFormatAttribs(Key.RTVFormats[rt]).Name;
            }
        }
        if (Key.EnableVRS)
            PassNameSS << "; VRS";

        const auto PassName{PassNameSS.str()};
        RPDesc.Name = PassName.c_str();

        RefCntAutoPtr<RenderPassVkImpl> pRenderPass;
        m_DeviceVkImpl.CreateRenderPass(RPDesc, pRenderPass.RawDblPtr<IRenderPass>(), /* IsDeviceInternal = */ true);
        if (pRenderPass == nullptr)
        {
            UNEXPECTED("Failed to create render pass");
            return nullptr;
        }
        it = m_Cache.emplace(Key, std::move(pRenderPass)).first;
    }

    return it->second;
}

} // namespace Diligent
