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
    fmt::vformat_to(std::back_inserter(ret), ToFmtStringView(formatString), fmt::make_format_args(args...));
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
