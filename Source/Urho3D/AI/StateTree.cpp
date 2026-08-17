// SPDX-License-Identifier: MIT

#include "StateTree.h"

#include <algorithm>

namespace Urho3D
{

bool StateTree::AddState(const StateTreeState& state)
{
    if (state.name.empty() || GetState(state.name) || (!state.parent.empty() && !GetState(state.parent)))
        return false;
    if (!state.parent.empty() && state.parent == state.name)
        return false;
    states_.push_back(state);
    if (initialState_.empty())
        initialState_ = state.name;
    return true;
}

bool StateTree::RemoveState(const ea::string& name)
{
    if (running_ && (currentState_ == name || IsDescendant(currentState_, name)))
        return false;
    const auto iter = std::find_if(states_.begin(), states_.end(), [&](const StateTreeState& state) { return state.name == name; });
    if (iter == states_.end())
        return false;
    for (const StateTreeState& state : states_)
    {
        if (state.parent == name)
            return false;
    }
    states_.erase(iter);
    transitions_.erase(std::remove_if(transitions_.begin(), transitions_.end(), [&](const StateTreeTransition& transition)
    {
        return transition.from == name || transition.to == name;
    }), transitions_.end());
    if (initialState_ == name)
        initialState_.clear();
    return true;
}

bool StateTree::AddTransition(const StateTreeTransition& transition)
{
    if (transition.from.empty() || transition.to.empty() || !GetState(transition.from) || !GetState(transition.to))
        return false;
    if (transition.from == transition.to)
        return false;
    for (const StateTreeTransition& existing : transitions_)
    {
        if (existing.from == transition.from && existing.to == transition.to)
            return false;
    }
    transitions_.push_back(transition);
    return true;
}

bool StateTree::RemoveTransition(const ea::string& from, const ea::string& to)
{
    const auto iter = std::find_if(transitions_.begin(), transitions_.end(), [&](const StateTreeTransition& transition)
    {
        return transition.from == from && transition.to == to;
    });
    if (iter == transitions_.end())
        return false;
    transitions_.erase(iter);
    return true;
}

bool StateTree::SetInitialState(const ea::string& name)
{
    if (!GetState(name))
        return false;
    initialState_ = name;
    return true;
}

bool StateTree::Start(Blackboard& blackboard)
{
    if (running_ || initialState_.empty())
        return false;
    running_ = true;
    currentState_.clear();
    if (!EnterState(initialState_, blackboard))
    {
        running_ = false;
        return false;
    }
    return true;
}

bool StateTree::Stop(Blackboard& blackboard)
{
    if (!running_)
        return false;
    ExitTo({}, blackboard);
    currentState_.clear();
    running_ = false;
    return true;
}

bool StateTree::Tick(Blackboard& blackboard, float deltaSeconds)
{
    if (!running_ || currentState_.empty() || deltaSeconds < 0.0f)
        return false;
    const StateTreeState* state = GetState(currentState_);
    if (!state)
        return false;
    if (state->onTick)
        state->onTick(blackboard, deltaSeconds);
    const StateTreeTransition* transition = FindTransition(currentState_, blackboard);
    if (!transition)
        return true;
    const ea::string target = transition->to;
    if (!ExitTo(target, blackboard))
        return false;
    return EnterState(target, blackboard);
}

const StateTreeState* StateTree::GetState(const ea::string& name) const
{
    const auto iter = std::find_if(states_.begin(), states_.end(), [&](const StateTreeState& state) { return state.name == name; });
    return iter != states_.end() ? &*iter : nullptr;
}

void StateTree::Clear()
{
    states_.clear();
    transitions_.clear();
    initialState_.clear();
    currentState_.clear();
    running_ = false;
}

bool StateTree::IsDescendant(const ea::string& state, const ea::string& ancestor) const
{
    ea::string current = state;
    while (!current.empty())
    {
        if (current == ancestor)
            return true;
        const StateTreeState* item = GetState(current);
        if (!item)
            break;
        current = item->parent;
    }
    return false;
}

ea::vector<ea::string> StateTree::BuildPath(const ea::string& state) const
{
    ea::vector<ea::string> path;
    ea::string current = state;
    while (!current.empty())
    {
        path.push_back(current);
        const StateTreeState* item = GetState(current);
        if (!item)
            break;
        current = item->parent;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

const StateTreeTransition* StateTree::FindTransition(const ea::string& state, const Blackboard& blackboard) const
{
    ea::string current = state;
    while (!current.empty())
    {
        for (const StateTreeTransition& transition : transitions_)
        {
            if (transition.from == current && (!transition.condition || transition.condition(blackboard)))
                return &transition;
        }
        const StateTreeState* item = GetState(current);
        if (!item)
            break;
        current = item->parent;
    }
    return nullptr;
}

bool StateTree::EnterState(const ea::string& state, Blackboard& blackboard)
{
    const ea::vector<ea::string> path = BuildPath(state);
    if (path.empty() || path.back() != state)
        return false;
    const ea::vector<ea::string> currentPath = BuildPath(currentState_);
    unsigned common = 0;
    while (common < currentPath.size() && common < path.size() && currentPath[common] == path[common])
        ++common;
    for (unsigned i = common; i < path.size(); ++i)
    {
        const StateTreeState* item = GetState(path[i]);
        if (item && item->onEnter)
            item->onEnter(blackboard, 0.0f);
    }
    currentState_ = state;
    return true;
}

bool StateTree::ExitTo(const ea::string& state, Blackboard& blackboard)
{
    const ea::vector<ea::string> currentPath = BuildPath(currentState_);
    const ea::vector<ea::string> targetPath = BuildPath(state);
    unsigned common = 0;
    while (common < currentPath.size() && common < targetPath.size() && currentPath[common] == targetPath[common])
        ++common;
    for (unsigned i = currentPath.size(); i > common; --i)
    {
        const StateTreeState* item = GetState(currentPath[i - 1]);
        if (item && item->onExit)
            item->onExit(blackboard, 0.0f);
    }
    currentState_ = common > 0 ? currentPath[common - 1] : ea::string();
    return true;
}

} // namespace Urho3D
