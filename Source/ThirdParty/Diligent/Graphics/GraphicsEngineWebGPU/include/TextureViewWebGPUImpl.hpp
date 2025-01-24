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

#pragma once

/// \file
/// Declaration of Diligent::TextureViewWebGPUImpl class

#include "EngineWebGPUImplTraits.hpp"
#include "TextureViewBase.hpp"
#include "WebGPUObjectWrappers.hpp"

namespace Diligent
{

/// Texture view implementation in WebGPU backend.
class TextureViewWebGPUImpl final : public TextureViewBase<EngineWebGPUImplTraits>
{
public:
    using TTextureViewBase = TextureViewBase<EngineWebGPUImplTraits>;

    TextureViewWebGPUImpl(IReferenceCounters*                     pRefCounters,
                          RenderDeviceWebGPUImpl*                 pDevice,
                          const TextureViewDesc&                  ViewDesc,
                          ITexture*                               pTexture,
                          WebGPUTextureViewWrapper&&              wgpuTextureView,
                          std::vector<WebGPUTextureViewWrapper>&& wgpuTextureMipSRVs,
                          std::vector<WebGPUTextureViewWrapper>&& wgpuTextureMipUAVs,
                          bool                                    bIsDefaultView,
                          bool                                    bIsDeviceInternal);

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_TextureViewWebGPU, TTextureViewBase)

    /// Implementation of ITextureViewWebGPU::GetWebGPUTextureView() in WebGPU backend.
    WGPUTextureView DILIGENT_CALL_TYPE GetWebGPUTextureView() const override;

    WGPUTextureView GetMipLevelUAV(Uint32 Mip);

    WGPUTextureView GetMipLevelSRV(Uint32 Mip);

    WGPUTextureView GetMipLevelRTV(Uint32 Slice, Uint32 Mip);

    WGPUTextureView GetMipLevelSRV(Uint32 Slice, Uint32 Mip);

private:
    WebGPUTextureViewWrapper              m_wgpuTextureView;
    std::vector<WebGPUTextureViewWrapper> m_wgpuTextureMipSRVs;
    std::vector<WebGPUTextureViewWrapper> m_wgpuTextureMipUAVs;
};

} // namespace Diligent
