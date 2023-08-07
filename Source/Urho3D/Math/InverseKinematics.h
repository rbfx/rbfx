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

#pragma once

#include "../Math/Matrix3x4.h"
#include "../Math/Vector3.h"
#include "../Math/Quaternion.h"

#include <EASTL/functional.h>
#include <EASTL/span.h>

namespace Urho3D
{

struct IKNodeSegment;

/// Aggregated settings of the IK solver.
struct URHO3D_API IKSettings
{
    unsigned maxIterations_{10};
    float tolerance_{0.001f};
    /// Whether to consider node rotations from the previous frame when solving.
    /// Results in smoother motion, but may cause rotation bleeding over time.
    bool continuousRotations_{};
};

/// Singular node of the IK chain.
/// Node should be used only once within a chain.
/// However, node may belong to multiple independet chains.
/// Positions and rotations are in world space.
struct URHO3D_API IKNode
{
    Vector3 localOriginalPosition_;
    Quaternion localOriginalRotation_;

    Vector3 originalPosition_;
    Quaternion originalRotation_;

    Vector3 previousPosition_;
    Quaternion previousRotation_;

    Vector3 position_;
    Quaternion rotation_;

    bool positionDirty_{};
    bool rotationDirty_{};

    IKNode() = default;
    IKNode(const Vector3& position, const Quaternion& rotation);

    void SetOriginalTransform(const Vector3& position, const Quaternion& rotation, const Matrix3x4& inverseWorldTransform);
    void UpdateOriginalTransform(const Matrix3x4& worldTransform);

    void RotateAround(const Vector3& point, const Quaternion& rotation);
    void ResetOriginalTransform();
    void StorePreviousTransform();
    void MarkPositionDirty() { positionDirty_ = true; }
    void MarkRotationDirty() { rotationDirty_ = true; }
};

/// Fixed-length segment that consists of two nodes.
struct URHO3D_API IKNodeSegment
{
    IKNode* beginNode_{};
    IKNode* endNode_{};
    float length_{};

    /// Calculate rotation delta from positions, using previous positions as reference.
    Quaternion CalculateRotationDeltaFromPrevious() const;
    /// Calculate rotation delta from positions, using original positions as reference.
    Quaternion CalculateRotationDeltaFromOriginal() const;
    /// Calculate current rotation.
    Quaternion CalculateRotation(const IKSettings& settings) const;
    /// Calculate current direction.
    Vector3 CalculateDirection() const;

    /// Update cached length.
    void UpdateLength();
    /// Update current rotation for nodes.
    void UpdateRotationInNodes(bool fromPrevious, bool isLastSegment);
    /// Twist the segment around its direction.
    void Twist(float angle, bool isLastSegment);
};

/// Trigonometric two-segment IK chain.
class URHO3D_API IKTrigonometricChain
{
public:
    void Initialize(IKNode* node1, IKNode* node2, IKNode* node3);
    void UpdateLengths();

    void Solve(const Vector3& target, const Vector3& originalDirection,
        const Vector3& currentDirection, float minAngle, float maxAngle);

    /// Return rotation of the entire chain.
    static Quaternion CalculateRotation(
        const Vector3& originalPos0, const Vector3& originalPos2, const Vector3& originalDirection,
        const Vector3& currentPos0, const Vector3& currentPos2, const Vector3& currentDirection);
    /// Return positions of second and third bones.
    static ea::pair<Vector3, Vector3> Solve(const Vector3& pos0, float len01, float len12,
        const Vector3& target, const Vector3& bendDirection, float minAngle, float maxAngle);

    IKNode* GetBeginNode() const { return segments_[0].beginNode_; }
    IKNode* GetMiddleNode() const { return segments_[1].beginNode_; }
    IKNode* GetEndNode() const { return segments_[1].endNode_; }
    float GetFirstLength() const { return segments_[0].length_; }
    float GetSecondLength() const { return segments_[1].length_; }
    Quaternion GetCurrentChainRotation() const { return currentChainRotation_; }

private:
    void ResetChainToOriginal();

    IKNodeSegment segments_[2];
    Quaternion currentChainRotation_;
};

/// Two-segment IK chain for eyes.
class URHO3D_API IKEyeChain
{
public:
    void Initialize(IKNode* rootNode);
    void SetLocalEyeTransform(const Vector3& eyeOffset, const Vector3& eyeDirection);
    void SetWorldEyeTransform(const Vector3& eyeOffset, const Vector3& eyeDirection);

    Quaternion SolveLookAt(const Vector3& lookAtTarget, const IKSettings& settings) const;
    Quaternion SolveLookTo(const Vector3& lookToDirection) const;

    const Vector3& GetLocalEyeOffset() const { return eyeOffset_; }
    const Vector3& GetLocalEyeDirection() const { return eyeDirection_; }

private:
    IKNode* rootNode_{};
    Vector3 eyeOffset_;
    Vector3 eyeDirection_;
};

/// Base class for generic IK chain.
class URHO3D_API IKChain
{
public:
    void Clear();
    void AddNode(IKNode* node);

    void UpdateLengths();

    const IKNodeSegment* FindSegment(const IKNode* node) const;
    ea::vector<IKNodeSegment>& GetSegments() { return segments_; }
    const ea::vector<IKNodeSegment>& GetSegments() const { return segments_; }
    ea::vector<IKNode*>& GetNodes() { return nodes_; }
    const ea::vector<IKNode*>& GetNodes() const { return nodes_; }

protected:
    void StorePreviousTransforms();
    void RestorePreviousTransforms();
    void UpdateSegmentRotations(const IKSettings& settings);

    bool isFirstSegmentIncomplete_{};
    ea::vector<IKNode*> nodes_;
    ea::vector<IKNodeSegment> segments_;
    float totalLength_{};
};

/// Uniformly bending IK chain.
class URHO3D_API IKSpineChain : public IKChain
{
public:
    using WeightFunction = ea::function<float(float)>;
    static float DefaultWeightFunction(float x) { return 1.0f; }

    void Solve(const Vector3& target, const Vector3& baseDirection,
        float maxRotation, const IKSettings& settings, const WeightFunction& weightFun = DefaultWeightFunction);
    void Twist(float angle, const IKSettings& settings);

private:
    void UpdateSegmentWeights(const WeightFunction& weightFun);

    template <class T>
    void EnumerateProjectedPositions(float totalRotation, const T& callback) const;
    ea::pair<float, Vector3> GetProjectionAndOffset(const Vector3& target,
        const Vector3& basePosition, const Vector3& baseDirection) const;
    Vector2 ProjectTarget(const Vector3& target,
        const Vector3& basePosition, const Vector3& baseDirection) const;
    Vector2 EvaluateProjectedEnd(float totalRotation) const;
    float EvaluateError(float totalRotation, const Vector2& target) const;
    float FindBestAngle(const Vector2& projectedTarget, float maxRotation, float angularTolerance) const;

    void EvaluateSegmentPositions(float totalRotation,
        const Vector3& baseDirection, const Vector3& bendDirection);

    ea::vector<float> weights_;
};

/// Generic unconstrained FABRIK chain.
class URHO3D_API IKFabrikChain : public IKChain
{
public:
    void Solve(const Vector3& target, const IKSettings& settings);

private:
    void SolveIteration(const Vector3& target, const IKSettings& settings, bool backward);
    bool TrySolve(const Vector3& target, const IKSettings& settings);
};

/// Solve error function f(x) for minimum value using bisection method.
template <class T, class U>
U SolveBisect(const T& f, U minValue, U maxValue, U tolerance, unsigned maxIterations = 100)
{
    using ValueAndError = ea::pair<U, double>;

    const float approximateNumSteps = Ceil(Ln(tolerance / (maxValue - minValue + M_EPSILON)) / Ln(0.5f));
    const auto numSteps = static_cast<unsigned>(Clamp(approximateNumSteps, 1.0f, static_cast<float>(maxIterations)));

    ValueAndError begin{minValue, static_cast<double>(f(minValue))};
    ValueAndError end{maxValue, static_cast<double>(f(maxValue))};

    for (unsigned i = 0; i < numSteps; ++i)
    {
        const auto middleValue = static_cast<U>((begin.first + end.first) / 2);
        const ValueAndError middle{middleValue, static_cast<double>(f(middleValue))};

        if (middle.second >= begin.second && middle.second >= end.second)
            break;

        if (begin.second >= middle.second && begin.second >= end.second)
            begin = middle;
        else
            end = middle;

        // If the error is small enough, we can stop
        if (end.first - begin.first < 2 * tolerance)
            break;
    }

    return static_cast<U>((begin.first + end.first) / 2);
}

}
