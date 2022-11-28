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

#include <EASTL/bonus/adaptors.h>
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

void IKNode::InitializeTransform(const Vector3& position, const Quaternion& rotation)
{
    originalPosition_ = position;
    originalRotation_ = rotation;
    position_ = position;
    rotation_ = rotation;
    previousPosition_ = position;
    previousRotation_ = rotation;
}

void IKNode::StorePreviousTransform()
{
    previousPosition_ = position_;
    previousRotation_ = rotation_;
    positionDirty_ = false;
    rotationDirty_ = false;
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

void IKNodeSegment::UpdateRotationInNodes(const IKSettings& settings, bool isLastSegment)
{
    if (settings.continuousRotations_)
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

ea::pair<Vector3, Vector3> IKTrigonometricChain::Solve(const Vector3& pos0, float len01, float len12,
    const Vector3& target, const Vector3& bendNormal, float minAngle, float maxAngle)
{
    const float minLen02 = Sqrt(len01 * len01 + len12 * len12 - 2 * len01 * len12 * Cos(minAngle));
    const float maxLen02 = Sqrt(len01 * len01 + len12 * len12 - 2 * len01 * len12 * Cos(maxAngle));
    const float len02 = Clamp((target - pos0).Length(), minLen02, maxLen02);
    const Vector3 newPos2 = (target - pos0).ReNormalized(len02, len02) + pos0;

    const Vector3 firstAxis = (newPos2 - pos0).NormalizedOrDefault(Vector3::DOWN);
    const Vector3 secondAxis = firstAxis.CrossProduct(bendNormal).NormalizedOrDefault(Vector3::RIGHT);

    // This is angle between begin-to-middle and begin-to-end vectors.
    const float cosAngle = Clamp((len01 * len01 + len02 * len02 - len12 * len12) / (2.0f * len01 * len02), -1.0f, 1.0f);
    const float sinAngle = Sqrt(1.0f - cosAngle * cosAngle);

    const Vector3 newPos1 = pos0 + (firstAxis * cosAngle + secondAxis * sinAngle) * len01;
    return {newPos1, newPos2};
}

void IKTrigonometricChain::Solve(const Vector3& target, const Vector3& bendNormal,
    float minAngle, float maxAngle, const IKSettings& settings)
{
    const Vector3 pos0 = segments_[0].beginNode_->position_;
    const float len01 = segments_[0].length_;
    const float len12 = segments_[1].length_;

    const auto [newPos1, newPos2] = Solve(pos0, len01, len12, target, bendNormal, minAngle, maxAngle);

    segments_[1].beginNode_->position_ = newPos1;
    segments_[1].endNode_->position_ = newPos2;
    //segments_[1].endNode_->position_ = (newPos2 - newPos1).ReNormalized(len12, len12) + newPos1;

    segments_[0].UpdateRotationInNodes(settings, false);
    segments_[1].UpdateRotationInNodes(settings, true);
}

void IKChain::Clear()
{
    segments_.clear();
}

void IKChain::AddNode(IKNode* node)
{
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
}

void IKChain::StorePreviousTransforms()
{
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
        segment.UpdateRotationInNodes(settings, isLastSegment);
    }
}

void IKSpineChain::Solve(const Vector3& target, float maxRotation, const IKSettings& settings)
{
    if (segments_.size() < 2)
        return;

    StorePreviousTransforms();
    UpdateSegmentWeights();

    const Vector3 basePosition = segments_[0].beginNode_->position_;
    const Vector3 baseDirection = segments_[0].CalculateDirection();
    const Vector3 bendDirection = GetProjectionAndOffset(
        target, basePosition, baseDirection).second.Normalized();

    //    Target
    //   /|
    //  / |
    // o--> Base Direction (= x axis)
    // ^
    // Base Position
    const Vector2 projectedTarget = ProjectTarget(target, basePosition, baseDirection);

    // TODO: Calculate number of iterations based on error.
    const float totalAngle = FindBestAngle(projectedTarget, maxRotation, settings.maxIterations_);

    EvaluateSegmentPositions(totalAngle, baseDirection, bendDirection);
    UpdateSegmentRotations(settings);
}

void IKSpineChain::UpdateSegmentWeights()
{
    weights_.resize(segments_.size());
    ea::fill(weights_.begin(), weights_.end(), 0.0f);

    float totalWeight = 0.0f;
    for (unsigned i = 1; i < segments_.size(); ++i)
    {
        const float length = segments_[i].length_;
        totalWeight += length;
        weights_[i] = length;
    }

    if (totalWeight > 0.0f)
    {
        for (unsigned i = 1; i < segments_.size(); ++i)
            weights_[i] /= totalWeight;
    }
    else
    {
        weights_[1] = 1.0f;
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

IKSpineChain::AngleAndError IKSpineChain::EvaluateError(
    float totalRotation, const Vector2& target) const
{
    const Vector2 endPosition = EvaluateProjectedEnd(totalRotation);
    const float error = (endPosition - target).LengthSquared();
    return {totalRotation, error};
}

float IKSpineChain::FindBestAngle(
    const Vector2& projectedTarget, float maxRotation, unsigned maxIterations) const
{
    AngleAndError begin = EvaluateError(0.0f, projectedTarget);
    AngleAndError end = EvaluateError(maxRotation, projectedTarget);
    AngleAndError middle;

    for (unsigned i = 0; i < maxIterations; ++i)
    {
        middle = EvaluateError((begin.angle_ + end.angle_) * 0.5f, projectedTarget);

        // This should not happen most of the time
        if (middle.error_ >= begin.error_ && middle.error_ >= end.error_)
            break;

        if (begin.error_ >= middle.error_ && begin.error_ >= end.error_)
            begin = middle;
        else
            end = middle;
    }

    return middle.angle_;
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
    StorePreviousTransforms();

    if (!TrySolve(target, settings))
    {
        bool solved = false;
        const Vector3 rotationAxis = GetDeadlockRotationAxis(target);
        for (float angle = 0.0f; angle < 360.0f; angle += 10.0f)
        {
            RestorePreviousTransforms();
            RotateChain(Quaternion{angle, rotationAxis});
            if (TrySolve(target, settings))
            {
                solved = true;
                break;
            }
        }
        if (!solved)
        {
            // TODO: Handle fail?
            //URHO3D_LOGERROR("Failed to solve IK chain");
            return;
        }
    }

    UpdateSegmentRotations(settings);
}

Vector3 IKFabrikChain::GetDeadlockRotationAxis(const Vector3& target) const
{
    const Vector3& beginPosition = segments_.front().beginNode_->position_;
    const Vector3& endPosition = segments_.back().endNode_->previousPosition_;
    const Vector3 directionToEnd = (endPosition - beginPosition).Normalized();
    const Vector3 directionToTarget = (target - beginPosition).Normalized();
    return directionToEnd.CrossProduct(directionToTarget).Normalized();
}

void IKFabrikChain::RotateChain(const Quaternion& rotation)
{
    for (IKNodeSegment& segment : segments_)
        RotateChainNode(*segment.beginNode_, rotation);
    RotateChainNode(*segments_.back().endNode_, rotation);
}

void IKFabrikChain::RotateChainNode(IKNode& node, const Quaternion& rotation) const
{
    const Vector3& origin = segments_.front().beginNode_->position_;

    node.rotation_ = rotation * node.rotation_;
    node.position_ = rotation * (node.position_ - origin) + origin;
}

bool IKFabrikChain::TrySolve(const Vector3& target, const IKSettings& settings)
{
    ea::optional<float> previousError;
    for (unsigned i = 0; i < settings.maxIterations_; ++i)
    {
        Vector3 startPosition = segments_.front().beginNode_->position_;
        SolveIteration(target, settings, true);
        SolveIteration(startPosition, settings, false);

        const Vector3 endPosition = segments_.back().endNode_->position_;
        const float error = (endPosition - target).Length();
        if (previousError && error >= *previousError)
            return false;
        previousError = error;

        if ((endPosition - target).Length() < settings.tolerance_)
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
