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

#include "Urho3D/Actions/FiniteTimeAction.h"
#include "Urho3D/Actions/AttributeAction.h"

namespace Urho3D
{
namespace Actions
{
class URHO3D_API MoveBy : public  AttributeAction
{
    URHO3D_OBJECT(MoveBy, AttributeAction)
public:
    /// Construct.
    explicit MoveBy(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Set delta.
    void SetDelta(const Vector3& delta);

    /// Get delta.
    const Vector3& GetDelta() const { return delta_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create GraphNode from the action. Required for action editor.
    GraphNode* ToGraphNode(Graph* graph) const override;

    /// Initialize action from GraphNode. Required for action editor.
    void FromGraphNode(GraphNode* node) override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    /// Populate fields in reversed action.
    void ReverseImpl(FiniteTimeAction*) const override;

private:
    Vector3 delta_{};
};

class URHO3D_API MoveByQuadratic : public  MoveBy
{
    URHO3D_OBJECT(MoveByQuadratic, MoveBy)
public:
    /// Construct.
    explicit MoveByQuadratic(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Set control.
    void SetControl(const Vector3& control);

    /// Get control.
    const Vector3& GetControl() const { return control_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create GraphNode from the action. Required for action editor.
    GraphNode* ToGraphNode(Graph* graph) const override;

    /// Initialize action from GraphNode. Required for action editor.
    void FromGraphNode(GraphNode* node) override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    /// Populate fields in reversed action.
    void ReverseImpl(FiniteTimeAction*) const override;

private:
    Vector3 control_{};
};

class URHO3D_API JumpBy : public  AttributeActionInstant
{
    URHO3D_OBJECT(JumpBy, AttributeActionInstant)
public:
    /// Construct.
    explicit JumpBy(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Set delta.
    void SetDelta(const Vector3& delta);

    /// Get delta.
    const Vector3& GetDelta() const { return delta_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create GraphNode from the action. Required for action editor.
    GraphNode* ToGraphNode(Graph* graph) const override;

    /// Initialize action from GraphNode. Required for action editor.
    void FromGraphNode(GraphNode* node) override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    /// Populate fields in reversed action.
    void ReverseImpl(FiniteTimeAction*) const override;

private:
    Vector3 delta_{};
};

class URHO3D_API ScaleBy : public  AttributeAction
{
    URHO3D_OBJECT(ScaleBy, AttributeAction)
public:
    /// Construct.
    explicit ScaleBy(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Set delta.
    void SetDelta(const Vector3& delta);

    /// Get delta.
    const Vector3& GetDelta() const { return delta_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create GraphNode from the action. Required for action editor.
    GraphNode* ToGraphNode(Graph* graph) const override;

    /// Initialize action from GraphNode. Required for action editor.
    void FromGraphNode(GraphNode* node) override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    /// Populate fields in reversed action.
    void ReverseImpl(FiniteTimeAction*) const override;

private:
    Vector3 delta_{1,1,1};
};

class URHO3D_API RotateBy : public  AttributeAction
{
    URHO3D_OBJECT(RotateBy, AttributeAction)
public:
    /// Construct.
    explicit RotateBy(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Set delta.
    void SetDelta(const Quaternion& delta);

    /// Get delta.
    const Quaternion& GetDelta() const { return delta_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create GraphNode from the action. Required for action editor.
    GraphNode* ToGraphNode(Graph* graph) const override;

    /// Initialize action from GraphNode. Required for action editor.
    void FromGraphNode(GraphNode* node) override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    /// Populate fields in reversed action.
    void ReverseImpl(FiniteTimeAction*) const override;

private:
    Quaternion delta_{};
};

class URHO3D_API RotateAround : public  AttributeAction
{
    URHO3D_OBJECT(RotateAround, AttributeAction)
public:
    /// Construct.
    explicit RotateAround(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Set delta.
    void SetDelta(const Quaternion& delta);

    /// Get delta.
    const Quaternion& GetDelta() const { return delta_; }

    /// Set pivot.
    void SetPivot(const Vector3& pivot);

    /// Get pivot.
    const Vector3& GetPivot() const { return pivot_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create GraphNode from the action. Required for action editor.
    GraphNode* ToGraphNode(Graph* graph) const override;

    /// Initialize action from GraphNode. Required for action editor.
    void FromGraphNode(GraphNode* node) override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    /// Populate fields in reversed action.
    void ReverseImpl(FiniteTimeAction*) const override;

private:
    Quaternion delta_{};
    Vector3 pivot_{};
};

class URHO3D_API RemoveSelf : public  ActionInstant
{
    URHO3D_OBJECT(RemoveSelf, ActionInstant)
public:
    /// Construct.
    explicit RemoveSelf(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API CloneMaterials : public  ActionInstant
{
    URHO3D_OBJECT(CloneMaterials, ActionInstant)
public:
    /// Construct.
    explicit CloneMaterials(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API Show : public  AttributeActionInstant
{
    URHO3D_OBJECT(Show, AttributeActionInstant)
public:
    /// Construct.
    explicit Show(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API Hide : public  AttributeActionInstant
{
    URHO3D_OBJECT(Hide, AttributeActionInstant)
public:
    /// Construct.
    explicit Hide(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API Enable : public  AttributeActionInstant
{
    URHO3D_OBJECT(Enable, AttributeActionInstant)
public:
    /// Construct.
    explicit Enable(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API Disable : public  AttributeActionInstant
{
    URHO3D_OBJECT(Disable, AttributeActionInstant)
public:
    /// Construct.
    explicit Disable(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API Blink : public  AttributeAction
{
    URHO3D_OBJECT(Blink, AttributeAction)
public:
    /// Construct.
    explicit Blink(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Set num of blinks.
    void SetNumOfBlinks(unsigned numOfBlinks);

    /// Get num of blinks.
    unsigned GetNumOfBlinks() const { return numOfBlinks_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create GraphNode from the action. Required for action editor.
    GraphNode* ToGraphNode(Graph* graph) const override;

    /// Initialize action from GraphNode. Required for action editor.
    void FromGraphNode(GraphNode* node) override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    /// Populate fields in reversed action.
    void ReverseImpl(FiniteTimeAction*) const override;

private:
    unsigned numOfBlinks_{};
};

class URHO3D_API DelayTime : public  FiniteTimeAction
{
    URHO3D_OBJECT(DelayTime, FiniteTimeAction)
public:
    /// Construct.
    explicit DelayTime(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API ActionEase : public  DynamicAction
{
    URHO3D_OBJECT(ActionEase, DynamicAction)
public:
    /// Construct.
    explicit ActionEase(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Get action duration.
    float GetDuration() const override;

    /// Set inner action.
    void SetInnerAction(FiniteTimeAction* innerAction);

    /// Get inner action.
    FiniteTimeAction* GetInnerAction() const { return innerAction_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create GraphNode from the action. Required for action editor.
    GraphNode* ToGraphNode(Graph* graph) const override;

    /// Initialize action from GraphNode. Required for action editor.
    void FromGraphNode(GraphNode* node) override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    /// Populate fields in reversed action.
    void ReverseImpl(FiniteTimeAction*) const override;

private:
    SharedPtr<FiniteTimeAction> innerAction_{};
};

class URHO3D_API EaseElastic : public  ActionEase
{
    URHO3D_OBJECT(EaseElastic, ActionEase)
public:
    /// Construct.
    explicit EaseElastic(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Set period.
    void SetPeriod(float period);

    /// Get period.
    float GetPeriod() const { return period_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Create GraphNode from the action. Required for action editor.
    GraphNode* ToGraphNode(Graph* graph) const override;

    /// Initialize action from GraphNode. Required for action editor.
    void FromGraphNode(GraphNode* node) override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

    /// Populate fields in reversed action.
    void ReverseImpl(FiniteTimeAction*) const override;

private:
    float period_{0.3f};
};

class URHO3D_API EaseBackIn : public  ActionEase
{
    URHO3D_OBJECT(EaseBackIn, ActionEase)
public:
    /// Construct.
    explicit EaseBackIn(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseBackOut : public  ActionEase
{
    URHO3D_OBJECT(EaseBackOut, ActionEase)
public:
    /// Construct.
    explicit EaseBackOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseBackInOut : public  ActionEase
{
    URHO3D_OBJECT(EaseBackInOut, ActionEase)
public:
    /// Construct.
    explicit EaseBackInOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseBounceOut : public  ActionEase
{
    URHO3D_OBJECT(EaseBounceOut, ActionEase)
public:
    /// Construct.
    explicit EaseBounceOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseBounceIn : public  ActionEase
{
    URHO3D_OBJECT(EaseBounceIn, ActionEase)
public:
    /// Construct.
    explicit EaseBounceIn(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseBounceInOut : public  ActionEase
{
    URHO3D_OBJECT(EaseBounceInOut, ActionEase)
public:
    /// Construct.
    explicit EaseBounceInOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseSineOut : public  ActionEase
{
    URHO3D_OBJECT(EaseSineOut, ActionEase)
public:
    /// Construct.
    explicit EaseSineOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseSineIn : public  ActionEase
{
    URHO3D_OBJECT(EaseSineIn, ActionEase)
public:
    /// Construct.
    explicit EaseSineIn(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseSineInOut : public  ActionEase
{
    URHO3D_OBJECT(EaseSineInOut, ActionEase)
public:
    /// Construct.
    explicit EaseSineInOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseExponentialOut : public  ActionEase
{
    URHO3D_OBJECT(EaseExponentialOut, ActionEase)
public:
    /// Construct.
    explicit EaseExponentialOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseExponentialIn : public  ActionEase
{
    URHO3D_OBJECT(EaseExponentialIn, ActionEase)
public:
    /// Construct.
    explicit EaseExponentialIn(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseExponentialInOut : public  ActionEase
{
    URHO3D_OBJECT(EaseExponentialInOut, ActionEase)
public:
    /// Construct.
    explicit EaseExponentialInOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseElasticIn : public  EaseElastic
{
    URHO3D_OBJECT(EaseElasticIn, EaseElastic)
public:
    /// Construct.
    explicit EaseElasticIn(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseElasticOut : public  EaseElastic
{
    URHO3D_OBJECT(EaseElasticOut, EaseElastic)
public:
    /// Construct.
    explicit EaseElasticOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

class URHO3D_API EaseElasticInOut : public  EaseElastic
{
    URHO3D_OBJECT(EaseElasticInOut, EaseElastic)
public:
    /// Construct.
    explicit EaseElasticInOut(Context* context);

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;
protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
};

} // namespace Actions
} // namespace Urho3D
