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

void IKTrigonometricChain::Solve(const Vector3& target, const Vector3& bendNormal,
    float minAngle, float maxAngle, const IKSettings& settings)
{
    const Vector3 oldPos0 = segments_[0].beginNode_->position_;
    const Vector3 oldPos1 = segments_[1].beginNode_->position_;
    const Vector3 oldPos2 = segments_[1].endNode_->position_;

    const float len01 = segments_[0].length_;
    const float len12 = segments_[1].length_;
    const float minLen02 = Sqrt(len01 * len01 + len12 * len12 - 2 * len01 * len12 * Cos(minAngle));
    const float maxLen02 = Sqrt(len01 * len01 + len12 * len12 - 2 * len01 * len12 * Cos(maxAngle));
    const float len02 = Clamp((target - oldPos0).Length(), minLen02, maxLen02);
    const Vector3 newPos2 = (target - oldPos0).ReNormalized(len02, len02) + oldPos0;

    const Vector3 firstAxis = (newPos2 - oldPos0).NormalizedOrDefault(Vector3::DOWN);
    const Vector3 secondAxis = firstAxis.CrossProduct(bendNormal).NormalizedOrDefault(Vector3::RIGHT);

    // This is angle between begin-to-middle and begin-to-end vectors.
    const float cosAngle = Clamp((len01 * len01 + len02 * len02 - len12 * len12) / (2.0f * len01 * len02), -1.0f, 1.0f);
    const float sinAngle = Sqrt(1.0f - cosAngle * cosAngle);

    const Vector3 newPos1 = oldPos0 + (firstAxis * cosAngle + secondAxis * sinAngle) * len01;

    segments_[1].beginNode_->position_ = newPos1;
    segments_[1].endNode_->position_ = (newPos2 - newPos1).ReNormalized(len12, len12) + newPos1;

    segments_[0].UpdateRotationInNodes(settings, false);
    segments_[1].UpdateRotationInNodes(settings, true);
}

void IKFabrikChain::Clear()
{
    segments_.clear();
}

void IKFabrikChain::AddNode(IKNode* node)
{
    if (segments_.empty())
    {
        IKNodeSegment segment;
        segment.beginNode_ = node;
        segments_.push_back(segment);
    }
    else if (segments_.size() == 1 && segments_.back().endNode_ == nullptr)
    {
        segments_.back().endNode_ = node;
    }
    else
    {
        IKNodeSegment segment;
        segment.beginNode_ = segments_.back().endNode_;
        segment.endNode_ = node;
        segments_.push_back(segment);
    }
}

void IKFabrikChain::UpdateLengths()
{
    for (IKNodeSegment& segment : segments_)
        segment.UpdateLength();
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

    for (IKNodeSegment& segment : segments_)
    {
        const bool isLastSegment = &segment == &segments_.back();
        segment.UpdateRotationInNodes(settings, isLastSegment);
    }
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

void IKFabrikChain::StorePreviousTransforms()
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

void IKFabrikChain::RestorePreviousTransforms()
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
