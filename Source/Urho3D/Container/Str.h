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

#pragma once

#include <Urho3D/Urho3D.h>

#include <EASTL/string.h>
#include <EASTL/unordered_map.h>

#include <cstdarg>
#include <cstring>
#include <cctype>

#include "../Container/Hash.h"

namespace Urho3D
{

static const int CONVERSION_BUFFER_LENGTH = 128;
static const int MATRIX_CONVERSION_BUFFER_LENGTH = 256;

class StringHash;

#if _WIN32
using WideChar = wchar_t;
using WideString = ea::wstring;
#else
using WideChar = char16_t;
using WideString = ea::string16;
#endif

static_assert(sizeof(WideChar) == 2, "Incorrect wide char size.");

/// Map of strings.
using StringMap = ea::unordered_map<StringHash, ea::string>;

/// Return length of a C string.
URHO3D_API unsigned CStringLength(const char* str);
/// Compare string a with string b, optionally ignoring case sensitivity.
URHO3D_API int Compare(const ea::string_view& a, const ea::string_view& b, bool caseSensitive=true);

/// Calculate number of characters in UTF8 content.
URHO3D_API unsigned LengthUTF8(const ea::string_view& string);
/// Return byte offset to char in UTF8 content.
URHO3D_API unsigned ByteOffsetUTF8(const ea::string_view& string, unsigned index);
/// Return next Unicode character from UTF8 content and increase byte offset.
URHO3D_API unsigned NextUTF8Char(const ea::string_view& string, unsigned& byteOffset);
/// Return Unicode character at index from UTF8 content.
URHO3D_API unsigned AtUTF8(const ea::string_view& string, unsigned index);
/// Replace Unicode character at index from UTF8 content.
URHO3D_API void ReplaceUTF8(ea::string& string, unsigned index, unsigned unicodeChar);
/// Append Unicode character at the end as UTF8.
URHO3D_API ea::string& AppendUTF8(ea::string& string, unsigned unicodeChar);
/// Return a UTF8 substring from position to end.
URHO3D_API ea::string SubstringUTF8(const ea::string_view& string, unsigned pos);
/// Return a UTF8 substring with length from position.
URHO3D_API ea::string SubstringUTF8(const ea::string_view& string, unsigned pos, unsigned length);

/// Encode Unicode character to UTF8. Pointer will be incremented.
URHO3D_API void EncodeUTF8(char*& dest, unsigned unicodeChar);
/// Decode Unicode character from UTF8. Pointer will be incremented.
URHO3D_API unsigned DecodeUTF8(const char*& src);
/// Encode Unicode character to UTF16. Pointer will be incremented.
URHO3D_API void EncodeUTF16(WideChar*& dest, unsigned unicodeChar);
/// Decode Unicode character from UTF16. Pointer will be incremented.
URHO3D_API unsigned DecodeUTF16(const WideChar*& src);
/// Converts ucs2 string to utf-8 string.
URHO3D_API ea::string Ucs2ToUtf8(const WideChar* string);
/// Converts utf-8 string to ucs2 string.
URHO3D_API WideString Utf8ToUcs2(const char* string);
/// Converts wide string (encoding is platform-dependent) to multibyte utf-8 string.
URHO3D_API ea::string WideToMultiByte(const wchar_t* string);
/// Converts wide string (encoding is platform-dependent) to multibyte utf-8 string.
inline ea::string WideToMultiByte(const ea::wstring& string) { return WideToMultiByte(string.c_str());  }
/// Converts multibyte utf-8 string to wide string (encoding is platform-dependent).
URHO3D_API ea::wstring MultiByteToWide(const char* string);
/// Converts multibyte utf-8 string to wide string (encoding is platform-dependent).
inline ea::wstring MultiByteToWide(const ea::string& string) { return MultiByteToWide(string.c_str()); }

URHO3D_API extern const ea::string EMPTY_STRING;

}
