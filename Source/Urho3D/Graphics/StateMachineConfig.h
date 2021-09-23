//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Resource/Resource.h"
#include "../Scene/Component.h"



namespace Urho3D
{

/// State machine transition condition.
struct StateMachineConfigTransitionCondition {
    /// Variable name
    ea::string parameter_;
    /// Desired value for condition to be satisfied
    bool value_ = true;
};



/// State machine transition. Belongs to a single StateMachineState instance
struct URHO3D_API StateMachineConfigTransition
{
    /// Initial state
    ea::string stateFrom_;
    /// Destination state
    ea::string stateTo_;
    /// The duration of the transition in seconds
    float duration_ = 0;
    /// Conditions for this transition
    ea::vector<StateMachineConfigTransitionCondition> conditions_;
    
    /// Construct.
    /// This constructor is required for hashmap
    StateMachineConfigTransition()
    {
        
    }

    /// Construct.
    StateMachineConfigTransition(const ea::string &stateFrom, const ea::string &stateTo)
    :stateFrom_(stateFrom)
    ,stateTo_(stateTo)
    {
    }

    bool operator ==(const StateMachineConfigTransition &other) {
        return stateTo_ == other.stateTo_ && stateFrom_ == other.stateFrom_;
    }
    
};


/// Single state machine of a state machine. Belongs to a single StateMachineConfig instance
class URHO3D_API StateMachineConfigState: public RefCounted
{
    friend class StateMachine;
    friend class StateMachineConfig;
    
public:
    /// Construct.
    explicit StateMachineConfigState(const ea::string &name);
    ~StateMachineConfigState();
    
    /// Create new transition from this state to a given state with given transition name
    void AddTransition(const StateMachineConfigTransition &transition);
    /// Get the state name
    ea::string GetName() const;
    /// Check if this parameter may cause transitions
    bool HaveTransitionsFor(const ea::string &parameterName);
    
private:
    
    // State name
    ea::string name_;
    
    /// All transitions from this state
    /// key represents transition name (trigger or event that executes this transition)
    ea::vector<StateMachineConfigTransition> transitions_;

};



/// State machine resource.
class URHO3D_API StateMachineConfig : public Resource
{
    URHO3D_OBJECT(StateMachineConfig, Resource);
    
    friend class StateMachine;
    
public:
    /// Construct.
    explicit StateMachineConfig(Context* context);
    /// Construct with default state.
    explicit StateMachineConfig(Context* context, const ea::string &defaultState);
    /// Destruct.
    ~StateMachineConfig() override;
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);
    
    bool BeginLoad(Deserializer& source) override;

    /// Get number of states
    unsigned int GetStatesCount() const;
    /// Get the default state
    ea::string GetDefaultState() const { return defaultState_; }
    /// Create new state
    bool AddState(const ea::string &stateName);
    /// Create new transition
    bool AddTransition(const StateMachineConfigTransition &transition);
    
    /// Load from JSON data. Removes all existing child nodes and components first. Return true if successful.
    bool LoadJSON(const JSONValue& source);
    /// Load from a JSON file. Return true if successful.
    bool LoadJSON(Deserializer& source);

private:
    /// Default state that should set for new instances
    ea::string defaultState_;
    /// Available states
    ea::map<ea::string, SharedPtr<StateMachineConfigState>> states_;

};

}
