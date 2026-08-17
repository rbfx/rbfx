// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "BuildGraph.h"

#include <algorithm>

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

bool BuildGraph::AddTask(const BuildTask& task, const BuildTaskExecutor& executor)
{
    if (task.key.empty() || !executor || entries_.find(task.key) != entries_.end())
    {
        lastError_ = "Build task key must be unique and executor must be valid.";
        return false;
    }

    Entry entry;
    entry.task = task;
    entry.executor = executor;
    entries_.emplace(task.key, entry);
    return true;
}

bool BuildGraph::RemoveTask(const ea::string& key)
{
    if (entries_.find(key) == entries_.end())
        return false;
    entries_.erase(key);
    for (auto& entry : entries_)
    {
        auto& dependencies = entry.second.task.dependencies;
        dependencies.erase(std::remove(dependencies.begin(), dependencies.end(), key), dependencies.end());
    }
    return true;
}

bool BuildGraph::AddDependency(const ea::string& task, const ea::string& dependency)
{
    auto it = entries_.find(task);
    if (it == entries_.end() || entries_.find(dependency) == entries_.end() || task == dependency)
    {
        lastError_ = "Build dependency references a missing task or a self dependency.";
        return false;
    }
    auto& dependencies = it->second.task.dependencies;
    if (std::find(dependencies.begin(), dependencies.end(), dependency) == dependencies.end())
        dependencies.push_back(dependency);
    it->second.hasResult = false;
    return Validate();
}

const BuildTask* BuildGraph::FindTask(const ea::string& key) const
{
    const auto it = entries_.find(key);
    return it == entries_.end() ? nullptr : &it->second.task;
}

const BuildTaskResult* BuildGraph::FindResult(const ea::string& key) const
{
    const auto it = entries_.find(key);
    return it == entries_.end() || !it->second.hasResult ? nullptr : &it->second.result;
}

ea::vector<ea::string> BuildGraph::GetBuildOrder(ea::string* error) const
{
    ea::vector<ea::string> keys;
    keys.reserve(entries_.size());
    for (const auto& entry : entries_)
        keys.push_back(entry.first);
    std::sort(keys.begin(), keys.end());

    ea::unordered_map<ea::string, unsigned char> marks;
    ea::vector<ea::string> order;
    for (const ea::string& key : keys)
    {
        if (!Visit(key, marks, order, error))
            return {};
    }
    return order;
}

bool BuildGraph::Visit(const ea::string& key, ea::unordered_map<ea::string, unsigned char>& marks,
    ea::vector<ea::string>& order, ea::string* error) const
{
    const auto entry = entries_.find(key);
    if (entry == entries_.end())
    {
        SetError(error, "Build graph references an unknown task: " + key);
        return false;
    }
    const unsigned char mark = marks[key];
    if (mark == 2)
        return true;
    if (mark == 1)
    {
        SetError(error, "Build graph contains a dependency cycle at: " + key);
        return false;
    }

    marks[key] = 1;
    ea::vector<ea::string> dependencies = entry->second.task.dependencies;
    std::sort(dependencies.begin(), dependencies.end());
    for (const ea::string& dependency : dependencies)
    {
        if (!Visit(dependency, marks, order, error))
            return false;
    }
    marks[key] = 2;
    order.push_back(key);
    return true;
}

bool BuildGraph::Validate(ea::string* error) const
{
    const ea::vector<ea::string> order = GetBuildOrder(error);
    if (entries_.empty())
        return true;
    return !order.empty();
}

unsigned long long BuildGraph::DigestTask(const BuildTask& task,
    const ea::vector<BuildTaskResult>& dependencyResults)
{
    unsigned long long hash = 1469598103934665603ull;
    HashString(hash, task.key);
    HashByte(hash, static_cast<unsigned char>(task.kind));

    ea::vector<ea::string> keys;
    keys.reserve(task.metadata.size());
    for (const auto& value : task.metadata)
        keys.push_back(value.first);
    std::sort(keys.begin(), keys.end());
    for (const ea::string& key : keys)
    {
        HashString(hash, key);
        const auto it = task.metadata.find(key);
        if (it != task.metadata.end())
            HashString(hash, it->second.ToString());
    }
    for (const BuildTaskResult& dependency : dependencyResults)
    {
        for (unsigned shift = 0; shift < 8; ++shift)
            HashByte(hash, static_cast<unsigned char>((dependency.digest >> (shift * 8)) & 0xff));
    }
    return hash;
}

bool BuildGraph::Execute(ea::vector<ea::string>* executed, ea::string* error)
{
    if (executed)
        executed->clear();
    const ea::vector<ea::string> order = GetBuildOrder(error);
    if (!entries_.empty() && order.empty())
        return false;

    for (const ea::string& key : order)
    {
        Entry& entry = entries_[key];
        ea::vector<BuildTaskResult> dependencyResults;
        dependencyResults.reserve(entry.task.dependencies.size());
        for (const ea::string& dependency : entry.task.dependencies)
        {
            const BuildTaskResult* result = FindResult(dependency);
            if (!result || !result->success)
            {
                SetError(error, "Build dependency has no successful result: " + dependency);
                return false;
            }
            dependencyResults.push_back(*result);
        }

        const unsigned long long digest = DigestTask(entry.task, dependencyResults);
        if (entry.hasResult && entry.result.success && entry.result.digest == digest)
        {
            entry.result.cacheHit = true;
            if (executed)
                executed->push_back(key);
            continue;
        }

        BuildTaskResult result;
        result.digest = digest;
        ea::string taskError;
        if (!entry.executor(entry.task, dependencyResults, result, taskError))
        {
            result.success = false;
            result.error = taskError.empty() ? "Build task executor failed." : taskError;
            entry.result = result;
            entry.hasResult = true;
            SetError(error, result.error);
            return false;
        }
        result.success = true;
        result.cacheHit = false;
        result.digest = digest;
        entry.result = result;
        entry.hasResult = true;
        if (executed)
            executed->push_back(key);
    }
    return true;
}

unsigned long long BuildGraph::ComputeDigest() const
{
    const ea::vector<ea::string> order = GetBuildOrder();
    unsigned long long hash = 1469598103934665603ull;
    for (const ea::string& key : order)
    {
        const Entry& entry = entries_.at(key);
        HashString(hash, key);
        HashByte(hash, static_cast<unsigned char>(entry.task.kind));
        for (const auto& metadata : entry.task.metadata)
        {
            HashString(hash, metadata.first);
            HashString(hash, metadata.second.ToString());
        }
    }
    return hash;
}

void BuildGraph::SetError(ea::string* error, const ea::string& message) const
{
    lastError_ = message;
    if (error)
        *error = message;
}

void BuildGraph::Clear()
{
    entries_.clear();
    lastError_.clear();
}

} // namespace Urho3D
