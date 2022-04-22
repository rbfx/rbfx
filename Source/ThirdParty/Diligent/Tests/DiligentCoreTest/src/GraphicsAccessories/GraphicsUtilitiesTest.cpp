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

#include "GraphicsUtilities.h"
#include "FastRand.hpp"
#include "ColorConversion.h"

#include <vector>
#include <array>

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(GraphicsTools_CalculateMipLevel, R8_BOX_AVE)
{
    const Uint8 FineData[] = //
        {
            0, 2, 254, 255, 127,
            4, 5, 251, 253, 129,
            2, 3, 201, 202, 63,
            6, 7, 203, 204, 61,
            8, 9, 101, 102, 31 //
        };
    const Uint8 RefCoarseData[] = //
        {
            2, 253,
            4, 202 //
        };

    for (auto fmt : {TEX_FORMAT_R8_UNORM, TEX_FORMAT_R8_UINT})
    {
        for (Uint32 width = 4; width <= 5; ++width)
        {
            for (Uint32 height = 4; height <= 5; ++height)
            {
                Uint8 CoarseData[4] = {};
                ComputeMipLevel({fmt, width, height, FineData, 5, CoarseData, 2, MIP_FILTER_TYPE_BOX_AVERAGE});
                EXPECT_EQ(CoarseData[0], RefCoarseData[0]);
                EXPECT_EQ(CoarseData[1], RefCoarseData[1]);
                EXPECT_EQ(CoarseData[2], RefCoarseData[2]);
                EXPECT_EQ(CoarseData[3], RefCoarseData[3]);
            }
        }

        for (Uint32 width = 4; width <= 5; ++width)
        {
            Uint8 CoarseData[2] = {};
            ComputeMipLevel({fmt, width, 1, FineData, 0, CoarseData, 0, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_EQ(CoarseData[0], Uint8{1});
            EXPECT_EQ(CoarseData[1], Uint8{254});
        }

        for (Uint32 height = 4; height <= 5; ++height)
        {
            Uint8 CoarseData[2] = {};
            ComputeMipLevel({fmt, 1, height, FineData, 5, CoarseData, 1, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_EQ(CoarseData[0], Uint8{2});
            EXPECT_EQ(CoarseData[1], Uint8{4});
        }
    }
}

TEST(GraphicsTools_CalculateMipLevel, S8_BOX_AVE)
{
    // clang-format off
    const Int8 FineData[] =
        {
              0,    2, 126, 127,  127,
              4,    5, 124, 125, -128,
           -128, -126,  61,  62,  -63,
           -127, -125,  63,  64,  -61,
             -8, -100, 101, 127,   31
        };

    const Int8 RefCoarseData[] =
        {
               2, 125,
            -126,  62
        };
    // clang-format on

    for (auto fmt : {TEX_FORMAT_R8_SNORM, TEX_FORMAT_R8_SINT})
    {
        for (Uint32 width = 4; width <= 5; ++width)
        {
            for (Uint32 height = 4; height <= 5; ++height)
            {
                Int8 CoarseData[4] = {};
                ComputeMipLevel({fmt, width, height, FineData, 5, CoarseData, 2, MIP_FILTER_TYPE_BOX_AVERAGE});
                EXPECT_EQ(CoarseData[0], RefCoarseData[0]);
                EXPECT_EQ(CoarseData[1], RefCoarseData[1]);
                EXPECT_EQ(CoarseData[2], RefCoarseData[2]);
                EXPECT_EQ(CoarseData[3], RefCoarseData[3]);
            }
        }

        for (Uint32 width = 4; width <= 5; ++width)
        {
            Int8 CoarseData[2] = {};
            ComputeMipLevel({fmt, width, 1, FineData, 0, CoarseData, 0, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_EQ(CoarseData[0], Int8{1});
            EXPECT_EQ(CoarseData[1], Int8{126});
        }

        for (Uint32 height = 4; height <= 5; ++height)
        {
            Int8 CoarseData[2] = {};
            ComputeMipLevel({fmt, 1, height, FineData, 5, CoarseData, 1, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_EQ(CoarseData[0], Int8{2});
            EXPECT_EQ(CoarseData[1], Int8{-127});
        }
    }
}


TEST(GraphicsTools_CalculateMipLevel, UINT8_MOST_FREQ)
{
    // clang-format off
    const Uint8 FineData[] =
        {
            1, 1,   2, 2,   3, 4,   5, 4,   5, 5,   0,
            1, 1,   2, 3,   3, 3,   4, 4,   6, 5,   1,

            7, 8,   7, 9,   1, 8,   9, 0,   1, 1,   2,
            6, 6,   7, 8,   8, 9,   2, 9,   3, 5,   3,

            7, 2,   1, 3,   3, 3,   5, 6,   7, 8,   4,
            9, 2,   3, 1,   4, 4,   5, 6,   9, 1,   5,

            0, 1,   2, 3,   4, 5,   6, 7,   8, 9
        };
    const Uint8 RefCoarseData[] = //
        {
            1,      2,     3,       4,      5,
            6,      7,     8,       9,      1,
            2,      1,     4,       5,      9
        };
    // clang-format on

    constexpr auto Fmt = TEX_FORMAT_R8_UINT;
    for (Uint32 width = 10; width <= 11; ++width)
    {
        for (Uint32 height = 6; height <= 7; ++height)
        {
            Uint8 CoarseData[_countof(RefCoarseData)] = {};
            ComputeMipLevel({Fmt, width, height, FineData, 11, CoarseData, 5});
            EXPECT_EQ(memcmp(CoarseData, RefCoarseData, sizeof(RefCoarseData)), 0);
        }
    }
}


TEST(GraphicsTools_CalculateMipLevel, R16_BOX_AVE)
{
    // clang-format off
    const Uint16 FineData[] =
        {
              0,     2, 65532, 65533,  32767,
              4,     5, 65534, 65535,      0,
          32767, 32768,    61,    62,  65000,
          32765, 32769,    63,    64,  16000,
           1024,   100,  1010,  1270,     31
        };

    const Uint16 RefCoarseData[] =
        {
                2,  65533,
            32767,     62
        };
    // clang-format on

    for (auto fmt : {TEX_FORMAT_R16_UNORM, TEX_FORMAT_R16_UINT})
    {
        for (Uint32 width = 4; width <= 5; ++width)
        {
            for (Uint32 height = 4; height <= 5; ++height)
            {
                Uint16 CoarseData[4] = {};
                ComputeMipLevel({fmt, width, height, FineData, 10, CoarseData, 4, MIP_FILTER_TYPE_BOX_AVERAGE});
                EXPECT_EQ(CoarseData[0], RefCoarseData[0]);
                EXPECT_EQ(CoarseData[1], RefCoarseData[1]);
                EXPECT_EQ(CoarseData[2], RefCoarseData[2]);
                EXPECT_EQ(CoarseData[3], RefCoarseData[3]);
            }
        }

        for (Uint32 width = 4; width <= 5; ++width)
        {
            Uint16 CoarseData[2] = {};
            ComputeMipLevel({fmt, width, 1, FineData, 0, CoarseData, 0, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_EQ(CoarseData[0], Uint16{1});
            EXPECT_EQ(CoarseData[1], Uint16{65532});
        }

        for (Uint32 height = 4; height <= 5; ++height)
        {
            Uint16 CoarseData[2] = {};
            ComputeMipLevel({fmt, 1, height, FineData, 10, CoarseData, 2, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_EQ(CoarseData[0], Uint16{2});
            EXPECT_EQ(CoarseData[1], Uint16{32766});
        }
    }
}

TEST(GraphicsTools_CalculateMipLevel, S16_BOX_AVE)
{
    // clang-format off
    const Int16 FineData[] =
        {
              0,      2, 32766, 32767,  32767,
              4,      5, 32761, 32763, -32768,
         -32767, -32768,    61,    62,  32000,
         -32766, -32762,    63,    64, -16000,
          -1024,    100, -1010, -1270,     31
        };

    const Int16 RefCoarseData[] =
        {
                2,  32764,
           -32765,     62
        };
    // clang-format on

    for (auto fmt : {TEX_FORMAT_R16_SNORM, TEX_FORMAT_R16_SINT})
    {
        for (Uint32 width = 4; width <= 5; ++width)
        {
            for (Uint32 height = 4; height <= 5; ++height)
            {
                Int16 CoarseData[4] = {};
                ComputeMipLevel({fmt, width, height, FineData, 10, CoarseData, 4, MIP_FILTER_TYPE_BOX_AVERAGE});
                EXPECT_EQ(CoarseData[0], RefCoarseData[0]);
                EXPECT_EQ(CoarseData[1], RefCoarseData[1]);
                EXPECT_EQ(CoarseData[2], RefCoarseData[2]);
                EXPECT_EQ(CoarseData[3], RefCoarseData[3]);
            }
        }

        for (Uint32 width = 4; width <= 5; ++width)
        {
            Int16 CoarseData[2] = {};
            ComputeMipLevel({fmt, width, 1, FineData, 0, CoarseData, 0, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_EQ(CoarseData[0], Int16{1});
            EXPECT_EQ(CoarseData[1], Int16{32766});
        }

        for (Uint32 height = 4; height <= 5; ++height)
        {
            Int16 CoarseData[2] = {};
            ComputeMipLevel({fmt, 1, height, FineData, 10, CoarseData, 2, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_EQ(CoarseData[0], Int16{2});
            EXPECT_EQ(CoarseData[1], Int16{-32766});
        }
    }
}


TEST(GraphicsTools_CalculateMipLevel, SINT16_MOST_FREQ)
{
    // clang-format off
    const Int16 FineData[] =
        {
            -201, -201,   -202, -202,   -203, -204,   -205, -204,   -205, -205,   0,
            -201, -201,   -202, -203,   -203, -203,   -204, -204,   -206, -205,   1,

            -207, -208,   -207, -209,   -201, -208,   -209, -200,   -201, -201,   2,
            -206, -206,   -207, -208,   -208, -209,   -202, -209,   -203, -205,   3,

            -207, -202,   -201, -203,   -203, -203,   -205, -206,   -207, -208,   4,
            -209, -202,   -203, -201,   -204, -204,   -205, -206,   -209, -201,   5,

            0, 1,   2, 3,   4, 5,   6, 7,   8, 9
        };
    const Int16 RefCoarseData[] = //
        {
            -201,      -202,     -203,       -204,      -205,
            -206,      -207,     -208,       -209,      -201,
            -202,      -201,     -204,       -205,      -209
        };
    // clang-format on

    constexpr auto Fmt = TEX_FORMAT_R16_SINT;
    for (Uint32 width = 10; width <= 11; ++width)
    {
        for (Uint32 height = 6; height <= 7; ++height)
        {
            Int16 CoarseData[_countof(RefCoarseData)] = {};
            ComputeMipLevel({Fmt, width, height, FineData, 22, CoarseData, 10});
            EXPECT_EQ(memcmp(CoarseData, RefCoarseData, sizeof(RefCoarseData)), 0);
        }
    }
}

TEST(GraphicsTools_CalculateMipLevel, UINT32_BOX_AVE)
{
    // clang-format off
    const Uint32 FineData[] =
        {
                 0,      2, 100000, 100001,  200000,
                 4,      5, 100003, 100005,  100000,
            200000, 200002,     61,     62,   65000,
            200005, 200003,     63,     64,   16000,
            300000, 400000,   1010,   1270,   31
        };

    const Uint32 RefCoarseData[] =
        {
                 2,  100002,
            200002,      62
        };
    // clang-format on

    const auto fmt = TEX_FORMAT_R32_UINT;
    for (Uint32 width = 4; width <= 5; ++width)
    {
        for (Uint32 height = 4; height <= 5; ++height)
        {
            Uint32 CoarseData[4] = {};
            ComputeMipLevel({fmt, width, height, FineData, 20, CoarseData, 8, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_EQ(CoarseData[0], RefCoarseData[0]);
            EXPECT_EQ(CoarseData[1], RefCoarseData[1]);
            EXPECT_EQ(CoarseData[2], RefCoarseData[2]);
            EXPECT_EQ(CoarseData[3], RefCoarseData[3]);
        }
    }

    for (Uint32 width = 4; width <= 5; ++width)
    {
        Uint32 CoarseData[2] = {};
        ComputeMipLevel({fmt, width, 1, FineData, 0, CoarseData, 0, MIP_FILTER_TYPE_BOX_AVERAGE});
        EXPECT_EQ(CoarseData[0], Uint32{1});
        EXPECT_EQ(CoarseData[1], Uint32{100000});
    }

    for (Uint32 height = 4; height <= 5; ++height)
    {
        Uint32 CoarseData[2] = {};
        ComputeMipLevel({fmt, 1, height, FineData, 20, CoarseData, 4, MIP_FILTER_TYPE_BOX_AVERAGE});
        EXPECT_EQ(CoarseData[0], Uint32{2});
        EXPECT_EQ(CoarseData[1], Uint32{200002});
    }
}

TEST(GraphicsTools_CalculateMipLevel, INT32_BOX_AVE)
{
    // clang-format off
    const Int32 FineData[] =
        {
                  0,       2, 100000, 100001,  200000,
                  4,       5, 100003, 100005, -100000,
            -200000, -200002,     61,     62,   65000,
            -200005, -200003,     63,     64,  -16000,
            -300000,  400000,   1010,  -1270,   31
        };

    const Int32 RefCoarseData[] =
        {
                  2,  100002,
            -200002,      62
        };
    // clang-format on

    const auto fmt = TEX_FORMAT_R32_SINT;
    for (Uint32 width = 4; width <= 5; ++width)
    {
        for (Uint32 height = 4; height <= 5; ++height)
        {
            Int32 CoarseData[4] = {};
            ComputeMipLevel({fmt, width, height, FineData, 20, CoarseData, 8, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_EQ(CoarseData[0], RefCoarseData[0]);
            EXPECT_EQ(CoarseData[1], RefCoarseData[1]);
            EXPECT_EQ(CoarseData[2], RefCoarseData[2]);
            EXPECT_EQ(CoarseData[3], RefCoarseData[3]);
        }
    }

    for (Uint32 width = 4; width <= 5; ++width)
    {
        Int32 CoarseData[2] = {};
        ComputeMipLevel({fmt, width, 1, FineData, 0, CoarseData, 0, MIP_FILTER_TYPE_BOX_AVERAGE});
        EXPECT_EQ(CoarseData[0], Int32{1});
        EXPECT_EQ(CoarseData[1], Int32{100000});
    }

    for (Uint32 height = 4; height <= 5; ++height)
    {
        Int32 CoarseData[2] = {};
        ComputeMipLevel({fmt, 1, height, FineData, 20, CoarseData, 4, MIP_FILTER_TYPE_BOX_AVERAGE});
        EXPECT_EQ(CoarseData[0], Int32{2});
        EXPECT_EQ(CoarseData[1], Int32{-200002});
    }
}

TEST(GraphicsTools_CalculateMipLevel, FLOAT32)
{
    // clang-format off
    const Float32 FineData[] =
        {
             0,      1,      128.50f,   129.25f,  200000,
             4,      6,      130.25f,   131.50f, -100000,
            -1.50f, -3.25f,   61,        62,       65000,
            -2.25f, -4.50f,   63,        64,      -16000,
            -3.50f,  4.25f, -110,     -1270,          31
        };

    const Float32 RefCoarseData[] =
        {
             2.75f,  129.875f,
           -2.875f,     62.5f
        };
    // clang-format on

    const auto fmt = TEX_FORMAT_R32_FLOAT;
    for (Uint32 width = 4; width <= 5; ++width)
    {
        for (Uint32 height = 4; height <= 5; ++height)
        {
            Float32 CoarseData[4] = {};
            ComputeMipLevel({fmt, width, height, FineData, 20, CoarseData, 8, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_EQ(CoarseData[0], RefCoarseData[0]);
            EXPECT_EQ(CoarseData[1], RefCoarseData[1]);
            EXPECT_EQ(CoarseData[2], RefCoarseData[2]);
            EXPECT_EQ(CoarseData[3], RefCoarseData[3]);
        }
    }

    for (Uint32 width = 4; width <= 5; ++width)
    {
        Float32 CoarseData[2] = {};
        ComputeMipLevel({fmt, width, 1, FineData, 0, CoarseData, 0, MIP_FILTER_TYPE_BOX_AVERAGE});
        EXPECT_EQ(CoarseData[0], 0.5f);
        EXPECT_EQ(CoarseData[1], 128.875f);
    }

    for (Uint32 height = 4; height <= 5; ++height)
    {
        Float32 CoarseData[2] = {};
        ComputeMipLevel({fmt, 1, height, FineData, 20, CoarseData, 4, MIP_FILTER_TYPE_BOX_AVERAGE});
        EXPECT_EQ(CoarseData[0], 2.f);
        EXPECT_EQ(CoarseData[1], -1.875f);
    }
}

TEST(GraphicsTools_CalculateMipLevel, RGBA_BOX_AVE)
{
    for (Uint32 NumChannels = 1; NumChannels <= 4; NumChannels *= 2)
    {
        const Uint32 FineWidth  = 15;
        const Uint32 FineHeight = 37;

        std::vector<Uint8> FineData(FineWidth * FineHeight * NumChannels);

        FastRandInt rnd(0, 0, 255);
        for (auto& c : FineData)
            c = static_cast<Uint8>(rnd());

        const Uint32 CoarseWidth  = FineWidth / 2;
        const Uint32 CoarseHeight = FineHeight / 2;

        std::vector<Uint8> RefCoarseData(CoarseWidth * CoarseHeight * NumChannels);
        for (Uint32 y = 0; y < CoarseHeight; ++y)
        {
            for (Uint32 x = 0; x < CoarseWidth; ++x)
            {
                for (Uint32 c = 0; c < NumChannels; ++c)
                {
                    RefCoarseData[(x + y * CoarseWidth) * NumChannels + c] =
                        (FineData[((x * 2 + 0) + (y * 2 + 0) * FineWidth) * NumChannels + c] +
                         FineData[((x * 2 + 1) + (y * 2 + 0) * FineWidth) * NumChannels + c] +
                         FineData[((x * 2 + 0) + (y * 2 + 1) * FineWidth) * NumChannels + c] +
                         FineData[((x * 2 + 1) + (y * 2 + 1) * FineWidth) * NumChannels + c]) /
                        4;
                }
            }
        }

        std::array<TEXTURE_FORMAT, 2> Formats = {};
        switch (NumChannels)
        {
            case 1:
                Formats[0] = TEX_FORMAT_R8_UNORM;
                Formats[1] = TEX_FORMAT_R8_UINT;
                break;

            case 2:
                Formats[0] = TEX_FORMAT_RG8_UNORM;
                Formats[1] = TEX_FORMAT_RG8_UINT;
                break;

            case 4:
                Formats[0] = TEX_FORMAT_RGBA8_UNORM;
                Formats[1] = TEX_FORMAT_RGBA8_UINT;
                break;

            default:
                UNEXPECTED("Unexpected number of components");
        }
        for (auto fmt : Formats)
        {
            std::vector<Uint8> CoarseData(RefCoarseData.size());
            ComputeMipLevel({fmt, FineWidth, FineHeight, FineData.data(), FineWidth * NumChannels, CoarseData.data(), CoarseWidth * NumChannels, MIP_FILTER_TYPE_BOX_AVERAGE});
            EXPECT_TRUE(CoarseData == RefCoarseData);
        }
    }
}



TEST(GraphicsTools_CalculateMipLevel, sRGB_BOX_AVE)
{
    const Uint32 FineWidth   = 225;
    const Uint32 FineHeight  = 137;
    const Uint32 NumChannels = 4;

    std::vector<Uint8> FineData(FineWidth * FineHeight * NumChannels);

    FastRandInt rnd(0, 0, 255);
    for (auto& c : FineData)
        c = static_cast<Uint8>(rnd());

    const Uint32 CoarseWidth  = FineWidth / 2;
    const Uint32 CoarseHeight = FineHeight / 2;

    std::vector<Uint8> RefCoarseData(CoarseWidth * CoarseHeight * NumChannels);
    for (Uint32 y = 0; y < CoarseHeight; ++y)
    {
        for (Uint32 x = 0; x < CoarseWidth; ++x)
        {
            for (Uint32 c = 0; c < NumChannels; ++c)
            {
                float fLinearAverage =
                    (FastSRGBToLinear(FineData[((x * 2 + 0) + (y * 2 + 0) * FineWidth) * NumChannels + c] / 255.f) +
                     FastSRGBToLinear(FineData[((x * 2 + 1) + (y * 2 + 0) * FineWidth) * NumChannels + c] / 255.f) +
                     FastSRGBToLinear(FineData[((x * 2 + 0) + (y * 2 + 1) * FineWidth) * NumChannels + c] / 255.f) +
                     FastSRGBToLinear(FineData[((x * 2 + 1) + (y * 2 + 1) * FineWidth) * NumChannels + c] / 255.f)) *
                    0.25f;
                fLinearAverage = std::min(std::max(fLinearAverage, 0.f), 255.f);
                float fSRGB    = FastLinearToSRGB(fLinearAverage);

                RefCoarseData[(x + y * CoarseWidth) * NumChannels + c] = static_cast<Uint8>(fSRGB * 255.f);
            }
        }
    }

    std::vector<Uint8> CoarseData(RefCoarseData.size());
    ComputeMipLevel({TEX_FORMAT_RGBA8_UNORM_SRGB, FineWidth, FineHeight, FineData.data(), FineWidth * NumChannels, CoarseData.data(), CoarseWidth * NumChannels});
    EXPECT_TRUE(CoarseData == RefCoarseData);
}

} // namespace
