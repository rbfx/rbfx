// Copyright (c) 2008-2020 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <EASTL/iterator.h>
#include <EASTL/string.h>

#if _MSC_VER
#   pragma warning(push, 0)
#endif
#include <fmt/format.h>
#if _MSC_VER
#   pragma warning(pop)
#endif

namespace Urho3D
{

/// Helper function to create fmt::string_view from any container.
template <class T>
inline EA_CONSTEXPR fmt::string_view ToFmtStringView(const T& str)
{
    return fmt::string_view{str.data(), str.size()};
}

/// Return a formatted string.
template <typename... Args>
ea::string Format(ea::string_view formatString, const Args&... args)
{
    ea::string ret;
    fmt::vformat_to(ea::back_inserter(ret), ToFmtStringView(formatString), fmt::make_format_args(args...));
    return ret;
}

}

template <> struct fmt::formatter<ea::string> : fmt::formatter<fmt::string_view>
{
    template <class FormatContext>
    auto format(const ea::string& value, FormatContext& ctx) const
    {
        return fmt::formatter<fmt::string_view>::format(Urho3D::ToFmtStringView(value), ctx);
    }
};

template <> struct fmt::formatter<ea::string_view> : fmt::formatter<fmt::string_view>
{
    template <class FormatContext>
    auto format(const ea::string_view& value, FormatContext& ctx) const
    {
        return fmt::formatter<fmt::string_view>::format(Urho3D::ToFmtStringView(value), ctx);
    }
};

template <class T>
struct fmt::formatter<T, char, ea::enable_if_t<ea::is_enum_v<T>>> : fmt::formatter<int>
{
    template <class FormatContext>
    auto format(const T& value, FormatContext& ctx) const
    {
        return fmt::formatter<int>::format(static_cast<int>(value), ctx);
    }
};
