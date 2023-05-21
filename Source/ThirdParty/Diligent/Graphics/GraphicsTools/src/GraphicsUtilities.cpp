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

#include <algorithm>
#include <cmath>
#include <limits>

#include "GraphicsUtilities.h"
#include "DebugUtilities.hpp"
#include "GraphicsAccessories.hpp"
#include "ColorConversion.h"

#define PI_F 3.1415926f

namespace Diligent
{

void CreateUniformBuffer(IRenderDevice*   pDevice,
                         Uint64           Size,
                         const Char*      Name,
                         IBuffer**        ppBuffer,
                         USAGE            Usage,
                         BIND_FLAGS       BindFlags,
                         CPU_ACCESS_FLAGS CPUAccessFlags,
                         void*            pInitialData)
{
    BufferDesc CBDesc;
    CBDesc.Name           = Name;
    CBDesc.Size           = Size;
    CBDesc.Usage          = Usage;
    CBDesc.BindFlags      = BindFlags;
    CBDesc.CPUAccessFlags = CPUAccessFlags;

    BufferData InitialData;
    if (pInitialData != nullptr)
    {
        InitialData.pData    = pInitialData;
        InitialData.DataSize = Size;
    }
    pDevice->CreateBuffer(CBDesc, pInitialData != nullptr ? &InitialData : nullptr, ppBuffer);
}

template <class TConverter>
void GenerateCheckerBoardPatternInternal(Uint32 Width, Uint32 Height, TEXTURE_FORMAT Fmt, Uint32 HorzCells, Uint32 VertCells, Uint8* pData, Uint64 StrideInBytes, TConverter Converter)
{
    const auto& FmtAttribs = GetTextureFormatAttribs(Fmt);
    for (Uint32 y = 0; y < Height; ++y)
    {
        for (Uint32 x = 0; x < Width; ++x)
        {
            float horzWave   = std::sin((static_cast<float>(x) + 0.5f) / static_cast<float>(Width) * PI_F * static_cast<float>(HorzCells));
            float vertWave   = std::sin((static_cast<float>(y) + 0.5f) / static_cast<float>(Height) * PI_F * static_cast<float>(VertCells));
            float val        = horzWave * vertWave;
            val              = std::max(std::min(val * 20.f, +1.f), -1.f);
            val              = val * 0.5f + 1.f;
            val              = val * 0.5f + 0.25f;
            Uint8* pDstTexel = pData + x * size_t{FmtAttribs.NumComponents} * size_t{FmtAttribs.ComponentSize} + y * StrideInBytes;
            Converter(pDstTexel, Uint32{FmtAttribs.NumComponents}, val);
        }
    }
}

void GenerateCheckerBoardPattern(Uint32 Width, Uint32 Height, TEXTURE_FORMAT Fmt, Uint32 HorzCells, Uint32 VertCells, Uint8* pData, Uint64 StrideInBytes)
{
    const auto& FmtAttribs = GetTextureFormatAttribs(Fmt);
    switch (FmtAttribs.ComponentType)
    {
        case COMPONENT_TYPE_UINT:
        case COMPONENT_TYPE_UNORM:
            GenerateCheckerBoardPatternInternal(
                Width, Height, Fmt, HorzCells, VertCells, pData, StrideInBytes,
                [](Uint8* pDstTexel, Uint32 NumComponents, float fVal) //
                {
                    Uint8 uVal = static_cast<Uint8>(fVal * 255.f);
                    for (Uint32 c = 0; c < NumComponents; ++c)
                        pDstTexel[c] = uVal;
                } //
            );
            break;

        case COMPONENT_TYPE_UNORM_SRGB:
            GenerateCheckerBoardPatternInternal(
                Width, Height, Fmt, HorzCells, VertCells, pData, StrideInBytes,
                [](Uint8* pDstTexel, Uint32 NumComponents, float fVal) //
                {
                    Uint8 uVal = static_cast<Uint8>(FastLinearToSRGB(fVal) * 255.f);
                    for (Uint32 c = 0; c < NumComponents; ++c)
                        pDstTexel[c] = uVal;
                } //
            );
            break;

        case COMPONENT_TYPE_FLOAT:
            GenerateCheckerBoardPatternInternal(
                Width, Height, Fmt, HorzCells, VertCells, pData, StrideInBytes,
                [](Uint8* pDstTexel, Uint32 NumComponents, float fVal) //
                {
                    for (Uint32 c = 0; c < NumComponents; ++c)
                        (reinterpret_cast<float*>(pDstTexel))[c] = fVal;
                } //
            );
            break;

        default:
            UNSUPPORTED("Unsupported component type");
            return;
    }
}



template <typename ChannelType>
ChannelType SRGBAverage(ChannelType c0, ChannelType c1, ChannelType c2, ChannelType c3, Uint32 /*col*/, Uint32 /*row*/)
{
    static_assert(std::numeric_limits<ChannelType>::is_integer && !std::numeric_limits<ChannelType>::is_signed, "Unsigned integers are expected");

    static constexpr float MaxVal    = static_cast<float>(std::numeric_limits<ChannelType>::max());
    static constexpr float MaxValInv = 1.f / MaxVal;

    float fc0 = static_cast<float>(c0) * MaxValInv;
    float fc1 = static_cast<float>(c1) * MaxValInv;
    float fc2 = static_cast<float>(c2) * MaxValInv;
    float fc3 = static_cast<float>(c3) * MaxValInv;

    float fLinearAverage = (FastSRGBToLinear(fc0) + FastSRGBToLinear(fc1) + FastSRGBToLinear(fc2) + FastSRGBToLinear(fc3)) * 0.25f;
    float fSRGBAverage   = FastLinearToSRGB(fLinearAverage) * MaxVal;

    // Clamping on both ends is essential because fast SRGB math is imprecise
    fSRGBAverage = std::max(fSRGBAverage, 0.f);
    fSRGBAverage = std::min(fSRGBAverage, MaxVal);

    return static_cast<ChannelType>(fSRGBAverage);
}

template <typename ChannelType>
ChannelType LinearAverage(ChannelType c0, ChannelType c1, ChannelType c2, ChannelType c3, Uint32 /*col*/, Uint32 /*row*/);

template <>
Uint8 LinearAverage<Uint8>(Uint8 c0, Uint8 c1, Uint8 c2, Uint8 c3, Uint32 /*col*/, Uint32 /*row*/)
{
    return static_cast<Uint8>((Uint32{c0} + Uint32{c1} + Uint32{c2} + Uint32{c3}) >> 2);
}

template <>
Uint16 LinearAverage<Uint16>(Uint16 c0, Uint16 c1, Uint16 c2, Uint16 c3, Uint32 /*col*/, Uint32 /*row*/)
{
    return static_cast<Uint16>((Uint32{c0} + Uint32{c1} + Uint32{c2} + Uint32{c3}) >> 2);
}

template <>
Uint32 LinearAverage<Uint32>(Uint32 c0, Uint32 c1, Uint32 c2, Uint32 c3, Uint32 /*col*/, Uint32 /*row*/)
{
    return (c0 + c1 + c2 + c3) >> 2;
}

template <>
Int8 LinearAverage<Int8>(Int8 c0, Int8 c1, Int8 c2, Int8 c3, Uint32 /*col*/, Uint32 /*row*/)
{
    return static_cast<Int8>((Int32{c0} + Int32{c1} + Int32{c2} + Int32{c3}) / 4);
}

template <>
Int16 LinearAverage<Int16>(Int16 c0, Int16 c1, Int16 c2, Int16 c3, Uint32 /*col*/, Uint32 /*row*/)
{
    return static_cast<Int16>((Int32{c0} + Int32{c1} + Int32{c2} + Int32{c3}) / 4);
}

template <>
Int32 LinearAverage<Int32>(Int32 c0, Int32 c1, Int32 c2, Int32 c3, Uint32 /*col*/, Uint32 /*row*/)
{
    return (c0 + c1 + c2 + c3) / 4;
}

template <>
float LinearAverage<float>(float c0, float c1, float c2, float c3, Uint32 /*col*/, Uint32 /*row*/)
{
    return (c0 + c1 + c2 + c3) * 0.25f;
}


template <typename ChannelType>
ChannelType MostFrequentSelector(ChannelType c0, ChannelType c1, ChannelType c2, ChannelType c3, Uint32 col, Uint32 row)
{
    //  c2      c3
    //   *      *
    //
    //   *      *
    //  c0      c1
    const auto _01 = c0 == c1;
    const auto _02 = c0 == c2;
    const auto _03 = c0 == c3;
    const auto _12 = c1 == c2;
    const auto _13 = c1 == c3;
    const auto _23 = c2 == c3;
    if (_01)
    {
        //      2     3
        //      *-----*
        //                Use row to pseudo-randomly make selection
        //      *-----*
        //      0     1
        return (!_23 || (row & 0x01) != 0) ? c0 : c2;
    }
    if (_02)
    {
        //      2     3
        //      *     *
        //      |     |   Use col to pseudo-randomly make selection
        //      *     *
        //      0     1
        return (!_13 || (col & 0x01) != 0) ? c0 : c1;
    }
    if (_03)
    {
        //      2     3
        //      *.   .*
        //        '.'
        //       .' '.
        //      *     *
        //      0     1
        return (!_12 || ((col + row) & 0x01) != 0) ? c0 : c1;
    }
    if (_12 || _13)
    {
        //      2     3         2     3
        //      *.    *         *     *
        //        '.                  |
        //          '.                |
        //      *     *         *     *
        //      0     1         0     1
        return c1;
    }
    if (_23)
    {
        //      2     3
        //      *-----*
        //
        //      *     *
        //      0     1
        return c2;
    }

    // Select pseudo-random element
    //      2     3
    //      *     *
    //
    //      *     *
    //      0     1
    switch ((col + row) % 4)
    {
        case 0: return c0;
        case 1: return c1;
        case 2: return c2;
        case 3: return c3;
        default:
            UNEXPECTED("Unexpected index");
            return c0;
    }
}

template <typename ChannelType,
          typename FilterType>
void FilterMipLevel(const ComputeMipLevelAttribs& Attribs,
                    Uint32                        NumChannels,
                    FilterType                    Filter)
{
    VERIFY_EXPR(Attribs.FineMipWidth > 0 && Attribs.FineMipHeight > 0);
    DEV_CHECK_ERR(Attribs.FineMipHeight == 1 || Attribs.FineMipStride >= Attribs.FineMipWidth * sizeof(ChannelType) * NumChannels, "Fine mip level stride is too small");

    const auto CoarseMipWidth  = std::max(Attribs.FineMipWidth / Uint32{2}, Uint32{1});
    const auto CoarseMipHeight = std::max(Attribs.FineMipHeight / Uint32{2}, Uint32{1});

    VERIFY(CoarseMipHeight == 1 || Attribs.CoarseMipStride >= CoarseMipWidth * sizeof(ChannelType) * NumChannels, "Coarse mip level stride is too small");

    for (Uint32 row = 0; row < CoarseMipHeight; ++row)
    {
        auto src_row0 = row * 2;
        auto src_row1 = std::min(row * 2 + 1, Attribs.FineMipHeight - 1);

        auto pSrcRow0 = reinterpret_cast<const ChannelType*>(reinterpret_cast<const Uint8*>(Attribs.pFineMipData) + src_row0 * Attribs.FineMipStride);
        auto pSrcRow1 = reinterpret_cast<const ChannelType*>(reinterpret_cast<const Uint8*>(Attribs.pFineMipData) + src_row1 * Attribs.FineMipStride);

        for (Uint32 col = 0; col < CoarseMipWidth; ++col)
        {
            auto src_col0 = col * 2;
            auto src_col1 = std::min(col * 2 + 1, Attribs.FineMipWidth - 1);

            for (Uint32 c = 0; c < NumChannels; ++c)
            {
                const auto Chnl00 = pSrcRow0[src_col0 * NumChannels + c];
                const auto Chnl10 = pSrcRow0[src_col1 * NumChannels + c];
                const auto Chnl01 = pSrcRow1[src_col0 * NumChannels + c];
                const auto Chnl11 = pSrcRow1[src_col1 * NumChannels + c];

                auto& DstCol = reinterpret_cast<ChannelType*>(reinterpret_cast<Uint8*>(Attribs.pCoarseMipData) + row * Attribs.CoarseMipStride)[col * NumChannels + c];

                DstCol = Filter(Chnl00, Chnl10, Chnl01, Chnl11, col, row);
            }
        }
    }
}

void RemapAlpha(const ComputeMipLevelAttribs& Attribs,
                Uint32                        NumChannels,
                Uint32                        AlphaChannelInd)
{
    const auto CoarseMipWidth  = std::max(Attribs.FineMipWidth / Uint32{2}, Uint32{1});
    const auto CoarseMipHeight = std::max(Attribs.FineMipHeight / Uint32{2}, Uint32{1});
    for (Uint32 row = 0; row < CoarseMipHeight; ++row)
    {
        for (Uint32 col = 0; col < CoarseMipWidth; ++col)
        {
            auto& Alpha = (reinterpret_cast<Uint8*>(Attribs.pCoarseMipData) + row * Attribs.CoarseMipStride)[col * NumChannels + AlphaChannelInd];

            // Remap alpha channel using the following formula to improve mip maps:
            //
            //      A_new = max(A_old; 1/3 * A_old + 2/3 * CutoffThreshold)
            //
            // https://asawicki.info/articles/alpha_test.php5

            auto AlphaNew = std::min((static_cast<float>(Alpha) + 2.f * (Attribs.AlphaCutoff * 255.f)) / 3.f, 255.f);

            Alpha = std::max(Alpha, static_cast<Uint8>(AlphaNew));
        }
    }
}

template <typename ChannelType>
void ComputeMipLevelInternal(const ComputeMipLevelAttribs& Attribs,
                             const TextureFormatAttribs&   FmtAttribs)
{
    auto FilterType = Attribs.FilterType;
    if (FilterType == MIP_FILTER_TYPE_DEFAULT)
    {
        FilterType = FmtAttribs.ComponentType == COMPONENT_TYPE_UINT || FmtAttribs.ComponentType == COMPONENT_TYPE_SINT ?
            MIP_FILTER_TYPE_MOST_FREQUENT :
            MIP_FILTER_TYPE_BOX_AVERAGE;
    }

    FilterMipLevel<ChannelType>(Attribs, FmtAttribs.NumComponents,
                                FilterType == MIP_FILTER_TYPE_BOX_AVERAGE ?
                                    LinearAverage<ChannelType> :
                                    MostFrequentSelector<ChannelType>);
}

void ComputeMipLevel(const ComputeMipLevelAttribs& Attribs)
{
    DEV_CHECK_ERR(Attribs.Format != TEX_FORMAT_UNKNOWN, "Format must not be unknown");
    DEV_CHECK_ERR(Attribs.FineMipWidth != 0, "Fine mip width must not be zero");
    DEV_CHECK_ERR(Attribs.FineMipHeight != 0, "Fine mip height must not be zero");
    DEV_CHECK_ERR(Attribs.pFineMipData != nullptr, "Fine level data must not be null");
    DEV_CHECK_ERR(Attribs.pCoarseMipData != nullptr, "Coarse level data must not be null");

    const auto& FmtAttribs = GetTextureFormatAttribs(Attribs.Format);

    VERIFY_EXPR(Attribs.AlphaCutoff >= 0 && Attribs.AlphaCutoff <= 1);
    VERIFY(Attribs.AlphaCutoff == 0 || FmtAttribs.NumComponents == 4 && FmtAttribs.ComponentSize == 1,
           "Alpha remapping is only supported for 4-channel 8-bit textures");

    switch (FmtAttribs.ComponentType)
    {
        case COMPONENT_TYPE_UNORM_SRGB:
            VERIFY(FmtAttribs.ComponentSize == 1, "Only 8-bit sRGB formats are expected");
            FilterMipLevel<Uint8>(Attribs, FmtAttribs.NumComponents,
                                  Attribs.FilterType == MIP_FILTER_TYPE_MOST_FREQUENT ?
                                      MostFrequentSelector<Uint8> :
                                      SRGBAverage<Uint8>);
            if (Attribs.AlphaCutoff > 0)
            {
                RemapAlpha(Attribs, FmtAttribs.NumComponents, FmtAttribs.NumComponents - 1);
            }
            break;

        case COMPONENT_TYPE_UNORM:
        case COMPONENT_TYPE_UINT:
            switch (FmtAttribs.ComponentSize)
            {
                case 1:
                    ComputeMipLevelInternal<Uint8>(Attribs, FmtAttribs);
                    if (Attribs.AlphaCutoff > 0)
                    {
                        RemapAlpha(Attribs, FmtAttribs.NumComponents, FmtAttribs.NumComponents - 1);
                    }
                    break;

                case 2:
                    ComputeMipLevelInternal<Uint16>(Attribs, FmtAttribs);
                    break;

                case 4:
                    ComputeMipLevelInternal<Uint32>(Attribs, FmtAttribs);
                    break;

                default:
                    UNEXPECTED("Unexpected component size (", FmtAttribs.ComponentSize, ") for UNORM/UINT texture format");
            }
            break;

        case COMPONENT_TYPE_SNORM:
        case COMPONENT_TYPE_SINT:
            switch (FmtAttribs.ComponentSize)
            {
                case 1:
                    ComputeMipLevelInternal<Int8>(Attribs, FmtAttribs);
                    break;

                case 2:
                    ComputeMipLevelInternal<Int16>(Attribs, FmtAttribs);
                    break;

                case 4:
                    ComputeMipLevelInternal<Int32>(Attribs, FmtAttribs);
                    break;

                default:
                    UNEXPECTED("Unexpected component size (", FmtAttribs.ComponentSize, ") for UINT/SINT texture format");
            }
            break;

        case COMPONENT_TYPE_FLOAT:
            VERIFY(FmtAttribs.ComponentSize == 4, "Only 32-bit float formats are currently supported");
            ComputeMipLevelInternal<Float32>(Attribs, FmtAttribs);
            break;

        default:
            UNEXPECTED("Unsupported component type");
    }
}

#if !METAL_SUPPORTED
void CreateSparseTextureMtl(IRenderDevice*     pDevice,
                            const TextureDesc& TexDesc,
                            IDeviceMemory*     pMemory,
                            ITexture**         ppTexture)
{
}
#endif

} // namespace Diligent


extern "C"
{
    void Diligent_CreateUniformBuffer(Diligent::IRenderDevice*   pDevice,
                                      Diligent::Uint64           Size,
                                      const Diligent::Char*      Name,
                                      Diligent::IBuffer**        ppBuffer,
                                      Diligent::USAGE            Usage,
                                      Diligent::BIND_FLAGS       BindFlags,
                                      Diligent::CPU_ACCESS_FLAGS CPUAccessFlags,
                                      void*                      pInitialData)
    {
        Diligent::CreateUniformBuffer(pDevice, Size, Name, ppBuffer, Usage, BindFlags, CPUAccessFlags, pInitialData);
    }

    void Diligent_GenerateCheckerBoardPattern(Diligent::Uint32         Width,
                                              Diligent::Uint32         Height,
                                              Diligent::TEXTURE_FORMAT Fmt,
                                              Diligent::Uint32         HorzCells,
                                              Diligent::Uint32         VertCells,
                                              Diligent::Uint8*         pData,
                                              Diligent::Uint64         StrideInBytes)
    {
        Diligent::GenerateCheckerBoardPattern(Width, Height, Fmt, HorzCells, VertCells, pData, StrideInBytes);
    }

    void Diligent_ComputeMipLevel(const Diligent::ComputeMipLevelAttribs& Attribs)
    {
        Diligent::ComputeMipLevel(Attribs);
    }

    void Diligent_CreateSparseTextureMtl(Diligent::IRenderDevice*     pDevice,
                                         const Diligent::TextureDesc& TexDesc,
                                         Diligent::IDeviceMemory*     pMemory,
                                         Diligent::ITexture**         ppTexture)
    {
        Diligent::CreateSparseTextureMtl(pDevice, TexDesc, pMemory, ppTexture);
    }
}
