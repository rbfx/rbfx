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

#pragma once

#include <string>
#include <sstream>
#include <locale>
#include <algorithm>
#include <vector>
#include <cctype>

#include "../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "StringTools.h"
#include "ParsingTools.hpp"

namespace Diligent
{

inline std::string NarrowString(const wchar_t* WideStr, size_t Len = 0)
{
    if (Len == 0)
        Len = wcslen(WideStr);
    else
        VERIFY_EXPR(Len <= wcslen(WideStr));

    std::string NarrowStr(Len, '\0');

    const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(std::wstringstream().getloc());
    for (size_t i = 0; i < Len; ++i)
        NarrowStr[i] = ctfacet.narrow(WideStr[i], 0);

    return NarrowStr;
}

inline std::string NarrowString(const std::wstring& WideStr)
{
    return NarrowString(WideStr.c_str(), WideStr.length());
}

inline std::wstring WidenString(const char* Str, size_t Len = 0)
{
    if (Len == 0)
        Len = strlen(Str);
    else
        VERIFY_EXPR(Len <= strlen(Str));

    std::wstring WideStr(Len, L'\0');

    const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(std::wstringstream().getloc());
    for (size_t i = 0; i < Len; ++i)
        WideStr[i] = ctfacet.widen(Str[i]);

    return WideStr;
}

inline size_t StrLen(const char* Str)
{
    return Str != nullptr ? strlen(Str) : 0;
}

inline size_t StrLen(const wchar_t* Str)
{
    return Str != nullptr ? wcslen(Str) : 0;
}

inline std::wstring WidenString(const std::string& Str)
{
    return WidenString(Str.c_str(), Str.length());
}

inline int StrCmpNoCase(const char* Str1, const char* Str2, size_t NumChars)
{
#if PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS || PLATFORM_TVOS || PLATFORM_EMSCRIPTEN
#    define _strnicmp strncasecmp
#endif

    return _strnicmp(Str1, Str2, NumChars);
}

inline int StrCmpNoCase(const char* Str1, const char* Str2)
{
#if PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS || PLATFORM_TVOS || PLATFORM_EMSCRIPTEN
#    define _stricmp strcasecmp
#endif

    return _stricmp(Str1, Str2);
}

// Returns true if RefStr == Str + Suff
// If Suff == nullptr or NoSuffixAllowed == true, also returns true if RefStr == Str
inline bool StreqSuff(const char* RefStr, const char* Str, const char* Suff, bool NoSuffixAllowed = false)
{
    VERIFY_EXPR(RefStr != nullptr && Str != nullptr);
    if (RefStr == nullptr)
        return false;

    const auto* r = RefStr;
    const auto* s = Str;
    // abc_def     abc
    // ^           ^
    // r           s
    for (; *r != 0 && *s != 0; ++r, ++s)
    {
        if (*r != *s)
        {
            // abc_def     abx
            //   ^           ^
            //   r           s
            return false;
        }
    }

    if (*s != 0)
    {
        // ab         abc
        //   ^          ^
        //   r          s
        VERIFY_EXPR(*r == 0);
        return false;
    }
    else
    {
        // abc_def     abc
        //    ^           ^
        //    r           s

        if (NoSuffixAllowed && *r == 0)
        {
            // abc         abc      _def
            //    ^           ^
            //    r           s
            return true;
        }

        if (Suff != nullptr)
        {
            // abc_def     abc       _def
            //    ^           ^      ^
            //    r           s      Suff
            return strcmp(r, Suff) == 0;
        }
        else
        {
            // abc         abc                abc_def         abc
            //    ^           ^       or         ^               ^
            //    r           s                  r               s
            return *r == 0;
        }
    }
}

inline void StrToLowerInPlace(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   // http://en.cppreference.com/w/cpp/string/byte/tolower
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}

inline std::string StrToLower(std::string str)
{
    StrToLowerInPlace(str);
    return str;
}


/// Returns the number of characters at the beginning of the string that form a
/// floating point number.
inline size_t CountFloatNumberChars(const char* Str)
{
    if (Str == nullptr)
        return 0;

    const auto* MemoryEnd = reinterpret_cast<const char*>(~uintptr_t{0});
    const auto* NumEnd    = Parsing::SkipFloatNumber(Str, MemoryEnd);
    return NumEnd - Str;
}

/// Splits string [Start, End) into chunks of length no more than MaxChunkLen.
/// For each chunk, searches back for the new line for no more than NewLineSearchLen symbols.
/// For each chunk, calls the Handler.
///
/// \note   This function is used to split long messages on Android to avoid
///         trunction in logcat.
template <typename IterType, typename HandlerType>
void SplitLongString(IterType Start, IterType End, size_t MaxChunkLen, size_t NewLineSearchLen, HandlerType&& Handler)
{
    // NB: do not use debug macros to avoid infinite recursion!

    if (MaxChunkLen == 0)
        MaxChunkLen = 32;
    while (Start != End)
    {
        auto ChunkEnd = End;
        if (static_cast<size_t>(ChunkEnd - Start) > MaxChunkLen)
        {
            ChunkEnd = Start + MaxChunkLen;
            // Try to find new line
            auto NewLine = ChunkEnd - 1;
            while (NewLine > Start + 1 && static_cast<size_t>(ChunkEnd - NewLine) < NewLineSearchLen && *NewLine != '\n')
                --NewLine;
            if (*NewLine == '\n')
            {
                // Found new line - use it as the end of the string
                ChunkEnd = NewLine + 1;
            }
        }
        Handler(Start, ChunkEnd);
        Start = ChunkEnd;
    }
}


/// Splits string [Start, End) into chunks separated by Delimiters.
/// Ignores all leading and trailing delimiters.
/// For each chunk, calls the Handler.
template <typename IterType, typename HandlerType>
void SplitString(IterType Start, IterType End, const char* Delimiters, HandlerType&& Handler)
{
    if (Delimiters == nullptr)
        Delimiters = " \t\r\n";

    auto Pos = Start;
    while (Pos != End)
    {
        while (Pos != End && strchr(Delimiters, *Pos) != nullptr)
            ++Pos;
        if (Pos == End)
            break;

        auto SubstrStart = Pos;
        while (Pos != End && strchr(Delimiters, *Pos) == nullptr)
            ++Pos;

        Handler(SubstrStart, Pos);
    }
}


/// Splits string [Start, End) into chunks separated by Delimiters and returns them as vector of strings.
/// Ignores all leading and trailing delimiters.
template <typename IterType>
std::vector<std::string> SplitString(IterType Start, IterType End, const char* Delimiters = nullptr)
{
    std::vector<std::string> Strings;
    SplitString(Start, End, Delimiters,
                [&Strings](IterType Start, IterType End) //
                {
                    Strings.emplace_back(Start, End);
                });
    return Strings;
}


/// Returns the print width of a number Num
template <typename Type>
size_t GetPrintWidth(Type Num, Type Base = 10)
{
    if (Num == 0)
        return 1;

    size_t W = Num < 0 ? 1 : 0;
    while (Num != 0)
    {
        ++W;
        Num /= Base;
    }

    return W;
}

} // namespace Diligent
