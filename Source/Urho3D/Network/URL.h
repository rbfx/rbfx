// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
