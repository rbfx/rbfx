//
// Copyright (c) 2017-2022 the rbfx project.
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

#include <Urho3D/Core/Format.h>
#include <Urho3D/Network/URL.h>
#include <string>
#include <regex>

namespace Urho3D
{

static unsigned short GetDefaultPort(const ea::string& scheme)
{
    if (scheme == "http")
        return 80;
    if (scheme == "https")
        return 443;
    return 0;
}

static bool IsDefaultPort(unsigned short port, const ea::string& scheme)
{
    if (scheme == "http" && port == 80)
        return true;
    if (scheme == "https" && port == 443)
        return true;
    return false;
}

void URL::ParseURL(ea::string_view url)
{
    scheme_ = user_ = password_ = host_ = path_ = query_ = hash_ = "";
    port_ = 0;
    std::regex rx (R"(^(([^:\/?#]+)://)?((([^:\/?#@]+)(:([^:\/?#@]+))?@)?([^\/?#:]*)(:([0-9]+))?)?(/([^?#]+))?(\?([^#]*))?(#(.*))?)", std::regex::extended);
    std::smatch matches;
    std::string stringUrl(url.data());
    if (std::regex_match(stringUrl, matches, rx) && matches.size() == 17)
    {
        scheme_ = matches[2].str().c_str();
        user_ = matches[5].str().c_str();
        password_ = matches[7].str().c_str();
        host_ = matches[8].str().c_str();
        port_ = atoi(matches[10].str().c_str());
        if (!port_)
            port_ = GetDefaultPort(scheme_);
        path_ = matches[11].str().c_str();
        query_ = matches[14].str().c_str();
        hash_ = matches[16].str().c_str();
    }
}

ea::string URL::ToString() const
{
    ea::string url;
    if (!scheme_.empty())
    {
        url += scheme_;
        url += "://";
    }
    if (!user_.empty())
    {
        url += Encode(user_);
        if (!password_.empty())
        {
            url += ":";
            url += Encode(password_);
        }
        url += "@";
    }
    url += host_;
    if (port_ && !IsDefaultPort(port_, scheme_))
        url += Format(":{}", port_);
    if (!path_.empty())
    {
        if (path_[0] != '/')
            url += "/";
        url += path_;
    }
    if (!query_.empty())
    {
        url += "?";
        url += query_;  // Query parts assumed to be encoded by the user
    }
    if (!hash_.empty())
    {
        url += "#";
        url += hash_;   // Assumed to be encoded by the user
    }
    return url;
}

URL::operator bool() const
{
    return !scheme_.empty() ||
        !user_.empty() ||
        !password_.empty() ||
        !host_.empty() ||
        port_ != 0 ||
        !path_.empty() ||
        !query_.empty() ||
        !hash_.empty();
}

ea::string URL::Encode(ea::string_view string)
{
    ea::string result;
    for (char c : string)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') // rfc3986, 2.3. Unreserved Characters
            result.append(1, c);
        else
            result.append_sprintf("%%%02X", c);
    }
    return result;
}

ea::string URL::Decode(ea::string_view string)
{
    ea::string result;
    for (const char* p = string.begin(); p < string.end();)
    {
        int c = *p++;
        if (c == '%')
        {
            char* end = nullptr;
            c = strtol(p, &end, 16);
            p = end;
        }
        result.append(1, static_cast<char>(c));
    }
    return result;
}

}
