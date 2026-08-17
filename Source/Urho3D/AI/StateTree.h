// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/AI/Blackboard.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

using StateTreeCondition = ea::function<bool(const Blackboard&)>;
using StateTreeCallback = ea::function<void(Blackboard&, float)>;

struct URHO3D_API StateTreeState
{
    ea::string name;
    ea::string parent;
    StateTreeCallback onEnter;
    StateTreeCallback onExit;
    StateTreeCallback onTick;
};

struct URHO3D_API StateTreeTransition
{
    ea::string from;
    ea::string to;
    StateTreeCondition condition;
};

/// Hierarchical gameplay state machine sharing Blackboard state with AI behaviors.
class URHO3D_API StateTree
{
public:
    bool AddState(const StateTreeState& state);
    bool RemoveState(const ea::string& name);
    bool AddTransition(const StateTreeTransition& transition);
    bool RemoveTransition(const ea::string& from, const ea::string& to);
    bool SetInitialState(const ea::string& name);
    bool Start(Blackboard& blackboard);
    bool Stop(Blackboard& blackboard);
    bool Tick(Blackboard& blackboard, float deltaSeconds);

    const StateTreeState* GetState(const ea::string& name) const;
    const ea::string& GetCurrentState() const { return currentState_; }
    bool IsRunning() const { return running_; }
    void Clear();

private:
    bool IsDescendant(const ea::string& state, const ea::string& ancestor) const;
    ea::vector<ea::string> BuildPath(const ea::string& state) const;
    const StateTreeTransition* FindTransition(const ea::string& state, const Blackboard& blackboard) const;
    bool EnterState(const ea::string& state, Blackboard& blackboard);
    bool ExitTo(const ea::string& state, Blackboard& blackboard);

    ea::vector<StateTreeState> states_;
    ea::vector<StateTreeTransition> transitions_;
    ea::string initialState_;
    ea::string currentState_;
    bool running_{};
};

} // namespace Urho3D
