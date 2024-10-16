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

#pragma once

/// \file
/// Implementation of the Diligent::RenderDeviceBase template class and related structures

#include "WinHPreface.h"
#include <dxgi.h>
#include "WinHPostface.h"

#include "RenderDeviceBase.hpp"
#include "NVApiLoader.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

/// Base implementation of a D3D render device

template <typename EngineImplTraits>
class RenderDeviceD3DBase : public RenderDeviceBase<EngineImplTraits>
{
public:
    RenderDeviceD3DBase(IReferenceCounters*        pRefCounters,
                        IMemoryAllocator&          RawMemAllocator,
                        IEngineFactory*            pEngineFactory,
                        const EngineCreateInfo&    EngineCI,
                        const GraphicsAdapterInfo& AdapterInfo) :
        RenderDeviceBase<EngineImplTraits>{pRefCounters, RawMemAllocator, pEngineFactory, EngineCI, AdapterInfo}
    {
        // Flag texture formats always supported in D3D11 and D3D12

        // clang-format off
#define FLAG_FORMAT(Fmt, IsSupported) this->m_TextureFormatsInfo[Fmt].Supported=IsSupported

        FLAG_FORMAT(TEX_FORMAT_RGBA32_TYPELESS,            true);
        FLAG_FORMAT(TEX_FORMAT_RGBA32_FLOAT,               true);
        FLAG_FORMAT(TEX_FORMAT_RGBA32_UINT,                true);
        FLAG_FORMAT(TEX_FORMAT_RGBA32_SINT,                true);
        FLAG_FORMAT(TEX_FORMAT_RGB32_TYPELESS,             true);
        FLAG_FORMAT(TEX_FORMAT_RGB32_FLOAT,                true);
        FLAG_FORMAT(TEX_FORMAT_RGB32_UINT,                 true);
        FLAG_FORMAT(TEX_FORMAT_RGB32_SINT,                 true);
        FLAG_FORMAT(TEX_FORMAT_RGBA16_TYPELESS,            true);
        FLAG_FORMAT(TEX_FORMAT_RGBA16_FLOAT,               true);
        FLAG_FORMAT(TEX_FORMAT_RGBA16_UNORM,               true);
        FLAG_FORMAT(TEX_FORMAT_RGBA16_UINT,                true);
        FLAG_FORMAT(TEX_FORMAT_RGBA16_SNORM,               true);
        FLAG_FORMAT(TEX_FORMAT_RGBA16_SINT,                true);
        FLAG_FORMAT(TEX_FORMAT_RG32_TYPELESS,              true);
        FLAG_FORMAT(TEX_FORMAT_RG32_FLOAT,                 true);
        FLAG_FORMAT(TEX_FORMAT_RG32_UINT,                  true);
        FLAG_FORMAT(TEX_FORMAT_RG32_SINT,                  true);
        FLAG_FORMAT(TEX_FORMAT_R32G8X24_TYPELESS,          true);
        FLAG_FORMAT(TEX_FORMAT_D32_FLOAT_S8X24_UINT,       true);
        FLAG_FORMAT(TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS,   true);
        FLAG_FORMAT(TEX_FORMAT_X32_TYPELESS_G8X24_UINT,    true);
        FLAG_FORMAT(TEX_FORMAT_RGB10A2_TYPELESS,           true);
        FLAG_FORMAT(TEX_FORMAT_RGB10A2_UNORM,              true);
        FLAG_FORMAT(TEX_FORMAT_RGB10A2_UINT,               true);
        FLAG_FORMAT(TEX_FORMAT_R11G11B10_FLOAT,            true);
        FLAG_FORMAT(TEX_FORMAT_RGBA8_TYPELESS,             true);
        FLAG_FORMAT(TEX_FORMAT_RGBA8_UNORM,                true);
        FLAG_FORMAT(TEX_FORMAT_RGBA8_UNORM_SRGB,           true);
        FLAG_FORMAT(TEX_FORMAT_RGBA8_UINT,                 true);
        FLAG_FORMAT(TEX_FORMAT_RGBA8_SNORM,                true);
        FLAG_FORMAT(TEX_FORMAT_RGBA8_SINT,                 true);
        FLAG_FORMAT(TEX_FORMAT_RG16_TYPELESS,              true);
        FLAG_FORMAT(TEX_FORMAT_RG16_FLOAT,                 true);
        FLAG_FORMAT(TEX_FORMAT_RG16_UNORM,                 true);
        FLAG_FORMAT(TEX_FORMAT_RG16_UINT,                  true);
        FLAG_FORMAT(TEX_FORMAT_RG16_SNORM,                 true);
        FLAG_FORMAT(TEX_FORMAT_RG16_SINT,                  true);
        FLAG_FORMAT(TEX_FORMAT_R32_TYPELESS,               true);
        FLAG_FORMAT(TEX_FORMAT_D32_FLOAT,                  true);
        FLAG_FORMAT(TEX_FORMAT_R32_FLOAT,                  true);
        FLAG_FORMAT(TEX_FORMAT_R32_UINT,                   true);
        FLAG_FORMAT(TEX_FORMAT_R32_SINT,                   true);
        FLAG_FORMAT(TEX_FORMAT_R24G8_TYPELESS,             true);
        FLAG_FORMAT(TEX_FORMAT_D24_UNORM_S8_UINT,          true);
        FLAG_FORMAT(TEX_FORMAT_R24_UNORM_X8_TYPELESS,      true);
        FLAG_FORMAT(TEX_FORMAT_X24_TYPELESS_G8_UINT,       true);
        FLAG_FORMAT(TEX_FORMAT_RG8_TYPELESS,               true);
        FLAG_FORMAT(TEX_FORMAT_RG8_UNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_RG8_UINT,                   true);
        FLAG_FORMAT(TEX_FORMAT_RG8_SNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_RG8_SINT,                   true);
        FLAG_FORMAT(TEX_FORMAT_R16_TYPELESS,               true);
        FLAG_FORMAT(TEX_FORMAT_R16_FLOAT,                  true);
        FLAG_FORMAT(TEX_FORMAT_D16_UNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_R16_UNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_R16_UINT,                   true);
        FLAG_FORMAT(TEX_FORMAT_R16_SNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_R16_SINT,                   true);
        FLAG_FORMAT(TEX_FORMAT_R8_TYPELESS,                true);
        FLAG_FORMAT(TEX_FORMAT_R8_UNORM,                   true);
        FLAG_FORMAT(TEX_FORMAT_R8_UINT,                    true);
        FLAG_FORMAT(TEX_FORMAT_R8_SNORM,                   true);
        FLAG_FORMAT(TEX_FORMAT_R8_SINT,                    true);
        FLAG_FORMAT(TEX_FORMAT_A8_UNORM,                   true);
        FLAG_FORMAT(TEX_FORMAT_R1_UNORM,                   true);
        FLAG_FORMAT(TEX_FORMAT_RGB9E5_SHAREDEXP,           true);
        FLAG_FORMAT(TEX_FORMAT_RG8_B8G8_UNORM,             true);
        FLAG_FORMAT(TEX_FORMAT_G8R8_G8B8_UNORM,            true);
        FLAG_FORMAT(TEX_FORMAT_BC1_TYPELESS,               true);
        FLAG_FORMAT(TEX_FORMAT_BC1_UNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_BC1_UNORM_SRGB,             true);
        FLAG_FORMAT(TEX_FORMAT_BC2_TYPELESS,               true);
        FLAG_FORMAT(TEX_FORMAT_BC2_UNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_BC2_UNORM_SRGB,             true);
        FLAG_FORMAT(TEX_FORMAT_BC3_TYPELESS,               true);
        FLAG_FORMAT(TEX_FORMAT_BC3_UNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_BC3_UNORM_SRGB,             true);
        FLAG_FORMAT(TEX_FORMAT_BC4_TYPELESS,               true);
        FLAG_FORMAT(TEX_FORMAT_BC4_UNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_BC4_SNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_BC5_TYPELESS,               true);
        FLAG_FORMAT(TEX_FORMAT_BC5_UNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_BC5_SNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_B5G6R5_UNORM,               true);
        FLAG_FORMAT(TEX_FORMAT_B5G5R5A1_UNORM,             true);
        FLAG_FORMAT(TEX_FORMAT_BGRA8_UNORM,                true);
        FLAG_FORMAT(TEX_FORMAT_BGRX8_UNORM,                true);
        FLAG_FORMAT(TEX_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, true);
        FLAG_FORMAT(TEX_FORMAT_BGRA8_TYPELESS,             true);
        FLAG_FORMAT(TEX_FORMAT_BGRA8_UNORM_SRGB,           true);
        FLAG_FORMAT(TEX_FORMAT_BGRX8_TYPELESS,             true);
        FLAG_FORMAT(TEX_FORMAT_BGRX8_UNORM_SRGB,           true);
        FLAG_FORMAT(TEX_FORMAT_BC6H_TYPELESS,              true);
        FLAG_FORMAT(TEX_FORMAT_BC6H_UF16,                  true);
        FLAG_FORMAT(TEX_FORMAT_BC6H_SF16,                  true);
        FLAG_FORMAT(TEX_FORMAT_BC7_TYPELESS,               true);
        FLAG_FORMAT(TEX_FORMAT_BC7_UNORM,                  true);
        FLAG_FORMAT(TEX_FORMAT_BC7_UNORM_SRGB,             true);
#undef FLAG_FORMAT
        // clang-format on

        this->m_DeviceInfo.NDC = NDCAttribs{0.0f, 1.0f, -0.5f};

        if (this->m_AdapterInfo.Vendor == ADAPTER_VENDOR_NVIDIA)
        {
            m_NVApi.Load();
        }
    }

    ~RenderDeviceD3DBase()
    {
        // There is a problem with NVApi: the dll must be unloaded only after the last
        // reference to Direct3D Device has been released, otherwise Release() will crash.
        // We cannot guarantee this because the engine may be attached to existing
        // D3D11/D3D12 device. So we have to keep the DLL loaded.
        m_NVApi.Invalidate();
    }

    bool IsNvApiEnabled() const
    {
        return m_NVApi.IsLoaded();
    }

protected:
    virtual SparseTextureFormatInfo DILIGENT_CALL_TYPE GetSparseTextureFormatInfo(TEXTURE_FORMAT     TexFormat,
                                                                                  RESOURCE_DIMENSION Dimension,
                                                                                  Uint32             SampleCount) const override
    {
        const auto ComponentType = CheckSparseTextureFormatSupport(TexFormat, Dimension, SampleCount, this->m_AdapterInfo.SparseResources);
        if (ComponentType == COMPONENT_TYPE_UNDEFINED)
            return {};

        TextureDesc TexDesc;
        TexDesc.Type        = Dimension;
        TexDesc.Format      = TexFormat;
        TexDesc.MipLevels   = 1;
        TexDesc.SampleCount = SampleCount;

        const auto SparseProps = GetStandardSparseTextureProperties(TexDesc);

        SparseTextureFormatInfo Info;
        Info.BindFlags   = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
        Info.TileSize[0] = SparseProps.TileSize[0];
        Info.TileSize[1] = SparseProps.TileSize[1];
        Info.TileSize[2] = SparseProps.TileSize[2];
        Info.Flags       = SparseProps.Flags;

        if (ComponentType == COMPONENT_TYPE_DEPTH || ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            Info.BindFlags |= BIND_DEPTH_STENCIL | BIND_INPUT_ATTACHMENT;
        else if (ComponentType != COMPONENT_TYPE_COMPRESSED)
            Info.BindFlags |= BIND_RENDER_TARGET | BIND_INPUT_ATTACHMENT;

        return Info;
    }

protected:
    NVApiLoader m_NVApi;
};

} // namespace Diligent
