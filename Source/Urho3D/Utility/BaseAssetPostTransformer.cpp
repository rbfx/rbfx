// Copyright (c) 2020-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Utility/BaseAssetPostTransformer.h"

#include "Urho3D/Resource/ResourceCache.h"

#include <regex>

namespace Urho3D
{

namespace
{

std::regex PatternToRegex(const ea::string& pattern)
{
    std::string r;
    for (const char ch : pattern)
    {
        if (ch == '*')
            r += "(.*)";
        else
        {
            if (IsCharacterEscapedInRegex(ch))
                r += '\\';
            r += ch;
        }
    }
    return std::regex(r, std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
}

} // namespace

bool BaseAssetPostTransformer::IsApplicable(const AssetTransformerInput& input)
{
    return input.resourceName_.ends_with(GetParametersFileName(), false);
}

ea::vector<BaseAssetPostTransformer::PatternMatch> BaseAssetPostTransformer::GetResourcesByPattern(
    const ea::string& baseResourceName, const ea::string& fileNamePattern) const
{
    auto cache = GetSubsystem<ResourceCache>();
    StringVector fileNames;
    cache->Scan(fileNames, baseResourceName, "*", SCAN_FILES | SCAN_RECURSIVE);

    ea::vector<PatternMatch> result;
    const std::regex regex = PatternToRegex(fileNamePattern);
    for (const ea::string& fileName : fileNames)
    {
        std::cmatch match;
        if (std::regex_match(fileName.c_str(), match, regex))
            result.push_back(PatternMatch{fileName, match.size() > 1 ? match[1].str().c_str() : ""});
    }
    return result;
}

ea::string BaseAssetPostTransformer::GetMatchFileName(
    const ea::string& fileNameTemplate, const PatternMatch& match) const
{
    return Format(fileNameTemplate, match.match_);
}

} // namespace Urho3D
