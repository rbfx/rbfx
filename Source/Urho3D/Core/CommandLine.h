// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#if DESKTOP

#include <sstream>
#include <EASTL/vector.h>
#include <EASTL/string.h>

namespace CLI
{

inline std::istringstream& operator>>(std::istringstream &in, ea::string& val)
{
    const std::string tmp(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>{});
    val = tmp.c_str();
    return in;
}

inline std::stringstream& operator<<(std::stringstream &out, ea::string& val)
{
    out << val.c_str();
    return out;
}

}

#include <CLI11/CLI11.hpp>


namespace CLI
{

namespace detail
{

template <> struct classify_object<ea::string, void>
{
    static constexpr object_category value{object_category::other};
};

template <> struct is_mutable_container<ea::string> : std::false_type
{
};

template <class T, enable_if_t<std::is_same<T, ea::string>::value>> inline std::string type_name()
{
    return "TEXT";
}

}

}

#endif  // defined(DESKTOP)
