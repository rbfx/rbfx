// SPDX-License-Identifier: MIT

#include "AnimationStateMachine.h"

#include <algorithm>
#include <cmath>

namespace Urho3D
{

bool AnimationStateMachine::AddState(const AnimationStateMachineState& state, ea::string* error)
{
    if (state.name.empty())
    {
        if (error)
            *error = "Animation state name cannot be empty.";
        return false;
    }
    if (state.duration <= 0.0f)
    {
        if (error)
            *error = "Animation state duration must be positive.";
        return false;
    }
    if (states_.find(state.name) != states_.end())
    {
        if (error)
            *error = "Animation state already exists.";
        return false;
    }

    states_[state.name] = state;
    if (initialState_.empty())
        initialState_ = state.name;
    return true;
}

bool AnimationStateMachine::RemoveState(const ea::string& name, ea::string* error)
{
    if (states_.erase(name) == 0)
    {
        if (error)
            *error = "Animation state does not exist.";
        return false;
    }

    transitions_.erase(std::remove_if(transitions_.begin(), transitions_.end(),
        [&](const AnimationTransition& transition)
        {
            return transition.fromState == name || transition.toState == name;
        }), transitions_.end());

    if (initialState_ == name)
        initialState_.clear();
    if (currentState_ == name)
        ResetPlayback();
    if (nextState_ == name)
        nextState_.clear();
    return true;
}

bool AnimationStateMachine::AddTransition(const AnimationTransition& transition, ea::string* error)
{
    if (states_.find(transition.fromState) == states_.end() || states_.find(transition.toState) == states_.end())
    {
        if (error)
            *error = "Animation transition references an unknown state.";
        return false;
    }
    if (transition.fromState == transition.toState)
    {
        if (error)
            *error = "Animation transition must connect two different states.";
        return false;
    }
    if (transition.blendDuration < 0.0f || transition.minimumStateTime < 0.0f)
    {
        if (error)
            *error = "Animation transition times cannot be negative.";
        return false;
    }
    for (const AnimationTransition& existing : transitions_)
    {
        if (existing.fromState == transition.fromState && existing.toState == transition.toState)
        {
            if (error)
                *error = "Animation transition already exists.";
            return false;
        }
    }

    transitions_.push_back(transition);
    return true;
}

bool AnimationStateMachine::RemoveTransition(const ea::string& fromState, const ea::string& toState)
{
    const auto oldSize = transitions_.size();
    transitions_.erase(std::remove_if(transitions_.begin(), transitions_.end(),
        [&](const AnimationTransition& transition)
        {
            return transition.fromState == fromState && transition.toState == toState;
        }), transitions_.end());
    return transitions_.size() != oldSize;
}

void AnimationStateMachine::Clear()
{
    states_.clear();
    transitions_.clear();
    parameters_.clear();
    initialState_.clear();
    ResetPlayback();
}

bool AnimationStateMachine::SetInitialState(const ea::string& name)
{
    if (states_.find(name) == states_.end())
        return false;
    initialState_ = name;
    return true;
}

const AnimationStateMachineState* AnimationStateMachine::GetState(const ea::string& name) const
{
    const auto iter = states_.find(name);
    return iter != states_.end() ? &iter->second : nullptr;
}

ea::vector<ea::string> AnimationStateMachine::GetStateNames() const
{
    ea::vector<ea::string> result;
    result.reserve(states_.size());
    for (const auto& entry : states_)
        result.push_back(entry.first);
    std::sort(result.begin(), result.end());
    return result;
}

bool AnimationStateMachine::Start(const ea::string& state)
{
    ea::string requested = state;
    if (requested.empty())
        requested = initialState_;
    if (requested.empty())
    {
        const ea::vector<ea::string> names = GetStateNames();
        if (!names.empty())
            requested = names.front();
    }
    if (states_.find(requested) == states_.end())
        return false;

    currentState_ = requested;
    nextState_.clear();
    stateTime_ = 0.0f;
    transitionTime_ = 0.0f;
    transitionDuration_ = 0.0f;
    playing_ = true;
    paused_ = false;
    return true;
}

bool AnimationStateMachine::Pause()
{
    if (!playing_ || paused_)
        return false;
    paused_ = true;
    return true;
}

bool AnimationStateMachine::Resume()
{
    if (!playing_ || !paused_)
        return false;
    paused_ = false;
    return true;
}

bool AnimationStateMachine::Stop()
{
    if (!playing_ && currentState_.empty())
        return false;
    ResetPlayback();
    return true;
}

bool AnimationStateMachine::Update(float deltaSeconds)
{
    if (!playing_ || paused_ || deltaSeconds <= 0.0f || currentState_.empty())
        return false;

    const AnimationStateMachineState* current = GetState(currentState_);
    if (!current)
    {
        ResetPlayback();
        return false;
    }

    stateTime_ += deltaSeconds * Max(current->playRate, 0.0f);
    if (current->looping)
    {
        stateTime_ = std::fmod(stateTime_, current->duration);
        if (stateTime_ < 0.0f)
            stateTime_ += current->duration;
    }
    else
        stateTime_ = Min(stateTime_, current->duration);

    if (!nextState_.empty())
    {
        transitionTime_ += deltaSeconds;
        if (transitionDuration_ <= 0.0f || transitionTime_ >= transitionDuration_)
        {
            currentState_ = nextState_;
            nextState_.clear();
            stateTime_ = 0.0f;
            transitionTime_ = 0.0f;
            transitionDuration_ = 0.0f;
        }
        return true;
    }

    const AnimationTransition* transition = FindTransition(currentState_);
    if (transition && stateTime_ >= transition->minimumStateTime)
        BeginTransition(*transition);
    return true;
}

bool AnimationStateMachine::SetParameter(const ea::string& name, const Variant& value)
{
    if (name.empty())
        return false;
    parameters_[name] = value;
    return true;
}

Variant AnimationStateMachine::GetParameter(const ea::string& name) const
{
    const auto iter = parameters_.find(name);
    return iter != parameters_.end() ? iter->second : Variant();
}

bool AnimationStateMachine::HasParameter(const ea::string& name) const
{
    return parameters_.find(name) != parameters_.end();
}

float AnimationStateMachine::GetTransitionAlpha() const
{
    if (nextState_.empty() || transitionDuration_ <= 0.0f)
        return 0.0f;
    return Clamp(transitionTime_ / transitionDuration_, 0.0f, 1.0f);
}

ea::vector<AnimationBlendResult> AnimationStateMachine::GetBlendResults() const
{
    ea::vector<AnimationBlendResult> result;
    if (currentState_.empty())
        return result;

    if (nextState_.empty())
    {
        result.push_back({currentState_, 1.0f});
        return result;
    }

    const float alpha = GetTransitionAlpha();
    result.push_back({currentState_, 1.0f - alpha});
    result.push_back({nextState_, alpha});
    return result;
}

const AnimationTransition* AnimationStateMachine::FindTransition(const ea::string& fromState) const
{
    for (const AnimationTransition& transition : transitions_)
    {
        if (transition.fromState != fromState)
            continue;
        if (!transition.condition || transition.condition(parameters_))
            return &transition;
    }
    return nullptr;
}

bool AnimationStateMachine::BeginTransition(const AnimationTransition& transition)
{
    if (!transition.canInterrupt && !nextState_.empty())
        return false;
    nextState_ = transition.toState;
    transitionTime_ = 0.0f;
    transitionDuration_ = transition.blendDuration;
    if (transitionDuration_ <= 0.0f)
    {
        currentState_ = nextState_;
        nextState_.clear();
        stateTime_ = 0.0f;
    }
    return true;
}

void AnimationStateMachine::ResetPlayback()
{
    currentState_.clear();
    nextState_.clear();
    stateTime_ = 0.0f;
    transitionTime_ = 0.0f;
    transitionDuration_ = 0.0f;
    playing_ = false;
    paused_ = false;
}

} // namespace Urho3D
