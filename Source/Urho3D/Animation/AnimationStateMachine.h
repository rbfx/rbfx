// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Variant.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// A named animation state consumed by AnimationStateMachine.
struct URHO3D_API AnimationStateMachineState
{
    ea::string name;
    ea::string clip;
    float duration{1.0f};
    float playRate{1.0f};
    bool looping{true};
};

/// A conditional edge between two animation states.
struct URHO3D_API AnimationTransition
{
    ea::string fromState;
    ea::string toState;
    float blendDuration{0.2f};
    float minimumStateTime{0.0f};
    bool canInterrupt{true};
    ea::function<bool(const StringVariantMap&)> condition;
};

/// The active blend between two animation states.
struct URHO3D_API AnimationBlendResult
{
    ea::string state;
    float weight{1.0f};
};

/// Deterministic, data-driven animation state machine independent from a renderer.
class URHO3D_API AnimationStateMachine
{
public:
    bool AddState(const AnimationStateMachineState& state, ea::string* error = nullptr);
    bool RemoveState(const ea::string& name, ea::string* error = nullptr);
    bool AddTransition(const AnimationTransition& transition, ea::string* error = nullptr);
    bool RemoveTransition(const ea::string& fromState, const ea::string& toState);
    void Clear();

    bool SetInitialState(const ea::string& name);
    const ea::string& GetInitialState() const { return initialState_; }
    const AnimationStateMachineState* GetState(const ea::string& name) const;
    ea::vector<ea::string> GetStateNames() const;
    ea::vector<AnimationTransition> GetTransitions() const { return transitions_; }

    bool Start(const ea::string& state = {});
    bool Pause();
    bool Resume();
    bool Stop();
    bool Update(float deltaSeconds);

    bool SetParameter(const ea::string& name, const Variant& value);
    Variant GetParameter(const ea::string& name) const;
    bool HasParameter(const ea::string& name) const;

    const ea::string& GetCurrentState() const { return currentState_; }
    const ea::string& GetNextState() const { return nextState_; }
    float GetStateTime() const { return stateTime_; }
    float GetTransitionTime() const { return transitionTime_; }
    float GetTransitionAlpha() const;
    bool IsPlaying() const { return playing_; }
    bool IsPaused() const { return paused_; }
    bool IsTransitioning() const { return !nextState_.empty(); }
    ea::vector<AnimationBlendResult> GetBlendResults() const;

private:
    const AnimationTransition* FindTransition(const ea::string& fromState) const;
    bool BeginTransition(const AnimationTransition& transition);
    void ResetPlayback();

    ea::unordered_map<ea::string, AnimationStateMachineState> states_;
    ea::vector<AnimationTransition> transitions_;
    StringVariantMap parameters_;
    ea::string initialState_;
    ea::string currentState_;
    ea::string nextState_;
    float stateTime_{};
    float transitionTime_{};
    float transitionDuration_{};
    bool playing_{};
    bool paused_{};
};

} // namespace Urho3D
