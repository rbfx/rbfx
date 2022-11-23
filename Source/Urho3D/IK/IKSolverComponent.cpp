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
    const Quaternion rotation{-toeAngle, bendNormal};
    return (rotation * toeToThigh).Normalized() * toeToHeelDistance;
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

void IKSolverComponent::NotifyPositionsReady()
{
    UpdateChainLengths();
}

void IKSolverComponent::Solve(const IKSettings& settings)
{
    for (const auto& [node, solverNode] : solverNodes_)
    {
        solverNode->position_ = node->GetWorldPosition();
        solverNode->rotation_ = node->GetWorldRotation();
        solverNode->StorePreviousTransform();
    }

    SolveInternal(settings);

    for (const auto& [node, solverNode] : solverNodes_)
    {
        if (solverNode->positionDirty_)
            node->SetWorldPosition(solverNode->position_);
        if (solverNode->rotationDirty_)
            node->SetWorldRotation(solverNode->rotation_);
    }
}

IKNode* IKSolverComponent::AddSolverNode(IKNodeCache& nodeCache, const ea::string& name)
{
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
    Node* boneNode = node_->GetChild(name, true);
    if (!boneNode)
    {
        URHO3D_LOGERROR("IKSolverComponent: Bone node '{}' is not found", name);
        return nullptr;
    }

    nodeCache.emplace(boneNode, IKNode{});
    return boneNode;
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

bool IKChainSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    targetNode_ = AddCheckedNode(nodeCache, targetName_);
    if (!targetNode_)
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

void IKChainSolver::UpdateChainLengths()
{
    chain_.UpdateLengths();

    // TODO: Temp
    /*for (auto& segment : chain_.segments_)
    {
        segment.angularConstraint_.enabled_ = true;
        segment.angularConstraint_.maxAngle_ = 90.0f;
        segment.angularConstraint_.axis_ = Vector3::DOWN;
    }*/
}

void IKChainSolver::SolveInternal(const IKSettings& settings)
{
    chain_.Solve(targetNode_->GetWorldPosition(), settings);
}

void IKChainSolver::OnTreeDirty()
{
    if (auto solver = GetComponent<IKSolver>())
        solver->MarkSolversDirty();
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

    URHO3D_ATTRIBUTE("Bone Name", ea::string, boneName_, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Target Name", ea::string, targetName_, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ACTION_STATIC_LABEL("Update Properties", UpdateProperties, "Set properties below from current bone positions");
    URHO3D_ATTRIBUTE("Identity Rotation", Quaternion, identityRotation_, Quaternion::ZERO, AM_DEFAULT);
}

void IKIdentitySolver::UpdateProperties()
{
    UpdateIdentityRotation();
}

void IKIdentitySolver::UpdateIdentityRotation()
{
    Node* boneNode = node_->GetChild(boneName_, true);
    if (boneNode)
        identityRotation_ = node_->GetWorldRotation().Inverse() * boneNode->GetWorldRotation();
}

void IKIdentitySolver::EnsureInitialized()
{
    if (identityRotation_ == Quaternion::ZERO)
        UpdateIdentityRotation();
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

void IKIdentitySolver::UpdateChainLengths()
{
}

void IKIdentitySolver::SolveInternal(const IKSettings& settings)
{
    EnsureInitialized();

    boneNode_->position_ = target_->GetWorldPosition();
    boneNode_->rotation_ = target_->GetWorldRotation() * identityRotation_;

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

    URHO3D_ATTRIBUTE("Bone 0 Name", ea::string, firstBoneName_, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bone 1 Name", ea::string, secondBoneName_, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bone 2 Name", ea::string, thirdBoneName_, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Target Name", ea::string, targetName_, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Min Angle", float, minAngle_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Angle", float, maxAngle_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Normal", Vector3, bendNormal_, Vector3::RIGHT, AM_DEFAULT);
}

bool IKTrigonometrySolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    IKNode* firstBone = AddSolverNode(nodeCache, firstBoneName_);
    if (!firstBone)
        return false;

    IKNode* secondBone = AddSolverNode(nodeCache, secondBoneName_);
    if (!secondBone)
        return false;

    IKNode* thirdBone = AddSolverNode(nodeCache, thirdBoneName_);
    if (!thirdBone)
        return false;

    chain_.Initialize(firstBone, secondBone, thirdBone);
    return true;
}

void IKTrigonometrySolver::UpdateChainLengths()
{
    chain_.UpdateLengths();
}

void IKTrigonometrySolver::SolveInternal(const IKSettings& settings)
{
    chain_.Solve(target_->GetWorldPosition(), bendNormal_, minAngle_, maxAngle_, settings);
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

    URHO3D_ATTRIBUTE("Thigh Bone Name", ea::string, thighBoneName_, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Calf Bone Name", ea::string, calfBoneName_, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Heel Bone Name", ea::string, heelBoneName_, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Toe Bone Name", ea::string, toeBoneName_, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Target Name", ea::string, targetName_, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Min Knee Angle", float, minKneeAngle_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Knee Angle", float, maxKneeAngle_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Weight", float, bendWeight_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Normal", Vector3, bendNormal_, Vector3::RIGHT, AM_DEFAULT);

    URHO3D_ACTION_STATIC_LABEL("Update Properties", UpdateProperties, "Set properties below from current bone positions");
    URHO3D_ATTRIBUTE("Min Heel Angle", float, minHeelAngle_, -1.0f, AM_DEFAULT);
}

void IKLegSolver::UpdateProperties()
{
    UpdateMinHeelAngle();
}

void IKLegSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    const float jointRadius = 0.02f;
    const float targetRadius = 0.05f;

    IKNode* thighBone = legChain_.GetBeginNode();
    IKNode* calfBone = legChain_.GetMiddleNode();
    IKNode* heelBone = legChain_.GetEndNode();
    IKNode* toeBone = footSegment_.endNode_;

    if (thighBone && calfBone && heelBone)
    {
        debug->AddLine(thighBone->position_, calfBone->position_, Color::YELLOW, false);
        debug->AddLine(calfBone->position_, heelBone->position_, Color::YELLOW, false);
        debug->AddSphere(Sphere(thighBone->position_, jointRadius), Color::YELLOW, false);
        debug->AddSphere(Sphere(calfBone->position_, jointRadius), Color::YELLOW, false);
        debug->AddSphere(Sphere(heelBone->position_, jointRadius), Color::YELLOW, false);
    }
    if (heelBone && toeBone)
    {
        debug->AddLine(heelBone->position_, toeBone->position_, Color::YELLOW, false);
        debug->AddSphere(Sphere(toeBone->position_, jointRadius), Color::YELLOW, false);
    }
    if (target_)
    {
        debug->AddSphere(Sphere(target_->GetWorldPosition(), targetRadius), Color::GREEN, false);
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

    legChain_.Initialize(thighBone, calfBone, heelBone);
    footSegment_ = {heelBone, toeBone};
    return true;
}

void IKLegSolver::UpdateChainLengths()
{
    legChain_.UpdateLengths();
    footSegment_.UpdateLength();
}

void IKLegSolver::UpdateMinHeelAngle()
{
    Node* thighNode = node_->GetChild(thighBoneName_, true);
    Node* heelNode = node_->GetChild(heelBoneName_, true);
    Node* toeNode = node_->GetChild(toeBoneName_, true);

    if (thighNode && heelNode && toeNode)
    {
        const Vector3 heelToThigh = thighNode->GetWorldPosition() - heelNode->GetWorldPosition();
        const Vector3 heelToToe = toeNode->GetWorldPosition() - heelNode->GetWorldPosition();

        minHeelAngle_ = heelToThigh.SignedAngle(heelToToe, bendNormal_);
    }
}

Vector3 IKLegSolver::CalculateFootDirectionStraight(const Vector3& toeTargetPosition) const
{
    IKNode* thighBone = legChain_.GetBeginNode();
    return GetToeToHeel(
        thighBone->position_, toeTargetPosition, footSegment_.length_, minHeelAngle_,
        GetMaxDistance(legChain_, maxKneeAngle_), bendNormal_);
}

Vector3 IKLegSolver::CalculateFootDirectionBent(const Vector3& toeTargetPosition) const
{
    IKNode* thighBone = legChain_.GetBeginNode();
    const auto [newPos1, newPos2] = IKTrigonometricChain::Solve(
        thighBone->position_, legChain_.GetFirstLength(), legChain_.GetSecondLength() + footSegment_.length_,
        toeTargetPosition, bendNormal_, minKneeAngle_, maxKneeAngle_);
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

void IKLegSolver::SolveInternal(const IKSettings& settings)
{
    EnsureInitialized();

    const Vector3& toeTargetPosition = target_->GetWorldPosition();

    IKNode* heelBone = legChain_.GetEndNode();

    const Vector3 toeToHeel0 = CalculateFootDirectionStraight(toeTargetPosition);
    const Vector3 toeToHeel1 = CalculateFootDirectionBent(toeTargetPosition);

    const Vector3 toeToHeel = InterpolateDirection(toeToHeel0, toeToHeel1, bendWeight_);
    const Vector3 heelTargetPosition = toeTargetPosition + toeToHeel;

    legChain_.Solve(heelTargetPosition, bendNormal_, minKneeAngle_, maxKneeAngle_, settings);

    const Vector3 toeTargetPositionAdjusted = heelBone->position_ - toeToHeel;
    footSegment_.endNode_->position_ = toeTargetPositionAdjusted;
    footSegment_.UpdateRotationInNodes(settings, true);
}

}
