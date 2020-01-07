//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Container/Str.h"
#include "../IO/Log.h"

#include <cstdio>

#include "../DebugNew.h"


#ifdef _MSC_VER
#pragma warning(disable:6293)
#endif

namespace Urho3D
{

const ea::string EMPTY_STRING{};

unsigned CStringLength(const char* str)
{
    return str ? (unsigned)strlen(str) : 0;
}

int Compare(const ea::string_view& a, const ea::string_view& b, bool caseSensitive)
{
    if (a.data() == b.data())
        return true;

    if (a.empty())
        return -1;

    if (b.empty())
        return 1;

    int cmp = 0;
    if (caseSensitive)
        cmp = ea::Compare(a.data(), b.data(), ea::min(a.length(), b.length()));
    else
        cmp = ea::CompareI(a.data(), b.data(), ea::min(a.length(), b.length()));

    return (cmp != 0 ? cmp : (a.length() < b.length() ? -1 : (a.length() > b.length() ? 1 : 0)));
}

unsigned LengthUTF8(const ea::string_view& string)
{
    unsigned ret = 0;
    const char* src = string.data();
    if (!src)
        return ret;

    while (src < string.end())
    {
        DecodeUTF8(src);
        ++ret;
    }

    return ret;
}

unsigned ByteOffsetUTF8(const ea::string_view& string, unsigned index)
{
    if (string.empty())
        return 0;

    unsigned byteOffset = 0;
    unsigned utfPos = 0;

    while (utfPos < index && byteOffset < string.length())
    {
        NextUTF8Char(string, byteOffset);
        ++utfPos;
    }

    return byteOffset;
}

unsigned NextUTF8Char(const ea::string_view& string, unsigned& byteOffset)
{
    if (string.empty())
        return ea::string::npos;

    const char* src = string.data() + byteOffset;
    unsigned ret = DecodeUTF8(src);
    byteOffset = (unsigned)(src - string.data());

    return ret;
}

unsigned AtUTF8(const ea::string_view& string, unsigned index)
{
    unsigned byteOffset = ByteOffsetUTF8(string, index);
    return NextUTF8Char(string, byteOffset);
}

void ReplaceUTF8(ea::string& string, unsigned index, unsigned unicodeChar)
{
    unsigned utfPos = 0;
    unsigned byteOffset = 0;

    while (utfPos < index && byteOffset < string.length())
    {
        NextUTF8Char(string, byteOffset);
        ++utfPos;
    }

    if (utfPos < index)
        return;

    unsigned beginCharPos = byteOffset;
    NextUTF8Char(string, byteOffset);

    char temp[7];
    char* dest = temp;
    EncodeUTF8(dest, unicodeChar);
    *dest = 0;

    string.replace(beginCharPos, byteOffset - beginCharPos, temp, (unsigned)(dest - temp));
}

ea::string& AppendUTF8(ea::string& string, unsigned unicodeChar)
{
    char temp[7];
    char* dest = temp;
    EncodeUTF8(dest, unicodeChar);
    *dest = 0;
    return string.append(temp);
}

ea::string SubstringUTF8(const ea::string_view& string, unsigned pos)
{
    unsigned utf8Length = LengthUTF8(string);
    unsigned byteOffset = ByteOffsetUTF8(string, pos);
    ea::string ret;

    while (pos < utf8Length)
    {
        AppendUTF8(ret, NextUTF8Char(string, byteOffset));
        ++pos;
    }

    return ret;
}

ea::string SubstringUTF8(const ea::string_view& string, unsigned pos, unsigned length)
{
    unsigned utf8Length = LengthUTF8(string);
    unsigned byteOffset = ByteOffsetUTF8(string, pos);
    unsigned endPos = pos + length;
    ea::string ret;

    while (pos < endPos && pos < utf8Length)
    {
        AppendUTF8(ret, NextUTF8Char(string, byteOffset));
        ++pos;
    }

    return ret;
}

void EncodeUTF8(char*& dest, unsigned unicodeChar)
{
    if (unicodeChar < 0x80)
        *dest++ = unicodeChar;
    else if (unicodeChar < 0x800)
    {
        dest[0] = (char)(0xc0u | ((unicodeChar >> 6u) & 0x1fu));
        dest[1] = (char)(0x80u | (unicodeChar & 0x3fu));
        dest += 2;
    }
    else if (unicodeChar < 0x10000)
    {
        dest[0] = (char)(0xe0u | ((unicodeChar >> 12u) & 0xfu));
        dest[1] = (char)(0x80u | ((unicodeChar >> 6u) & 0x3fu));
        dest[2] = (char)(0x80u | (unicodeChar & 0x3fu));
        dest += 3;
    }
    else if (unicodeChar < 0x200000)
    {
        dest[0] = (char)(0xf0u | ((unicodeChar >> 18u) & 0x7u));
        dest[1] = (char)(0x80u | ((unicodeChar >> 12u) & 0x3fu));
        dest[2] = (char)(0x80u | ((unicodeChar >> 6u) & 0x3fu));
        dest[3] = (char)(0x80u | (unicodeChar & 0x3fu));
        dest += 4;
    }
    else if (unicodeChar < 0x4000000)
    {
        dest[0] = (char)(0xf8u | ((unicodeChar >> 24u) & 0x3u));
        dest[1] = (char)(0x80u | ((unicodeChar >> 18u) & 0x3fu));
        dest[2] = (char)(0x80u | ((unicodeChar >> 12u) & 0x3fu));
        dest[3] = (char)(0x80u | ((unicodeChar >> 6u) & 0x3fu));
        dest[4] = (char)(0x80u | (unicodeChar & 0x3fu));
        dest += 5;
    }
    else
    {
        dest[0] = (char)(0xfcu | ((unicodeChar >> 30u) & 0x1u));
        dest[1] = (char)(0x80u | ((unicodeChar >> 24u) & 0x3fu));
        dest[2] = (char)(0x80u | ((unicodeChar >> 18u) & 0x3fu));
        dest[3] = (char)(0x80u | ((unicodeChar >> 12u) & 0x3fu));
        dest[4] = (char)(0x80u | ((unicodeChar >> 6u) & 0x3fu));
        dest[5] = (char)(0x80u | (unicodeChar & 0x3fu));
        dest += 6;
    }
}

#define GET_NEXT_CONTINUATION_BYTE(ptr) *(ptr); if ((unsigned char)*(ptr) < 0x80 || (unsigned char)*(ptr) >= 0xc0) return '?'; else ++(ptr);

unsigned DecodeUTF8(const char*& src)
{
    if (src == nullptr)
        return 0;

    unsigned char char1 = *src++;

    // Check if we are in the middle of a UTF8 character
    if (char1 >= 0x80 && char1 < 0xc0)
    {
        while ((unsigned char)*src >= 0x80 && (unsigned char)*src < 0xc0)
            ++src;
        return '?';
    }

    if (char1 < 0x80)
        return char1;
    else if (char1 < 0xe0)
    {
        unsigned char char2 = GET_NEXT_CONTINUATION_BYTE(src);
        return (unsigned)((char2 & 0x3fu) | ((char1 & 0x1fu) << 6u));
    }
    else if (char1 < 0xf0)
    {
        unsigned char char2 = GET_NEXT_CONTINUATION_BYTE(src);
        unsigned char char3 = GET_NEXT_CONTINUATION_BYTE(src);
        return (unsigned)((char3 & 0x3fu) | ((char2 & 0x3fu) << 6u) | ((char1 & 0xfu) << 12u));
    }
    else if (char1 < 0xf8)
    {
        unsigned char char2 = GET_NEXT_CONTINUATION_BYTE(src);
        unsigned char char3 = GET_NEXT_CONTINUATION_BYTE(src);
        unsigned char char4 = GET_NEXT_CONTINUATION_BYTE(src);
        return (unsigned)((char4 & 0x3fu) | ((char3 & 0x3fu) << 6u) | ((char2 & 0x3fu) << 12u) | ((char1 & 0x7u) << 18u));
    }
    else if (char1 < 0xfc)
    {
        unsigned char char2 = GET_NEXT_CONTINUATION_BYTE(src);
        unsigned char char3 = GET_NEXT_CONTINUATION_BYTE(src);
        unsigned char char4 = GET_NEXT_CONTINUATION_BYTE(src);
        unsigned char char5 = GET_NEXT_CONTINUATION_BYTE(src);
        return (unsigned)((char5 & 0x3fu) | ((char4 & 0x3fu) << 6u) | ((char3 & 0x3fu) << 12u) | ((char2 & 0x3fu) << 18u) |
                          ((char1 & 0x3u) << 24u));
    }
    else
    {
        unsigned char char2 = GET_NEXT_CONTINUATION_BYTE(src);
        unsigned char char3 = GET_NEXT_CONTINUATION_BYTE(src);
        unsigned char char4 = GET_NEXT_CONTINUATION_BYTE(src);
        unsigned char char5 = GET_NEXT_CONTINUATION_BYTE(src);
        unsigned char char6 = GET_NEXT_CONTINUATION_BYTE(src);
        return (unsigned)((char6 & 0x3fu) | ((char5 & 0x3fu) << 6u) | ((char4 & 0x3fu) << 12u) | ((char3 & 0x3fu) << 18u) |
                          ((char2 & 0x3fu) << 24u) | ((char1 & 0x1u) << 30u));
    }
}

void EncodeUTF16(WideChar*& dest, unsigned unicodeChar)
{
    if (unicodeChar < 0x10000)
        *dest++ = unicodeChar;
    else
    {
        unicodeChar -= 0x10000;
        *dest++ = 0xd800 | ((unicodeChar >> 10) & 0x3ff);
        *dest++ = 0xdc00 | (unicodeChar & 0x3ff);
    }
}

unsigned DecodeUTF16(const WideChar*& src)
{
    if (src == nullptr)
        return 0;

    unsigned short word1 = *src++;

    // Check if we are at a low surrogate
    if (word1 >= 0xdc00 && word1 < 0xe000)
    {
        while (*src >= 0xdc00 && *src < 0xe000)
            ++src;
        return '?';
    }

    if (word1 < 0xd800 || word1 >= 0xe000)
        return word1;
    else
    {
        unsigned short word2 = *src++;
        if (word2 < 0xdc00 || word2 >= 0xe000)
        {
            --src;
            return '?';
        }
        else
            return (((word1 & 0x3ff) << 10) | (word2 & 0x3ff)) + 0x10000;
    }
}


ea::string Ucs2ToUtf8(const WideChar* string)
{
    ea::string result{};
    char temp[7];

    if (!string)
        return result;

    while (*string)
    {
        unsigned unicodeChar = DecodeUTF16(string);
        char* dest = temp;
        EncodeUTF8(dest, unicodeChar);
        *dest = 0;
        result.append(temp);
    }
    return result;
}

WideString Utf8ToUcs2(const char* string)
{
    WideString result{};
    unsigned neededSize = 0;
    WideChar temp[3];

    unsigned byteOffset = 0;
    auto length = CStringLength(string);
    while (byteOffset < length)
    {
        WideChar* dest = temp;
        EncodeUTF16(dest, NextUTF8Char(string, byteOffset));
        neededSize += dest - temp;
    }

    result.resize(neededSize);

    byteOffset = 0;
    WideChar* dest = &result[0];
    while (byteOffset < length)
        EncodeUTF16(dest, NextUTF8Char(string, byteOffset));

    return result;
}

ea::string WideToMultiByte(const wchar_t* string)
{
#if _WIN32
    return Ucs2ToUtf8(string);
#else
    ea::string result{};
    char temp[7];
    while (*string)
    {
        char* dest = temp;
        EncodeUTF8(dest, (unsigned)*string++);
        *dest = 0;
        result.append(temp);
    }
    return result;
#endif
}

ea::wstring MultiByteToWide(const char* string)
{
#if _WIN32
    return Utf8ToUcs2(string);
#else
    ea::wstring result{};
    result.resize(LengthUTF8(string));

    unsigned byteOffset = 0;
    wchar_t* dest = &result[0];
    unsigned length = CStringLength(string);
    while (byteOffset < length)
        *dest++ = (wchar_t)NextUTF8Char(string, byteOffset);
    return result;
#endif
}

}
