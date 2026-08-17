// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Variant.h>

#include <EASTL/functional.h>
#include <EASTL/vector.h>

namespace Urho3D
{

enum class HotReloadAssetKind
{
    CppModule,
    Blueprint,
    RbScript,
    Resource
};

struct URHO3D_API HotReloadRequest
{
    ea::string key;
    HotReloadAssetKind kind{HotReloadAssetKind::Resource};
    unsigned version{};
    StringVariantMap payload;
    bool preserveState{true};
};

struct URHO3D_API HotReloadResult
{
    bool success{};
    bool stateRestored{};
    unsigned restoredValues{};
    ea::string error;
};

/// Loader contract used by native modules and script/resource frontends.
/// The previous state is provided for schema migration and the callback returns the new state.
using HotReloadLoader = ea::function<bool(const HotReloadRequest&, const StringVariantMap&, StringVariantMap&, ea::string&)>;

struct URHO3D_API HotReloadEntry
{
    ea::string key;
    HotReloadAssetKind kind{HotReloadAssetKind::Resource};
    unsigned version{};
    unsigned long long contentDigest{};
    StringVariantMap state;
};

/// Runtime-safe hot reload coordinator with state capture, versioning and migration callbacks.
class URHO3D_API HotReloadManager
{
public:
    bool Register(const ea::string& key, HotReloadAssetKind kind, unsigned version,
        const StringVariantMap& initialState, const HotReloadLoader& loader);
    bool Unregister(const ea::string& key);
    bool CaptureState(const ea::string& key, const StringVariantMap& state);
    HotReloadResult Reload(const HotReloadRequest& request);

    const HotReloadEntry* Find(const ea::string& key) const;
    ea::vector<HotReloadEntry> GetEntries() const;
    unsigned long long ComputeDigest() const;
    void Clear();

private:
    struct Entry
    {
        HotReloadEntry publicEntry;
        HotReloadLoader loader;
    };

    static unsigned long long ComputeStateDigest(const StringVariantMap& state);
    ea::vector<Entry> entries_;
};

} // namespace Urho3D
