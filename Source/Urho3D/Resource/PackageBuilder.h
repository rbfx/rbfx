//
// Copyright (c) 2026 the rbfx-blueprint project.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include "Urho3D/Urho3D.h"
#include "Urho3D/Resource/JSONValue.h"

#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

enum class PackagePlatform
{
    Linux,
    Windows,
    macOS,
    WebAssembly,
    Android,
    iOS
};

enum class PackageOptimization
{
    Debug,
    Development,
    Shipping
};

struct URHO3D_API PackageAssetFilter
{
    ea::string pattern;
    bool exclude{};
};

/// Versioned, reproducible build profile consumed by package tools and CI.
struct URHO3D_API PackageBuildProfile
{
    unsigned version{1};
    ea::string name{"Default"};
    PackagePlatform platform{PackagePlatform::Linux};
    ea::string architecture{"x64"};
    PackageOptimization optimization{PackageOptimization::Development};
    ea::string outputPath{"Build"};
    bool reproducible{true};
    /// Optional semantic graph digest used to make World Fabric-aware exports reproducible.
    unsigned long long worldFabricDigest{};
    ea::vector<PackageAssetFilter> assetFilters;

    JSONValue ToJSON() const;
    bool FromJSON(const JSONValue& value, ea::string* error = nullptr);
    bool IncludesAsset(const ea::string& assetPath) const;
};

struct URHO3D_API PackageFileEntry
{
    ea::string sourcePath;
    ea::string packagePath;
    unsigned contentHash{};
    unsigned long long size{};
};

/// Manifest describing the exact files emitted by one package build.
struct URHO3D_API PackageManifest
{
    unsigned version{1};
    ea::string profileName;
    PackagePlatform platform{PackagePlatform::Linux};
    ea::string architecture;
    /// Digest of the semantic graph used to produce this manifest, or zero when not bound.
    unsigned long long worldFabricDigest{};
    ea::vector<PackageFileEntry> files;

    JSONValue ToJSON() const;
    bool FromJSON(const JSONValue& value, ea::string* error = nullptr);
};

struct URHO3D_API PackageValidationResult
{
    bool valid{};
    ea::vector<ea::string> errors;
};

/// Deterministic package profile and manifest utilities shared by tools and CI.
class URHO3D_API PackageBuilder
{
public:
    static ea::string ToString(PackagePlatform platform);
    static bool FromString(const ea::string& value, PackagePlatform& platform);
    static ea::string ToString(PackageOptimization optimization);
    static bool FromString(const ea::string& value, PackageOptimization& optimization);

    static PackageValidationResult ValidateProfile(const PackageBuildProfile& profile);
    static PackageValidationResult ValidateManifest(const PackageManifest& manifest);

    /// Build a manifest from candidate files, applying profile asset filters.
    static bool BuildManifest(const PackageBuildProfile& profile, const ea::vector<PackageFileEntry>& candidates,
        PackageManifest& manifest, ea::string* error = nullptr);
};

} // namespace Urho3D
