// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "HotReloadManager.h"

#include <algorithm>
#include <cstdint>

namespace Urho3D
{

namespace
{

void HashByte(unsigned long long& hash, unsigned char value)
{
    hash ^= value;
    hash *= 1099511628211ull;
}

void HashString(unsigned long long& hash, const ea::string& value)
{
    for (unsigned char byte : value)
        HashByte(hash, byte);
    HashByte(hash, 0);
}

} // namespace

bool HotReloadManager::Register(const ea::string& key, HotReloadAssetKind kind, unsigned version,
    const StringVariantMap& initialState, const HotReloadLoader& loader)
{
    if (key.empty() || !loader)
        return false;
    if (Find(key))
        return false;

    Entry entry;
    entry.publicEntry.key = key;
    entry.publicEntry.kind = kind;
    entry.publicEntry.version = version;
    entry.publicEntry.state = initialState;
    entry.publicEntry.contentDigest = ComputeStateDigest(initialState);
    entry.loader = loader;
    entries_.push_back(entry);
    return true;
}

bool HotReloadManager::Unregister(const ea::string& key)
{
    const auto it = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& entry)
    {
        return entry.publicEntry.key == key;
    });
    if (it == entries_.end())
        return false;
    entries_.erase(it);
    return true;
}

bool HotReloadManager::CaptureState(const ea::string& key, const StringVariantMap& state)
{
    auto it = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& entry)
    {
        return entry.publicEntry.key == key;
    });
    if (it == entries_.end())
        return false;
    it->publicEntry.state = state;
    it->publicEntry.contentDigest = ComputeStateDigest(state);
    return true;
}

HotReloadResult HotReloadManager::Reload(const HotReloadRequest& request)
{
    HotReloadResult result;
    auto it = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& entry)
    {
        return entry.publicEntry.key == request.key;
    });
    if (it == entries_.end())
    {
        result.error = "Hot reload entry is not registered.";
        return result;
    }
    if (request.version == 0)
    {
        result.error = "Hot reload version must be non-zero.";
        return result;
    }

    const StringVariantMap previousState = it->publicEntry.state;
    StringVariantMap nextState;
    ea::string error;
    if (!it->loader(request, request.preserveState ? previousState : StringVariantMap{}, nextState, error))
    {
        result.error = error.empty() ? "Hot reload loader rejected the request." : error;
        return result;
    }

    it->publicEntry.version = request.version;
    it->publicEntry.state = nextState;
    it->publicEntry.contentDigest = ComputeStateDigest(nextState);
    result.success = true;
    result.stateRestored = request.preserveState;
    result.restoredValues = request.preserveState ? nextState.size() : 0;
    return result;
}

const HotReloadEntry* HotReloadManager::Find(const ea::string& key) const
{
    for (const Entry& entry : entries_)
    {
        if (entry.publicEntry.key == key)
            return &entry.publicEntry;
    }
    return nullptr;
}

ea::vector<HotReloadEntry> HotReloadManager::GetEntries() const
{
    ea::vector<HotReloadEntry> result;
    result.reserve(entries_.size());
    for (const Entry& entry : entries_)
        result.push_back(entry.publicEntry);
    std::sort(result.begin(), result.end(), [](const HotReloadEntry& lhs, const HotReloadEntry& rhs)
    {
        return lhs.key < rhs.key;
    });
    return result;
}

unsigned long long HotReloadManager::ComputeStateDigest(const StringVariantMap& state)
{
    ea::vector<ea::string> keys;
    keys.reserve(state.size());
    for (const auto& entry : state)
        keys.push_back(entry.first);
    std::sort(keys.begin(), keys.end());

    unsigned long long hash = 1469598103934665603ull;
    for (const ea::string& key : keys)
    {
        HashString(hash, key);
        const auto it = state.find(key);
        if (it != state.end())
        {
            HashByte(hash, static_cast<unsigned char>(it->second.GetType()));
            HashString(hash, it->second.ToString());
        }
    }
    return hash;
}

unsigned long long HotReloadManager::ComputeDigest() const
{
    unsigned long long hash = 1469598103934665603ull;
    for (const HotReloadEntry& entry : GetEntries())
    {
        HashString(hash, entry.key);
        HashByte(hash, static_cast<unsigned char>(entry.kind));
        HashByte(hash, static_cast<unsigned char>(entry.version & 0xff));
        HashByte(hash, static_cast<unsigned char>((entry.version >> 8) & 0xff));
        for (unsigned shift = 0; shift < 8; ++shift)
            HashByte(hash, static_cast<unsigned char>((entry.contentDigest >> (shift * 8)) & 0xff));
    }
    return hash;
}

void HotReloadManager::Clear()
{
    entries_.clear();
}

} // namespace Urho3D
