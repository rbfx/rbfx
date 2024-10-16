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

#include "GLSLParsingTools.hpp"

namespace Diligent
{

namespace Parsing
{

static TEXTURE_FORMAT ParseStandardGLSLImageFormat(const std::string& Format)
{
    Uint32 NumComponents = 0;

    const std::string Components = "rgba";
    auto              Pos        = Format.begin();
    while (Pos != Format.end() && NumComponents < 4 && *Pos == Components[NumComponents])
    {
        ++NumComponents;
        ++Pos;
    }
    if (NumComponents == 0 || Pos == Format.end())
        return TEX_FORMAT_UNKNOWN;

    int ComponentSize = 0;
    Pos               = Parsing::ParseInteger(Pos, Format.end(), ComponentSize);
    if (ComponentSize != 8 && ComponentSize != 16 && ComponentSize != 32)
        return TEX_FORMAT_UNKNOWN;

    if (Pos == Format.end())
    {
        // Unorm
        switch (ComponentSize)
        {
            case 8:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R8_UNORM;
                    case 2: return TEX_FORMAT_RG8_UNORM;
                    case 3: return TEX_FORMAT_UNKNOWN;
                    case 4: return TEX_FORMAT_RGBA8_UNORM;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            case 16:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R16_UNORM;
                    case 2: return TEX_FORMAT_RG16_UNORM;
                    case 3: return TEX_FORMAT_UNKNOWN;
                    case 4: return TEX_FORMAT_RGBA16_UNORM;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            default:
                return TEX_FORMAT_UNKNOWN;
        }
    }
    else if (strcmp(&*Pos, "f") == 0)
    {
        // Float
        switch (ComponentSize)
        {
            case 16:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R16_FLOAT;
                    case 2: return TEX_FORMAT_RG16_FLOAT;
                    case 3: return TEX_FORMAT_UNKNOWN;
                    case 4: return TEX_FORMAT_RGBA16_FLOAT;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            case 32:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R32_FLOAT;
                    case 2: return TEX_FORMAT_RG32_FLOAT;
                    case 3: return TEX_FORMAT_RGB32_FLOAT;
                    case 4: return TEX_FORMAT_RGBA32_FLOAT;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            default: return TEX_FORMAT_UNKNOWN;
        }
    }
    else if (strcmp(&*Pos, "i") == 0)
    {
        // Signed integer
        switch (ComponentSize)
        {
            case 8:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R8_SINT;
                    case 2: return TEX_FORMAT_RG8_SINT;
                    case 3: return TEX_FORMAT_UNKNOWN;
                    case 4: return TEX_FORMAT_RGBA8_SINT;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            case 16:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R16_SINT;
                    case 2: return TEX_FORMAT_RG16_SINT;
                    case 3: return TEX_FORMAT_UNKNOWN;
                    case 4: return TEX_FORMAT_RGBA16_SINT;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            case 32:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R32_SINT;
                    case 2: return TEX_FORMAT_RG32_SINT;
                    case 3: return TEX_FORMAT_RGB32_SINT;
                    case 4: return TEX_FORMAT_RGBA32_SINT;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            default: return TEX_FORMAT_UNKNOWN;
        }
    }
    else if (strcmp(&*Pos, "ui") == 0)
    {
        // Unsigned integer
        switch (ComponentSize)
        {
            case 8:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R8_UINT;
                    case 2: return TEX_FORMAT_RG8_UINT;
                    case 3: return TEX_FORMAT_UNKNOWN;
                    case 4: return TEX_FORMAT_RGBA8_UINT;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            case 16:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R16_UINT;
                    case 2: return TEX_FORMAT_RG16_UINT;
                    case 3: return TEX_FORMAT_UNKNOWN;
                    case 4: return TEX_FORMAT_RGBA16_UINT;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            case 32:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R32_UINT;
                    case 2: return TEX_FORMAT_RG32_UINT;
                    case 3: return TEX_FORMAT_RGB32_UINT;
                    case 4: return TEX_FORMAT_RGBA32_UINT;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            default: return TEX_FORMAT_UNKNOWN;
        }
    }
    else if (strcmp(&*Pos, "_snorm") == 0)
    {
        // Signed normalized
        switch (ComponentSize)
        {
            case 8:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R8_SNORM;
                    case 2: return TEX_FORMAT_RG8_SNORM;
                    case 3: return TEX_FORMAT_UNKNOWN;
                    case 4: return TEX_FORMAT_RGBA8_SNORM;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            case 16:
                switch (NumComponents)
                {
                    case 1: return TEX_FORMAT_R16_SNORM;
                    case 2: return TEX_FORMAT_RG16_SNORM;
                    case 3: return TEX_FORMAT_UNKNOWN;
                    case 4: return TEX_FORMAT_RGBA16_SNORM;
                    default: return TEX_FORMAT_UNKNOWN;
                }

            default: return TEX_FORMAT_UNKNOWN;
        }
    }
    else
    {
        return TEX_FORMAT_UNKNOWN;
    }
}

TEXTURE_FORMAT ParseGLSLImageFormat(const std::string& Format)
{
    if (Format.empty())
        return TEX_FORMAT_UNKNOWN;

    TEXTURE_FORMAT TexFmt = ParseStandardGLSLImageFormat(Format);
    if (TexFmt != TEX_FORMAT_UNKNOWN)
        return TexFmt;
    else if (Format == "r11f_g11f_b10f")
        return TEX_FORMAT_R11G11B10_FLOAT;
    else if (Format == "rgb10_a2")
        return TEX_FORMAT_RGB10A2_UNORM;
    else if (Format == "rgb10_a2ui")
        return TEX_FORMAT_RGB10A2_UINT;
    else
        return TEX_FORMAT_UNKNOWN;
}

} // namespace Parsing

} // namespace Diligent
