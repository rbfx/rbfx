// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#pragma once

#include <Urho3D/IO/FileSystem.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace Urho3D
{

namespace AssetViewerDetail
{

inline bool IsPathSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

inline ea::string TrimPathSpace(const ea::string& value)
{
    unsigned begin = 0;
    unsigned end = value.size();
    while (begin < end && IsPathSpace(value[begin]))
        ++begin;
    while (end > begin && IsPathSpace(value[end - 1]))
        --end;
    return value.substr(begin, end - begin);
}

inline int HexDigitValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/// Decode %XX sequences. An invalid sequence stays as it is.
inline ea::string PercentDecodePath(const ea::string& value)
{
    ea::string result;
    result.reserve(value.size());
    for (unsigned i = 0; i < value.size(); ++i)
    {
        if (value[i] == '%' && i + 2 < value.size() && HexDigitValue(value[i + 1]) >= 0
            && HexDigitValue(value[i + 2]) >= 0)
        {
            result += static_cast<char>(HexDigitValue(value[i + 1]) * 16 + HexDigitValue(value[i + 2]));
            i += 2;
        }
        else
            result += value[i];
    }
    return result;
}

inline bool StartsWithNoCase(const ea::string& value, const char* prefix)
{
    const unsigned length = static_cast<unsigned>(strlen(prefix));
    if (value.size() < length)
        return false;
    for (unsigned i = 0; i < length; ++i)
    {
        const char a = value[i];
        const char b = prefix[i];
        const char lowerA = (a >= 'A' && a <= 'Z') ? static_cast<char>(a - 'A' + 'a') : a;
        if (lowerA != b)
            return false;
    }
    return true;
}

/// Describe a raw drop string for the log: the length, the text and the first bytes in hex.
inline ea::string DescribeRawDrop(const ea::string& raw)
{
    static const char digits[] = "0123456789abcdef";
    ea::string hex;
    const unsigned count = raw.size() < 64 ? raw.size() : 64;
    for (unsigned i = 0; i < count; ++i)
    {
        const unsigned char byte = static_cast<unsigned char>(raw[i]);
        hex += digits[byte >> 4];
        hex += digits[byte & 15];
        hex += ' ';
    }
    if (raw.size() > count)
        hex += "...";
    return Format("length {}, bytes [{}]", raw.size(), hex.empty() ? ea::string("none") : TrimPathSpace(hex));
}

inline ea::string GetProcessCurrentDir()
{
    std::error_code error;
    const std::filesystem::path path = std::filesystem::current_path(error);
    if (error)
        return EMPTY_STRING;
    return AddTrailingSlash(ea::string(path.generic_string().c_str()));
}

/// Make a path from a drop, a command line argument or a pasted text ready for loading.
/// Remove spaces, line breaks and quotes. Use only the first line. Convert file:// URIs.
/// Expand a leading ~/ and make the path absolute. A plain path is not percent-decoded.
inline ea::string NormalizeDroppedPath(const ea::string& input, const ea::string& currentDir = EMPTY_STRING)
{
    ea::string path = TrimPathSpace(input);
    const unsigned lineEnd = path.find_first_of("\r\n");
    if (lineEnd != ea::string::npos)
        path = TrimPathSpace(path.substr(0, lineEnd));

    while (path.size() >= 2 && (path.front() == '"' || path.front() == '\'') && path.back() == path.front())
        path = TrimPathSpace(path.substr(1, path.size() - 2));
    if (path.empty())
        return path;

    if (StartsWithNoCase(path, "file://"))
    {
        path = path.substr(7);
        if (StartsWithNoCase(path, "localhost/"))
            path = path.substr(9);
        // A Windows URI looks like file:///C:/dir/file. Remove the slash before the drive letter.
        if (path.size() >= 3 && path[0] == '/' && path[2] == ':'
            && ((path[1] >= 'A' && path[1] <= 'Z') || (path[1] >= 'a' && path[1] <= 'z')))
            path = path.substr(1);
        path = PercentDecodePath(path);
    }

    if (path == "~" || path.starts_with("~/"))
    {
        const char* home = std::getenv("HOME");
        if (!home || !*home)
            home = std::getenv("USERPROFILE");
        if (home && *home)
            path = AddTrailingSlash(ea::string(home)) + (path.size() > 2 ? path.substr(2) : EMPTY_STRING);
    }

    const ea::string baseDir = currentDir.empty() ? GetProcessCurrentDir() : AddTrailingSlash(currentDir);
    return GetAbsolutePath(path, baseDir);
}

} // namespace AssetViewerDetail

} // namespace Urho3D
