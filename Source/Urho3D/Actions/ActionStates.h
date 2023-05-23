//
// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2023 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//
#pragma once

#include "Actions.h"
#include "Urho3D/Actions/AttributeActionState.h"
#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Graphics/StaticModel.h"

#include <EASTL/variant.h>

namespace Urho3D
{
namespace Actions
{
namespace Detail
{

struct NopAttributeActionState
{
    void Init(const AttributeActionState* state) {}
    void Update(float time, Variant& value) {}
};

class MoveByState : public AttributeActionState
{
    struct Vec3State
    {
        Vector3 positionDelta_;
        Vector3 startPosition_;
        Vector3 previousPosition_;
        void Init(const MoveByState* state);
        void Update(float time, Variant& value);
    };

    struct IntVec3State
    {
        Vector3 positionDelta_;
        IntVector3 startPosition_;
        IntVector3 previousPosition_;
        void Init(const MoveByState* state);
        void Update(float time, Variant& value);
    };

    struct Vec2State
    {
        Vector2 positionDelta_;
        Vector2 startPosition_;
        Vector2 previousPosition_;
        void Init(const MoveByState* state);
        void Update(float time, Variant& value);
    };

    struct IntVec2State
    {
        Vector2 positionDelta_;
        IntVector2 startPosition_;
        IntVector2 previousPosition_;
        void Init(const MoveByState* state);
        void Update(float time, Variant& value);
    };

public:
    MoveByState(MoveBy* action, Object* target);
    const Vector3& GetDelta() const { return static_cast<MoveBy*>(GetAction())->GetDelta(); }
    void Update(float dt, Variant& value) override;
private:
    ea::variant<NopAttributeActionState, IntVec2State, IntVec3State, Vec2State, Vec3State> state_;
};

class MoveByQuadraticState : public AttributeActionState
{
    struct Vec3State
    {
        Vector3 positionDelta_;
        Vector3 controlDelta_;
        Vector3 startPosition_;
        Vector3 previousPosition_;
        void Init(const MoveByQuadraticState* state);
        void Update(float time, Variant& value);
    };

    struct IntVec3State
    {
        Vector3 positionDelta_;
        Vector3 controlDelta_;
        IntVector3 startPosition_;
        IntVector3 previousPosition_;
        void Init(const MoveByQuadraticState* state);
        void Update(float time, Variant& value);
    };

    struct Vec2State
    {
        Vector2 positionDelta_;
        Vector2 controlDelta_;
        Vector2 startPosition_;
        Vector2 previousPosition_;
        void Init(const MoveByQuadraticState* state);
        void Update(float time, Variant& value);
    };

    struct IntVec2State
    {
        Vector2 positionDelta_;
        Vector2 controlDelta_;
        IntVector2 startPosition_;
        IntVector2 previousPosition_;
        void Init(const MoveByQuadraticState* state);
        void Update(float time, Variant& value);
    };

public:
    MoveByQuadraticState(MoveByQuadratic* action, Object* target);
    const Vector3& GetDelta() const { return static_cast<MoveByQuadratic*>(GetAction())->GetDelta(); }
    const Vector3& GetControl() const { return static_cast<MoveByQuadratic*>(GetAction())->GetControl(); }
    void Update(float dt, Variant& value) override;

private:
    ea::variant<NopAttributeActionState, IntVec2State, IntVec3State, Vec2State, Vec3State> state_;
};

class JumpByState : public AttributeActionState
{
    template <typename T>
    struct State
    {
        T positionDelta_{};
        bool triggered_{};
        void Init(const JumpByState* state);
        void Update(float time, Variant& value);
    };

public:
    JumpByState(JumpBy* action, Object* target);
    const Vector3& GetDelta() const { return static_cast<JumpBy*>(GetAction())->GetDelta(); }
    void Update(float dt, Variant& value) override;

private:
    ea::variant<NopAttributeActionState, State<IntVector2>, State<IntVector3>, State<Vector2>, State<Vector3>> state_;
};

template <typename T> void JumpByState::State<T>::Init(const JumpByState* state)
{
    positionDelta_ = static_cast<T>(state->GetDelta());
}

template <typename T> void JumpByState::State<T>::Update(float time, Variant& value)
{
    if (triggered_)
        return;
    triggered_ = true;
    value = value.Get<T>() + static_cast<T>(positionDelta_);
}

class ScaleByState : public AttributeActionState
{
    template <typename T> struct State
    {
        T scaleDelta_;
        T startScale_;
        T previousScale_;
        void Init(const ScaleByState* state);
        void Update(float time, Variant& value);
    };

public:
    ScaleByState(ScaleBy* action, Object* target);
    const Vector3& GetDelta() const { return static_cast<ScaleBy*>(GetAction())->GetDelta(); }
    void Update(float time, Variant& value) override;
private:
    ea::variant<NopAttributeActionState, State<Vector2>, State<Vector3>> state_;
};

template <typename T> void ScaleByState::State<T>::Init(const ScaleByState* state)
{
    scaleDelta_ = static_cast<T>(state->GetDelta());
    previousScale_ = startScale_ = state->Get<T>();
}

template <typename T> void ScaleByState::State<T>::Update(float time, Variant& value)
{
    const auto currentScale = value.Get<T>();
    const auto diff = currentScale / previousScale_;
    startScale_ = startScale_ * diff;
    const auto newScale = startScale_ * T::ONE.Lerp(scaleDelta_, time);
    value = newScale;
    previousScale_ = newScale;
}

class RotateByState : public AttributeActionState
{
    Quaternion rotationDelta_;
    Quaternion startRotation_;
    Quaternion previousRotation_;

public:
    RotateByState(RotateBy* action, Object* target);
    void Update(float time, Variant& value) override;
};

class RotateAroundState : public FiniteTimeActionState
{
    Quaternion rotationDelta_;
    Quaternion startRotation_;
    Quaternion previousRotation_;
    Vector3 pivot_;
    AttributeInfo* rotationAttribute_{};
    AttributeInfo* positionAttribute_{};

public:
    RotateAroundState(RotateAround* action, Object* target);

    void Update(float time) override;
};

class RemoveSelfState : public FiniteTimeActionState
{
public:
    using FiniteTimeActionState::FiniteTimeActionState;

    void Update(float time) override;

private:
    bool triggered_{};
};

class CloneMaterialsState : public FiniteTimeActionState
{
public:
    using FiniteTimeActionState::FiniteTimeActionState;

    void Update(float time) override;

private:
    bool triggered_{};
};

class SetAttributeState : public AttributeActionState
{
public:
    /// Construct.
    SetAttributeState(AttributeAction* action, Object* target, const Variant& value);
    /// Construct.
    SetAttributeState(AttributeActionInstant* action, Object* target, const Variant& value);
    /// Construct.
    SetAttributeState(SetAttribute* action, Object* target);

private:
    void Update(float time, Variant& var) override;

private:
    Variant value_;
    bool triggered_{};
};

struct ShowState : public SetAttributeState
{
    ShowState(Show* action, Object* target);
};

struct HideState : public SetAttributeState
{
    HideState(Hide* action, Object* target);
};

struct EnableState : public SetAttributeState
{
    EnableState(Enable* action, Object* target);
};

struct DisableState : public SetAttributeState
{
    DisableState(Disable* action, Object* target);
};

class AttributeBlinkState : public AttributeActionState
{
public:
    AttributeBlinkState(AttributeAction* action, Object* target, Variant from, Variant to, unsigned times);
    AttributeBlinkState(AttributeBlink* action, Object* target);

    void Update(float time, Variant& var) override;

    void Stop() override;

private:
    unsigned times_;
    Variant originalState_;
    Variant from_;
    Variant to_;
};


struct BlinkState : public AttributeBlinkState
{
    BlinkState(Blink* action, Object* target);
};

struct DelayTimeState : public FiniteTimeActionState
{
    using FiniteTimeActionState::FiniteTimeActionState;
};

class ActionEaseState : public FiniteTimeActionState
{
public:
    ActionEaseState(ActionEase* action, Object* target);

    void Update(float time) override;

    virtual float Ease(float time) const;

private:
    SharedPtr<FiniteTimeActionState> innerState_;
};

struct EaseBackInState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseBackOutState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseBackInOutState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseBounceInState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseBounceOutState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseBounceInOutState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseSineInState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseSineOutState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseSineInOutState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseExponentialInState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseExponentialOutState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseExponentialInOutState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseElasticState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseElasticInState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseElasticOutState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

struct EaseElasticInOutState : public ActionEaseState
{
    using ActionEaseState::ActionEaseState;

    float Ease(float time) const override;
};

class AttributeFromToState : public AttributeActionState
{
    Variant from_;
    Variant to_;

public:
    AttributeFromToState(AttributeFromTo* action, Object* target);

    void Update(float time, Variant& value) override;
};

class AttributeToState : public AttributeActionState
{
    Variant from_;
    Variant to_;

public:
    AttributeToState(AttributeTo* action, Object* target);

    void Update(float time, Variant& value) override;
};
class ShaderParameterActionState : public FiniteTimeActionState
{
    Variant from_;
    Variant to_;
    ea::string name_;
    SharedPtr<Material> material_;

public:
    ShaderParameterActionState(ShaderParameterAction* action, Object* target);
};

class ShaderParameterFromToState : public FiniteTimeActionState
{
    Variant from_;
    Variant to_;
    ea::string name_;
    SharedPtr<Material> material_;

public:
    ShaderParameterFromToState(ShaderParameterFromTo* action, Object* target);

    static Material* GetMaterial(Object* target);

    void Update(float time) override;
};

class ShaderParameterToState : public FiniteTimeActionState
{
    Variant from_;
    Variant to_;
    ea::string name_;
    SharedPtr<Material> material_;

public:
    ShaderParameterToState(ShaderParameterTo* action, Object* target);

    void Update(float time) override;
};

} // namespace Detail
} // namespace Actions
} // namespace Urho3D
