// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
