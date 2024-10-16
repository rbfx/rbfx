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

#include "DXGITypeConversions.hpp"
#include "EngineFactoryBase.hpp"

/// \file
/// Implementation of the Diligent::EngineFactoryD3DBase template class

namespace Diligent
{

bool CheckAdapterD3D11Compatibility(IDXGIAdapter1* pDXGIAdapter, D3D_FEATURE_LEVEL FeatureLevel);
bool CheckAdapterD3D12Compatibility(IDXGIAdapter1* pDXGIAdapter, D3D_FEATURE_LEVEL FeatureLevel);

template <typename BaseInterface, RENDER_DEVICE_TYPE DevType>
class EngineFactoryD3DBase : public EngineFactoryBase<BaseInterface>
{
public:
    using TEngineFactoryBase = EngineFactoryBase<BaseInterface>;

    EngineFactoryD3DBase(const INTERFACE_ID& FactoryIID) :
        TEngineFactoryBase{FactoryIID}
    {}


    virtual void DILIGENT_CALL_TYPE EnumerateAdapters(Version              MinVersion,
                                                      Uint32&              NumAdapters,
                                                      GraphicsAdapterInfo* Adapters) const override
    {
        auto DXGIAdapters = FindCompatibleAdapters(MinVersion);

        if (Adapters == nullptr)
            NumAdapters = static_cast<Uint32>(DXGIAdapters.size());
        else
        {
            NumAdapters = std::min(NumAdapters, static_cast<Uint32>(DXGIAdapters.size()));
            for (Uint32 adapter = 0; adapter < NumAdapters; ++adapter)
            {
                IDXGIAdapter1* pDXIAdapter = DXGIAdapters[adapter];
                auto&          AdapterInfo = Adapters[adapter];

                AdapterInfo = GetGraphicsAdapterInfo(nullptr, pDXIAdapter);

                AdapterInfo.NumOutputs = 0;
                CComPtr<IDXGIOutput> pOutput;
                while (pDXIAdapter->EnumOutputs(AdapterInfo.NumOutputs, &pOutput) != DXGI_ERROR_NOT_FOUND)
                {
                    ++AdapterInfo.NumOutputs;
                    pOutput.Release();
                };
            }
        }
    }


    virtual void DILIGENT_CALL_TYPE EnumerateDisplayModes(Version             MinVersion,
                                                          Uint32              AdapterId,
                                                          Uint32              OutputId,
                                                          TEXTURE_FORMAT      Format,
                                                          Uint32&             NumDisplayModes,
                                                          DisplayModeAttribs* DisplayModes) override
    {
        auto DXGIAdapters = FindCompatibleAdapters(MinVersion);
        if (AdapterId >= DXGIAdapters.size())
        {
            LOG_ERROR("Incorrect adapter id ", AdapterId);
            return;
        }

        IDXGIAdapter1* pDXIAdapter = DXGIAdapters[AdapterId];

        DXGI_FORMAT          DXIGFormat = TexFormatToDXGI_Format(Format);
        CComPtr<IDXGIOutput> pOutput;
        if (pDXIAdapter->EnumOutputs(OutputId, &pOutput) == DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC1 AdapterDesc;
            pDXIAdapter->GetDesc1(&AdapterDesc);
            char DescriptionMB[_countof(AdapterDesc.Description)];
            WideCharToMultiByte(CP_ACP, 0, AdapterDesc.Description, -1, DescriptionMB, _countof(DescriptionMB), NULL, FALSE);
            LOG_ERROR_MESSAGE("Failed to enumerate output ", OutputId, " of adapter ", AdapterId, " (", DescriptionMB, ')');
            return;
        }

        UINT numModes = 0;

        // Get the number of elements
        auto hr = pOutput->GetDisplayModeList(DXIGFormat, 0, &numModes, NULL);
        (void)hr; // Suppress warning

        if (DisplayModes != nullptr)
        {
            // Get the list
            std::vector<DXGI_MODE_DESC> DXIDisplayModes(numModes);
            hr = pOutput->GetDisplayModeList(DXIGFormat, 0, &numModes, DXIDisplayModes.data());
            (void)hr; // Suppress warning

            for (Uint32 m = 0; m < std::min(NumDisplayModes, numModes); ++m)
            {
                const auto& SrcMode            = DXIDisplayModes[m];
                auto&       DstMode            = DisplayModes[m];
                DstMode.Width                  = SrcMode.Width;
                DstMode.Height                 = SrcMode.Height;
                DstMode.Format                 = DXGI_FormatToTexFormat(SrcMode.Format);
                DstMode.RefreshRateNumerator   = SrcMode.RefreshRate.Numerator;
                DstMode.RefreshRateDenominator = SrcMode.RefreshRate.Denominator;
                DstMode.Scaling                = static_cast<SCALING_MODE>(SrcMode.Scaling);
                DstMode.ScanlineOrder          = static_cast<SCANLINE_ORDER>(SrcMode.ScanlineOrdering);
            }
            NumDisplayModes = std::min(NumDisplayModes, numModes);
        }
        else
        {
            NumDisplayModes = numModes;
        }
    }


    std::vector<CComPtr<IDXGIAdapter1>> FindCompatibleAdapters(Version MinVersion) const
    {
        std::vector<CComPtr<IDXGIAdapter1>> DXGIAdapters;

        CComPtr<IDXGIFactory2> pFactory;
        if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)&pFactory)))
        {
            LOG_ERROR_MESSAGE("Failed to create DXGI Factory");
            return DXGIAdapters;
        }

        CComPtr<IDXGIAdapter1> pDXIAdapter;

        const auto d3dFeatureLevel = GetD3DFeatureLevel(MinVersion);
        for (UINT adapter = 0; pFactory->EnumAdapters1(adapter, &pDXIAdapter) != DXGI_ERROR_NOT_FOUND; ++adapter, pDXIAdapter.Release())
        {
            DXGI_ADAPTER_DESC1 AdapterDesc;
            pDXIAdapter->GetDesc1(&AdapterDesc);
            bool IsCompatibleAdapter = CheckAdapterCompatibility<DevType>(pDXIAdapter, d3dFeatureLevel);
            if (IsCompatibleAdapter)
            {
                DXGIAdapters.emplace_back(std::move(pDXIAdapter));
            }
        }

        return DXGIAdapters;
    }


    virtual GraphicsAdapterInfo GetGraphicsAdapterInfo(void*          pd3Device,
                                                       IDXGIAdapter1* pDXIAdapter) const
    {
        DXGI_ADAPTER_DESC1 dxgiAdapterDesc = {};
        if (pDXIAdapter)
            pDXIAdapter->GetDesc1(&dxgiAdapterDesc);

        GraphicsAdapterInfo AdapterInfo;

        // Set graphics adapter properties
        {
            WideCharToMultiByte(CP_ACP, 0, dxgiAdapterDesc.Description, -1, AdapterInfo.Description, _countof(AdapterInfo.Description), NULL, FALSE);

            if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                AdapterInfo.Type = ADAPTER_TYPE_SOFTWARE;
            else if (dxgiAdapterDesc.DedicatedVideoMemory != 0)
                AdapterInfo.Type = ADAPTER_TYPE_DISCRETE;
            else
                AdapterInfo.Type = ADAPTER_TYPE_INTEGRATED;

            AdapterInfo.Vendor     = VendorIdToAdapterVendor(dxgiAdapterDesc.VendorId);
            AdapterInfo.VendorId   = dxgiAdapterDesc.VendorId;
            AdapterInfo.DeviceId   = dxgiAdapterDesc.DeviceId;
            AdapterInfo.NumOutputs = 0;
        }

        // Enable features
        {
            auto& Features{AdapterInfo.Features};
            Features.SeparablePrograms             = DEVICE_FEATURE_STATE_ENABLED;
            Features.ShaderResourceQueries         = DEVICE_FEATURE_STATE_ENABLED;
            Features.WireframeFill                 = DEVICE_FEATURE_STATE_ENABLED;
            Features.MultithreadedResourceCreation = DEVICE_FEATURE_STATE_ENABLED;
            Features.ComputeShaders                = DEVICE_FEATURE_STATE_ENABLED;
            Features.GeometryShaders               = DEVICE_FEATURE_STATE_ENABLED;
            Features.Tessellation                  = DEVICE_FEATURE_STATE_ENABLED;
            Features.OcclusionQueries              = DEVICE_FEATURE_STATE_ENABLED;
            Features.BinaryOcclusionQueries        = DEVICE_FEATURE_STATE_ENABLED;
            Features.TimestampQueries              = DEVICE_FEATURE_STATE_ENABLED;
            Features.PipelineStatisticsQueries     = DEVICE_FEATURE_STATE_ENABLED;
            Features.DurationQueries               = DEVICE_FEATURE_STATE_ENABLED;
            Features.DepthBiasClamp                = DEVICE_FEATURE_STATE_ENABLED;
            Features.DepthClamp                    = DEVICE_FEATURE_STATE_ENABLED;
            Features.IndependentBlend              = DEVICE_FEATURE_STATE_ENABLED;
            Features.DualSourceBlend               = DEVICE_FEATURE_STATE_ENABLED;
            Features.MultiViewport                 = DEVICE_FEATURE_STATE_ENABLED;
            Features.TextureCompressionBC          = DEVICE_FEATURE_STATE_ENABLED;
            Features.PixelUAVWritesAndAtomics      = DEVICE_FEATURE_STATE_ENABLED;
            Features.TextureUAVExtendedFormats     = DEVICE_FEATURE_STATE_ENABLED;
            Features.ShaderResourceStaticArrays    = DEVICE_FEATURE_STATE_ENABLED;
            Features.InstanceDataStepRate          = DEVICE_FEATURE_STATE_ENABLED;
            Features.TileShaders                   = DEVICE_FEATURE_STATE_DISABLED;
            Features.SubpassFramebufferFetch       = DEVICE_FEATURE_STATE_DISABLED;
            Features.TextureComponentSwizzle       = DEVICE_FEATURE_STATE_DISABLED;
            Features.TextureSubresourceViews       = DEVICE_FEATURE_STATE_ENABLED;
            Features.NativeMultiDraw               = DEVICE_FEATURE_STATE_DISABLED;
            Features.AsyncShaderCompilation        = DEVICE_FEATURE_STATE_ENABLED;
            Features.FormattedBuffers              = DEVICE_FEATURE_STATE_ENABLED;
        }

        // Set memory properties
        {
            auto& Mem               = AdapterInfo.Memory;
            Mem.LocalMemory         = dxgiAdapterDesc.DedicatedVideoMemory;
            Mem.HostVisibleMemory   = dxgiAdapterDesc.SharedSystemMemory;
            Mem.UnifiedMemory       = 0;
            Mem.MaxMemoryAllocation = 0; // no way to query

            ASSERT_SIZEOF(Mem, 40, "Did you add a new member to AdapterMemoryInfo? Please initialize it here.");
        }

        // Draw command properties
        {
            auto& DrawCommand{AdapterInfo.DrawCommand};
            DrawCommand.MaxDrawIndirectCount = ~0u;
            DrawCommand.CapFlags =
                DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT |
                DRAW_COMMAND_CAP_FLAG_DRAW_INDIRECT_FIRST_INSTANCE;
        }

        // Set queue info
        {
            AdapterInfo.NumQueues                           = 1;
            AdapterInfo.Queues[0].QueueType                 = COMMAND_QUEUE_TYPE_GRAPHICS;
            AdapterInfo.Queues[0].MaxDeviceContexts         = 1;
            AdapterInfo.Queues[0].TextureCopyGranularity[0] = 1;
            AdapterInfo.Queues[0].TextureCopyGranularity[1] = 1;
            AdapterInfo.Queues[0].TextureCopyGranularity[2] = 1;
        }

        return AdapterInfo;
    }

protected:
    static D3D_FEATURE_LEVEL GetD3DFeatureLevel(Version MinVersion)
    {
        const D3D_FEATURE_LEVEL FeatureLevel = static_cast<D3D_FEATURE_LEVEL>((Uint32{MinVersion.Major} << 12u) | (Uint32{MinVersion.Minor} << 8u));

#ifdef DILIGENT_DEBUG
        switch (MinVersion.Major)
        {
            case 10:
                switch (MinVersion.Minor)
                {
                    case 0: VERIFY_EXPR(FeatureLevel == D3D_FEATURE_LEVEL_10_0); break;
                    case 1: VERIFY_EXPR(FeatureLevel == D3D_FEATURE_LEVEL_10_1); break;
                    default: UNEXPECTED("unknown feature level 10.", Uint32{MinVersion.Minor});
                }
                break;
            case 11:
                switch (MinVersion.Minor)
                {
                    case 0: VERIFY_EXPR(FeatureLevel == D3D_FEATURE_LEVEL_11_0); break;
                    case 1: VERIFY_EXPR(FeatureLevel == D3D_FEATURE_LEVEL_11_1); break;
                    default: UNEXPECTED("unknown feature level 11.", Uint32{MinVersion.Minor});
                }
                break;
#    if defined(_WIN32_WINNT_WIN10) && (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
            case 12:
                switch (MinVersion.Minor)
                {
                    case 0: VERIFY_EXPR(FeatureLevel == D3D_FEATURE_LEVEL_12_0); break;
                    case 1: VERIFY_EXPR(FeatureLevel == D3D_FEATURE_LEVEL_12_1); break;
                    default: UNEXPECTED("unknown feature level 12.", Uint32{MinVersion.Minor});
                }
                break;
#    endif
            default:
                UNEXPECTED("Unknown major version of the feature level");
        }
#endif

        return FeatureLevel;
    }

private:
    template <RENDER_DEVICE_TYPE MyDevType>
    bool CheckAdapterCompatibility(IDXGIAdapter1*    pDXGIAdapter,
                                   D3D_FEATURE_LEVEL FeatureLevels) const;

    template <>
    bool CheckAdapterCompatibility<RENDER_DEVICE_TYPE_D3D11>(IDXGIAdapter1*    pDXGIAdapter,
                                                             D3D_FEATURE_LEVEL FeatureLevel) const
    {
        return CheckAdapterD3D11Compatibility(pDXGIAdapter, FeatureLevel);
    }

    template <>
    bool CheckAdapterCompatibility<RENDER_DEVICE_TYPE_D3D12>(IDXGIAdapter1*    pDXGIAdapter,
                                                             D3D_FEATURE_LEVEL FeatureLevel) const
    {
        return CheckAdapterD3D12Compatibility(pDXGIAdapter, FeatureLevel);
    }
};

} // namespace Diligent
