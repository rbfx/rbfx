// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <Urho3D/Urho3D.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

using GameplayTestCallback = ea::function<bool(ea::string&)>;

struct URHO3D_API GameplayTestCase
{
    ea::string name;
    ea::vector<ea::string> tags;
    GameplayTestCallback callback;
};

struct URHO3D_API GameplayTestResult
{
    ea::string name;
    bool passed{};
    unsigned assertions{};
    ea::string error;
};

/// Deterministic in-engine gameplay test runner for CI, editor validation and package gates.
class URHO3D_API GameplayTestRunner
{
public:
    bool Register(const GameplayTestCase& test);
    bool Unregister(const ea::string& name);
    bool Run(const ea::string& tag = ea::string(), ea::string* error = nullptr);
    void ClearResults() { results_.clear(); }

    const ea::vector<GameplayTestResult>& GetResults() const { return results_; }
    unsigned GetPassedCount() const;
    unsigned GetFailedCount() const;
    unsigned GetTestCount() const { return tests_.size(); }
    const ea::string& GetLastError() const { return lastError_; }

private:
    ea::unordered_map<ea::string, GameplayTestCase> tests_;
    ea::vector<GameplayTestResult> results_;
    mutable ea::string lastError_;
};

} // namespace Urho3D
