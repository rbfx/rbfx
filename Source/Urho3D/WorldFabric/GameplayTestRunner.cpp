// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "GameplayTestRunner.h"

#include <algorithm>

namespace Urho3D
{

namespace
{

bool HasTag(const GameplayTestCase& test, const ea::string& tag)
{
    if (tag.empty())
        return true;
    return std::find(test.tags.begin(), test.tags.end(), tag) != test.tags.end();
}

} // namespace

bool GameplayTestRunner::Register(const GameplayTestCase& test)
{
    if (test.name.empty() || !test.callback || tests_.find(test.name) != tests_.end())
    {
        lastError_ = "Gameplay test name must be unique and callback must be valid.";
        return false;
    }
    tests_.emplace(test.name, test);
    return true;
}

bool GameplayTestRunner::Unregister(const ea::string& name)
{
    return tests_.erase(name) != 0;
}

bool GameplayTestRunner::Run(const ea::string& tag, ea::string* error)
{
    results_.clear();
    lastError_.clear();

    ea::vector<ea::string> names;
    names.reserve(tests_.size());
    for (const auto& entry : tests_)
    {
        if (HasTag(entry.second, tag))
            names.push_back(entry.first);
    }
    std::sort(names.begin(), names.end());

    bool allPassed = true;
    for (const ea::string& name : names)
    {
        const GameplayTestCase& test = tests_.at(name);
        GameplayTestResult result;
        result.name = name;
        ea::string testError;
        result.passed = test.callback(testError);
        result.assertions = result.passed ? 1u : 0u;
        result.error = testError;
        if (!result.passed)
        {
            allPassed = false;
            if (lastError_.empty())
                lastError_ = testError.empty() ? "Gameplay test failed: " + name : testError;
        }
        results_.push_back(result);
    }

    if (!allPassed && error)
        *error = lastError_;
    return allPassed;
}

unsigned GameplayTestRunner::GetPassedCount() const
{
    unsigned count = 0;
    for (const GameplayTestResult& result : results_)
        count += result.passed ? 1 : 0;
    return count;
}

unsigned GameplayTestRunner::GetFailedCount() const
{
    unsigned count = 0;
    for (const GameplayTestResult& result : results_)
        count += result.passed ? 0 : 1;
    return count;
}

} // namespace Urho3D
