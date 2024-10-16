/*
 *  Copyright 2024 Diligent Graphics LLC
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

#include <unordered_map>

#include "ParsingTools.hpp"
#include "GraphicsTypes.h"
#include "HashUtils.hpp"

namespace Diligent
{

namespace Parsing
{

/// Parses HLSL source code and extracts image formats from RWTexture comments, for example:
/// HLSL:
///     RWTexture2D<unorm float4 /*format=rgba8*/> g_Tex2D;
///     RWTexture3D</*format=rg16f*/ float2>       g_Tex3D;
/// Output:
///     {
///         {"g_Tex2D", TEX_FORMAT_RGBA8_UNORM},
///         {"g_Tex3D", TEX_FORMAT_RG16_FLOAT}
///		}
///
/// \param[in] HLSLSource - HLSL source code.
/// \return A map containing image formats extracted from RWTexture comments.
///
/// \remarks	Only texture declarations in global scope are processed.
std::unordered_map<HashMapStringKey, TEXTURE_FORMAT> ExtractGLSLImageFormatsFromHLSL(const std::string& HLSLSource);

} // namespace Parsing

} // namespace Diligent
