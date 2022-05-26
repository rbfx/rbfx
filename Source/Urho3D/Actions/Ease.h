//
// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022 the rbfx project.
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

#include "../Math/EaseMath.h"
#include "FiniteTimeAction.h"

namespace Urho3D
{
namespace Actions
{

/// Base action state.
class URHO3D_API ActionEase : public FiniteTimeAction
{
    URHO3D_OBJECT(ActionEase, FiniteTimeAction)
public:
    /// Construct.
    explicit ActionEase(Context* context);

    /// Get action duration.
    float GetDuration() const override;

    /// Set inner action.
    void SetInnerAction(FiniteTimeAction* action);

    /// Get inner action.
    FiniteTimeAction* GetInnerAction() const { return innerAction_.Get(); }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    virtual float Ease(float time) const;

private:
    SharedPtr<FiniteTimeAction> innerAction_;
};

/// ElasticIn easing action.
class URHO3D_API EaseElastic : public ActionEase
{
    URHO3D_OBJECT(EaseElastic, ActionEase)

public:
    /// Construct.
    explicit EaseElastic(Context* context);

    /// Set period
    void SetPeriod(float period) { period_ = period; }
    /// Get period
    float GetPeriod() const { return period_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

private:
    float period_{0.3f};
};


/// -------------------------------------------------------------------
/// BackIn easing action.
class URHO3D_API EaseBackIn : public ActionEase
{
    URHO3D_OBJECT(EaseBackIn, ActionEase)
public:
    /// Construct.
    explicit EaseBackIn(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return BackIn(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// BackOut easing action.
class URHO3D_API EaseBackOut : public ActionEase
{
    URHO3D_OBJECT(EaseBackOut, ActionEase)
public:
    /// Construct.
    explicit EaseBackOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return BackOut(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// BackInOut easing action.
class URHO3D_API EaseBackInOut : public ActionEase
{
    URHO3D_OBJECT(EaseBackInOut, ActionEase)
public:
    /// Construct.
    explicit EaseBackInOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return BackInOut(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// BounceOut easing action.
class URHO3D_API EaseBounceOut : public ActionEase
{
    URHO3D_OBJECT(EaseBounceOut, ActionEase)
public:
    /// Construct.
    explicit EaseBounceOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return BounceOut(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// BounceIn easing action.
class URHO3D_API EaseBounceIn : public ActionEase
{
    URHO3D_OBJECT(EaseBounceIn, ActionEase)
public:
    /// Construct.
    explicit EaseBounceIn(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return BounceIn(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// BounceInOut easing action.
class URHO3D_API EaseBounceInOut : public ActionEase
{
    URHO3D_OBJECT(EaseBounceInOut, ActionEase)
public:
    /// Construct.
    explicit EaseBounceInOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return BounceInOut(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// SineOut easing action.
class URHO3D_API EaseSineOut : public ActionEase
{
    URHO3D_OBJECT(EaseSineOut, ActionEase)
public:
    /// Construct.
    explicit EaseSineOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return SineOut(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// SineIn easing action.
class URHO3D_API EaseSineIn : public ActionEase
{
    URHO3D_OBJECT(EaseSineIn, ActionEase)
public:
    /// Construct.
    explicit EaseSineIn(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return SineIn(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// SineInOut easing action.
class URHO3D_API EaseSineInOut : public ActionEase
{
    URHO3D_OBJECT(EaseSineInOut, ActionEase)
public:
    /// Construct.
    explicit EaseSineInOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return SineInOut(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// ExponentialOut easing action.
class URHO3D_API EaseExponentialOut : public ActionEase
{
    URHO3D_OBJECT(EaseExponentialOut, ActionEase)
public:
    /// Construct.
    explicit EaseExponentialOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return ExponentialOut(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// ExponentialIn easing action.
class URHO3D_API EaseExponentialIn : public ActionEase
{
    URHO3D_OBJECT(EaseExponentialIn, ActionEase)
public:
    /// Construct.
    explicit EaseExponentialIn(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return ExponentialIn(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// ExponentialInOut easing action.
class URHO3D_API EaseExponentialInOut : public ActionEase
{
    URHO3D_OBJECT(EaseExponentialInOut, ActionEase)
public:
    /// Construct.
    explicit EaseExponentialInOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return ExponentialInOut(time); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// ElasticIn easing action.
class URHO3D_API EaseElasticIn : public EaseElastic
{
    URHO3D_OBJECT(EaseElasticIn, EaseElastic)
public:
    /// Construct.
    explicit EaseElasticIn(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return ElasticIn(time, GetPeriod()); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// ElasticOut easing action.
class URHO3D_API EaseElasticOut : public EaseElastic
{
    URHO3D_OBJECT(EaseElasticOut, EaseElastic)
public:
    /// Construct.
    explicit EaseElasticOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return ElasticOut(time, GetPeriod()); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

/// -------------------------------------------------------------------
/// ElasticInOut easing action.
class URHO3D_API EaseElasticInOut : public EaseElastic
{
    URHO3D_OBJECT(EaseElasticInOut, EaseElastic)
public:
    /// Construct.
    explicit EaseElasticInOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Apply easing function to the time argument.
    float Ease(float time) const override { return ElasticInOut(time, GetPeriod()); }

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;
};

} // namespace Actions
} // namespace Urho3D
