// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "ProductionProfiler.h"

#include <EASTL/algorithm.h>

#include <algorithm>
#include <chrono>

namespace Urho3D
{

namespace
{

template <class MapType>
ea::vector<ProfilerScopeStats> BuildScopeStats(const MapType& source)
{
    ea::vector<ProfilerScopeStats> result;
    result.reserve(source.size());
    for (const auto& entry : source)
    {
        ProfilerScopeStats stats;
        stats.name = entry.first;
        stats.calls = entry.second.calls;
        stats.totalMilliseconds = entry.second.totalMilliseconds;
        stats.minimumMilliseconds = entry.second.minimumMilliseconds;
        stats.maximumMilliseconds = entry.second.maximumMilliseconds;
        result.push_back(ea::move(stats));
    }
    std::sort(result.begin(), result.end(), [](const ProfilerScopeStats& lhs, const ProfilerScopeStats& rhs)
    {
        return lhs.name < rhs.name;
    });
    return result;
}

}

ProductionProfiler::ProductionProfiler(unsigned historyFrames)
    : historyFrames_(historyFrames)
{
    currentFrame_.scopes.reserve(64);
}

void ProductionProfiler::SetHistoryFrames(unsigned frames)
{
    historyFrames_ = frames;
    TrimHistory();
}

void ProductionProfiler::BeginFrame(unsigned long long frameIndex)
{
    if (!enabled_)
        return;
    if (frameActive_)
        EndFrame();
    currentFrame_ = {};
    currentFrame_.frameIndex = frameIndex;
    currentFrame_.scopes.reserve(64);
    frameStartMilliseconds_ = GetTimeMilliseconds();
    frameActive_ = true;
    activeScopes_.clear();
}

void ProductionProfiler::EndFrame()
{
    if (!enabled_ || !frameActive_)
        return;
    while (!activeScopes_.empty())
        EndScope();
    currentFrame_.frameTimeMilliseconds = Max(0.0, GetTimeMilliseconds() - frameStartMilliseconds_);
    history_.push_back(currentFrame_);
    TrimHistory();
    frameActive_ = false;
}

void ProductionProfiler::BeginScope(const ea::string& name)
{
    if (!enabled_ || !frameActive_ || name.empty())
        return;
    ActiveScope scope;
    scope.name = name;
    scope.startMilliseconds = GetTimeMilliseconds();
    scope.depth = static_cast<unsigned>(activeScopes_.size());
    activeScopes_.push_back(ea::move(scope));
}

void ProductionProfiler::EndScope(const ea::string& name)
{
    if (!enabled_ || activeScopes_.empty())
        return;
    const ActiveScope scope = activeScopes_.back();
    activeScopes_.pop_back();
    if (!name.empty() && scope.name != name)
        return;
    RecordScope(scope.name, Max(0.0, GetTimeMilliseconds() - scope.startMilliseconds), scope.depth);
}

void ProductionProfiler::RecordScope(const ea::string& name, double durationMilliseconds, unsigned depth)
{
    if (!enabled_ || !frameActive_ || name.empty())
        return;
    ProfilerScopeSample sample;
    sample.name = name;
    sample.durationMilliseconds = Max(0.0, durationMilliseconds);
    sample.startMilliseconds = currentFrame_.frameTimeMilliseconds;
    sample.depth = depth;
    currentFrame_.scopes.push_back(ea::move(sample));
}

void ProductionProfiler::RecordCounter(const ea::string& name, double value)
{
    if (!enabled_ || !frameActive_ || name.empty())
        return;
    currentFrame_.counters[name] = value;
}

void ProductionProfiler::TrackAllocation(const ea::string& category, long long bytes)
{
    if (!enabled_ || category.empty())
        return;
    auto& stats = memory_[category];
    stats.category = category;
    stats.currentBytes += Max(0LL, bytes);
    stats.peakBytes = Max(stats.peakBytes, stats.currentBytes);
    ++stats.allocations;
}

void ProductionProfiler::TrackFree(const ea::string& category, long long bytes)
{
    if (!enabled_ || category.empty())
        return;
    auto& stats = memory_[category];
    stats.category = category;
    stats.currentBytes = Max(0LL, stats.currentBytes - Max(0LL, bytes));
    ++stats.frees;
}

void ProductionProfiler::RecordGpuPass(const ea::string& name, double durationMilliseconds)
{
    if (!enabled_ || name.empty())
        return;
    AddScopeToAccumulator(gpuPasses_, name, durationMilliseconds);
}

void ProductionProfiler::RecordScriptFunction(const ea::string& name, double durationMilliseconds)
{
    if (!enabled_ || name.empty())
        return;
    AddScopeToAccumulator(scriptFunctions_, name, durationMilliseconds);
}

void ProductionProfiler::RecordAudio(const ea::string& bus, unsigned long long voices,
    unsigned long long samples, double cpuMilliseconds)
{
    if (!enabled_ || bus.empty())
        return;
    auto& stats = audio_[bus];
    stats.bus = bus;
    stats.voices += voices;
    stats.samples += samples;
    stats.cpuMilliseconds += Max(0.0, cpuMilliseconds);
}

void ProductionProfiler::RecordNetwork(const ea::string& connection, unsigned long long bytesSent,
    unsigned long long bytesReceived, unsigned long long packetsLost, double roundTripMilliseconds)
{
    if (!enabled_ || connection.empty())
        return;
    auto& stats = network_[connection];
    stats.connection = connection;
    stats.bytesSent += bytesSent;
    stats.bytesReceived += bytesReceived;
    stats.packetsLost += packetsLost;
    stats.roundTripMilliseconds = roundTripMilliseconds;
}

double ProductionProfiler::GetLastFrameTimeMilliseconds() const
{
    if (frameActive_)
        return currentFrame_.frameTimeMilliseconds;
    return history_.empty() ? 0.0 : history_.back().frameTimeMilliseconds;
}

ProfilerReport ProductionProfiler::BuildReport() const
{
    ProfilerReport report;
    report.frameCount = history_.size();
    if (!history_.empty())
    {
        report.minimumFrameMilliseconds = history_.front().frameTimeMilliseconds;
        for (const ProfilerFrame& frame : history_)
        {
            report.averageFrameMilliseconds += frame.frameTimeMilliseconds;
            report.minimumFrameMilliseconds = Min(report.minimumFrameMilliseconds, frame.frameTimeMilliseconds);
            report.maximumFrameMilliseconds = Max(report.maximumFrameMilliseconds, frame.frameTimeMilliseconds);
        }
        report.averageFrameMilliseconds /= static_cast<double>(history_.size());
    }

    ea::unordered_map<ea::string, CounterAccumulator> cpu;
    for (const ProfilerFrame& frame : history_)
    {
        for (const ProfilerScopeSample& sample : frame.scopes)
            AddScopeToAccumulator(cpu, sample.name, sample.durationMilliseconds);
    }
    report.scopes = BuildScopeStats(cpu);
    report.gpuPasses = BuildScopeStats(gpuPasses_);
    report.scriptFunctions = BuildScopeStats(scriptFunctions_);

    report.memory.reserve(memory_.size());
    for (const auto& entry : memory_)
        report.memory.push_back(entry.second);
    std::sort(report.memory.begin(), report.memory.end(), [](const ProfilerMemoryStats& lhs, const ProfilerMemoryStats& rhs)
    {
        return lhs.category < rhs.category;
    });

    report.network.reserve(network_.size());
    for (const auto& entry : network_)
        report.network.push_back(entry.second);
    std::sort(report.network.begin(), report.network.end(), [](const ProfilerNetworkStats& lhs, const ProfilerNetworkStats& rhs)
    {
        return lhs.connection < rhs.connection;
    });

    report.audio.reserve(audio_.size());
    for (const auto& entry : audio_)
        report.audio.push_back(entry.second);
    std::sort(report.audio.begin(), report.audio.end(), [](const ProfilerAudioStats& lhs, const ProfilerAudioStats& rhs)
    {
        return lhs.bus < rhs.bus;
    });
    return report;
}

void ProductionProfiler::Reset()
{
    frameActive_ = false;
    currentFrame_ = {};
    history_.clear();
    activeScopes_.clear();
    gpuPasses_.clear();
    scriptFunctions_.clear();
    audio_.clear();
    memory_.clear();
    network_.clear();
    frameStartMilliseconds_ = 0.0;
}

double ProductionProfiler::GetTimeMilliseconds()
{
    using Clock = std::chrono::steady_clock;
    static const auto epoch = Clock::now();
    return std::chrono::duration<double, std::milli>(Clock::now() - epoch).count();
}

void ProductionProfiler::AddScopeToAccumulator(ea::unordered_map<ea::string, CounterAccumulator>& target,
    const ea::string& name, double durationMilliseconds) const
{
    auto& stats = target[name];
    const double duration = Max(0.0, durationMilliseconds);
    ++stats.calls;
    stats.totalMilliseconds += duration;
    if (stats.calls == 1)
        stats.minimumMilliseconds = duration;
    else
        stats.minimumMilliseconds = Min(stats.minimumMilliseconds, duration);
    stats.maximumMilliseconds = Max(stats.maximumMilliseconds, duration);
}

void ProductionProfiler::TrimHistory()
{
    while (history_.size() > historyFrames_)
        history_.erase(history_.begin());
}

ProfilerScope::ProfilerScope(ProductionProfiler* profiler, const ea::string& name)
    : profiler_(profiler), name_(name)
{
    if (profiler_)
        profiler_->BeginScope(name_);
}

ProfilerScope::~ProfilerScope()
{
    if (profiler_)
        profiler_->EndScope(name_);
}

} // namespace Urho3D
