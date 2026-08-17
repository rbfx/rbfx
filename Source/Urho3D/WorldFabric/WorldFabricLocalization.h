// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <Urho3D/Urho3D.h>

#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Deterministic runtime localization catalog used by UI, Blueprint and rbscript.
class URHO3D_API WorldFabricLocalization
{
public:
    bool AddLocale(const ea::string& locale);
    bool RemoveLocale(const ea::string& locale);
    bool SetLocale(const ea::string& locale);
    bool SetFallbackLocale(const ea::string& locale);
    bool SetText(const ea::string& locale, const ea::string& key, const ea::string& text);
    bool RemoveText(const ea::string& locale, const ea::string& key);

    ea::string Translate(const ea::string& key, const ea::string& fallback = ea::string()) const;
    const ea::string& GetLocale() const { return locale_; }
    const ea::string& GetFallbackLocale() const { return fallbackLocale_; }
    ea::vector<ea::string> GetLocales() const;
    unsigned long long ComputeDigest() const;

private:
    using Catalog = ea::unordered_map<ea::string, ea::string>;
    ea::unordered_map<ea::string, Catalog> catalogs_;
    ea::string locale_;
    ea::string fallbackLocale_;
};

} // namespace Urho3D
