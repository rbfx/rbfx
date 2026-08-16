//
// Copyright (c) 2026 the rbfx-blueprint project.
//
// SPDX-License-Identifier: MIT
//

#include "PackageBuilder.h"

#include "../Core/StringUtils.h"

#include <EASTL/sort.h>

#include <cstdlib>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

void SetError(ea::string* error, const ea::string& message)
{
    if (error)
        *error = message;
}

bool WildcardMatch(const char* pattern, const char* value)
{
    const char* star = nullptr;
    const char* retry = nullptr;
    while (*value)
    {
        if (*pattern == '?' || *pattern == *value)
        {
            ++pattern;
            ++value;
        }
        else if (*pattern == '*')
        {
            star = pattern++;
            retry = value;
        }
        else if (star)
        {
            pattern = star + 1;
            value = ++retry;
        }
        else
            return false;
    }
    while (*pattern == '*')
        ++pattern;
    return *pattern == '\0';
}

bool Matches(const ea::string& pattern, const ea::string& value)
{
    return WildcardMatch(pattern.c_str(), value.c_str());
}

bool ParseUnsignedString(const ea::string& text, unsigned long long& value)
{
    if (text.empty())
        return false;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);
    if (!end || *end != '\0')
        return false;
    value = parsed;
    return true;
}

} // namespace

JSONValue PackageBuildProfile::ToJSON() const
{
    JSONValue root(JSON_OBJECT);
    root.Set("version", version);
    root.Set("name", name);
    root.Set("platform", PackageBuilder::ToString(platform));
    root.Set("architecture", architecture);
    root.Set("optimization", PackageBuilder::ToString(optimization));
    root.Set("outputPath", outputPath);
    root.Set("reproducible", reproducible);

    JSONValue filters(JSON_ARRAY);
    for (const PackageAssetFilter& filter : assetFilters)
    {
        JSONValue item(JSON_OBJECT);
        item.Set("pattern", filter.pattern);
        item.Set("exclude", filter.exclude);
        filters.Push(ea::move(item));
    }
    root.Set("assetFilters", ea::move(filters));
    return root;
}

bool PackageBuildProfile::FromJSON(const JSONValue& value, ea::string* error)
{
    if (!value.IsObject())
    {
        SetError(error, "Package build profile root must be an object.");
        return false;
    }

    PackageBuildProfile parsed;
    parsed.version = value.Contains("version") ? value["version"].GetUInt(1) : 1;
    parsed.name = value.Contains("name") ? value["name"].GetString() : EMPTY_STRING;
    parsed.architecture = value.Contains("architecture") ? value["architecture"].GetString() : EMPTY_STRING;
    parsed.outputPath = value.Contains("outputPath") ? value["outputPath"].GetString() : EMPTY_STRING;
    parsed.reproducible = value.Contains("reproducible") ? value["reproducible"].GetBool(true) : true;

    if (!value.Contains("platform") || !PackageBuilder::FromString(value["platform"].GetString(), parsed.platform))
    {
        SetError(error, "Package build profile has an unsupported platform.");
        return false;
    }
    if (!value.Contains("optimization") || !PackageBuilder::FromString(value["optimization"].GetString(), parsed.optimization))
    {
        SetError(error, "Package build profile has an unsupported optimization level.");
        return false;
    }

    if (value.Contains("assetFilters"))
    {
        if (!value["assetFilters"].IsArray())
        {
            SetError(error, "Package build profile assetFilters must be an array.");
            return false;
        }
        for (const JSONValue& item : value["assetFilters"].GetArray())
        {
            if (!item.IsObject() || !item.Contains("pattern"))
            {
                SetError(error, "Each package asset filter must contain a pattern.");
                return false;
            }
            PackageAssetFilter filter;
            filter.pattern = item["pattern"].GetString();
            filter.exclude = item.Contains("exclude") && item["exclude"].GetBool();
            parsed.assetFilters.push_back(ea::move(filter));
        }
    }

    PackageValidationResult validation = PackageBuilder::ValidateProfile(parsed);
    if (!validation.valid)
    {
        SetError(error, validation.errors.empty() ? "Invalid package build profile." : validation.errors.front());
        return false;
    }
    *this = ea::move(parsed);
    return true;
}

bool PackageBuildProfile::IncludesAsset(const ea::string& assetPath) const
{
    bool hasIncludeRule = false;
    bool included = false;
    for (const PackageAssetFilter& filter : assetFilters)
    {
        if (!filter.exclude)
        {
            hasIncludeRule = true;
            if (Matches(filter.pattern, assetPath))
                included = true;
        }
        else if (Matches(filter.pattern, assetPath))
            return false;
    }
    return hasIncludeRule ? included : true;
}

JSONValue PackageManifest::ToJSON() const
{
    JSONValue root(JSON_OBJECT);
    root.Set("version", version);
    root.Set("profileName", profileName);
    root.Set("platform", PackageBuilder::ToString(platform));
    root.Set("architecture", architecture);

    ea::vector<PackageFileEntry> sortedFiles = files;
    ea::sort(sortedFiles.begin(), sortedFiles.end(), [](const PackageFileEntry& lhs, const PackageFileEntry& rhs)
    {
        if (lhs.packagePath != rhs.packagePath)
            return lhs.packagePath < rhs.packagePath;
        return lhs.sourcePath < rhs.sourcePath;
    });

    JSONValue entries(JSON_ARRAY);
    for (const PackageFileEntry& file : sortedFiles)
    {
        JSONValue item(JSON_OBJECT);
        item.Set("sourcePath", file.sourcePath);
        item.Set("packagePath", file.packagePath);
        item.Set("contentHash", file.contentHash);
        item.Set("size", Format("{}", file.size));
        entries.Push(ea::move(item));
    }
    root.Set("files", ea::move(entries));
    return root;
}

bool PackageManifest::FromJSON(const JSONValue& value, ea::string* error)
{
    if (!value.IsObject())
    {
        SetError(error, "Package manifest root must be an object.");
        return false;
    }

    PackageManifest parsed;
    parsed.version = value.Contains("version") ? value["version"].GetUInt(1) : 1;
    parsed.profileName = value.Contains("profileName") ? value["profileName"].GetString() : EMPTY_STRING;
    parsed.architecture = value.Contains("architecture") ? value["architecture"].GetString() : EMPTY_STRING;
    if (!value.Contains("platform") || !PackageBuilder::FromString(value["platform"].GetString(), parsed.platform))
    {
        SetError(error, "Package manifest has an unsupported platform.");
        return false;
    }
    if (!value.Contains("files") || !value["files"].IsArray())
    {
        SetError(error, "Package manifest files must be an array.");
        return false;
    }

    for (const JSONValue& item : value["files"].GetArray())
    {
        if (!item.IsObject() || !item.Contains("sourcePath") || !item.Contains("packagePath"))
        {
            SetError(error, "Each package manifest entry must contain sourcePath and packagePath.");
            return false;
        }
        PackageFileEntry file;
        file.sourcePath = item["sourcePath"].GetString();
        file.packagePath = item["packagePath"].GetString();
        file.contentHash = item.Contains("contentHash") ? item["contentHash"].GetUInt() : 0;
        if (item.Contains("size") && item["size"].IsString())
        {
            if (!ParseUnsignedString(item["size"].GetString(), file.size))
            {
                SetError(error, Format("Invalid size for package entry '{}'.", file.packagePath));
                return false;
            }
        }
        else if (item.Contains("size") && item["size"].IsNumber())
            file.size = static_cast<unsigned long long>(item["size"].GetDouble());
        parsed.files.push_back(ea::move(file));
    }

    PackageValidationResult validation = PackageBuilder::ValidateManifest(parsed);
    if (!validation.valid)
    {
        SetError(error, validation.errors.empty() ? "Invalid package manifest." : validation.errors.front());
        return false;
    }
    *this = ea::move(parsed);
    return true;
}

ea::string PackageBuilder::ToString(PackagePlatform platform)
{
    switch (platform)
    {
    case PackagePlatform::Linux: return "Linux";
    case PackagePlatform::Windows: return "Windows";
    case PackagePlatform::macOS: return "macOS";
    case PackagePlatform::WebAssembly: return "WebAssembly";
    case PackagePlatform::Android: return "Android";
    case PackagePlatform::iOS: return "iOS";
    default: return "Unknown";
    }
}

bool PackageBuilder::FromString(const ea::string& value, PackagePlatform& platform)
{
    if (value == "Linux") platform = PackagePlatform::Linux;
    else if (value == "Windows") platform = PackagePlatform::Windows;
    else if (value == "macOS") platform = PackagePlatform::macOS;
    else if (value == "WebAssembly") platform = PackagePlatform::WebAssembly;
    else if (value == "Android") platform = PackagePlatform::Android;
    else if (value == "iOS") platform = PackagePlatform::iOS;
    else return false;
    return true;
}

ea::string PackageBuilder::ToString(PackageOptimization optimization)
{
    switch (optimization)
    {
    case PackageOptimization::Debug: return "Debug";
    case PackageOptimization::Development: return "Development";
    case PackageOptimization::Shipping: return "Shipping";
    default: return "Unknown";
    }
}

bool PackageBuilder::FromString(const ea::string& value, PackageOptimization& optimization)
{
    if (value == "Debug") optimization = PackageOptimization::Debug;
    else if (value == "Development") optimization = PackageOptimization::Development;
    else if (value == "Shipping") optimization = PackageOptimization::Shipping;
    else return false;
    return true;
}

PackageValidationResult PackageBuilder::ValidateProfile(const PackageBuildProfile& profile)
{
    PackageValidationResult result;
    result.valid = true;
    if (profile.version == 0)
    {
        result.valid = false;
        result.errors.push_back("Package build profile version must be non-zero.");
    }
    if (profile.name.empty())
    {
        result.valid = false;
        result.errors.push_back("Package build profile name must not be empty.");
    }
    if (profile.architecture.empty())
    {
        result.valid = false;
        result.errors.push_back("Package build profile architecture must not be empty.");
    }
    if (profile.outputPath.empty())
    {
        result.valid = false;
        result.errors.push_back("Package build profile outputPath must not be empty.");
    }
    for (const PackageAssetFilter& filter : profile.assetFilters)
    {
        if (filter.pattern.empty())
        {
            result.valid = false;
            result.errors.push_back("Package asset filter patterns must not be empty.");
        }
    }
    return result;
}

PackageValidationResult PackageBuilder::ValidateManifest(const PackageManifest& manifest)
{
    PackageValidationResult result;
    result.valid = true;
    if (manifest.version == 0)
    {
        result.valid = false;
        result.errors.push_back("Package manifest version must be non-zero.");
    }
    if (manifest.profileName.empty())
    {
        result.valid = false;
        result.errors.push_back("Package manifest profileName must not be empty.");
    }
    if (manifest.architecture.empty())
    {
        result.valid = false;
        result.errors.push_back("Package manifest architecture must not be empty.");
    }

    ea::vector<ea::string> packagePaths;
    for (const PackageFileEntry& file : manifest.files)
    {
        if (file.sourcePath.empty() || file.packagePath.empty())
        {
            result.valid = false;
            result.errors.push_back("Package manifest entries must have non-empty paths.");
            continue;
        }
        if (ea::find(packagePaths.begin(), packagePaths.end(), file.packagePath) != packagePaths.end())
        {
            result.valid = false;
            result.errors.push_back(Format("Duplicate package path '{}'.", file.packagePath));
        }
        else
            packagePaths.push_back(file.packagePath);
    }
    return result;
}

bool PackageBuilder::BuildManifest(const PackageBuildProfile& profile, const ea::vector<PackageFileEntry>& candidates,
    PackageManifest& manifest, ea::string* error)
{
    const PackageValidationResult profileValidation = ValidateProfile(profile);
    if (!profileValidation.valid)
    {
        SetError(error, profileValidation.errors.empty() ? "Invalid package build profile." : profileValidation.errors.front());
        return false;
    }

    PackageManifest built;
    built.profileName = profile.name;
    built.platform = profile.platform;
    built.architecture = profile.architecture;
    for (const PackageFileEntry& candidate : candidates)
    {
        if (profile.IncludesAsset(candidate.sourcePath))
            built.files.push_back(candidate);
    }

    const PackageValidationResult manifestValidation = ValidateManifest(built);
    if (!manifestValidation.valid)
    {
        SetError(error, manifestValidation.errors.empty() ? "Invalid package manifest." : manifestValidation.errors.front());
        return false;
    }
    manifest = ea::move(built);
    return true;
}

} // namespace Urho3D
