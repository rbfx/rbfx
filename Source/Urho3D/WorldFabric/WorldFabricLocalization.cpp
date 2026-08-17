// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "WorldFabricLocalization.h"

#include <algorithm>

namespace Urho3D
{

bool WorldFabricLocalization::AddLocale(const ea::string& locale)
{
    if (locale.empty() || catalogs_.find(locale) != catalogs_.end())
        return false;
    catalogs_.emplace(locale, Catalog{});
    if (locale_.empty())
        locale_ = locale;
    if (fallbackLocale_.empty())
        fallbackLocale_ = locale;
    return true;
}

bool WorldFabricLocalization::RemoveLocale(const ea::string& locale)
{
    if (catalogs_.erase(locale) == 0)
        return false;
    if (locale_ == locale)
        locale_ = catalogs_.empty() ? ea::string() : catalogs_.begin()->first;
    if (fallbackLocale_ == locale)
        fallbackLocale_ = catalogs_.empty() ? ea::string() : catalogs_.begin()->first;
    return true;
}

bool WorldFabricLocalization::SetLocale(const ea::string& locale)
{
    if (catalogs_.find(locale) == catalogs_.end())
        return false;
    locale_ = locale;
    return true;
}

bool WorldFabricLocalization::SetFallbackLocale(const ea::string& locale)
{
    if (catalogs_.find(locale) == catalogs_.end())
        return false;
    fallbackLocale_ = locale;
    return true;
}

bool WorldFabricLocalization::SetText(const ea::string& locale, const ea::string& key, const ea::string& text)
{
    if (locale.empty() || key.empty() || catalogs_.find(locale) == catalogs_.end())
        return false;
    catalogs_[locale][key] = text;
    return true;
}

bool WorldFabricLocalization::RemoveText(const ea::string& locale, const ea::string& key)
{
    auto localeIt = catalogs_.find(locale);
    if (localeIt == catalogs_.end())
        return false;
    return localeIt->second.erase(key) != 0;
}

ea::string WorldFabricLocalization::Translate(const ea::string& key, const ea::string& fallback) const
{
    const auto findText = [&key](const Catalog* catalog) -> ea::string
    {
        if (!catalog)
            return ea::string();
        const auto it = catalog->find(key);
        return it != catalog->end() ? it->second : ea::string();
    };

    const auto localeIt = catalogs_.find(locale_);
    ea::string result = findText(localeIt != catalogs_.end() ? &localeIt->second : nullptr);
    if (!result.empty())
        return result;
    const auto fallbackIt = catalogs_.find(fallbackLocale_);
    result = findText(fallbackIt != catalogs_.end() ? &fallbackIt->second : nullptr);
    return result.empty() ? fallback : result;
}

ea::vector<ea::string> WorldFabricLocalization::GetLocales() const
{
    ea::vector<ea::string> result;
    result.reserve(catalogs_.size());
    for (const auto& entry : catalogs_)
        result.push_back(entry.first);
    std::sort(result.begin(), result.end());
    return result;
}

unsigned long long WorldFabricLocalization::ComputeDigest() const
{
    unsigned long long digest = 1469598103934665603ull;
    const auto mix = [&digest](const ea::string& value)
    {
        for (char character : value)
        {
            digest ^= static_cast<unsigned char>(character);
            digest *= 1099511628211ull;
        }
        digest ^= 0xff;
        digest *= 1099511628211ull;
    };
    mix(locale_);
    mix(fallbackLocale_);
    const ea::vector<ea::string> locales = GetLocales();
    for (const ea::string& locale : locales)
    {
        mix(locale);
        const Catalog& catalog = catalogs_.at(locale);
        ea::vector<ea::string> keys;
        keys.reserve(catalog.size());
        for (const auto& entry : catalog)
            keys.push_back(entry.first);
        std::sort(keys.begin(), keys.end());
        for (const ea::string& key : keys)
        {
            mix(key);
            mix(catalog.at(key));
        }
    }
    return digest;
}

} // namespace Urho3D
