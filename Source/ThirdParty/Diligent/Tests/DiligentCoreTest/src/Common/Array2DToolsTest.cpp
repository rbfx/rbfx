/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include "Array2DTools.hpp"

#include <algorithm>

#include "gtest/gtest.h"

#include "FastRand.hpp"

using namespace Diligent;

namespace
{

TEST(Common_Array2DTools, GetArray2DMinMaxValue)
{
    auto Test = [](const float* pData, size_t Stride, Uint32 Width, Uint32 Height) {
        auto RefMin = pData[0];
        auto RefMax = pData[0];
        for (size_t row = 0; row < Height; ++row)
        {
            for (size_t col = 0; col < Width; ++col)
            {
                auto Val = pData[col + row * Stride];
                RefMin   = std::min(Val, RefMin);
                RefMax   = std::max(Val, RefMax);
            }
        }

        float Min, Max;
        GetArray2DMinMaxValue(pData, Stride, Width, Height, Min, Max);
        EXPECT_EQ(Min, RefMin);
        EXPECT_EQ(Max, RefMax);
    };


    // Test min/max at different positions
    FastRandFloat Rnd{0, -100, +100};
    for (Uint32 Width = 1; Width <= 32; ++Width)
    {
        constexpr Uint32   Height = 1;
        std::vector<float> Data(Width);
        for (Uint32 test_max = 0; test_max < 2; ++test_max)
        {
            for (size_t test = 0; test < Data.size(); ++test)
            {
                for (size_t i = 0; i < Data.size(); ++i)
                {
                    if (i == test)
                        Data[i] = test_max != 0 ? +1000.f : -1000.f;
                    else
                        Data[i] = Rnd();
                }

                Test(Data.data(), Width, Width, Height);
            }
        }
    }

    // Test misalignment
    for (size_t misalign_offset = 0; misalign_offset < 8; ++misalign_offset)
    {
        for (Uint32 Width = 1; Width < 32; ++Width)
        {
            constexpr Uint32   Height = 1;
            std::vector<float> Data(size_t{Width} + 8);
            for (auto& Val : Data)
                Val = Rnd();
            Test(&Data[misalign_offset], Width, Width, Height);
        }
    }


    {
        for (Uint32 test = 0; test < 128; ++test)
        {
            const Uint32 Width  = 32 + (test % 8);
            const Uint32 Height = 24 + (test / 8);
            const size_t Sride  = Width + test / 10;

            std::vector<float> Data(Sride * size_t{Height});
            for (auto& Val : Data)
                Val = Rnd();
            Test(Data.data(), Width, Width, Height);
        }
    }
}

} // namespace
