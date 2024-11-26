/*
 *  Copyright 2023-2024 Diligent Graphics LLC
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

#include "TextureViewWebGPUImpl.hpp"
#include "RenderDeviceWebGPUImpl.hpp"
#include "DeviceContextWebGPUImpl.hpp"

namespace Diligent
{

TextureViewWebGPUImpl::TextureViewWebGPUImpl(IReferenceCounters*                     pRefCounters,
                                             RenderDeviceWebGPUImpl*                 pDevice,
                                             const TextureViewDesc&                  ViewDesc,
                                             ITexture*                               pTexture,
                                             WebGPUTextureViewWrapper&&              wgpuTextureView,
                                             std::vector<WebGPUTextureViewWrapper>&& wgpuTextureMipSRVs,
                                             std::vector<WebGPUTextureViewWrapper>&& wgpuTextureMipUAVs,
                                             bool                                    bIsDefaultView,
                                             bool                                    bIsDeviceInternal) :
    // clang-format off
    TTextureViewBase
    {
        pRefCounters,
        pDevice,
        ViewDesc,
        pTexture,
        bIsDefaultView,
        bIsDeviceInternal
    }, m_wgpuTextureView(std::move(wgpuTextureView))
     , m_wgpuTextureMipSRVs(std::move(wgpuTextureMipSRVs))
     , m_wgpuTextureMipUAVs(std::move(wgpuTextureMipUAVs))
// clang-format on
{
}

WGPUTextureView TextureViewWebGPUImpl::GetWebGPUTextureView() const
{
    return m_wgpuTextureView.Get();
}

WGPUTextureView TextureViewWebGPUImpl::GetMipLevelUAV(Uint32 Mip)
{
    VERIFY_EXPR((m_Desc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION) != 0 && !m_wgpuTextureMipUAVs.empty() && Mip < m_Desc.NumMipLevels);
    return m_wgpuTextureMipUAVs[Mip].Get();
}

WGPUTextureView TextureViewWebGPUImpl::GetMipLevelSRV(Uint32 Mip)
{
    VERIFY_EXPR((m_Desc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION) != 0 && !m_wgpuTextureMipSRVs.empty() && Mip < m_Desc.NumMipLevels);
    return m_wgpuTextureMipSRVs[Mip].Get();
}

WGPUTextureView TextureViewWebGPUImpl::GetMipLevelRTV(Uint32 Slice, Uint32 Mip)
{
    VERIFY_EXPR((m_Desc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION) != 0 && !m_wgpuTextureMipUAVs.empty() && Mip < m_Desc.NumMipLevels && Slice < m_Desc.NumArraySlices);
    return m_wgpuTextureMipUAVs[Mip + Slice * m_Desc.NumMipLevels].Get();
}

WGPUTextureView TextureViewWebGPUImpl::GetMipLevelSRV(Uint32 Slice, Uint32 Mip)
{
    VERIFY_EXPR((m_Desc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION) != 0 && !m_wgpuTextureMipSRVs.empty() && Mip < m_Desc.NumMipLevels && Slice < m_Desc.NumArraySlices);
    return m_wgpuTextureMipSRVs[Mip + Slice * m_Desc.NumMipLevels].Get();
}

} // namespace Diligent
