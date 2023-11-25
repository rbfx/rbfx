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
#include "SamplerVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "VulkanTypeConversions.hpp"
#include "GraphicsAccessories.hpp"
#include "BasicMath.hpp"

namespace Diligent
{

SamplerVkImpl::SamplerVkImpl(IReferenceCounters* pRefCounters, RenderDeviceVkImpl* pRenderDeviceVk, const SamplerDesc& SamplerDesc) :
    // clang-format off
    TSamplerBase
    {
        pRefCounters,
        pRenderDeviceVk,
        SamplerDesc
    }
// clang-format on
{
    const auto& LogicalDevice = pRenderDeviceVk->GetLogicalDevice();
    const auto& Limits        = pRenderDeviceVk->GetPhysicalDevice().GetProperties().limits;

    VkSamplerCreateInfo SamplerCI{};
    SamplerCI.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCI.pNext        = nullptr;
    SamplerCI.flags        = 0; // reserved for future use
    SamplerCI.magFilter    = FilterTypeToVkFilter(m_Desc.MagFilter);
    SamplerCI.minFilter    = FilterTypeToVkFilter(m_Desc.MinFilter);
    SamplerCI.mipmapMode   = FilterTypeToVkMipmapMode(m_Desc.MipFilter);
    SamplerCI.addressModeU = AddressModeToVkAddressMode(m_Desc.AddressU);
    SamplerCI.addressModeV = AddressModeToVkAddressMode(m_Desc.AddressV);
    SamplerCI.addressModeW = AddressModeToVkAddressMode(m_Desc.AddressW);
    SamplerCI.mipLodBias   = m_Desc.MipLODBias;

    SamplerCI.anisotropyEnable = IsAnisotropicFilter(m_Desc.MinFilter);
    DEV_CHECK_ERR(!SamplerCI.anisotropyEnable || (m_Desc.MaxAnisotropy >= 1 && static_cast<float>(m_Desc.MaxAnisotropy) <= Limits.maxSamplerAnisotropy),
                  "MaxAnisotropy (", m_Desc.MaxAnisotropy, ") must be in range 1 .. ", Limits.maxSamplerAnisotropy, ".");
    SamplerCI.maxAnisotropy = SamplerCI.anisotropyEnable ? clamp(static_cast<float>(m_Desc.MaxAnisotropy), 1.f, Limits.maxSamplerAnisotropy) : 0.f;
    DEV_CHECK_ERR((SamplerCI.anisotropyEnable != VK_FALSE) == IsAnisotropicFilter(m_Desc.MagFilter),
                  "Min and mag filters must both be either anisotropic filters or non-anisotropic ones");

    SamplerCI.compareEnable = IsComparisonFilter(m_Desc.MinFilter);
    DEV_CHECK_ERR((SamplerCI.compareEnable != VK_FALSE) == IsComparisonFilter(m_Desc.MagFilter),
                  "Min and mag filters must both be either comparison filters or non-comparison ones");

    SamplerCI.compareOp               = ComparisonFuncToVkCompareOp(m_Desc.ComparisonFunc);
    SamplerCI.minLod                  = m_Desc.UnnormalizedCoords ? 0 : m_Desc.MinLOD;
    SamplerCI.maxLod                  = m_Desc.UnnormalizedCoords ? 0 : m_Desc.MaxLOD;
    SamplerCI.borderColor             = BorderColorToVkBorderColor(m_Desc.BorderColor);
    SamplerCI.unnormalizedCoordinates = m_Desc.UnnormalizedCoords ? VK_TRUE : VK_FALSE;

    if (m_Desc.Flags & SAMPLER_FLAG_SUBSAMPLED)
    {
        SamplerCI.flags |= VK_SAMPLER_CREATE_SUBSAMPLED_BIT_EXT;
    }
    if (m_Desc.Flags & SAMPLER_FLAG_SUBSAMPLED_COARSE_RECONSTRUCTION)
        SamplerCI.flags |= VK_SAMPLER_CREATE_SUBSAMPLED_COARSE_RECONSTRUCTION_BIT_EXT;

    m_VkSampler = LogicalDevice.CreateSampler(SamplerCI);
}

SamplerVkImpl::SamplerVkImpl(IReferenceCounters* pRefCounters, const SamplerDesc& SamplerDesc) noexcept :
    TSamplerBase{pRefCounters, SamplerDesc}
{
}

SamplerVkImpl::~SamplerVkImpl()
{
    if (m_VkSampler)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_VkSampler), m_ImmediateContextMask);
}

} // namespace Diligent
