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

#include "../Precompiled.h"

#include "Urho3D/Actions/Actions.h"
#include "Urho3D/Actions/ActionManager.h"
#include "Urho3D/Actions/ActionStates.h"
#include "Urho3D/IO/ArchiveSerializationBasic.h"
#include "Urho3D/Resource/GraphNode.h"

namespace Urho3D
{
namespace Actions
{

void RegisterActions(ActionManager* manager)
{
    manager->AddFactoryReflection<ActionEase>();
    manager->AddFactoryReflection<AttributeBlink>();
    manager->AddFactoryReflection<AttributeFromTo>();
    manager->AddFactoryReflection<AttributeTo>();
    manager->AddFactoryReflection<Blink>();
    manager->AddFactoryReflection<CloneMaterials>();
    manager->AddFactoryReflection<DelayTime>();
    manager->AddFactoryReflection<Disable>();
    manager->AddFactoryReflection<EaseBackIn>();
    manager->AddFactoryReflection<EaseBackInOut>();
    manager->AddFactoryReflection<EaseBackOut>();
    manager->AddFactoryReflection<EaseBounceIn>();
    manager->AddFactoryReflection<EaseBounceInOut>();
    manager->AddFactoryReflection<EaseBounceOut>();
    manager->AddFactoryReflection<EaseElastic>();
    manager->AddFactoryReflection<EaseElasticIn>();
    manager->AddFactoryReflection<EaseElasticInOut>();
    manager->AddFactoryReflection<EaseElasticOut>();
    manager->AddFactoryReflection<EaseExponentialIn>();
    manager->AddFactoryReflection<EaseExponentialInOut>();
    manager->AddFactoryReflection<EaseExponentialOut>();
    manager->AddFactoryReflection<EaseSineIn>();
    manager->AddFactoryReflection<EaseSineInOut>();
    manager->AddFactoryReflection<EaseSineOut>();
    manager->AddFactoryReflection<Enable>();
    manager->AddFactoryReflection<Hide>();
    manager->AddFactoryReflection<JumpBy>();
    manager->AddFactoryReflection<MoveBy>();
    manager->AddFactoryReflection<MoveByQuadratic>();
    manager->AddFactoryReflection<RemoveSelf>();
    manager->AddFactoryReflection<RotateAround>();
    manager->AddFactoryReflection<RotateBy>();
    manager->AddFactoryReflection<ScaleBy>();
    manager->AddFactoryReflection<SetAttribute>();
    manager->AddFactoryReflection<ShaderParameterAction>();
    manager->AddFactoryReflection<ShaderParameterFromTo>();
    manager->AddFactoryReflection<ShaderParameterTo>();
    manager->AddFactoryReflection<Show>();
}


/// Construct.
ActionEase::ActionEase(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> ActionEase::StartAction(Object* target) { return MakeShared<Detail::ActionEaseState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> ActionEase::Reverse() const
{
    auto action = MakeShared<ActionEase>(context_);
    ReverseImpl(action);
    return action;
}

/// Set inner action.
void ActionEase::SetInnerAction(FiniteTimeAction* innerAction)
{
    innerAction_ = innerAction;
    SetDuration(GetDuration());
}


/// Serialize content from/to archive. May throw ArchiveException.
void ActionEase::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "innerAction", innerAction_, SharedPtr<FiniteTimeAction>{});
}

GraphNode* ActionEase::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithExit("innerAction", innerAction_ ? innerAction_->ToGraphNode(graph)->GetEnter(0) : GraphPinRef<GraphEnterPin>{});
}

void ActionEase::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto innerAction = node->GetExit("innerAction"))
    {
        const auto internalAction = MakeActionFromGraphNode(innerAction.GetConnectedPin<GraphEnterPin>().GetNode());
        innerAction_.DynamicCast(internalAction);
    }
}


/// Construct.
AttributeBlink::AttributeBlink(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> AttributeBlink::StartAction(Object* target) { return MakeShared<Detail::AttributeBlinkState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> AttributeBlink::Reverse() const
{
    auto action = MakeShared<AttributeBlink>(context_);
    ReverseImpl(action);
    return action;
}

/// Set from.
void AttributeBlink::SetFrom(const Variant& from)
{
    from_ = from;
}


/// Set to.
void AttributeBlink::SetTo(const Variant& to)
{
    to_ = to;
}


/// Set num of blinks.
void AttributeBlink::SetNumOfBlinks(unsigned numOfBlinks)
{
    numOfBlinks_ = numOfBlinks;
}


/// Serialize content from/to archive. May throw ArchiveException.
void AttributeBlink::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "from", from_, Variant{});
    SerializeOptionalValue(archive, "to", to_, Variant{});
    SerializeOptionalValue(archive, "numOfBlinks", numOfBlinks_, unsigned{});
}

GraphNode* AttributeBlink::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithAnyInput("from", from_)->WithAnyInput("to", to_)->WithInput("numOfBlinks", numOfBlinks_);
}

void AttributeBlink::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto from = node->GetInput("from"))
    {
        from_ = from.GetPin()->GetValue();
    }
    if (const auto to = node->GetInput("to"))
    {
        to_ = to.GetPin()->GetValue();
    }
    if (const auto numOfBlinks = node->GetInput("numOfBlinks"))
    {
        numOfBlinks_ = numOfBlinks.GetPin()->GetValue().Get<unsigned>();
    }
}


/// Construct.
AttributeFromTo::AttributeFromTo(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> AttributeFromTo::StartAction(Object* target) { return MakeShared<Detail::AttributeFromToState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> AttributeFromTo::Reverse() const
{
    auto action = MakeShared<AttributeFromTo>(context_);
    ReverseImpl(action);
    return action;
}

/// Set from.
void AttributeFromTo::SetFrom(const Variant& from)
{
    from_ = from;
}


/// Set to.
void AttributeFromTo::SetTo(const Variant& to)
{
    to_ = to;
}


/// Serialize content from/to archive. May throw ArchiveException.
void AttributeFromTo::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "from", from_, Variant{});
    SerializeOptionalValue(archive, "to", to_, Variant{});
}

GraphNode* AttributeFromTo::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithAnyInput("from", from_)->WithAnyInput("to", to_);
}

void AttributeFromTo::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto from = node->GetInput("from"))
    {
        from_ = from.GetPin()->GetValue();
    }
    if (const auto to = node->GetInput("to"))
    {
        to_ = to.GetPin()->GetValue();
    }
}


/// Construct.
AttributeTo::AttributeTo(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> AttributeTo::StartAction(Object* target) { return MakeShared<Detail::AttributeToState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> AttributeTo::Reverse() const
{
    auto action = MakeShared<AttributeTo>(context_);
    ReverseImpl(action);
    return action;
}

/// Set to.
void AttributeTo::SetTo(const Variant& to)
{
    to_ = to;
}


/// Serialize content from/to archive. May throw ArchiveException.
void AttributeTo::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "to", to_, Variant{});
}

GraphNode* AttributeTo::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithAnyInput("to", to_);
}

void AttributeTo::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto to = node->GetInput("to"))
    {
        to_ = to.GetPin()->GetValue();
    }
}


/// Construct.
Blink::Blink(Context* context)
    : BaseClassName(context)
{
    SetAttributeName("Is Enabled");
}

/// Create new action state from the action.
SharedPtr<ActionState> Blink::StartAction(Object* target) { return MakeShared<Detail::BlinkState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> Blink::Reverse() const
{
    auto action = MakeShared<Blink>(context_);
    ReverseImpl(action);
    return action;
}

/// Set num of blinks.
void Blink::SetNumOfBlinks(unsigned numOfBlinks)
{
    numOfBlinks_ = numOfBlinks;
}


/// Serialize content from/to archive. May throw ArchiveException.
void Blink::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "numOfBlinks", numOfBlinks_, unsigned{});
}

GraphNode* Blink::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithInput("numOfBlinks", numOfBlinks_);
}

void Blink::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto numOfBlinks = node->GetInput("numOfBlinks"))
    {
        numOfBlinks_ = numOfBlinks.GetPin()->GetValue().Get<unsigned>();
    }
}


/// Construct.
CloneMaterials::CloneMaterials(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> CloneMaterials::StartAction(Object* target) { return MakeShared<Detail::CloneMaterialsState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> CloneMaterials::Reverse() const
{
    auto action = MakeShared<CloneMaterials>(context_);
    ReverseImpl(action);
    return action;
}

/// Construct.
DelayTime::DelayTime(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> DelayTime::StartAction(Object* target) { return MakeShared<Detail::DelayTimeState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> DelayTime::Reverse() const
{
    auto action = MakeShared<DelayTime>(context_);
    ReverseImpl(action);
    return action;
}

/// Construct.
Disable::Disable(Context* context)
    : BaseClassName(context)
{
    SetAttributeName("Is Enabled");
}

/// Create new action state from the action.
SharedPtr<ActionState> Disable::StartAction(Object* target) { return MakeShared<Detail::DisableState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> Disable::Reverse() const
{
    auto action = MakeShared<Disable>(context_);
    ReverseImpl(action);
    return action;
}

/// Construct.
EaseBackIn::EaseBackIn(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBackIn::StartAction(Object* target) { return MakeShared<Detail::EaseBackInState>(this, target); }


/// Construct.
EaseBackInOut::EaseBackInOut(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBackInOut::StartAction(Object* target) { return MakeShared<Detail::EaseBackInOutState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseBackInOut::Reverse() const
{
    auto action = MakeShared<EaseBackInOut>(context_);
    ReverseImpl(action);
    return action;
}

/// Construct.
EaseBackOut::EaseBackOut(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBackOut::StartAction(Object* target) { return MakeShared<Detail::EaseBackOutState>(this, target); }


/// Construct.
EaseBounceIn::EaseBounceIn(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBounceIn::StartAction(Object* target) { return MakeShared<Detail::EaseBounceInState>(this, target); }


/// Construct.
EaseBounceInOut::EaseBounceInOut(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBounceInOut::StartAction(Object* target) { return MakeShared<Detail::EaseBounceInOutState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseBounceInOut::Reverse() const
{
    auto action = MakeShared<EaseBounceInOut>(context_);
    ReverseImpl(action);
    return action;
}

/// Construct.
EaseBounceOut::EaseBounceOut(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseBounceOut::StartAction(Object* target) { return MakeShared<Detail::EaseBounceOutState>(this, target); }


/// Construct.
EaseElastic::EaseElastic(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseElastic::StartAction(Object* target) { return MakeShared<Detail::EaseElasticState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseElastic::Reverse() const
{
    auto action = MakeShared<EaseElastic>(context_);
    ReverseImpl(action);
    return action;
}

/// Set period.
void EaseElastic::SetPeriod(float period)
{
    period_ = period;
}


/// Serialize content from/to archive. May throw ArchiveException.
void EaseElastic::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "period", period_, float{0.3f});
}

GraphNode* EaseElastic::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithInput("period", period_);
}

void EaseElastic::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto period = node->GetInput("period"))
    {
        period_ = period.GetPin()->GetValue().Get<float>();
    }
}


/// Construct.
EaseElasticIn::EaseElasticIn(Context* context)
    : BaseClassName(context)
{
    SetPeriod(0.3f);
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseElasticIn::StartAction(Object* target) { return MakeShared<Detail::EaseElasticInState>(this, target); }


/// Construct.
EaseElasticInOut::EaseElasticInOut(Context* context)
    : BaseClassName(context)
{
    SetPeriod(0.3f);
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseElasticInOut::StartAction(Object* target) { return MakeShared<Detail::EaseElasticInOutState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseElasticInOut::Reverse() const
{
    auto action = MakeShared<EaseElasticInOut>(context_);
    ReverseImpl(action);
    return action;
}

/// Construct.
EaseElasticOut::EaseElasticOut(Context* context)
    : BaseClassName(context)
{
    SetPeriod(0.3f);
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseElasticOut::StartAction(Object* target) { return MakeShared<Detail::EaseElasticOutState>(this, target); }


/// Construct.
EaseExponentialIn::EaseExponentialIn(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseExponentialIn::StartAction(Object* target) { return MakeShared<Detail::EaseExponentialInState>(this, target); }


/// Construct.
EaseExponentialInOut::EaseExponentialInOut(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseExponentialInOut::StartAction(Object* target) { return MakeShared<Detail::EaseExponentialInOutState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseExponentialInOut::Reverse() const
{
    auto action = MakeShared<EaseExponentialInOut>(context_);
    ReverseImpl(action);
    return action;
}

/// Construct.
EaseExponentialOut::EaseExponentialOut(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseExponentialOut::StartAction(Object* target) { return MakeShared<Detail::EaseExponentialOutState>(this, target); }


/// Construct.
EaseSineIn::EaseSineIn(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseSineIn::StartAction(Object* target) { return MakeShared<Detail::EaseSineInState>(this, target); }


/// Construct.
EaseSineInOut::EaseSineInOut(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseSineInOut::StartAction(Object* target) { return MakeShared<Detail::EaseSineInOutState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> EaseSineInOut::Reverse() const
{
    auto action = MakeShared<EaseSineInOut>(context_);
    ReverseImpl(action);
    return action;
}

/// Construct.
EaseSineOut::EaseSineOut(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> EaseSineOut::StartAction(Object* target) { return MakeShared<Detail::EaseSineOutState>(this, target); }


/// Construct.
Enable::Enable(Context* context)
    : BaseClassName(context)
{
    SetAttributeName("Is Enabled");
}

/// Create new action state from the action.
SharedPtr<ActionState> Enable::StartAction(Object* target) { return MakeShared<Detail::EnableState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> Enable::Reverse() const
{
    auto action = MakeShared<Enable>(context_);
    ReverseImpl(action);
    return action;
}

/// Construct.
Hide::Hide(Context* context)
    : BaseClassName(context)
{
    SetAttributeName("Is Visible");
}

/// Create new action state from the action.
SharedPtr<ActionState> Hide::StartAction(Object* target) { return MakeShared<Detail::HideState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> Hide::Reverse() const
{
    auto action = MakeShared<Hide>(context_);
    ReverseImpl(action);
    return action;
}

/// Construct.
JumpBy::JumpBy(Context* context)
    : BaseClassName(context)
{
    SetAttributeName("Position");
}

/// Create new action state from the action.
SharedPtr<ActionState> JumpBy::StartAction(Object* target) { return MakeShared<Detail::JumpByState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> JumpBy::Reverse() const
{
    auto action = MakeShared<JumpBy>(context_);
    ReverseImpl(action);
    return action;
}

/// Set delta.
void JumpBy::SetDelta(const Vector3& delta)
{
    delta_ = delta;
}


/// Serialize content from/to archive. May throw ArchiveException.
void JumpBy::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Vector3{});
}

GraphNode* JumpBy::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithInput("delta", delta_);
}

void JumpBy::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto delta = node->GetInput("delta"))
    {
        delta_ = delta.GetPin()->GetValue().Get<Vector3>();
    }
}


/// Construct.
MoveBy::MoveBy(Context* context)
    : BaseClassName(context)
{
    SetAttributeName("Position");
}

/// Create new action state from the action.
SharedPtr<ActionState> MoveBy::StartAction(Object* target) { return MakeShared<Detail::MoveByState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> MoveBy::Reverse() const
{
    auto action = MakeShared<MoveBy>(context_);
    ReverseImpl(action);
    return action;
}

/// Set delta.
void MoveBy::SetDelta(const Vector3& delta)
{
    delta_ = delta;
}


/// Serialize content from/to archive. May throw ArchiveException.
void MoveBy::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Vector3{});
}

GraphNode* MoveBy::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithInput("delta", delta_);
}

void MoveBy::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto delta = node->GetInput("delta"))
    {
        delta_ = delta.GetPin()->GetValue().Get<Vector3>();
    }
}


/// Construct.
MoveByQuadratic::MoveByQuadratic(Context* context)
    : BaseClassName(context)
{
    SetAttributeName("Position");
}

/// Create new action state from the action.
SharedPtr<ActionState> MoveByQuadratic::StartAction(Object* target) { return MakeShared<Detail::MoveByQuadraticState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> MoveByQuadratic::Reverse() const
{
    auto action = MakeShared<MoveByQuadratic>(context_);
    ReverseImpl(action);
    return action;
}

/// Set control.
void MoveByQuadratic::SetControl(const Vector3& control)
{
    control_ = control;
}


/// Serialize content from/to archive. May throw ArchiveException.
void MoveByQuadratic::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "control", control_, Vector3{});
}

GraphNode* MoveByQuadratic::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithInput("control", control_);
}

void MoveByQuadratic::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto control = node->GetInput("control"))
    {
        control_ = control.GetPin()->GetValue().Get<Vector3>();
    }
}


/// Construct.
RemoveSelf::RemoveSelf(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> RemoveSelf::StartAction(Object* target) { return MakeShared<Detail::RemoveSelfState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> RemoveSelf::Reverse() const
{
    auto action = MakeShared<RemoveSelf>(context_);
    ReverseImpl(action);
    return action;
}

/// Construct.
RotateAround::RotateAround(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> RotateAround::StartAction(Object* target) { return MakeShared<Detail::RotateAroundState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> RotateAround::Reverse() const
{
    auto action = MakeShared<RotateAround>(context_);
    ReverseImpl(action);
    return action;
}

/// Set delta.
void RotateAround::SetDelta(const Quaternion& delta)
{
    delta_ = delta;
}


/// Set pivot.
void RotateAround::SetPivot(const Vector3& pivot)
{
    pivot_ = pivot;
}


/// Serialize content from/to archive. May throw ArchiveException.
void RotateAround::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Quaternion{});
    SerializeOptionalValue(archive, "pivot", pivot_, Vector3{});
}

GraphNode* RotateAround::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithInput("delta", delta_)->WithInput("pivot", pivot_);
}

void RotateAround::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto delta = node->GetInput("delta"))
    {
        delta_ = delta.GetPin()->GetValue().Get<Quaternion>();
    }
    if (const auto pivot = node->GetInput("pivot"))
    {
        pivot_ = pivot.GetPin()->GetValue().Get<Vector3>();
    }
}


/// Construct.
RotateBy::RotateBy(Context* context)
    : BaseClassName(context)
{
    SetAttributeName("Rotation");
}

/// Create new action state from the action.
SharedPtr<ActionState> RotateBy::StartAction(Object* target) { return MakeShared<Detail::RotateByState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> RotateBy::Reverse() const
{
    auto action = MakeShared<RotateBy>(context_);
    ReverseImpl(action);
    return action;
}

/// Set delta.
void RotateBy::SetDelta(const Quaternion& delta)
{
    delta_ = delta;
}


/// Serialize content from/to archive. May throw ArchiveException.
void RotateBy::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Quaternion{});
}

GraphNode* RotateBy::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithInput("delta", delta_);
}

void RotateBy::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto delta = node->GetInput("delta"))
    {
        delta_ = delta.GetPin()->GetValue().Get<Quaternion>();
    }
}


/// Construct.
ScaleBy::ScaleBy(Context* context)
    : BaseClassName(context)
{
    SetAttributeName("Scale");
}

/// Create new action state from the action.
SharedPtr<ActionState> ScaleBy::StartAction(Object* target) { return MakeShared<Detail::ScaleByState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> ScaleBy::Reverse() const
{
    auto action = MakeShared<ScaleBy>(context_);
    ReverseImpl(action);
    return action;
}

/// Set delta.
void ScaleBy::SetDelta(const Vector3& delta)
{
    delta_ = delta;
}


/// Serialize content from/to archive. May throw ArchiveException.
void ScaleBy::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "delta", delta_, Vector3{1,1,1});
}

GraphNode* ScaleBy::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithInput("delta", delta_);
}

void ScaleBy::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto delta = node->GetInput("delta"))
    {
        delta_ = delta.GetPin()->GetValue().Get<Vector3>();
    }
}


/// Construct.
SetAttribute::SetAttribute(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> SetAttribute::StartAction(Object* target) { return MakeShared<Detail::SetAttributeState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> SetAttribute::Reverse() const
{
    auto action = MakeShared<SetAttribute>(context_);
    ReverseImpl(action);
    return action;
}

/// Set value.
void SetAttribute::SetValue(const Variant& value)
{
    value_ = value;
}


/// Serialize content from/to archive. May throw ArchiveException.
void SetAttribute::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "value", value_, Variant{});
}

GraphNode* SetAttribute::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithAnyInput("value", value_);
}

void SetAttribute::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto value = node->GetInput("value"))
    {
        value_ = value.GetPin()->GetValue();
    }
}


/// Construct.
ShaderParameterAction::ShaderParameterAction(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> ShaderParameterAction::StartAction(Object* target) { return MakeShared<Detail::ShaderParameterActionState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> ShaderParameterAction::Reverse() const
{
    auto action = MakeShared<ShaderParameterAction>(context_);
    ReverseImpl(action);
    return action;
}

/// Set name.
void ShaderParameterAction::SetName(const ea::string& name)
{
    name_ = name;
}


/// Serialize content from/to archive. May throw ArchiveException.
void ShaderParameterAction::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "name", name_, ea::string{});
}

GraphNode* ShaderParameterAction::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithInput("name", name_);
}

void ShaderParameterAction::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto name = node->GetInput("name"))
    {
        name_ = name.GetPin()->GetValue().Get<ea::string>();
    }
}


/// Construct.
ShaderParameterFromTo::ShaderParameterFromTo(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> ShaderParameterFromTo::StartAction(Object* target) { return MakeShared<Detail::ShaderParameterFromToState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> ShaderParameterFromTo::Reverse() const
{
    auto action = MakeShared<ShaderParameterFromTo>(context_);
    ReverseImpl(action);
    return action;
}

/// Set from.
void ShaderParameterFromTo::SetFrom(const Variant& from)
{
    from_ = from;
}


/// Set to.
void ShaderParameterFromTo::SetTo(const Variant& to)
{
    to_ = to;
}


/// Serialize content from/to archive. May throw ArchiveException.
void ShaderParameterFromTo::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "from", from_, Variant{});
    SerializeOptionalValue(archive, "to", to_, Variant{});
}

GraphNode* ShaderParameterFromTo::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithAnyInput("from", from_)->WithAnyInput("to", to_);
}

void ShaderParameterFromTo::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto from = node->GetInput("from"))
    {
        from_ = from.GetPin()->GetValue();
    }
    if (const auto to = node->GetInput("to"))
    {
        to_ = to.GetPin()->GetValue();
    }
}


/// Construct.
ShaderParameterTo::ShaderParameterTo(Context* context)
    : BaseClassName(context)
{
}

/// Create new action state from the action.
SharedPtr<ActionState> ShaderParameterTo::StartAction(Object* target) { return MakeShared<Detail::ShaderParameterToState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> ShaderParameterTo::Reverse() const
{
    auto action = MakeShared<ShaderParameterTo>(context_);
    ReverseImpl(action);
    return action;
}

/// Set to.
void ShaderParameterTo::SetTo(const Variant& to)
{
    to_ = to;
}


/// Serialize content from/to archive. May throw ArchiveException.
void ShaderParameterTo::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "to", to_, Variant{});
}

GraphNode* ShaderParameterTo::ToGraphNode(Graph* graph) const
{
    return BaseClassName::ToGraphNode(graph)->WithAnyInput("to", to_);
}

void ShaderParameterTo::FromGraphNode(GraphNode* node)
{
    BaseClassName::FromGraphNode(node);
    if (const auto to = node->GetInput("to"))
    {
        to_ = to.GetPin()->GetValue();
    }
}


/// Construct.
Show::Show(Context* context)
    : BaseClassName(context)
{
    SetAttributeName("Is Visible");
}

/// Create new action state from the action.
SharedPtr<ActionState> Show::StartAction(Object* target) { return MakeShared<Detail::ShowState>(this, target); }


/// Create reversed action.
SharedPtr<FiniteTimeAction> Show::Reverse() const
{
    auto action = MakeShared<Show>(context_);
    ReverseImpl(action);
    return action;
}

} // namespace Actions
} // namespace Urho3D
