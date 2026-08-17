// Copyright (c) 2026 the rbfx-blueprint project.
//
// SPDX-License-Identifier: MIT
//

#include "WorldFabricProfiler.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Profiler/ProductionProfiler.h>

#include <EASTL/sort.h>

#include <cmath>
#include <cstring>

namespace Urho3D
{

namespace
{

unsigned long long HashBytes(unsigned long long hash, const void* data, size_t size)
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < size; ++i)
    {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

unsigned long long HashString(unsigned long long hash, const ea::string& value)
{
    return HashBytes(hash, value.data(), value.size());
}

} // namespace

size_t WorldFabricProfiler::KeyHash::operator()(const Key& key) const
{
    unsigned long long hash = 1469598103934665603ULL;
    hash = HashBytes(hash, &key.node, sizeof(key.node));
    hash = HashString(hash, key.channel);
    return static_cast<size_t>(hash);
}

ea::string WorldFabricProfiler::NormalizeChannel(const ea::string& channel)
{
    return channel.empty() ? ea::string("CPU") : channel;
}

void WorldFabricProfiler::SetError(ea::string* error, const ea::string& message) const
{
    if (error)
        *error = message;
}

bool WorldFabricProfiler::AnnotateNode(WorldFabricId node, double durationMilliseconds,
    const ea::string& channel, ea::string* error)
{
    if (!graph_)
    {
        SetError(error, "World Fabric profiling requires a bound WorldFabricGraph.");
        return false;
    }
    const WorldFabricNode* semanticNode = graph_->GetNode(node);
    if (!semanticNode)
    {
        SetError(error, "World Fabric profiling annotation referenced an unknown node.");
        return false;
    }
    if (!std::isfinite(durationMilliseconds) || durationMilliseconds < 0.0)
    {
        SetError(error, "World Fabric profiling duration must be finite and non-negative.");
        return false;
    }

    const ea::string normalizedChannel = NormalizeChannel(channel);
    Key key;
    key.node = node;
    key.channel = normalizedChannel;
    WorldFabricNodeProfile& profile = profiles_[key];
    profile.node = node;
    profile.key = semanticNode->key;
    profile.channel = normalizedChannel;
    ++profile.calls;
    profile.totalMilliseconds += durationMilliseconds;
    if (profile.calls == 1)
        profile.minimumMilliseconds = durationMilliseconds;
    else
        profile.minimumMilliseconds = Min(profile.minimumMilliseconds, durationMilliseconds);
    profile.maximumMilliseconds = Max(profile.maximumMilliseconds, durationMilliseconds);

    if (profiler_)
    {
        const ea::string scopeName = Format("WorldFabric/{}/{}", semanticNode->key, normalizedChannel);
        if (normalizedChannel == "GPU")
            profiler_->RecordGpuPass(scopeName, durationMilliseconds);
        else if (normalizedChannel == "rbscript" || normalizedChannel == "Script")
            profiler_->RecordScriptFunction(scopeName, durationMilliseconds);
        else
            profiler_->RecordScope(scopeName, durationMilliseconds);
    }
    return true;
}

bool WorldFabricProfiler::GetNodeStats(WorldFabricId node, const ea::string& channel,
    WorldFabricNodeProfile& stats) const
{
    Key key;
    key.node = node;
    key.channel = NormalizeChannel(channel);
    const auto it = profiles_.find(key);
    if (it == profiles_.end())
        return false;
    stats = it->second;
    return true;
}

ea::vector<WorldFabricNodeProfile> WorldFabricProfiler::GetAllNodeStats() const
{
    ea::vector<WorldFabricNodeProfile> result;
    result.reserve(profiles_.size());
    for (const auto& item : profiles_)
        result.push_back(item.second);
    ea::sort(result.begin(), result.end(), [](const WorldFabricNodeProfile& lhs, const WorldFabricNodeProfile& rhs)
    {
        if (lhs.node != rhs.node)
            return lhs.node < rhs.node;
        return lhs.channel < rhs.channel;
    });
    return result;
}

void WorldFabricProfiler::Reset()
{
    profiles_.clear();
}

unsigned long long WorldFabricProfiler::ComputeDigest() const
{
    unsigned long long digest = 1469598103934665603ULL;
    const ea::vector<WorldFabricNodeProfile> profiles = GetAllNodeStats();
    for (const WorldFabricNodeProfile& profile : profiles)
    {
        digest = HashBytes(digest, &profile.node, sizeof(profile.node));
        digest = HashString(digest, profile.key);
        digest = HashString(digest, profile.channel);
        digest = HashBytes(digest, &profile.calls, sizeof(profile.calls));
        digest = HashBytes(digest, &profile.totalMilliseconds, sizeof(profile.totalMilliseconds));
        digest = HashBytes(digest, &profile.minimumMilliseconds, sizeof(profile.minimumMilliseconds));
        digest = HashBytes(digest, &profile.maximumMilliseconds, sizeof(profile.maximumMilliseconds));
    }
    return digest;
}

} // namespace Urho3D
