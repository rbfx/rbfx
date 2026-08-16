// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Math/MathDefs.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// One completed hierarchical CPU scope captured in a frame.
struct URHO3D_API ProfilerScopeSample
{
    ea::string name;
    double startMilliseconds{};
    double durationMilliseconds{};
    unsigned depth{};
};

/// One captured frame, including CPU scopes and named counters.
struct URHO3D_API ProfilerFrame
{
    unsigned long long frameIndex{};
    double frameTimeMilliseconds{};
    ea::vector<ProfilerScopeSample> scopes;
    ea::unordered_map<ea::string, double> counters;
};

/// Aggregated statistics for a named profiler scope or metric.
struct URHO3D_API ProfilerScopeStats
{
    ea::string name;
    unsigned long long calls{};
    double totalMilliseconds{};
    double minimumMilliseconds{};
    double maximumMilliseconds{};

    double GetAverageMilliseconds() const
    {
        return calls ? totalMilliseconds / static_cast<double>(calls) : 0.0;
    }
};

/// Memory usage grouped by an engine-owned category.
struct URHO3D_API ProfilerMemoryStats
{
    ea::string category;
    long long currentBytes{};
    long long peakBytes{};
    unsigned long long allocations{};
    unsigned long long frees{};
};

/// Network counters grouped by connection or logical channel.
struct URHO3D_API ProfilerNetworkStats
{
    ea::string connection;
    unsigned long long bytesSent{};
    unsigned long long bytesReceived{};
    unsigned long long packetsLost{};
    double roundTripMilliseconds{};
};

/// Audio workload grouped by a mixer bus or logical voice category.
struct URHO3D_API ProfilerAudioStats
{
    ea::string bus;
    unsigned long long voices{};
    unsigned long long samples{};
    double cpuMilliseconds{};
};

/// Immutable-style report assembled from the captured profiler history.
struct URHO3D_API ProfilerReport
{
    unsigned long long frameCount{};
    double averageFrameMilliseconds{};
    double minimumFrameMilliseconds{};
    double maximumFrameMilliseconds{};
    ea::vector<ProfilerScopeStats> scopes;
    ea::vector<ProfilerMemoryStats> memory;
    ea::vector<ProfilerNetworkStats> network;
    ea::vector<ProfilerScopeStats> gpuPasses;
    ea::vector<ProfilerScopeStats> scriptFunctions;
    ea::vector<ProfilerAudioStats> audio;
};

/// Production profiler collecting bounded, deterministic telemetry for tools and tests.
class URHO3D_API ProductionProfiler
{
public:
    explicit ProductionProfiler(unsigned historyFrames = 120);

    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }
    void SetHistoryFrames(unsigned frames);
    unsigned GetHistoryFrames() const { return historyFrames_; }

    void BeginFrame(unsigned long long frameIndex);
    void EndFrame();
    bool IsFrameActive() const { return frameActive_; }
    const ProfilerFrame& GetCurrentFrame() const { return currentFrame_; }
    const ea::vector<ProfilerFrame>& GetHistory() const { return history_; }

    void BeginScope(const ea::string& name);
    void EndScope(const ea::string& name = {});
    void RecordScope(const ea::string& name, double durationMilliseconds, unsigned depth = 0);
    void RecordCounter(const ea::string& name, double value);

    void TrackAllocation(const ea::string& category, long long bytes);
    void TrackFree(const ea::string& category, long long bytes);
    void RecordGpuPass(const ea::string& name, double durationMilliseconds);
    void RecordScriptFunction(const ea::string& name, double durationMilliseconds);
    void RecordAudio(const ea::string& bus, unsigned long long voices, unsigned long long samples,
        double cpuMilliseconds);
    void RecordNetwork(const ea::string& connection, unsigned long long bytesSent,
        unsigned long long bytesReceived, unsigned long long packetsLost, double roundTripMilliseconds);

    double GetLastFrameTimeMilliseconds() const;
    ProfilerReport BuildReport() const;
    void Reset();

private:
    struct ActiveScope
    {
        ea::string name;
        double startMilliseconds{};
        unsigned depth{};
    };

    struct CounterAccumulator
    {
        unsigned long long calls{};
        double totalMilliseconds{};
        double minimumMilliseconds{};
        double maximumMilliseconds{};
    };

    static double GetTimeMilliseconds();
    void AddScopeToAccumulator(ea::unordered_map<ea::string, CounterAccumulator>& target,
        const ea::string& name, double durationMilliseconds) const;
    void TrimHistory();

    bool enabled_{true};
    bool frameActive_{};
    double frameStartMilliseconds_{};
    unsigned historyFrames_{};
    ProfilerFrame currentFrame_;
    ea::vector<ProfilerFrame> history_;
    ea::vector<ActiveScope> activeScopes_;
    ea::unordered_map<ea::string, CounterAccumulator> gpuPasses_;
    ea::unordered_map<ea::string, CounterAccumulator> scriptFunctions_;
    ea::unordered_map<ea::string, ProfilerAudioStats> audio_;
    ea::unordered_map<ea::string, ProfilerMemoryStats> memory_;
    ea::unordered_map<ea::string, ProfilerNetworkStats> network_;
};

/// RAII helper for a named production CPU scope.
class URHO3D_API ProfilerScope
{
public:
    ProfilerScope(ProductionProfiler* profiler, const ea::string& name);
    ~ProfilerScope();

    ProfilerScope(const ProfilerScope&) = delete;
    ProfilerScope& operator=(const ProfilerScope&) = delete;

private:
    ProductionProfiler* profiler_{};
    ea::string name_;
};

} // namespace Urho3D
