// Copyright (c) 2026 the rbfx-blueprint project.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/WorldFabric/WorldFabric.h>

#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

class ProductionProfiler;

/// Aggregated profiling data correlated to one semantic World Fabric node.
struct URHO3D_API WorldFabricNodeProfile
{
    WorldFabricId node{InvalidWorldFabricId};
    ea::string key;
    ea::string channel;
    unsigned long long calls{};
    double totalMilliseconds{};
    double minimumMilliseconds{};
    double maximumMilliseconds{};

    double GetAverageMilliseconds() const
    {
        return calls ? totalMilliseconds / static_cast<double>(calls) : 0.0;
    }
};

/// Correlates CPU, GPU, script and custom annotations with World Fabric semantic IDs.
class URHO3D_API WorldFabricProfiler
{
public:
    explicit WorldFabricProfiler(WorldFabricGraph* graph = nullptr, ProductionProfiler* profiler = nullptr)
        : graph_(graph), profiler_(profiler)
    {
    }

    void SetGraph(WorldFabricGraph* graph) { graph_ = graph; }
    WorldFabricGraph* GetGraph() const { return graph_; }
    void SetProfiler(ProductionProfiler* profiler) { profiler_ = profiler; }
    ProductionProfiler* GetProfiler() const { return profiler_; }

    /// Record an annotation and mirror it into the active ProductionProfiler frame when available.
    bool AnnotateNode(WorldFabricId node, double durationMilliseconds, const ea::string& channel = "CPU",
        ea::string* error = nullptr);
    /// Return accumulated statistics for one semantic node and channel.
    bool GetNodeStats(WorldFabricId node, const ea::string& channel, WorldFabricNodeProfile& stats) const;
    /// Return all node statistics in deterministic node/channel order.
    ea::vector<WorldFabricNodeProfile> GetAllNodeStats() const;
    /// Reset correlated statistics without changing the bound graph or profiler.
    void Reset();
    /// Compute a deterministic digest of all correlated statistics.
    unsigned long long ComputeDigest() const;

private:
    struct Key
    {
        WorldFabricId node{InvalidWorldFabricId};
        ea::string channel;

        bool operator==(const Key& rhs) const { return node == rhs.node && channel == rhs.channel; }
    };

    struct KeyHash
    {
        size_t operator()(const Key& key) const;
    };

    static ea::string NormalizeChannel(const ea::string& channel);
    void SetError(ea::string* error, const ea::string& message) const;

    WorldFabricGraph* graph_{};
    ProductionProfiler* profiler_{};
    ea::unordered_map<Key, WorldFabricNodeProfile, KeyHash> profiles_;
};

} // namespace Urho3D
