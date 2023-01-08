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
#pragma once

#include <Urho3D/Urho3D.h>
#include <EASTL/string.h>

namespace Urho3D
{

class URHO3D_API URL
{
public:
    URL() = default;
    URL(unsigned short port) { port_ = port; }
    URL(ea::string_view url) { ParseURL(url); }
    URL& operator=(ea::string_view url) { ParseURL(url); return *this; }
    /// Returns false if URL fails to parse.
    operator bool() const;
    /// Format an url after modifications were applied. No validation is done! Invalid input will result in invalid URL!
    ea::string ToString() const;

    /// Encode strings for inclusion in URLs, or decode them when obtained from URLs.
    /// @{
    static ea::string Encode(ea::string_view string);
    static ea::string Decode(ea::string_view string);
    /// @}

    /// @{
    ea::string scheme_;
    ea::string user_;
    ea::string password_;
    ea::string host_;
    unsigned short port_;
    ea::string path_;
    ea::string query_;
    ea::string hash_;
    /// @}

protected:
    void ParseURL(ea::string_view url);
};

}
