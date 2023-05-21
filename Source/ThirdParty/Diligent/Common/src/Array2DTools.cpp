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

#include "Intrinsics.hpp"
#include "DebugUtilities.hpp"
#include "Align.hpp"

namespace Diligent
{

namespace
{

void GetArray2DMinMaxValueGeneric(const float* pData,
                                  size_t       StrideInFloats,
                                  Uint32       Width,
                                  Uint32       Height,
                                  float&       MinValue,
                                  float&       MaxValue)
{
    for (size_t row = 0; row < Height; ++row)
    {
        const auto* pRowStart = pData + row * StrideInFloats;
        const auto* pRowEnd   = pRowStart + Width;
        for (const auto* ptr = pRowStart; ptr < pRowEnd; ++ptr)
        {
            MinValue = std::min(MinValue, *ptr);
            MaxValue = std::max(MaxValue, *ptr);
        }
    }
}

#if DILIGENT_AVX2_ENABLED
bool GetArray2DMinMaxValueAVX2(const float* pData,
                               size_t       StrideInFloats,
                               Uint32       Width,
                               Uint32       Height,
                               float&       MinValue,
                               float&       MaxValue)
{
    MinValue   = pData[0];
    MaxValue   = pData[0];
    auto mmMin = _mm256_set1_ps(MinValue);
    auto mmMax = _mm256_set1_ps(MaxValue);
    for (size_t row = 0; row < Height; ++row)
    {
        const float* pRowStart = pData + row * StrideInFloats;
        const float* pRowEnd   = pRowStart + Width;
        const float* pAVXEnd   = pRowEnd - 8;

        const float* Ptr = pRowStart;
        for (; Ptr < pAVXEnd; Ptr += 8)
        {
            // NOTE: MSVC generates vmovups when using _mm256_load_ps regardless,
            //       so no reason to bother with aligning the pointer.
            auto mmVal = _mm256_loadu_ps(Ptr);

            mmMin = _mm256_min_ps(mmMin, mmVal);
            mmMax = _mm256_max_ps(mmMax, mmVal);
        }

        for (; Ptr < pRowEnd; ++Ptr)
        {
            MinValue = std::min(MinValue, *Ptr);
            MaxValue = std::max(MaxValue, *Ptr);
        }
    }

// Shuffle only works within 128-bit halves
#    define MAKE_SHUFFLE(i0, i1, i2, i3) ((i0 << 0) | (i1 << 2) | (i2 << 4) | (i3 << 6))
    constexpr int Shuffle1032 = MAKE_SHUFFLE(1, 0, 3, 2);
    // |  0  |  1  |  2  |  3  |       |  1  |  0  |  3  |  2  |
    // |  A  |  B  |  C  |  D  |   =>  |  B  |  A  |  D  |  C  |
    mmMin = _mm256_min_ps(mmMin, _mm256_permute_ps(mmMin, Shuffle1032));
    mmMax = _mm256_max_ps(mmMax, _mm256_permute_ps(mmMax, Shuffle1032));

    constexpr int Shuffle2301 = MAKE_SHUFFLE(2, 3, 0, 1);
    //       0           1           2          3                    2           3           0           1
    // | max(A, B) | max(A, B) | max(C, D) | max(C, D) |  =>  | max(C, D) | max(C, D) | max(A, B) | max(A, B) |
    mmMin = _mm256_min_ps(mmMin, _mm256_permute_ps(mmMin, Shuffle2301));
    mmMax = _mm256_max_ps(mmMax, _mm256_permute_ps(mmMax, Shuffle2301));

    // Note that _mm256_permute_ps is faster than _mm256_permutevar8x32_ps
    const auto SelectElement4 = _mm256_set1_epi32(4);
    // We only need to do min/max between elements 4 and 0
    mmMin = _mm256_min_ps(mmMin, _mm256_permutevar8x32_ps(mmMin, SelectElement4));
    mmMax = _mm256_max_ps(mmMax, _mm256_permutevar8x32_ps(mmMax, SelectElement4));

    // Extract lower 128 bits
    auto mMin0123 = _mm256_castps256_ps128(mmMin);
    auto mMax0123 = _mm256_castps256_ps128(mmMax);

    float Min0, Max0;
    _mm_store_ss(&Min0, mMin0123);
    _mm_store_ss(&Max0, mMax0123);
    MinValue = std::min(Min0, MinValue);
    MaxValue = std::max(Max0, MaxValue);

    return true;
}
#endif

} // namespace

void GetArray2DMinMaxValue(const float* pData,
                           size_t       StrideInFloats,
                           Uint32       Width,
                           Uint32       Height,
                           float&       MinValue,
                           float&       MaxValue)
{
    if (Width == 0 || Height == 0)
        return;

    DEV_CHECK_ERR(pData != nullptr, "Data pointer must not be null");
    DEV_CHECK_ERR(Height == 1 || StrideInFloats >= Width, "Row stride (", StrideInFloats, ") must be at least ", Width);
    DEV_CHECK_ERR(AlignDown(pData, alignof(float)) == pData, "Data pointer is not naturally aligned");

    MinValue = MaxValue = pData[0];
#if DILIGENT_AVX2_ENABLED
    if (GetArray2DMinMaxValueAVX2(pData, StrideInFloats, Width, Height, MinValue, MaxValue))
        return;
#endif

    GetArray2DMinMaxValueGeneric(pData, StrideInFloats, Width, Height, MinValue, MaxValue);
}

} // namespace Diligent
