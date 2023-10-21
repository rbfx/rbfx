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

#include <string.h>

#include "../../Primitives/interface/BasicTypes.h"
#include "../../Primitives/interface/CommonDefinitions.h"

#if !(defined(_MSC_VER) || defined(__MINGW64__) || defined(__MINGW32__))
inline int strcpy_s(char* dest, size_t destsz, const char* src)
{
    strncpy(dest, src, destsz);
    return 0;
}
#endif

DILIGENT_BEGIN_NAMESPACE(Diligent)

inline bool IsNum(char c)
{
    return c >= '0' && c <= '9';
}

inline bool SafeStrEqual(const char* Str0, const char* Str1)
{
    if ((Str0 == NULL) != (Str1 == NULL))
        return false;

    if (Str0 != NULL && Str1 != NULL)
        return strcmp(Str0, Str1) == 0;

    return true;
}

inline bool IsNullOrEmptyStr(const char* Str)
{
    return Str == NULL || Str[0] == '\0';
}

inline const char* GetOrdinalNumberSuffix(unsigned int Num)
{
    if (Num == 11 || Num == 12 || Num == 13)
        return "th";

    switch (Num % 10)
    {
        case 1: return "st";
        case 2: return "nd";
        case 3: return "rd";
        default: return "th";
    }
}

DILIGENT_END_NAMESPACE // namespace Diligent
