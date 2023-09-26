//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Precompiled.h"

#include "../Math/InverseKinematics.h"
#include "../Math/Transform.h"

#include <EASTL/bonus/adaptors.h>
#include <EASTL/numeric.h>
#include <EASTL/optional.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

/// Return previous processed node in the chain.
const IKNode* GetPreviousNode(const IKNodeSegment* previousSegment, bool backward)
{
    if (!previousSegment)
        return nullptr;
    return backward ? previousSegment->endNode_ : previousSegment->beginNode_;
}

Vector3 IterateSegment(const IKSettings& settings,
    const IKNodeSegment& segment, const Vector3& target, bool backward)
{
    IKNode& targetNode = backward ? *segment.endNode_ : *segment.beginNode_;
    IKNode& adjustedNode = backward ? *segment.beginNode_ : *segment.endNode_;

    targetNode.position_ = target;

    const Vector3 direction = (adjustedNode.position_ - targetNode.position_).Normalized();
    adjustedNode.position_ = direction * segment.length_ + targetNode.position_;

    return adjustedNode.position_;
}

}

IKNode::IKNode(const Vector3& position, const Quaternion& rotation)
{
    SetOriginalTransform(position, rotation, Matrix3x4::IDENTITY);
}

void IKNode::SetOriginalTransform(
    const Vector3& position, const Quaternion& rotation, const Matrix3x4& inverseWorldTransform)
{
    localOriginalPosition_ = inverseWorldTransform * position;
    localOriginalRotation_ = inverseWorldTransform.Rotation() * rotation;
    originalPosition_ = position;
    originalRotation_ = rotation;
    position_ = position;
    rotation_ = rotation;
    previousPosition_ = position;
    previousRotation_ = rotation;
}

void IKNode::UpdateOriginalTransform(const Matrix3x4& worldTransform)
{
    originalPosition_ = worldTransform * localOriginalPosition_;
    originalRotation_ = worldTransform.Rotation() * localOriginalRotation_;
}

void IKNode::RotateAround(const Vector3& point, const Quaternion& rotation)
{
    position_ = rotation * (position_ - point) + point;
    rotation_ = rotation * rotation_;
    rotationDirty_ = true;
}

void IKNode::StorePreviousTransform()
{
    previousPosition_ = position_;
    previousRotation_ = rotation_;
    positionDirty_ = false;
    rotationDirty_ = false;
}

void IKNode::ResetOriginalTransform()
{
    position_ = originalPosition_;
    rotation_ = originalRotation_;
}

Quaternion IKNodeSegment::CalculateRotationDeltaFromPrevious() const
{
    const Vector3 previousDirection = endNode_->previousPosition_ - beginNode_->previousPosition_;
    const Vector3 currentDirection = endNode_->position_ - beginNode_->position_;
    return Quaternion{previousDirection, currentDirection};
}

Quaternion IKNodeSegment::CalculateRotationDeltaFromOriginal() const
{
    const Vector3 originalDirection = endNode_->originalPosition_ - beginNode_->originalPosition_;
    const Vector3 currentDirection = endNode_->position_ - beginNode_->position_;
    return Quaternion{originalDirection, currentDirection};
}

Quaternion IKNodeSegment::CalculateRotation(const IKSettings& settings) const
{
    if (settings.continuousRotations_)
    {
        const Quaternion delta = CalculateRotationDeltaFromPrevious();
        return delta * beginNode_->previousRotation_;
    }
    else
    {
        const Quaternion delta = CalculateRotationDeltaFromOriginal();
        return delta * beginNode_->originalRotation_;
    }
}

Vector3 IKNodeSegment::CalculateDirection() const
{
    return (endNode_->position_ - beginNode_->position_).Normalized();
}

void IKNodeSegment::UpdateLength()
{
    length_ = (endNode_->position_ - beginNode_->position_).Length();
}

void IKNodeSegment::UpdateRotationInNodes(bool fromPrevious, bool isLastSegment)
{
    if (fromPrevious)
    {
        const Quaternion delta = CalculateRotationDeltaFromPrevious();
        beginNode_->rotation_ = delta * beginNode_->previousRotation_;

        if (isLastSegment)
            endNode_->rotation_ = delta * endNode_->previousRotation_;
    }
    else
    {
        const Quaternion delta = CalculateRotationDeltaFromOriginal();
        beginNode_->rotation_ = delta * beginNode_->originalRotation_;

        if (isLastSegment)
            endNode_->rotation_ = delta * endNode_->originalRotation_;
    }

    beginNode_->MarkRotationDirty();
    endNode_->MarkRotationDirty();
}

void IKNodeSegment::Twist(float angle, bool isLastSegment)
{
    const Quaternion rotation{angle, CalculateDirection()};
    beginNode_->rotation_ = rotation * beginNode_->rotation_;
    if (isLastSegment)
        endNode_->rotation_ = rotation * endNode_->rotation_;

    beginNode_->MarkRotationDirty();
    endNode_->MarkRotationDirty();
}

void IKTrigonometricChain::Initialize(IKNode* node1, IKNode* node2, IKNode* node3)
{
    segments_[0] = IKNodeSegment{node1, node2};
    segments_[1] = IKNodeSegment{node2, node3};
}

void IKTrigonometricChain::UpdateLengths()
{
    segments_[0].UpdateLength();
    segments_[1].UpdateLength();
}

Quaternion IKTrigonometricChain::CalculateRotation(
    const Vector3& originalPos0, const Vector3& originalPos2, const Vector3& originalDirection,
    const Vector3& currentPos0, const Vector3& currentPos2, const Vector3& currentDirection)
{
    // Calculate swing
    const Vector3 originalTargetDirection = (originalPos2 - originalPos0).Normalized();
    const Vector3 currentTargetDirection = (currentPos2 - currentPos0).Normalized();
    const Quaternion swing{originalTargetDirection, currentTargetDirection};

    // Calculate twist
    const Vector3 originalBendDirection = (swing * originalDirection).Orthogonalize(currentTargetDirection);
    const Vector3 currentBendDirection = currentDirection.Orthogonalize(currentTargetDirection);
    const Quaternion bendDirectionDelta{originalBendDirection, currentBendDirection};
    const auto [_, twist] = bendDirectionDelta.ToSwingTwist(currentTargetDirection);

    return twist * swing;
}

ea::pair<Vector3, Vector3> IKTrigonometricChain::Solve(const Vector3& pos0, float len01, float len12,
    const Vector3& target, const Vector3& bendDirection, float minAngle, float maxAngle)
{
    const float minLen02 = Sqrt(len01 * len01 + len12 * len12 - 2 * len01 * len12 * Cos(minAngle));
    const float maxLen02 = Sqrt(len01 * len01 + len12 * len12 - 2 * len01 * len12 * Cos(maxAngle));
    const float len02 = Clamp((target - pos0).Length(), minLen02, maxLen02);
    const Vector3 newPos2 = (target - pos0).ReNormalized(len02, len02) + pos0;

    const Vector3 firstAxis = (newPos2 - pos0).NormalizedOrDefault(Vector3::DOWN);
    const Vector3 secondAxis = bendDirection.Orthogonalize(firstAxis);

    // This is angle between begin-to-middle and begin-to-end vectors.
    const float cosAngle = Clamp((len01 * len01 + len02 * len02 - len12 * len12) / (2.0f * len01 * len02), -1.0f, 1.0f);
    const float sinAngle = Sqrt(1.0f - cosAngle * cosAngle);

    const Vector3 newPos1 = pos0 + (firstAxis * cosAngle + secondAxis * sinAngle) * len01;
    return {newPos1, newPos2};
}

void IKTrigonometricChain::Solve(const Vector3& target, const Vector3& originalDirection,
    const Vector3& currentDirection, float minAngle, float maxAngle)
{
    ResetChainToOriginal();

    // Solve chain positions
    const Vector3 pos0 = segments_[0].beginNode_->position_;
    const float len01 = segments_[0].length_;
    const float len12 = segments_[1].length_;
    const auto [newPos1, newPos2] = Solve(pos0, len01, len12, target, currentDirection, minAngle, maxAngle);

    // Calculate base chain rotation
    currentChainRotation_ = CalculateRotation(
        pos0, segments_[1].endNode_->position_, originalDirection, pos0, target, currentDirection);

    segments_[0].beginNode_->RotateAround(pos0, currentChainRotation_);
    segments_[1].beginNode_->RotateAround(pos0, currentChainRotation_);
    segments_[1].endNode_->RotateAround(pos0, currentChainRotation_);

    // Rotate segments in the chain
    const Quaternion firstSegmentRotation{segments_[0].CalculateDirection(), newPos1 - pos0};
    segments_[0].beginNode_->RotateAround(pos0, firstSegmentRotation);
    segments_[1].beginNode_->RotateAround(pos0, firstSegmentRotation);
    segments_[1].endNode_->RotateAround(pos0, firstSegmentRotation);

    const Quaternion secondSegmentRotation{segments_[1].CalculateDirection(), newPos2 - newPos1};
    segments_[1].beginNode_->RotateAround(newPos1, secondSegmentRotation);
    segments_[1].endNode_->RotateAround(newPos1, secondSegmentRotation);

    segments_[1].beginNode_->position_ = newPos1;
    segments_[1].endNode_->position_ = newPos2;
}

void IKTrigonometricChain::ResetChainToOriginal()
{
    const Vector3 initialOffset = segments_[0].beginNode_->position_ - segments_[0].beginNode_->originalPosition_;

    segments_[0].beginNode_->ResetOriginalTransform();
    segments_[1].beginNode_->ResetOriginalTransform();
    segments_[1].endNode_->ResetOriginalTransform();

    segments_[0].beginNode_->position_ += initialOffset;
    segments_[1].beginNode_->position_ += initialOffset;
    segments_[1].endNode_->position_ += initialOffset;
}

void IKEyeChain::Initialize(IKNode* rootNode)
{
    rootNode_ = rootNode;
}

void IKEyeChain::SetLocalEyeTransform(const Vector3& eyeOffset, const Vector3& eyeDirection)
{
    eyeOffset_ = eyeOffset;
    eyeDirection_ = eyeDirection;
}

void IKEyeChain::SetWorldEyeTransform(const Vector3& eyeOffset, const Vector3& eyeDirection)
{
    eyeOffset_ = rootNode_->rotation_.Inverse() * eyeOffset;
    eyeDirection_ = rootNode_->rotation_.Inverse() * eyeDirection;
}

Quaternion IKEyeChain::SolveLookAt(const Vector3& lookAtTarget, const IKSettings& settings) const
{
    const Transform parentTransform{rootNode_->position_, rootNode_->rotation_};
    Transform newTransform = parentTransform;
    for (unsigned i = 0; i < settings.maxIterations_; ++i)
    {
        const Vector3 initialEyeDirection = newTransform.rotation_ * eyeDirection_;
        const Vector3 desiredEyeRotation = lookAtTarget - newTransform * eyeOffset_;
        const Quaternion rotation{initialEyeDirection, desiredEyeRotation};
        newTransform.rotation_ = rotation * newTransform.rotation_;
    }
    return newTransform.rotation_ * parentTransform.rotation_.Inverse();
}

Quaternion IKEyeChain::SolveLookTo(const Vector3& lookToDirection) const
{
    const Transform parentTransform{rootNode_->position_, rootNode_->rotation_};
    const Vector3 initialEyeDirection = parentTransform.rotation_ * eyeDirection_;
    return Quaternion{initialEyeDirection, lookToDirection};
}

void IKChain::Clear()
{
    nodes_.clear();
    segments_.clear();
}

void IKChain::AddNode(IKNode* node)
{
    nodes_.push_back(node);

    if (segments_.empty())
    {
        IKNodeSegment segment;
        segment.beginNode_ = node;
        segment.endNode_ = node;
        segments_.push_back(segment);
        isFirstSegmentIncomplete_ = true;
    }
    else if (isFirstSegmentIncomplete_)
    {
        segments_.back().endNode_ = node;
        isFirstSegmentIncomplete_ = false;
    }
    else
    {
        IKNodeSegment segment;
        segment.beginNode_ = segments_.back().endNode_;
        segment.endNode_ = node;
        segments_.push_back(segment);
    }
}

void IKChain::UpdateLengths()
{
    for (IKNodeSegment& segment : segments_)
        segment.UpdateLength();

    totalLength_ = ea::accumulate(segments_.begin(), segments_.end(), 0.0f,
        [](float sum, const IKNodeSegment& segment) { return sum + segment.length_; });
}

const IKNodeSegment* IKChain::FindSegment(const IKNode* node) const
{
    const auto isNodeReferenced = [&](const IKNodeSegment& segment)
    {
        return segment.beginNode_ == node || segment.endNode_ == node;
    };
    const auto iter = ea::find_if(segments_.begin(), segments_.end(), isNodeReferenced);
    return iter != segments_.end() ? &(*iter) : nullptr;
}

void IKChain::StorePreviousTransforms()
{
    if (segments_.empty())
        return;

    for (const IKNodeSegment& segment : segments_)
    {
        segment.beginNode_->previousPosition_ = segment.beginNode_->position_;
        segment.beginNode_->previousRotation_ = segment.beginNode_->rotation_;
    }

    IKNode& lastNode = *segments_.back().endNode_;
    lastNode.previousPosition_ = lastNode.position_;
    lastNode.previousRotation_ = lastNode.rotation_;
}

void IKChain::RestorePreviousTransforms()
{
    for (const IKNodeSegment& segment : segments_)
    {
        segment.beginNode_->position_ = segment.beginNode_->previousPosition_;
        segment.beginNode_->rotation_ = segment.beginNode_->previousRotation_;
    }

    IKNode& lastNode = *segments_.back().endNode_;
    lastNode.position_ = lastNode.previousPosition_;
    lastNode.rotation_ = lastNode.previousRotation_;
}

void IKChain::UpdateSegmentRotations(const IKSettings& settings)
{
    for (IKNodeSegment& segment : segments_)
    {
        const bool isLastSegment = &segment == &segments_.back();
        segment.UpdateRotationInNodes(settings.continuousRotations_, isLastSegment);
    }
}

void IKSpineChain::Solve(const Vector3& target, const Vector3& baseDirection,
    float maxRotation, const IKSettings& settings, const WeightFunction& weightFun)
{
    if (nodes_.size() < 2)
        return;

    StorePreviousTransforms();
    UpdateSegmentWeights(weightFun);

    const Vector3 basePosition = nodes_[0]->position_;
    const Vector3 bendDirection = GetProjectionAndOffset(
        target, basePosition, baseDirection).second.Normalized();

    //    Target
    //   /|
    //  / |
    // o--> Base Direction (= x axis)
    // ^
    // Base Position
    const Vector2 projectedTarget = ProjectTarget(target, basePosition, baseDirection);

    const float angularTolerance = settings.tolerance_ / totalLength_ * M_RADTODEG;
    const float totalAngle = FindBestAngle(projectedTarget, maxRotation, angularTolerance);
    EvaluateSegmentPositions(totalAngle, baseDirection, bendDirection);

    for (IKNodeSegment& segment : segments_)
    {
        const bool isLastSegment = &segment == &segments_.back();
        segment.UpdateRotationInNodes(true, isLastSegment);
    }
}

void IKSpineChain::Twist(float angle, const IKSettings& settings)
{
    if (segments_.size() < 2)
        return;

    float accumulatedAngle = 0.0f;
    for (size_t i = 0; i < segments_.size(); ++i)
    {
        accumulatedAngle += angle * weights_[i];
        segments_[i].Twist(accumulatedAngle, i == segments_.size() - 1);
    }
}

void IKSpineChain::UpdateSegmentWeights(const WeightFunction& weightFun)
{
    weights_.resize(segments_.size());

    ea::fill(weights_.begin(), weights_.end(), 0.0f);

    float totalWeight = 0.0f;
    for (unsigned i = 0; i < segments_.size(); ++i)
    {
        const float fract = static_cast<float>(i) / (segments_.size() - 1);
        const float weight = segments_[i].length_ * weightFun(fract);
        totalWeight += weight;
        weights_[i] = weight;
    }

    if (totalWeight > 0.0f)
    {
        for (unsigned i = 0; i < segments_.size(); ++i)
            weights_[i] /= totalWeight;
    }
    else
    {
        weights_[0] = 1.0f;
    }
}

ea::pair<float, Vector3> IKSpineChain::GetProjectionAndOffset(const Vector3& target,
    const Vector3& basePosition, const Vector3& baseDirection) const
{
    const Vector3 targetOffset = target - basePosition;
    const float projection = targetOffset.ProjectOntoAxis(baseDirection);
    const Vector3 normalOffset = targetOffset - baseDirection * projection;
    return {projection, normalOffset};
}

Vector2 IKSpineChain::ProjectTarget(const Vector3& target,
    const Vector3& basePosition, const Vector3& baseDirection) const
{
    const auto [projection, offset] = GetProjectionAndOffset(target, basePosition, baseDirection);
    return {projection, offset.Length()};
}

template <class T>
void IKSpineChain::EnumerateProjectedPositions(float totalRotation, const T& callback) const
{
    Vector2 position;
    float angle = 0.0f;
    for (unsigned i = 0; i < segments_.size(); ++i)
    {
        const float length = segments_[i].length_;
        const float rotation = totalRotation * weights_[i];

        angle += rotation;
        position += Vector2{Cos(angle), Sin(angle)} * length;
        callback(position);
    }
}

Vector2 IKSpineChain::EvaluateProjectedEnd(float totalRotation) const
{
    Vector2 position;
    EnumerateProjectedPositions(totalRotation, [&](const Vector2& pos) { position = pos; });
    return position;
}

float IKSpineChain::EvaluateError(float totalRotation, const Vector2& target) const
{
    const Vector2 endPosition = EvaluateProjectedEnd(totalRotation);
    const float error = (endPosition - target).LengthSquared();
    return error;
}

float IKSpineChain::FindBestAngle(const Vector2& projectedTarget, float maxRotation, float angularTolerance) const
{
    const auto errorFunction = [&](float angle) { return EvaluateError(angle, projectedTarget); };
    return SolveBisect(errorFunction, 0.0f, maxRotation, angularTolerance);
}

void IKSpineChain::EvaluateSegmentPositions(float totalRotation,
    const Vector3& baseDirection, const Vector3& bendDirection)
{
    const Vector3 basePosition = segments_[0].beginNode_->position_;
    unsigned segmentIndex = 0;
    EnumerateProjectedPositions(totalRotation, [&](const Vector2& position)
    {
        IKNodeSegment& segment = segments_[segmentIndex++];
        const Vector3 offset = baseDirection * position.x_ + bendDirection * position.y_;
        segment.endNode_->position_ = basePosition + offset;
    });
}

void IKFabrikChain::Solve(const Vector3& target, const IKSettings& settings)
{
    if (segments_.empty())
        return;

    const IKNode& targetNode = *segments_.back().endNode_;
    if ((targetNode.position_ - target).Length() < settings.tolerance_)
        return;

    StorePreviousTransforms();
    // Don't do more than one attempt for now
    TrySolve(target, settings);
    UpdateSegmentRotations(settings);
}

bool IKFabrikChain::TrySolve(const Vector3& target, const IKSettings& settings)
{
    const IKNode& targetNode = *segments_.back().endNode_;
    ea::optional<float> previousError;
    for (unsigned i = 0; i < settings.maxIterations_; ++i)
    {
        Vector3 startPosition = segments_.front().beginNode_->position_;
        SolveIteration(target, settings, true);
        SolveIteration(startPosition, settings, false);

        const float error = (targetNode.position_ - target).Length();
        if (previousError && error >= *previousError)
            return false;
        previousError = error;

        if ((targetNode.position_ - target).Length() < settings.tolerance_)
            break;
    }
    return true;
}

void IKFabrikChain::SolveIteration(const Vector3& target, const IKSettings& settings, bool backward)
{
    Vector3 nextPosition = target;
    if (backward)
    {
        for (const IKNodeSegment& segment : ea::reverse(segments_))
            nextPosition = IterateSegment(settings, segment, nextPosition, backward);
    }
    else
    {
        for (const IKNodeSegment& segment : segments_)
            nextPosition = IterateSegment(settings, segment, nextPosition, backward);
    }
}

}
