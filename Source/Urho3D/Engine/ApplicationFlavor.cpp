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

#include "../Precompiled.h"

#include "../Engine/ApplicationFlavor.h"
#include "../IO/Log.h"

#include <regex>

namespace Urho3D
{

namespace
{

ApplicationFlavorMap ParseString(const ea::string& str)
{
    static const std::regex r(R"(\s*([\w\*]*)\s*=\s*((?:[\w\*]+\s*,\s*)*(?:[\w\*]+))\s*)");

    ApplicationFlavorMap result;
    const StringVector components = str.split(';');
    for (const ea::string& component : components)
    {
        std::cmatch match;
        if (!std::regex_match(component.c_str(), match, r))
        {
            URHO3D_LOGERROR("Invalid application flavor component: '{}', should be like 'component=tag1[,tag2,...]'", component);
            continue;
        }

        const ea::string key = match.str(1).c_str();
        const StringVector tags = ea::string(match.str(2).c_str()).split(',');
        result[key].insert(tags.begin(), tags.end());
    }
    return result;
}

unsigned GetStringWeight(ea::string_view str)
{
    return ea::count(str.begin(), str.end(), '.');
}

ea::optional<unsigned> GetDistanceFromPattern(ea::string_view line, ea::string_view pattern)
{
    if (pattern == "*")
        return GetStringWeight(line);
    else if (pattern.starts_with("*"))
    {
        const ea::string_view suffix = pattern.substr(1);

        if (!line.ends_with(suffix))
            return ea::nullopt;

        return GetStringWeight(line) - GetStringWeight(suffix);
    }
    else if (pattern.ends_with("*"))
    {
        const ea::string_view prefix = pattern.substr(0, pattern.length() - 1);

        if (!line.starts_with(prefix))
            return ea::nullopt;

        return GetStringWeight(line) - GetStringWeight(prefix);
    }
    else
    {
        if (line != pattern)
            return ea::nullopt;
        return 0;
    }
}

}

ApplicationFlavor ApplicationFlavor::Universal{{"*", {"*"}}};
ApplicationFlavor ApplicationFlavor::Empty;

ApplicationFlavorPattern::ApplicationFlavorPattern(const ea::string& str)
    : components_(ParseString(str))
{
}

ApplicationFlavorPattern::ApplicationFlavorPattern(std::initializer_list<ApplicationFlavorMap::value_type> components)
    : components_(components)
{
}

ApplicationFlavor::ApplicationFlavor(const ea::string& str)
    : components_(ParseString(str))
{
}

ApplicationFlavor::ApplicationFlavor(std::initializer_list<ApplicationFlavorMap::value_type> components)
    : components_(components)
{
}

ea::optional<unsigned> ApplicationFlavor::Matches(const ApplicationFlavorPattern& pattern) const
{
    // Matches any pattern with default distance.
    if (components_ == ApplicationFlavor::Universal.components_)
        return M_MAX_UNSIGNED;

    unsigned distance = 0;
    for (const auto& [key, patternTags] : pattern.components_)
    {
        // If universal flavor pattern is present, ignore this line.
        if (patternTags.contains("*"))
            continue;

        // If there are pattern tags without corresponding flavor tags, the flavor doesn't match.
        const auto iter = components_.find(key);
        if (iter == components_.end())
            return ea::nullopt;

        // If universal flavor tag is present, don't check pattern tags.
        // Increase penalty for every ignored pattern tag.
        const ApplicationFlavorComponent& flavorTags = iter->second;
        if (flavorTags.contains("*"))
        {
            distance += patternTags.size();
            continue;
        }

        // If there's a pattern tag with no corresponding flavor tag, the flavor doesn't match.
        for (const ea::string& tag : patternTags)
        {
            if (!flavorTags.contains(tag))
                return ea::nullopt;
        }

        // Increase distance for every unused flavor tag.
        distance += flavorTags.size() - patternTags.size();
    }

    // Increase distance for unused flavor tags.
    for (const auto& [key, flavorTags] : components_)
    {
        const auto iter = pattern.components_.find(key);
        if (iter != pattern.components_.end() && !iter->second.contains("*"))
            continue;

        if (flavorTags.contains("*"))
            continue;

        distance += flavorTags.size();
    }

    return distance;
}

}
