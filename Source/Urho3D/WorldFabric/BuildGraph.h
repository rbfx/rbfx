// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/WorldFabric/WorldFabric.h>

#include <EASTL/functional.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

enum class BuildTaskKind
{
    ImportAsset,
    CompileShader,
    CompileScript,
    CookVFX,
    BuildPackage,
    Custom
};

struct URHO3D_API BuildTaskResult
{
    bool success{};
    bool cacheHit{};
    unsigned long long digest{};
    ea::string output;
    ea::string error;
};

struct URHO3D_API BuildTask
{
    ea::string key;
    BuildTaskKind kind{BuildTaskKind::Custom};
    StringVariantMap metadata;
    ea::vector<ea::string> dependencies;
};

using BuildTaskExecutor = ea::function<bool(const BuildTask&, const ea::vector<BuildTaskResult>&,
    BuildTaskResult&, ea::string&)>;

/// Deterministic build scheduler shared by asset, shader, script, VFX and package pipelines.
class URHO3D_API BuildGraph
{
public:
    bool AddTask(const BuildTask& task, const BuildTaskExecutor& executor);
    bool RemoveTask(const ea::string& key);
    bool AddDependency(const ea::string& task, const ea::string& dependency);
    bool Validate(ea::string* error = nullptr) const;
    bool Execute(ea::vector<ea::string>* executed = nullptr, ea::string* error = nullptr);

    const BuildTask* FindTask(const ea::string& key) const;
    const BuildTaskResult* FindResult(const ea::string& key) const;
    ea::vector<ea::string> GetBuildOrder(ea::string* error = nullptr) const;
    unsigned long long ComputeDigest() const;
    const ea::string& GetLastError() const { return lastError_; }
    void Clear();

private:
    struct Entry
    {
        BuildTask task;
        BuildTaskExecutor executor;
        BuildTaskResult result;
        bool hasResult{};
    };

    bool Visit(const ea::string& key, ea::unordered_map<ea::string, unsigned char>& marks,
        ea::vector<ea::string>& order, ea::string* error) const;
    static unsigned long long DigestTask(const BuildTask& task,
        const ea::vector<BuildTaskResult>& dependencyResults);
    void SetError(ea::string* error, const ea::string& message) const;

    ea::unordered_map<ea::string, Entry> entries_;
    mutable ea::string lastError_;
};

} // namespace Urho3D
