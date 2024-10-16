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

#include "ParsingTools.hpp"
#include "GraphicsTypes.h"

namespace Diligent
{

namespace Parsing
{

/// Extracts GLSL image format from the comment, e.g.:
///   /* format = r32f */
///   ^					 ^
///  Start				End
/// The function returns "r32f"
/// If the comment does not contain format specifier, the function returns an empty string.
///
/// \param[in] Start - iterator to the beginning of the comment
/// \param[in] End   - iterator to the end of the comment
///
/// \return GLSL image format
template <typename InteratorType>
std::string ExtractGLSLImageFormatFromComment(const InteratorType& Start, const InteratorType& End)
{
    //    /* format = r32f */
    // ^
    auto Pos = SkipDelimiters(Start, End);
    if (Pos == End)
        return {};

    //    /* format = r32f */
    //    ^
    if (*Pos != '/')
        return {};

    ++Pos;
    //    /* format = r32f */
    //     ^
    //    // format = r32f
    //     ^
    if (Pos == End || (*Pos != '/' && *Pos != '*'))
        return {};

    ++Pos;
    //    /* format = r32f */
    //      ^
    Pos = SkipDelimiters(Pos, End);
    if (Pos == End)
        return {};

    //    /* format = r32f */
    //       ^
    if (!SkipString(Pos, End, "format", Pos))
        return {};

    //    /* format = r32f */
    //             ^
    Pos = SkipDelimiters(Pos, End);
    if (Pos == End)
        return {};

    //    /* format = r32f */
    //              ^
    if (*Pos != '=')
        return {};

    ++Pos;
    //    /* format = r32f */
    //               ^
    Pos = SkipDelimiters(Pos, End);
    if (Pos == End)
        return {};

    //    /* format = r32f */
    //                ^

    auto ImgFmtEndPos = SkipIdentifier(Pos, End);
    return {Pos, ImgFmtEndPos};
}

/// Parses GLSL image format string, e.g. "rgba8" or "rg32f" and returns the corresponding texture format.
///
/// \param[in]  Format - GLSL image format string
/// \return     Texture format, or TEXTURE_FORMAT_UNKNOWN if the format is not recognized.
TEXTURE_FORMAT ParseGLSLImageFormat(const std::string& Format);

} // namespace Parsing

} // namespace Diligent
