//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../IK/IKSolverComponent.h"

#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../IK/IKSolver.h"
#include "../IO/Log.h"
#include "../Math/Sphere.h"
#include "../Scene/Node.h"
#include "../Scene/SceneEvents.h"

namespace Urho3D
{

namespace
{

/// Given two sides and the angle opposite to the first side,
/// calculate the (smallest) angle opposite to the second side.
ea::optional<float> SolveAmbiguousTriangle(float sideAB, float sideBC, float angleACB)
{
    const float sinAngleBAC = sideBC * Sin(angleACB) / sideAB;
    if (sinAngleBAC > 1.0f)
        return ea::nullopt;

    // Take smallest angle, BAC>90 is not realistic for solving foot.
    return Asin(sinAngleBAC);
}

float GetTriangleAngle(float sideAB, float sideBC, float sideAC)
{
    return Acos((sideAB * sideAB + sideBC * sideBC - sideAC * sideAC) / (2.0f * sideAB * sideBC));
}

float GetMaxDistance(const IKTrigonometricChain& chain, float maxAngle)
{
    const float a = chain.GetFirstLength();
    const float b = chain.GetSecondLength();
    return Sqrt(a * a + b * b - 2 * a * b * Cos(maxAngle));
}

Vector3 InterpolateDirection(const Vector3& from, const Vector3& to, float t)
{
    const Quaternion rotation{from, to};
    return Quaternion::IDENTITY.Slerp(rotation, t) * from;
}

float GetThighToHeelDistance(float thighToToeDistance, float toeToHeelDistance,
    float heelAngle, float maxDistance)
{
    // A - thigh position
    // .|
    // .|
    // . |
    // . |
    // .  |
    // .__|
    // B  C - heel position
    // ^
    // toe position
    const auto thighAngle = SolveAmbiguousTriangle(thighToToeDistance, toeToHeelDistance, heelAngle);
    if (!thighAngle)
        return ea::min(thighToToeDistance + toeToHeelDistance, maxDistance);

    const float toeAngle = 180 - heelAngle - *thighAngle;
    const float distance = thighToToeDistance * Sin(toeAngle) / Sin(heelAngle);
    return ea::min(distance, maxDistance);
}

Vector3 GetToeToHeel(const Vector3& thighPosition, const Vector3& toePosition, float toeToHeelDistance,
    float heelAngle, float maxDistance, const Vector3& bendNormal)
{
    const float thighToToeDistance = (toePosition - thighPosition).Length();
    const float thighToHeelDistance = GetThighToHeelDistance(
        thighToToeDistance, toeToHeelDistance, heelAngle, maxDistance);
    const float toeAngle = GetTriangleAngle(thighToToeDistance, toeToHeelDistance, thighToHeelDistance);

    const Vector3 toeToThigh = (thighPosition - toePosition).Normalized();
    const Quaternion rotation{toeAngle, bendNormal};
    return (rotation * toeToThigh).Normalized() * toeToHeelDistance;
}

void DrawNode(DebugRenderer* debug, bool oriented,
    const Vector3& position, const Quaternion& rotation, const Color& color, float radius)
{
    static const BoundingBox box{-1.0f, 1.0f};

    if (!oriented)
        debug->AddSphere(Sphere(position, radius), color, false);
    else
    {
        const Matrix3x4 transform{position, rotation, Vector3::ONE * radius};
        debug->AddBoundingBox(box, transform, color, false);
    }
}

Quaternion MixRotation(const Quaternion& from, const Quaternion& to, float factor)
{
    if (factor <= 0.0f)
        return from;
    else if (factor >= 1.0f)
        return to;
    else
        return from.Slerp(to, factor);
}

}

IKSolverComponent::IKSolverComponent(Context* context)
    : Component(context)
{
}

IKSolverComponent::~IKSolverComponent()
{
}

void IKSolverComponent::RegisterObject(Context* context)
{
    context->AddAbstractReflection<IKSolverComponent>(Category_IK);
}

void IKSolverComponent::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (previousNode)
    {
        if (auto solver = previousNode->GetComponent<IKSolver>())
            solver->MarkSolversDirty();
    }
    if (currentNode)
    {
        if (auto solver = currentNode->GetComponent<IKSolver>())
            solver->MarkSolversDirty();
    }
}

bool IKSolverComponent::Initialize(IKNodeCache& nodeCache)
{
    solverNodes_.clear();
    return InitializeNodes(nodeCache);
}

Transform IKSolverComponent::GetFrameOfReferenceTransform() const
{
    if (frameOfReferenceNode_)
        return {frameOfReferenceNode_->GetWorldPosition(), frameOfReferenceNode_->GetWorldRotation()};
    else
        return {};
}

void IKSolverComponent::NotifyPositionsReady()
{
    const Transform frameOfReference = GetFrameOfReferenceTransform();
    UpdateChainLengths(frameOfReference.Inverse());
}

void IKSolverComponent::Solve(const IKSettings& settings)
{
    for (const auto& [node, solverNode] : solverNodes_)
    {
        solverNode->position_ = node->GetWorldPosition();
        solverNode->rotation_ = node->GetWorldRotation();
        solverNode->StorePreviousTransform();
    }

    const Transform frameOfReference = GetFrameOfReferenceTransform();
    SolveInternal(frameOfReference, settings);

    for (const auto& [node, solverNode] : solverNodes_)
    {
        if (solverNode->positionDirty_)
            node->SetWorldPosition(solverNode->position_);
        if (solverNode->rotationDirty_)
            node->SetWorldRotation(solverNode->rotation_);
    }
}

void IKSolverComponent::OnTreeDirty()
{
    if (auto solver = GetComponent<IKSolver>())
        solver->MarkSolversDirty();
}

IKNode* IKSolverComponent::AddSolverNode(IKNodeCache& nodeCache, const ea::string& name)
{
    if (name.empty())
        return nullptr;

    Node* boneNode = node_->GetChild(name, true);
    if (!boneNode)
    {
        URHO3D_LOGERROR("IKSolverComponent: Bone node '{}' is not found", name);
        return nullptr;
    }

    IKNode& solverNode = nodeCache.emplace(WeakPtr<Node>(boneNode), IKNode{}).first->second;

    solverNodes_.emplace_back(boneNode, &solverNode);
    return &solverNode;
}

Node* IKSolverComponent::AddCheckedNode(IKNodeCache& nodeCache, const ea::string& name) const
{
    if (name.empty())
        return nullptr;

    Node* boneNode = node_->GetChild(name, true);
    if (!boneNode)
    {
        URHO3D_LOGERROR("IKSolverComponent: Bone node '{}' is not found", name);
        return nullptr;
    }

    nodeCache.emplace(WeakPtr<Node>{boneNode}, IKNode{});
    return boneNode;
}

Node* IKSolverComponent::FindNode(const IKNode& node) const
{
    for (const auto& [sceneNode, ikNode] : solverNodes_)
    {
        if (ikNode == &node)
            return sceneNode;
    }
    return nullptr;
}

void IKSolverComponent::SetFrameOfReference(Node* node)
{
    if (!node || !(node == node_ || node->IsChildOf(node_)))
    {
        URHO3D_LOGERROR("IKSolverComponent has invalid frame of reference");
        return;
    }

    frameOfReferenceNode_ = node;
}

void IKSolverComponent::SetFrameOfReference(const IKNode& node)
{
    SetFrameOfReference(FindNode(node));
}

void IKSolverComponent::SetParentAsFrameOfReference(const IKNode& childNode)
{
    auto node = FindNode(childNode);
    auto parent = node ? node->GetParent() : nullptr;
    SetFrameOfReference(parent);
}

void IKSolverComponent::DrawIKNode(DebugRenderer* debug, const IKNode& node, bool oriented) const
{
    static const float radius = 0.02f;
    static const Color color = Color::YELLOW;

    DrawNode(debug, oriented, node.position_, node.rotation_, color, radius);
}

void IKSolverComponent::DrawIKSegment(DebugRenderer* debug, const IKNode& beginNode, const IKNode& endNode) const
{
    static const Color color = Color::YELLOW;

    debug->AddLine(beginNode.position_, endNode.position_, color, false);
}

void IKSolverComponent::DrawIKTarget(DebugRenderer* debug, const Node* node, bool oriented) const
{
    static const float radius = 0.05f;
    static const Color color = Color::GREEN;

    DrawNode(debug, oriented, node->GetWorldPosition(), node->GetWorldRotation(), color, radius);
}

void IKSolverComponent::DrawDirection(
    DebugRenderer* debug, const Vector3& position, const Vector3& direction, bool markBegin, bool markEnd) const
{
    const float radius = 0.02f;
    const float length = 0.1f;
    static const Color color = Color::GREEN;

    const Vector3 endPosition = position + direction * length;
    if (markBegin)
        debug->AddSphere(Sphere(position, radius), Color::GREEN, false);
    debug->AddLine(position, endPosition, Color::GREEN, false);
    if (markEnd)
        debug->AddSphere(Sphere(endPosition, radius), Color::GREEN, false);
}

IKChainSolver::IKChainSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKChainSolver::~IKChainSolver()
{
}

void IKChainSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKChainSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Bone Names", StringVector, boneNames_, OnTreeDirty, Variant::emptyStringVector, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
}

void IKChainSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    const auto& segments = chain_.GetSegments();
    for (const IKNodeSegment& segment : segments)
    {
        const bool isLastSegment = &segment == &segments.back();
        DrawIKNode(debug, *segment.beginNode_, isLastSegment);
        DrawIKSegment(debug, *segment.beginNode_, *segment.endNode_);
        if (isLastSegment)
            DrawIKNode(debug, *segment.endNode_, false);
    }

    if (target_)
        DrawIKTarget(debug, target_, false);
}

bool IKChainSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    if (boneNames_.size() < 2)
        return false;

    IKFabrikChain chain;
    for (const ea::string& boneName : boneNames_)
    {
        IKNode* boneNode = AddSolverNode(nodeCache, boneName);
        if (!boneNode)
            return false;

        chain.AddNode(boneNode);
    }

    chain_ = ea::move(chain);
    return true;
}

void IKChainSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    chain_.UpdateLengths();

}

void IKChainSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings)
{
    chain_.Solve(target_->GetWorldPosition(), settings);
}

IKIdentitySolver::IKIdentitySolver(Context* context)
    : IKSolverComponent(context)
{
}

IKIdentitySolver::~IKIdentitySolver()
{
}

void IKIdentitySolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKIdentitySolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Bone Name", ea::string, boneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ACTION_STATIC_LABEL("Update Properties", UpdateProperties, "Set properties below from current bone positions");
    URHO3D_ATTRIBUTE("Rotation Offset", Quaternion, rotationOffset_, Quaternion::ZERO, AM_DEFAULT);
}

void IKIdentitySolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (boneNode_)
        DrawIKNode(debug, *boneNode_, true);
    if (target_)
        DrawIKTarget(debug, target_, true);
}

void IKIdentitySolver::UpdateProperties()
{
    UpdateRotationOffset();
}

void IKIdentitySolver::UpdateRotationOffset()
{
    Node* boneNode = node_->GetChild(boneName_, true);
    if (boneNode)
        rotationOffset_ = node_->GetWorldRotation().Inverse() * boneNode->GetWorldRotation();
}

void IKIdentitySolver::EnsureInitialized()
{
    if (rotationOffset_ == Quaternion::ZERO)
        UpdateRotationOffset();
}

bool IKIdentitySolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    boneNode_ = AddSolverNode(nodeCache, boneName_);
    if (!boneNode_)
        return false;

    return true;
}

void IKIdentitySolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
}

void IKIdentitySolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings)
{
    EnsureInitialized();

    boneNode_->position_ = target_->GetWorldPosition();
    boneNode_->rotation_ = target_->GetWorldRotation() * rotationOffset_;

    boneNode_->MarkPositionDirty();
    boneNode_->MarkRotationDirty();
}

IKTrigonometrySolver::IKTrigonometrySolver(Context* context)
    : IKSolverComponent(context)
{
}

IKTrigonometrySolver::~IKTrigonometrySolver()
{
}

void IKTrigonometrySolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKTrigonometrySolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Bone 0 Name", ea::string, firstBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bone 1 Name", ea::string, secondBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bone 2 Name", ea::string, thirdBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bend Target Name", ea::string, bendTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Position Weight", float, positionWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Target Weight", float, bendTargetWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Min Angle", float, minAngle_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Angle", float, maxAngle_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Direction", Vector3, bendDirection_, Vector3::FORWARD, AM_DEFAULT);
}

void IKTrigonometrySolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    IKNode* firstBone = chain_.GetBeginNode();
    IKNode* secondBone = chain_.GetMiddleNode();
    IKNode* thirdBone = chain_.GetEndNode();

    if (firstBone && secondBone && thirdBone)
    {
        DrawIKNode(debug, *firstBone, false);
        DrawIKNode(debug, *secondBone, false);
        DrawIKNode(debug, *thirdBone, false);
        DrawIKSegment(debug, *firstBone, *secondBone);
        DrawIKSegment(debug, *secondBone, *thirdBone);

        const Vector3 currentBendDirection = chain_.GetCurrentChainRotation()
            * node_->GetWorldRotation() * bendDirection_;
        DrawDirection(debug, secondBone->position_, currentBendDirection);
    }
    if (target_)
        DrawIKTarget(debug, target_, false);
    if (bendTarget_)
        DrawIKTarget(debug, bendTarget_, false);
}

bool IKTrigonometrySolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    bendTarget_ = AddCheckedNode(nodeCache, bendTargetName_);

    IKNode* firstBone = AddSolverNode(nodeCache, firstBoneName_);
    if (!firstBone)
        return false;

    IKNode* secondBone = AddSolverNode(nodeCache, secondBoneName_);
    if (!secondBone)
        return false;

    IKNode* thirdBone = AddSolverNode(nodeCache, thirdBoneName_);
    if (!thirdBone)
        return false;

    SetParentAsFrameOfReference(*firstBone);
    chain_.Initialize(firstBone, secondBone, thirdBone);
    return true;
}

void IKTrigonometrySolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    chain_.UpdateLengths();

    local_.defaultDirection_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * bendDirection_;
}

void IKTrigonometrySolver::EnsureInitialized()
{
    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    bendTargetWeight_ = Clamp(bendTargetWeight_, 0.0f, 1.0f);
    minAngle_ = Clamp(minAngle_, 0.0f, 180.0f);
    maxAngle_ = Clamp(maxAngle_, minAngle_, 180.0f);
}

void IKTrigonometrySolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings)
{
    EnsureInitialized();

    IKNode& firstBone = *chain_.GetBeginNode();
    IKNode& secondBone = *chain_.GetMiddleNode();
    IKNode& thirdBone = *chain_.GetEndNode();

    const Quaternion firstBoneRotation = firstBone.rotation_;
    const Quaternion secondBoneRotation = secondBone.rotation_;
    const Quaternion thirdBoneRotation = thirdBone.rotation_;

    const float bendTargetWeight = bendTarget_ ? bendTargetWeight_ : 0.0f;
    const Vector3 bendTargetPosition = bendTarget_ ? bendTarget_->GetWorldPosition() : Vector3::ZERO;

    const Vector3 targetPosition = target_->GetWorldPosition();
    const Vector3 bendTargetDirection = bendTargetPosition - Lerp(firstBone.position_, targetPosition, 0.5f);

    const Vector3 originalDirection = node_->GetWorldRotation() * bendDirection_;
    const Vector3 currentDirection0 = frameOfReference.rotation_ * local_.defaultDirection_;
    const Vector3 currentDirection1 = bendTargetDirection.Normalized();
    const Vector3 currentDirection = Lerp(currentDirection0, currentDirection1, bendTargetWeight);
    chain_.Solve(targetPosition, originalDirection, currentDirection, minAngle_, maxAngle_);

    firstBone.rotation_ = firstBoneRotation.Slerp(firstBone.rotation_, positionWeight_);
    secondBone.rotation_ = secondBoneRotation.Slerp(secondBone.rotation_, positionWeight_);
    thirdBone.rotation_ = thirdBoneRotation.Slerp(thirdBone.rotation_, positionWeight_);
}

IKLegSolver::IKLegSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKLegSolver::~IKLegSolver()
{
}

void IKLegSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKLegSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Thigh Bone Name", ea::string, thighBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Calf Bone Name", ea::string, calfBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Heel Bone Name", ea::string, heelBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Toe Bone Name", ea::string, toeBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Min Knee Angle", float, minKneeAngle_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Knee Angle", float, maxKneeAngle_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Weight", float, bendWeight_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Direction", Vector3, bendDirection_, Vector3::FORWARD, AM_DEFAULT);

    URHO3D_ACTION_STATIC_LABEL("Update Properties", UpdateProperties, "Set properties below from current bone positions");
    URHO3D_ATTRIBUTE("Min Heel Angle", float, minHeelAngle_, -1.0f, AM_DEFAULT);
}

void IKLegSolver::UpdateProperties()
{
    UpdateMinHeelAngle();
}

void IKLegSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    IKNode* thighBone = legChain_.GetBeginNode();
    IKNode* calfBone = legChain_.GetMiddleNode();
    IKNode* heelBone = legChain_.GetEndNode();
    IKNode* toeBone = footSegment_.endNode_;

    if (thighBone && calfBone && heelBone)
    {
        DrawIKNode(debug, *thighBone, false);
        DrawIKNode(debug, *calfBone, false);
        DrawIKNode(debug, *heelBone, false);
        DrawIKSegment(debug, *thighBone, *calfBone);
        DrawIKSegment(debug, *calfBone, *heelBone);

        const Vector3 currentBendDirection = legChain_.GetCurrentChainRotation()
            * node_->GetWorldRotation() * bendDirection_;
        DrawDirection(debug, calfBone->position_, currentBendDirection);
    }
    if (heelBone && toeBone)
    {
        DrawIKNode(debug, *toeBone, false);
        DrawIKSegment(debug, *heelBone, *toeBone);
    }
    if (target_)
    {
        DrawIKTarget(debug, target_, false);
    }
}

bool IKLegSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    IKNode* thighBone = AddSolverNode(nodeCache, thighBoneName_);
    if (!thighBone)
        return false;

    IKNode* calfBone = AddSolverNode(nodeCache, calfBoneName_);
    if (!calfBone)
        return false;

    IKNode* heelBone = AddSolverNode(nodeCache, heelBoneName_);
    if (!heelBone)
        return false;

    IKNode* toeBone = AddSolverNode(nodeCache, toeBoneName_);
    if (!toeBone)
        return false;

    SetParentAsFrameOfReference(*thighBone);
    legChain_.Initialize(thighBone, calfBone, heelBone);
    footSegment_ = {heelBone, toeBone};
    return true;
}

void IKLegSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    legChain_.UpdateLengths();
    footSegment_.UpdateLength();

    const IKNode& calfBone = *legChain_.GetMiddleNode();
    const IKNode& heelBone = *legChain_.GetEndNode();
    const IKNode& toeBone = *footSegment_.endNode_;

    local_.defaultDirection_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * bendDirection_;
    local_.defaultFootRotation_ = calfBone.rotation_.Inverse() * heelBone.rotation_;
    local_.defaultToeOffset_ = heelBone.rotation_.Inverse() * (toeBone.position_ - heelBone.position_);
    local_.defaultToeRotation_ = heelBone.rotation_.Inverse() * toeBone.rotation_;
}

void IKLegSolver::UpdateMinHeelAngle()
{
    Node* thighNode = node_->GetChild(thighBoneName_, true);
    Node* heelNode = node_->GetChild(heelBoneName_, true);
    Node* toeNode = node_->GetChild(toeBoneName_, true);

    if (thighNode && heelNode && toeNode)
    {
        const Vector3 thighToToe = toeNode->GetWorldPosition() - thighNode->GetWorldPosition();
        const Vector3 heelToThigh = thighNode->GetWorldPosition() - heelNode->GetWorldPosition();
        const Vector3 heelToToe = toeNode->GetWorldPosition() - heelNode->GetWorldPosition();

        const Vector3 bendNormal = -thighToToe.CrossProduct(node_->GetWorldRotation() * bendDirection_);
        minHeelAngle_ = heelToThigh.SignedAngle(heelToToe, bendNormal);
    }
}

Vector3 IKLegSolver::GetApproximateBendDirection(const Vector3& toeTargetPosition,
    const Vector3& originalDirection, const Vector3& currentDirection) const
{
    IKNode* thighBone = legChain_.GetBeginNode();
    IKNode* toeBone = footSegment_.endNode_;

    const Quaternion chainRotation = IKTrigonometricChain::CalculateRotation(
        thighBone->originalPosition_, toeBone->originalPosition_, originalDirection,
        thighBone->position_, toeTargetPosition, currentDirection);
    return chainRotation * originalDirection;
}

Vector3 IKLegSolver::CalculateFootDirectionStraight(
    const Vector3& toeTargetPosition, const Vector3& approximateBendDirection) const
{
    IKNode* thighBone = legChain_.GetBeginNode();

    const Vector3 thighToToe = toeTargetPosition - thighBone->position_;
    const Vector3 bendNormal = thighToToe.CrossProduct(approximateBendDirection);

    return GetToeToHeel(
        thighBone->position_, toeTargetPosition, footSegment_.length_, minHeelAngle_,
        GetMaxDistance(legChain_, maxKneeAngle_), bendNormal);
}

Vector3 IKLegSolver::CalculateFootDirectionBent(
    const Vector3& toeTargetPosition, const Vector3& approximateBendDirection) const
{
    IKNode* thighBone = legChain_.GetBeginNode();
    const auto [newPos1, newPos2] = IKTrigonometricChain::Solve(
        thighBone->position_, legChain_.GetFirstLength(), legChain_.GetSecondLength() + footSegment_.length_,
        toeTargetPosition, approximateBendDirection, minKneeAngle_, maxKneeAngle_);
    return (newPos1 - newPos2).Normalized() * footSegment_.length_;
}

void IKLegSolver::EnsureInitialized()
{
    if (minHeelAngle_ < 0.0f)
        UpdateMinHeelAngle();
    bendWeight_ = Clamp(bendWeight_, 0.0f, 1.0f);
    minKneeAngle_ = Clamp(minKneeAngle_, 0.0f, 180.0f);
    maxKneeAngle_ = Clamp(maxKneeAngle_, 0.0f, 180.0f);
}

void IKLegSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings)
{
    EnsureInitialized();

    const Vector3& toeTargetPosition = target_->GetWorldPosition();
    const Vector3 originalDirection = node_->GetWorldRotation() * bendDirection_;
    const Vector3 currentDirection = frameOfReference.rotation_ * local_.defaultDirection_;
    const Vector3 approximateBendDirection = GetApproximateBendDirection(
        toeTargetPosition, originalDirection, currentDirection);

    const Vector3 toeToHeel0 = CalculateFootDirectionStraight(toeTargetPosition, approximateBendDirection);
    const Vector3 toeToHeel1 = CalculateFootDirectionBent(toeTargetPosition, approximateBendDirection);

    const Vector3 toeToHeel = InterpolateDirection(toeToHeel0, toeToHeel1, bendWeight_);
    const Vector3 heelTargetPosition = toeTargetPosition + toeToHeel;

    legChain_.Solve(heelTargetPosition, originalDirection, currentDirection, minKneeAngle_, maxKneeAngle_);

    // Update foot segment
    IKNode& calfBone = *legChain_.GetMiddleNode();
    IKNode& heelBone = *legChain_.GetEndNode();
    IKNode& toeBone = *footSegment_.endNode_;

    heelBone.previousPosition_ = heelBone.position_;
    heelBone.previousRotation_ = calfBone.rotation_ * local_.defaultFootRotation_;
    toeBone.previousPosition_ = heelBone.previousPosition_ + heelBone.previousRotation_ * local_.defaultToeOffset_;
    toeBone.previousRotation_ = heelBone.previousRotation_ * local_.defaultToeRotation_;
    // heelBone.position is already set by legChain_.Solve()
    toeBone.position_ = heelBone.position_ - toeToHeel;
    footSegment_.UpdateRotationInNodes(true, true);
}

IKSpineSolver::IKSpineSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKSpineSolver::~IKSpineSolver()
{
}

void IKSpineSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKSpineSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Bone Names", StringVector, boneNames_, OnTreeDirty, Variant::emptyStringVector, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Twist Target Name", ea::string, twistTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Max Angle", float, maxAngle_, 90.0f, AM_DEFAULT);
}

void IKSpineSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    const auto& segments = chain_.GetSegments();
    for (const IKNodeSegment& segment : segments)
    {
        const bool isLastSegment = &segment == &segments.back();
        DrawIKNode(debug, *segment.beginNode_, isLastSegment);
        DrawIKSegment(debug, *segment.beginNode_, *segment.endNode_);
        if (isLastSegment)
            DrawIKNode(debug, *segment.endNode_, false);
    }

    if (twistTarget_)
        DrawIKTarget(debug, twistTarget_, true);
    if (target_)
        DrawIKTarget(debug, target_, false);
}

bool IKSpineSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    IKSpineChain chain;
    for (const ea::string& boneName : boneNames_)
    {
        IKNode* boneNode = AddSolverNode(nodeCache, boneName);
        if (!boneNode)
            return false;

        chain.AddNode(boneNode);
    }

    if (chain.GetSegments().size() < 2)
    {
        URHO3D_LOGERROR("Spine solver must have at least 3 bones");
        return false;
    }

    if (!twistTargetName_.empty())
    {
        twistTarget_ = AddCheckedNode(nodeCache, twistTargetName_);
        if (!twistTarget_)
            return false;
    }

    SetFrameOfReference(*chain.GetSegments().front().beginNode_);
    chain_ = ea::move(chain);
    return true;
}

void IKSpineSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    chain_.UpdateLengths();

    const auto& segments = chain_.GetSegments();
    local_.defaultTransforms_.resize(segments.size());
    for (size_t i = 0; i < segments.size(); ++i)
    {
        const IKNode& node = *segments[i].endNode_;
        local_.defaultTransforms_[i] = inverseFrameOfReference * Transform{node.position_, node.rotation_};
    }
}

void IKSpineSolver::SetOriginalTransforms(const Transform& frameOfReference)
{
    const auto& segments = chain_.GetSegments();
    for (size_t i = 0; i < segments.size(); ++i)
    {
        IKNode& node = *segments[i].endNode_;
        const Transform& defaultTransform = local_.defaultTransforms_[i];
        node.position_ = frameOfReference * defaultTransform.position_;
        node.rotation_ = frameOfReference * defaultTransform.rotation_;
    }
}

void IKSpineSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings)
{
    SetOriginalTransforms(frameOfReference);
    chain_.Solve(target_->GetWorldPosition(), maxAngle_, settings);

    const auto& segments = chain_.GetSegments();
    const float twistAngle = twistTarget_ ? GetTwistAngle(segments.back(), twistTarget_) : 0.0f;
    chain_.Twist(twistAngle, settings);
}

float IKSpineSolver::GetTwistAngle(const IKNodeSegment& segment, Node* targetNode) const
{
    const Quaternion targetRotation = node_->GetWorldRotation().Inverse() * targetNode->GetWorldRotation();
    const Vector3 direction = (segment.endNode_->position_ - segment.beginNode_->position_).Normalized();
    const auto [_, twist] = targetRotation.ToSwingTwist(direction);
    const float angle = twist.Angle();
    const float sign = twist.Axis().DotProduct(direction) > 0.0f ? 1.0f : -1.0f;
    return sign * (angle > 180.0f ? angle - 360.0f : angle);
}

IKArmSolver::IKArmSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKArmSolver::~IKArmSolver()
{
}

void IKArmSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKArmSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Shoulder Bone Name", ea::string, shoulderBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Arm Bone Name", ea::string, armBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Forearm Bone Name", ea::string, forearmBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Hand Bone Name", ea::string, handBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Min Elbow Angle", float, minElbowAngle_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Elbow Angle", float, maxElbowAngle_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Shoulder Weight", Vector2, shoulderWeight_, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Direction", Vector3, bendDirection_, Vector3::FORWARD, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Up Direction", Vector3, upDirection_, Vector3::UP, AM_DEFAULT);
}

void IKArmSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    IKNode* shoulderBone = shoulderSegment_.beginNode_;
    IKNode* armBone = armChain_.GetBeginNode();
    IKNode* forearmBone = armChain_.GetMiddleNode();
    IKNode* handBone = armChain_.GetEndNode();

    if (armBone && forearmBone && handBone)
    {
        DrawIKNode(debug, *armBone, false);
        DrawIKNode(debug, *forearmBone, false);
        DrawIKNode(debug, *handBone, false);
        DrawIKSegment(debug, *armBone, *forearmBone);
        DrawIKSegment(debug, *forearmBone, *handBone);

        const Vector3 currentBendDirection = armChain_.GetCurrentChainRotation()
            * node_->GetWorldRotation() * bendDirection_;
        DrawDirection(debug, forearmBone->position_, currentBendDirection);
    }
    if (shoulderBone && armBone)
    {
        DrawIKNode(debug, *shoulderBone, false);
        DrawIKSegment(debug, *shoulderBone, *armBone);
    }
    if (target_)
        DrawIKTarget(debug, target_, false);
}

bool IKArmSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    IKNode* shoulderBone = AddSolverNode(nodeCache, shoulderBoneName_);
    if (!shoulderBone)
        return false;

    IKNode* armBone = AddSolverNode(nodeCache, armBoneName_);
    if (!armBone)
        return false;

    IKNode* forearmBone = AddSolverNode(nodeCache, forearmBoneName_);
    if (!forearmBone)
        return false;

    IKNode* handBone = AddSolverNode(nodeCache, handBoneName_);
    if (!handBone)
        return false;

    SetParentAsFrameOfReference(*shoulderBone);
    armChain_.Initialize(armBone, forearmBone, handBone);
    shoulderSegment_ = {shoulderBone, armBone};
    return true;
}

void IKArmSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    armChain_.UpdateLengths();
    shoulderSegment_.UpdateLength();

    local_.defaultDirection_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * bendDirection_;
    local_.up_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * upDirection_;

    local_.shoulderRotation_ = inverseFrameOfReference * shoulderSegment_.beginNode_->rotation_;
    local_.armOffset_ = inverseFrameOfReference * shoulderSegment_.endNode_->position_;
    local_.armRotation_ = inverseFrameOfReference * shoulderSegment_.endNode_->rotation_;
}

void IKArmSolver::EnsureInitialized()
{
    minElbowAngle_ = Clamp(minElbowAngle_, 0.0f, 180.0f);
    maxElbowAngle_ = Clamp(maxElbowAngle_, 0.0f, 180.0f);
    shoulderWeight_ = VectorClamp(shoulderWeight_, Vector2::ZERO, Vector2::ONE);
}

void IKArmSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings)
{
    EnsureInitialized();

    shoulderSegment_.beginNode_->rotation_ = frameOfReference * local_.shoulderRotation_;
    shoulderSegment_.endNode_->position_ = frameOfReference * local_.armOffset_;
    shoulderSegment_.endNode_->rotation_ = frameOfReference * local_.armRotation_;

    const Vector3 originalDirection = node_->GetWorldRotation() * bendDirection_;
    const Vector3 currentDirection = frameOfReference.rotation_ * local_.defaultDirection_;

    const Vector3 handTargetPosition = target_->GetWorldPosition();

    const Quaternion maxShoulderRotation = CalculateMaxShoulderRotation(handTargetPosition);
    const auto [swing, twist] = maxShoulderRotation.ToSwingTwist(frameOfReference.rotation_ * local_.up_);
    const Quaternion shoulderRotation = Quaternion::IDENTITY.Slerp(swing, shoulderWeight_.y_)
        * Quaternion::IDENTITY.Slerp(twist, shoulderWeight_.x_);
    RotateShoulder(shoulderRotation);

    armChain_.Solve(handTargetPosition, originalDirection, currentDirection, minElbowAngle_, maxElbowAngle_);
}

void IKArmSolver::RotateShoulder(const Quaternion& rotation)
{
    const Vector3 shoulderPosition = shoulderSegment_.beginNode_->position_;
    shoulderSegment_.beginNode_->RotateAround(shoulderPosition, rotation);
    shoulderSegment_.endNode_->RotateAround(shoulderPosition, rotation);
}

Quaternion IKArmSolver::CalculateMaxShoulderRotation(const Vector3& handTargetPosition) const
{
    const Vector3 shoulderPosition = shoulderSegment_.beginNode_->position_;
    const Vector3 shoulderToArmMax = (handTargetPosition - shoulderPosition).ReNormalized(
        shoulderSegment_.length_, shoulderSegment_.length_);
    const Vector3 armTargetPosition = shoulderPosition + shoulderToArmMax;

    const Vector3 originalShoulderToArm = shoulderSegment_.endNode_->position_ - shoulderSegment_.beginNode_->position_;
    const Vector3 maxShoulderToArm = armTargetPosition - shoulderPosition;

    return Quaternion{originalShoulderToArm, maxShoulderToArm};
}

IKLookAtSolver::IKLookAtSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKLookAtSolver::~IKLookAtSolver()
{
}

void IKLookAtSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKLookAtSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Neck Bone Name", ea::string, neckBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Head Bone Name", ea::string, headBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Head Target Name", ea::string, headTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Look At Target Name", ea::string, lookAtTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Eye Direction", Vector3, eyeDirection_, OnTreeDirty, Vector3::FORWARD, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Eye Offset", Vector3, eyeOffset_, OnTreeDirty, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Neck Weight", float, neckWeight_, OnTreeDirty, 0.5f, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Look At Weight", float, lookAtWeight_, OnTreeDirty, 0.0f, AM_DEFAULT);
}

void IKLookAtSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    IKNode* neckBone = neckSegment_.beginNode_;
    IKNode* headBone = neckSegment_.endNode_;

    if (neckBone && headBone)
    {
        DrawIKNode(debug, *neckBone, false);
        DrawIKNode(debug, *headBone, false);
        DrawIKSegment(debug, *neckBone, *headBone);

        const Ray eyeRay = GetEyeRay();
        DrawDirection(debug, eyeRay.origin_, eyeRay.direction_, true, false);
    }
    if (headTarget_)
        DrawIKTarget(debug, headTarget_, true);
    if (lookAtTarget_)
        DrawIKTarget(debug, lookAtTarget_, false);
}

bool IKLookAtSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    headTarget_ = AddCheckedNode(nodeCache, headTargetName_);
    lookAtTarget_ = AddCheckedNode(nodeCache, lookAtTargetName_);
    if (!headTarget_ && !lookAtTarget_)
    {
        URHO3D_LOGERROR("IKLookAtSolver: Either head or look at target must be specified");
        return false;
    }

    IKNode* neckNode = AddSolverNode(nodeCache, neckBoneName_);
    if (!neckNode)
        return false;

    IKNode* headNode = AddSolverNode(nodeCache, headBoneName_);
    if (!headNode)
        return false;

    SetParentAsFrameOfReference(*neckNode);
    neckChain_.Initialize(neckNode);
    headChain_.Initialize(headNode);
    neckSegment_ = {neckNode, headNode};
    return true;
}

void IKLookAtSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    neckSegment_.UpdateLength();

    const IKNode& neckNode = *neckSegment_.beginNode_;
    const IKNode& headNode = *neckSegment_.endNode_;

    local_.defaultNeckTransform_ = inverseFrameOfReference * Transform{neckNode.position_, neckNode.rotation_};
    local_.defaultHeadTransform_ = inverseFrameOfReference * Transform{headNode.position_, headNode.rotation_};

    const Vector3 eyeDirection = node_->GetWorldRotation() * eyeDirection_;
    const Vector3 eyeOffset = node_->GetWorldRotation() * eyeOffset_;
    neckChain_.SetWorldEyeTransform(eyeOffset, eyeDirection);
    headChain_.SetWorldEyeTransform(eyeOffset, eyeDirection);
}

void IKLookAtSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings)
{
    EnsureInitialized();

    IKNode& neckNode = *neckSegment_.beginNode_;
    IKNode& headNode = *neckSegment_.endNode_;

    neckNode.rotation_ = frameOfReference * local_.defaultNeckTransform_.rotation_;
    headNode.position_ = frameOfReference * local_.defaultHeadTransform_.position_;
    headNode.rotation_ = frameOfReference * local_.defaultHeadTransform_.rotation_;
    neckNode.StorePreviousTransform();
    headNode.StorePreviousTransform();

    const float lookAtWeight = !lookAtTarget_ ? 0.0f : !headTarget_ ? 1.0f : lookAtWeight_;
    const Vector3 headDirection = headTarget_ ? headTarget_->GetWorldDirection() : Vector3::DOWN;
    const Vector3 lookAtTarget = lookAtTarget_ ? lookAtTarget_->GetWorldPosition() : Vector3::ZERO;

    {
        const Transform parentTransform{neckNode.position_, neckNode.rotation_};
        const Quaternion neckRotation0 = neckChain_.SolveLookTo(headDirection);
        const Quaternion neckRotation1 = neckChain_.SolveLookAt(lookAtTarget, settings);

        const Quaternion neckRotation = MixRotation(neckRotation0, neckRotation1, lookAtWeight);
        const Quaternion neckRotationWeighted = Quaternion::IDENTITY.Slerp(neckRotation, neckWeight_);
        neckNode.rotation_ = neckRotationWeighted * neckNode.rotation_;
        headNode.RotateAround(neckNode.position_, neckRotationWeighted);
    }

    {
        const Transform parentTransform{headNode.position_, headNode.rotation_};
        const Quaternion headRotation0 = headChain_.SolveLookTo(headDirection);
        const Quaternion headRotation1 = headChain_.SolveLookAt(lookAtTarget, settings);

        const Quaternion headRotation = MixRotation(headRotation0, headRotation1, lookAtWeight);
        headNode.rotation_ = headRotation * headNode.rotation_;
    }

    neckNode.MarkRotationDirty();
    headNode.MarkRotationDirty();
}

void IKLookAtSolver::EnsureInitialized()
{
    neckWeight_ = Clamp(neckWeight_, 0.0f, 1.0f);
    lookAtWeight_ = Clamp(lookAtWeight_, 0.0f, 1.0f);
}

Ray IKLookAtSolver::GetEyeRay() const
{
    const IKNode& headBone = *neckSegment_.endNode_;
    const Vector3 origin = headBone.position_ + headBone.rotation_ * headChain_.GetLocalEyeOffset();
    const Vector3 direction = headBone.rotation_ * headChain_.GetLocalEyeDirection();
    return {origin, direction};
}

}
