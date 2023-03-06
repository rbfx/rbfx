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

#pragma once

/// \file
/// 2D array processing utilities.

#include "../../Primitives/interface/BasicTypes.h"

namespace Diligent
{

/// Computes the minimum and the maximum value in a 2D floating-point array

/// \param[in]  pData		   - A pointer to the array data.
/// \param[in]  StrideInFloats - Row stride in 32-bit floats.
/// \param[in]  Width		   - 2D array width.
/// \param[in]  Height		   - 2D array height.
/// \param[out] MinValue	   - Minimum value.
/// \param[out] MaxValue	   - Maximum value.
void GetArray2DMinMaxValue(const float* pData,
                           size_t       StrideInFloats,
                           Uint32       Width,
                           Uint32       Height,
                           float&       MinValue,
                           float&       MaxValue);

} // namespace Diligent
