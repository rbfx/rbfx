// Copyright (c) 2026 the rbfx-blueprint project.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include "PackageBuilder.h"
#include <Urho3D/Core/Variant.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Explicit capabilities exposed by a native or WebAssembly export backend.
struct URHO3D_API PlatformExportCapabilities
{
    bool supportsThreads{};
    bool supportsGpu{};
    bool supportsNetworking{};
    bool supportsAot{};
    bool supportsDynamicCode{};
    ea::vector<ea::string> architectures;
};

/// Platform-specific export contract used by packaging tools, CI and Blueprint.
class URHO3D_API PlatformExportAdapter
{
public:
    virtual ~PlatformExportAdapter() = default;

    virtual PackagePlatform GetPlatform() const = 0;
    virtual const char* GetName() const = 0;
    virtual const PlatformExportCapabilities& GetCapabilities() const = 0;

    /// Validate that a package profile can be exported by this adapter.
    bool Validate(const PackageBuildProfile& profile, ea::string* error = nullptr) const;

    /// Find the deterministic adapter for a package platform, or nullptr when unsupported.
    static const PlatformExportAdapter* Find(PackagePlatform platform);
    /// Convert platform capabilities to a Blueprint/package-friendly Variant map.
    static StringVariantMap Describe(PackagePlatform platform, bool* supported = nullptr);
};

} // namespace Urho3D
