//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Core/IniHelpers.h"

namespace Urho3D
{

namespace
{

// TODO(editor): Move to EASTL?
void SplitString(ea::vector<ea::string_view>& result,
    ea::string_view source, ea::string_view separator, bool keepEmptyStrings = false)
{
    result.clear();

    eastl_size_t start = 0;
    eastl_size_t end = source.find(separator);
    while (end != ea::string::npos)
    {
        ea::string_view token = source.substr(start, end - start);
        if (keepEmptyStrings || !token.empty())
            result.push_back(token);
        start = end + separator.length();
        end = source.find(separator, start);
    }

    ea::string_view token = source.substr(start, end);
    if (keepEmptyStrings || !token.empty())
        result.push_back(token);
}

}

void WriteStringToIni(ImGuiTextBuffer& output, ea::string_view name, ea::string_view value)
{
    output.append(name.begin(), name.end());
    output.append("=");
    output.append(value.begin(), value.end());
    output.append("\n");
}

ea::optional<ea::string> ReadStringFromIni(ea::string_view line, ea::string_view name)
{
    if (!line.starts_with(name))
        return ea::nullopt;

    line = line.substr(name.size());

    if (line.empty() || line[0] != '=')
        return ea::nullopt;

    return ea::string(line.substr(1));
}

void WriteIntToIni(ImGuiTextBuffer& output, ea::string_view name, int value)
{
    WriteStringToIni(output, name, ea::to_string(value));
}

ea::optional<int> ReadIntFromIni(ea::string_view line, ea::string_view name)
{
    if (const auto string = ReadStringFromIni(line, name))
        return ToInt(*string);
    return ea::nullopt;
}

}
