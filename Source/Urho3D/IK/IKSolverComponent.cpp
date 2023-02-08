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

void IKSolverComponent::Solve(const IKSettings& settings, float timeStep)
{
    for (const auto& [node, solverNode] : solverNodes_)
    {
        solverNode->position_ = node->GetWorldPosition();
        solverNode->rotation_ = node->GetWorldRotation();
        solverNode->StorePreviousTransform();
    }

    const Transform frameOfReference = GetFrameOfReferenceTransform();
    SolveInternal(frameOfReference, settings, timeStep);

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

void IKSolverComponent::DrawIKTarget(DebugRenderer* debug,
    const Vector3& position, const Quaternion& rotation, bool oriented) const
{
    static const float radius = 0.05f;
    static const Color color = Color::GREEN;

    DrawNode(debug, oriented, position, rotation, color, radius);
}

void IKSolverComponent::DrawIKTarget(DebugRenderer* debug, const Node* node, bool oriented) const
{
    DrawIKTarget(debug, node->GetWorldPosition(), node->GetWorldRotation(), oriented);
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
        IKNode* bone = AddSolverNode(nodeCache, boneName);
        if (!bone)
            return false;

        chain.AddNode(bone);
    }

    chain_ = ea::move(chain);
    return true;
}

void IKChainSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    chain_.UpdateLengths();

}

void IKChainSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
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

void IKIdentitySolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    boneNode_->position_ = target_->GetWorldPosition();
    boneNode_->rotation_ = target_->GetWorldRotation() * rotationOffset_;

    boneNode_->MarkPositionDirty();
    boneNode_->MarkRotationDirty();
}

IKLimbSolver::IKLimbSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKLimbSolver::~IKLimbSolver()
{
}

void IKLimbSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKLimbSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Bone 0 Name", ea::string, firstBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bone 1 Name", ea::string, secondBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bone 2 Name", ea::string, thirdBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bend Target Name", ea::string, bendTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Position Weight", float, positionWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Weight", float, rotationWeight_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Weight", float, bendWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Min Angle", float, minAngle_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Angle", float, maxAngle_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Direction", Vector3, bendDirection_, Vector3::FORWARD, AM_DEFAULT);
}

void IKLimbSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
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
        DrawIKTarget(debug, latestTargetPosition_, Quaternion::IDENTITY, false);
    if (bendTarget_)
        DrawIKTarget(debug, bendTarget_, false);
}

bool IKLimbSolver::InitializeNodes(IKNodeCache& nodeCache)
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

void IKLimbSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    chain_.UpdateLengths();

    local_.defaultDirection_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * bendDirection_;
}

void IKLimbSolver::EnsureInitialized()
{
    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    rotationWeight_ = Clamp(rotationWeight_, 0.0f, 1.0f);
    bendWeight_ = Clamp(bendWeight_, 0.0f, 1.0f);
    minAngle_ = Clamp(minAngle_, 0.0f, 180.0f);
    maxAngle_ = Clamp(maxAngle_, minAngle_, 180.0f);
}

void IKLimbSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    // Store original rotation
    IKNode& firstBone = *chain_.GetBeginNode();
    IKNode& secondBone = *chain_.GetMiddleNode();
    IKNode& thirdBone = *chain_.GetEndNode();

    const Quaternion firstBoneRotation = firstBone.rotation_;
    const Quaternion secondBoneRotation = secondBone.rotation_;
    const Quaternion thirdBoneRotation = thirdBone.rotation_;

    // Solve rotations for full solver weight
    latestTargetPosition_ = GetTargetPosition();
    const auto [originalDirection, currentDirection] = CalculateBendDirections(
        frameOfReference, latestTargetPosition_);

    chain_.Solve(latestTargetPosition_, originalDirection, currentDirection, minAngle_, maxAngle_);

    // Interpolate rotation to apply solver weight
    firstBone.rotation_ = firstBoneRotation.Slerp(firstBone.rotation_, positionWeight_);
    secondBone.rotation_ = secondBoneRotation.Slerp(secondBone.rotation_, positionWeight_);
    thirdBone.rotation_ = thirdBoneRotation.Slerp(thirdBone.rotation_, positionWeight_);

    // Apply target rotation if needed
    if (rotationWeight_ > 0.0f)
        thirdBone.rotation_ = thirdBone.rotation_.Slerp(target_->GetWorldRotation(), rotationWeight_);
}

Vector3 IKLimbSolver::GetTargetPosition() const
{
    IKNode& firstBone = *chain_.GetBeginNode();

    const float minDistance = 0.001f;
    const float maxDistance = GetMaxDistance(chain_, maxAngle_);
    const Vector3& origin = firstBone.position_;
    const Vector3& target = target_->GetWorldPosition();
    return origin + (target - origin).ReNormalized(minDistance, maxDistance);
}

ea::pair<Vector3, Vector3> IKLimbSolver::CalculateBendDirections(
    const Transform& frameOfReference, const Vector3& toeTargetPosition) const
{
    IKNode& firstBone = *chain_.GetBeginNode();

    const float bendTargetWeight = bendTarget_ ? bendWeight_ : 0.0f;
    const Vector3 bendTargetPosition = bendTarget_ ? bendTarget_->GetWorldPosition() : Vector3::ZERO;
    const Vector3 bendTargetDirection =
        bendTargetPosition - Lerp(firstBone.position_, toeTargetPosition, 0.5f);

    const Vector3 originalDirection = node_->GetWorldRotation() * bendDirection_;
    const Vector3 currentDirection0 = frameOfReference.rotation_ * local_.defaultDirection_;
    const Vector3 currentDirection1 = bendTargetDirection.Normalized();
    const Vector3 currentDirection = Lerp(currentDirection0, currentDirection1, bendTargetWeight);

    return {originalDirection, currentDirection};
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
    URHO3D_ATTRIBUTE_EX("Bend Target Name", ea::string, bendTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Ground Target Name", ea::string, groundTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Position Weight", float, positionWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Weight", float, rotationWeight_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Weight", float, bendWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Min Angle", float, minKneeAngle_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Angle", float, maxKneeAngle_, 180.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Base Tiptoe", Vector2, baseTiptoe_, Vector2(0.5f, 0.0f), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Ground Tiptoe Tweaks", Vector4, groundTiptoeTweaks_, Vector4(0.2f, 0.2f, 0.2f, 0.2f), AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Direction", Vector3, bendDirection_, Vector3::FORWARD, AM_DEFAULT);

    URHO3D_ACTION_STATIC_LABEL("Update Properties", UpdateProperties, "Set properties below from current bone positions");
    URHO3D_ATTRIBUTE("Heel Ground Offset", float, heelGroundOffset_, -1.0f, AM_DEFAULT);
}

void IKLegSolver::UpdateProperties()
{
    UpdateHeelGroundOffset();
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
        DrawIKTarget(debug, latestTargetPosition_, Quaternion::IDENTITY, false);

        const BoundingBox tiptoeBoxA{
            latestTargetPosition_ + Vector3{-0.02f, 0.0f + 0.05f, -0.02f},
            latestTargetPosition_ + Vector3{0.02f, latestTiptoeFactor_ * 0.2f + 0.05f, 0.02f}
        };
        const BoundingBox tiptoeBoxB{
            latestTargetPosition_ + Vector3{-0.02f, latestTiptoeFactor_ * 0.2f + 0.05f, -0.02f},
            latestTargetPosition_ + Vector3{0.02f, 0.2f + 0.05f, 0.02f}
        };

        debug->AddBoundingBox(tiptoeBoxA, Color{1.0f, 1.0f, 0.0f, 1.0f}, false);
        debug->AddBoundingBox(tiptoeBoxB, Color{1.0f, 1.0f, 0.0f, 0.2f}, false);
    }
    if (bendTarget_)
    {
        DrawIKTarget(debug, bendTarget_, false);
    }

    {
        Node* groundNode = groundTarget_ ? groundTarget_ : node_;
        const BoundingBox groundBox{
            Vector3{-0.5f, -0.2f, -0.5f},
            Vector3{0.5f, 0.0f, 0.5f}};
        const Matrix3x4 groundTransform = groundNode->GetWorldTransform();
        debug->AddBoundingBox(groundBox, groundTransform, Color::GREEN, false);

        const float offset = local_.tiptoeTweakOffset_;
        const Vector3 tiptoeOffsets[4] = {
            {-offset, 0.0f, 0.0f},
            {offset, 0.0f, 0.0f},
            {0.0f, 0.0f, -offset},
            {0.0f, 0.0f, offset},
        };
        for (int i = 0; i < 4; ++i)
        {
            const float tiptoe = groundTiptoeTweaks_[i];
            const BoundingBox tiptoeBoxA{
                tiptoeOffsets[i] + Vector3{-0.02f, 0.0f, -0.02f},
                tiptoeOffsets[i] + Vector3{0.02f, tiptoe * 0.2f, 0.02f}
            };
            const BoundingBox tiptoeBoxB{
                tiptoeOffsets[i] + Vector3{-0.02f, tiptoe * 0.2f, -0.02f},
                tiptoeOffsets[i] + Vector3{0.02f, 0.2f, 0.02f}
            };

            debug->AddBoundingBox(tiptoeBoxA, groundTransform, Color{1.0f, 1.0f, 0.0f, 1.0f}, false);
            debug->AddBoundingBox(tiptoeBoxB, groundTransform, Color{1.0f, 1.0f, 0.0f, 0.2f}, false);
        }
    }
}

bool IKLegSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    bendTarget_ = AddCheckedNode(nodeCache, bendTargetName_);
    groundTarget_ = AddCheckedNode(nodeCache, groundTargetName_);

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

    const IKNode& thighBone = *legChain_.GetBeginNode();
    const IKNode& calfBone = *legChain_.GetMiddleNode();
    const IKNode& heelBone = *legChain_.GetEndNode();
    const IKNode& toeBone = *footSegment_.endNode_;

    local_.toeToHeel_ = node_->GetWorldRotation().Inverse() * (heelBone.position_ - toeBone.position_);
    local_.defaultThighToToeDistance_ = (toeBone.position_ - thighBone.position_).Length();
    local_.tiptoeTweakOffset_ = local_.defaultThighToToeDistance_ * 0.5f;

    local_.defaultDirection_ = inverseFrameOfReference.rotation_ * node_->GetWorldRotation() * bendDirection_;
    local_.defaultFootRotation_ = calfBone.rotation_.Inverse() * heelBone.rotation_;
    local_.defaultToeOffset_ = heelBone.rotation_.Inverse() * (toeBone.position_ - heelBone.position_);
    local_.defaultToeRotation_ = heelBone.rotation_.Inverse() * toeBone.rotation_;
}

void IKLegSolver::EnsureInitialized()
{
    if (heelGroundOffset_ < 0.0f)
        UpdateHeelGroundOffset();

    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    rotationWeight_ = Clamp(rotationWeight_, 0.0f, 1.0f);
    bendWeight_ = Clamp(bendWeight_, 0.0f, 1.0f);
    minKneeAngle_ = Clamp(minKneeAngle_, 0.0f, 180.0f);
    maxKneeAngle_ = Clamp(maxKneeAngle_, 0.0f, 180.0f);
    baseTiptoe_ = VectorClamp(baseTiptoe_, Vector2::ZERO, Vector2::ONE);
    groundTiptoeTweaks_ = VectorClamp(groundTiptoeTweaks_, Vector4::ZERO, Vector4::ONE);
}

void IKLegSolver::UpdateHeelGroundOffset()
{
    Node* heelNode = node_->GetChild(heelBoneName_, true);
    if (heelNode)
    {
        const Vector3 heelOffset = heelNode->GetWorldPosition() - node_->GetWorldPosition();
        heelGroundOffset_ = ea::max(0.0f, heelOffset.y_);
    }
}

Vector3 IKLegSolver::GetTargetPosition() const
{
    IKNode& firstBone = *legChain_.GetBeginNode();

    const float minDistance = 0.001f;
    const float maxDistance = GetToeReachDistance();
    const Vector3& origin = firstBone.position_;
    const Vector3& target = target_->GetWorldPosition();
    return origin + (target - origin).ReNormalized(minDistance, maxDistance);
}

Plane IKLegSolver::GetGroundPlane() const
{
    Node* groundNode = groundTarget_ ? groundTarget_ : node_;
    return Plane{groundNode->GetWorldUp(), groundNode->GetWorldPosition()};
}

Vector2 IKLegSolver::ProjectOnGround(const Vector3& position) const
{
    Node* groundNode = groundTarget_ ? groundTarget_ : node_;
    const Vector3 right = groundNode->GetWorldRotation() * Vector3::RIGHT;
    const Vector3 forward = groundNode->GetWorldRotation() * Vector3::FORWARD;
    const Vector3 localPos = position - groundNode->GetWorldPosition();
    return {right.DotProduct(localPos), forward.DotProduct(localPos)};
}

float IKLegSolver::GetHeelReachDistance() const
{
    return GetMaxDistance(legChain_, maxKneeAngle_);
}

float IKLegSolver::GetToeReachDistance() const
{
    return GetHeelReachDistance() + footSegment_.length_;
}

Vector3 IKLegSolver::RecoverFromGroundPenetration(const Vector3& toeToHeel, const Vector3& toePosition) const
{
    const Plane groundPlane = GetGroundPlane();
    const Vector3& yAxis = groundPlane.normal_;
    const Vector3& xAxis = toeToHeel.Orthogonalize(yAxis);

    // Decompose the vector into vertical and horizontal components relative to ground normal
    //
    //      o-heel
    //     / } (x,y)
    //    o-toe
    //    | } y0
    // ___|_____
    const float x = xAxis.DotProduct(toeToHeel);
    const float y = yAxis.DotProduct(toeToHeel);
    const float y0 = groundPlane.Distance(toePosition);

    // Clamp heel y to the minimum distance from the ground
    const float len = footSegment_.length_;
    const float y2 = ea::min(ea::max(y, heelGroundOffset_ - y0), len);
    const float x2 = Sqrt(ea::max(len * len - y2 * y2, 0.0f)) * (x < 0.0f ? -1.0f : 1.0f);

    return xAxis * x2 + yAxis * y2;
}

Vector3 IKLegSolver::SnapToReachablePosition(const Vector3& toeToHeel, const Vector3& toePosition) const
{
    const auto& thighBone = *legChain_.GetBeginNode();
    const Sphere reachableSphere{thighBone.position_, GetHeelReachDistance()};
    if (reachableSphere.IsInside(toePosition + toeToHeel) != OUTSIDE)
        return toeToHeel;

    const Sphere availableSphere{toePosition, toeToHeel.Length()};
    const Circle availableHeelPositions = reachableSphere.Intersect(availableSphere);

    const Vector3 heelPosition = availableHeelPositions.GetPoint(toeToHeel);
    return heelPosition - toePosition;
}

ea::pair<Vector3, Vector3> IKLegSolver::CalculateBendDirections(
    const Transform& frameOfReference, const Vector3& toeTargetPosition) const
{
    IKNode& thighBone = *legChain_.GetBeginNode();

    const float bendTargetWeight = bendTarget_ ? bendWeight_ : 0.0f;
    const Vector3 bendTargetPosition = bendTarget_ ? bendTarget_->GetWorldPosition() : Vector3::ZERO;
    const Vector3 bendTargetDirection = bendTargetPosition - Lerp(thighBone.position_, toeTargetPosition, 0.5f);

    const Vector3 originalDirection = node_->GetWorldRotation() * bendDirection_;
    const Vector3 currentDirection0 = frameOfReference.rotation_ * local_.defaultDirection_;
    const Vector3 currentDirection1 = bendTargetDirection.Normalized();
    const Vector3 currentDirection = Lerp(currentDirection0, currentDirection1, bendTargetWeight);

    return {originalDirection, currentDirection};
}

Quaternion IKLegSolver::CalculateLegRotation(
    const Vector3& toeTargetPosition, const Vector3& originalDirection, const Vector3& currentDirection) const
{
    IKNode* thighBone = legChain_.GetBeginNode();
    IKNode* toeBone = footSegment_.endNode_;

    const Quaternion chainRotation = IKTrigonometricChain::CalculateRotation(
        thighBone->originalPosition_, toeBone->originalPosition_, originalDirection,
        thighBone->position_, toeTargetPosition, currentDirection);
    return chainRotation;
}

float IKLegSolver::CalculateTiptoeFactor(const Vector3& toeTargetPosition) const
{
    const auto& thighBone = *legChain_.GetBeginNode();
    const float thighToToeDistance = (toeTargetPosition - thighBone.position_).Length();
    const float stretchFactor = ea::min(thighToToeDistance / local_.defaultThighToToeDistance_, 1.0f);

    const Vector2 groundFactorXY = VectorClamp(
        ProjectOnGround(toeTargetPosition) / local_.tiptoeTweakOffset_,
        -Vector2::ONE, Vector2::ONE);

    const float baseTiptoe = Lerp(baseTiptoe_.x_, baseTiptoe_.y_, stretchFactor);
    const float tiptoeTweakX = groundFactorXY.x_ * (groundFactorXY.x_ < 0.0f ? -groundTiptoeTweaks_.x_ : groundTiptoeTweaks_.y_);
    const float tiptoeTweakY = groundFactorXY.y_ * (groundFactorXY.y_ < 0.0f ? -groundTiptoeTweaks_.z_ : groundTiptoeTweaks_.w_);
    return Clamp(baseTiptoe + tiptoeTweakX + tiptoeTweakY, 0.0f, baseTiptoe_.x_);
}

Vector3 IKLegSolver::CalculateToeToHeelBent(
    const Vector3& toeTargetPosition, const Vector3& approximateBendDirection) const
{
    IKNode* thighBone = legChain_.GetBeginNode();
    const auto [newPos1, newPos2] = IKTrigonometricChain::Solve(
        thighBone->position_, legChain_.GetFirstLength(), legChain_.GetSecondLength() + footSegment_.length_,
        toeTargetPosition, approximateBendDirection, minKneeAngle_, maxKneeAngle_);
    return (newPos1 - newPos2).Normalized() * footSegment_.length_;
}

Vector3 IKLegSolver::CalculateToeToHeel(const Transform& frameOfReference, float tiptoeFactor,
    const Vector3& toeTargetPosition, const Vector3& originalDirection, const Vector3& currentDirection) const
{
    const auto legRotation = CalculateLegRotation(
        toeTargetPosition, originalDirection, currentDirection);
    const Vector3 approximateBendDirection = legRotation * originalDirection;

    const Vector3 toeToHeelMin = legRotation * node_->GetWorldRotation() * local_.toeToHeel_;
    const Vector3 toeToHeelMax = CalculateToeToHeelBent(toeTargetPosition, approximateBendDirection);

    const Vector3 toeToHeelDirection = Lerp(toeToHeelMin, toeToHeelMax, tiptoeFactor);
    const Vector3 toeToHeel = toeToHeelDirection.ReNormalized(footSegment_.length_, footSegment_.length_);

    return SnapToReachablePosition(RecoverFromGroundPenetration(toeToHeel, toeTargetPosition), toeTargetPosition);
}

void IKLegSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    // Store original rotation
    IKNode& thighBone = *legChain_.GetBeginNode();
    IKNode& calfBone = *legChain_.GetMiddleNode();
    IKNode& heelBone = *legChain_.GetEndNode();
    IKNode& toeBone = *footSegment_.endNode_;

    const Quaternion thighBoneRotation = thighBone.rotation_;
    const Quaternion calfBoneRotation = calfBone.rotation_;
    const Quaternion heelBoneRotation = heelBone.rotation_;
    const Quaternion toeBoneRotation = toeBone.rotation_;

    // Solve rotations for full solver weight
    latestTargetPosition_ = GetTargetPosition();
    latestTiptoeFactor_ = CalculateTiptoeFactor(latestTargetPosition_);

    const auto [originalDirection, currentDirection] = CalculateBendDirections(frameOfReference, latestTargetPosition_);
    const Vector3 toeToHeel = CalculateToeToHeel(
        frameOfReference, latestTiptoeFactor_, latestTargetPosition_, originalDirection, currentDirection);
    const Vector3 heelTargetPosition = latestTargetPosition_ + toeToHeel;

    legChain_.Solve(heelTargetPosition, originalDirection, currentDirection, minKneeAngle_, maxKneeAngle_);
    RotateFoot(toeToHeel);

    // Interpolate rotation to apply solver weight
    thighBone.rotation_ = thighBoneRotation.Slerp(thighBone.rotation_, positionWeight_);
    calfBone.rotation_ = calfBoneRotation.Slerp(calfBone.rotation_, positionWeight_);
    heelBone.rotation_ = heelBoneRotation.Slerp(heelBone.rotation_, positionWeight_);
    toeBone.rotation_ = toeBoneRotation.Slerp(toeBone.rotation_, positionWeight_);

    // Apply target rotation if needed
    if (rotationWeight_ > 0.0f)
        toeBone.rotation_ = toeBone.rotation_.Slerp(target_->GetWorldRotation(), rotationWeight_);
}

void IKLegSolver::RotateFoot(const Vector3& toeToHeel)
{
    IKNode& calfBone = *legChain_.GetMiddleNode();
    IKNode& heelBone = *legChain_.GetEndNode();
    IKNode& toeBone = *footSegment_.endNode_;

    // heelBone.position should already set by legChain_.Solve()
    heelBone.previousPosition_ = heelBone.position_;
    heelBone.previousRotation_ = calfBone.rotation_ * local_.defaultFootRotation_;
    toeBone.previousPosition_ = heelBone.previousPosition_ + heelBone.previousRotation_ * local_.defaultToeOffset_;
    toeBone.previousRotation_ = heelBone.previousRotation_ * local_.defaultToeRotation_;
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
    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Twist Target Name", ea::string, twistTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Position Weight", float, positionWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Weight", float, rotationWeight_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Twist Weight", float, twistWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Angle", float, maxAngle_, 90.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Tweak", float, bendTweak_, 0.0f, AM_DEFAULT);

    URHO3D_ACTION_STATIC_LABEL("Update Properties", UpdateProperties, "Set properties below from current bone positions");
    URHO3D_ATTRIBUTE("Twist Rotation Offset", Quaternion, twistRotationOffset_, Quaternion::ZERO, AM_DEFAULT);
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

void IKSpineSolver::UpdateProperties()
{
    UpdateTwistRotationOffset();
}

bool IKSpineSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    if (!target_)
        return false;

    if (boneNames_.size() < 2)
    {
        URHO3D_LOGERROR("Spine solver must have at least 2 bones");
        return false;
    }

    IKSpineChain chain;
    for (const ea::string& boneName : boneNames_)
    {
        IKNode* bone = AddSolverNode(nodeCache, boneName);
        if (!bone)
            return false;

        chain.AddNode(bone);
    }

    if (!twistTargetName_.empty())
    {
        twistTarget_ = AddCheckedNode(nodeCache, twistTargetName_);
        if (!twistTarget_)
            return false;
    }

    SetParentAsFrameOfReference(*chain.GetNodes().front());
    chain_ = ea::move(chain);
    return true;
}

void IKSpineSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    chain_.UpdateLengths();

    const auto& bones = chain_.GetNodes();
    local_.defaultTransforms_.resize(bones.size());
    for (size_t i = 0; i < bones.size(); ++i)
    {
        const IKNode& bone = *bones[i];
        local_.defaultTransforms_[i] = inverseFrameOfReference * Transform{bone.position_, bone.rotation_};
    }

    const Vector3 baseDirection = (bones[1]->position_ - bones[0]->position_).Normalized();
    local_.baseDirection_ = inverseFrameOfReference.rotation_ * baseDirection;
}

void IKSpineSolver::SetOriginalTransforms(const Transform& frameOfReference)
{
    auto& nodes = chain_.GetNodes();
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        IKNode& bone = *nodes[i];
        const Transform& defaultTransform = local_.defaultTransforms_[i];
        bone.position_ = frameOfReference * defaultTransform.position_;
        bone.rotation_ = frameOfReference * defaultTransform.rotation_;
    }
}

void IKSpineSolver::EnsureInitialized()
{
    if (twistRotationOffset_ == Quaternion::ZERO)
        UpdateTwistRotationOffset();

    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    rotationWeight_ = Clamp(rotationWeight_, 0.0f, 1.0f);
    twistWeight_ = Clamp(twistWeight_, 0.0f, 1.0f);
    maxAngle_ = Clamp(maxAngle_, 0.0f, 180.0f);
}

void IKSpineSolver::UpdateTwistRotationOffset()
{
    if (boneNames_.size() >= 2)
    {
        const ea::string& twistBoneName = boneNames_[boneNames_.size() - 2];
        Node* boneNode = node_->GetChild(twistBoneName, true);
        if (boneNode)
            twistRotationOffset_ = node_->GetWorldRotation().Inverse() * boneNode->GetWorldRotation();
    }
}

float IKSpineSolver::WeightFunction(float x) const
{
    if (bendTweak_ == 0.0f)
        return 1.0f;
    else if (bendTweak_ > 0.0f)
    {
        if (bendTweak_ < 1.0f)
            return Lerp(1.0f - bendTweak_, 1.0f, x);
        else
            return Pow(x, bendTweak_);
    }
    else
    {
        if (bendTweak_ > -1.0f)
            return Lerp(1.0f - (-bendTweak_), 1.0f, 1.0f - x);
        else
            return Pow(1.0f - x, -bendTweak_);
    }
}

void IKSpineSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    auto& bones = chain_.GetNodes();
    if (bones.size() < 2)
        return;

    // Store original rotation
    originalBoneRotations_.resize(bones.size());
    for (size_t i = 0; i < bones.size(); ++i)
        originalBoneRotations_[i] = bones[i]->rotation_;

    // Solve rotations for full solver weight for position target
    SetOriginalTransforms(frameOfReference);
    const Vector3 baseDirection = frameOfReference.rotation_ * local_.baseDirection_;
    const auto weightFunction = [this](float x) { return WeightFunction(x); };
    chain_.Solve(target_->GetWorldPosition(), baseDirection, maxAngle_, settings, weightFunction);

    // Interpolate rotation to apply solver weight
    for (size_t i = 0; i < bones.size(); ++i)
        bones[i]->rotation_ = originalBoneRotations_[i].Slerp(bones[i]->rotation_, positionWeight_);

    // Solve rotations for partial solver weight for twist target
    const float twistAngle =
        twistTarget_ ? GetTwistAngle(frameOfReference, chain_.GetSegments().back(), twistTarget_) : 0.0f;
    chain_.Twist(twistAngle * twistWeight_, settings);

    // Apply target rotation if needed
    if (rotationWeight_ > 0.0f)
    {
        IKNode& lastBone = *bones.back();
        lastBone.rotation_ = lastBone.rotation_.Slerp(target_->GetWorldRotation(), rotationWeight_);
    }
}

float IKSpineSolver::GetTwistAngle(
    const Transform& frameOfReference, const IKNodeSegment& segment, Node* targetNode) const
{
    const Quaternion targetRotation =
        frameOfReference.rotation_.Inverse() * targetNode->GetWorldRotation() * twistRotationOffset_;
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
    URHO3D_ATTRIBUTE_EX("Bend Target Name", ea::string, bendTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Position Weight", float, positionWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Weight", float, rotationWeight_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Bend Weight", float, bendWeight_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Min Angle", float, minElbowAngle_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Angle", float, maxElbowAngle_, 180.0f, AM_DEFAULT);
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

    bendTarget_ = AddCheckedNode(nodeCache, bendTargetName_);

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
    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    rotationWeight_ = Clamp(rotationWeight_, 0.0f, 1.0f);
    bendWeight_ = Clamp(bendWeight_, 0.0f, 1.0f);
    minElbowAngle_ = Clamp(minElbowAngle_, 0.0f, 180.0f);
    maxElbowAngle_ = Clamp(maxElbowAngle_, 0.0f, 180.0f);
    shoulderWeight_ = VectorClamp(shoulderWeight_, Vector2::ZERO, Vector2::ONE);
}

void IKArmSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    // Store original rotation
    IKNode& shoulderBone = *shoulderSegment_.beginNode_;
    IKNode& armBone = *armChain_.GetBeginNode();
    IKNode& forearmBone = *armChain_.GetMiddleNode();
    IKNode& handBone = *armChain_.GetEndNode();

    const Quaternion shoulderBoneRotation = shoulderBone.rotation_;
    const Quaternion armBoneRotation = armBone.rotation_;
    const Quaternion forearmBoneRotation = forearmBone.rotation_;
    const Quaternion handBoneRotation = handBone.rotation_;

    // Solve rotations for full solver weight
    shoulderSegment_.beginNode_->rotation_ = frameOfReference * local_.shoulderRotation_;
    shoulderSegment_.endNode_->position_ = frameOfReference * local_.armOffset_;
    shoulderSegment_.endNode_->rotation_ = frameOfReference * local_.armRotation_;

    const Vector3 handTargetPosition = target_->GetWorldPosition();
    const auto [originalDirection, currentDirection] = CalculateBendDirections(
        frameOfReference, handTargetPosition);

    const Quaternion maxShoulderRotation = CalculateMaxShoulderRotation(handTargetPosition);
    const auto [swing, twist] = maxShoulderRotation.ToSwingTwist(frameOfReference.rotation_ * local_.up_);
    const Quaternion shoulderRotation = Quaternion::IDENTITY.Slerp(swing, shoulderWeight_.y_)
        * Quaternion::IDENTITY.Slerp(twist, shoulderWeight_.x_);
    RotateShoulder(shoulderRotation);

    armChain_.Solve(handTargetPosition, originalDirection, currentDirection, minElbowAngle_, maxElbowAngle_);

    // Interpolate rotation to apply solver weight
    shoulderBone.rotation_ = shoulderBoneRotation.Slerp(shoulderBone.rotation_, positionWeight_);
    armBone.rotation_ = armBoneRotation.Slerp(armBone.rotation_, positionWeight_);
    forearmBone.rotation_ = forearmBoneRotation.Slerp(forearmBone.rotation_, positionWeight_);
    handBone.rotation_ = handBoneRotation.Slerp(handBone.rotation_, positionWeight_);

    // Apply target rotation if needed
    if (rotationWeight_ > 0.0f)
        handBone.rotation_ = handBone.rotation_.Slerp(target_->GetWorldRotation(), rotationWeight_);
}

void IKArmSolver::RotateShoulder(const Quaternion& rotation)
{
    const Vector3 shoulderPosition = shoulderSegment_.beginNode_->position_;
    shoulderSegment_.beginNode_->RotateAround(shoulderPosition, rotation);
    shoulderSegment_.endNode_->RotateAround(shoulderPosition, rotation);
}

ea::pair<Vector3, Vector3> IKArmSolver::CalculateBendDirections(
    const Transform& frameOfReference, const Vector3& handTargetPosition) const
{
    IKNode& shoulderBone = *shoulderSegment_.beginNode_;

    const float bendTargetWeight = bendTarget_ ? bendWeight_ : 0.0f;
    const Vector3 bendTargetPosition = bendTarget_ ? bendTarget_->GetWorldPosition() : Vector3::ZERO;
    const Vector3 bendTargetDirection = bendTargetPosition - Lerp(shoulderBone.position_, handTargetPosition, 0.5f);

    const Vector3 originalDirection = node_->GetWorldRotation() * bendDirection_;
    const Vector3 currentDirection0 = frameOfReference.rotation_ * local_.defaultDirection_;
    const Vector3 currentDirection1 = bendTargetDirection.Normalized();
    const Vector3 currentDirection = Lerp(currentDirection0, currentDirection1, bendTargetWeight);

    return {originalDirection, currentDirection};
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

IKHeadSolver::IKHeadSolver(Context* context)
    : IKSolverComponent(context)
{
}

IKHeadSolver::~IKHeadSolver()
{
}

void IKHeadSolver::RegisterObject(Context* context)
{
    context->AddFactoryReflection<IKHeadSolver>(Category_IK);

    URHO3D_ATTRIBUTE_EX("Neck Bone Name", ea::string, neckBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Head Bone Name", ea::string, headBoneName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Target Name", ea::string, targetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Look At Target Name", ea::string, lookAtTargetName_, OnTreeDirty, EMPTY_STRING, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Position Weight", float, positionWeight_, OnTreeDirty, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Direction Weight", float, directionWeight_, OnTreeDirty, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Rotation Weight", float, rotationWeight_, OnTreeDirty, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Look At Weight", float, lookAtWeight_, OnTreeDirty, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Eye Direction", Vector3, eyeDirection_, OnTreeDirty, Vector3::FORWARD, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Eye Offset", Vector3, eyeOffset_, OnTreeDirty, Vector3::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Neck Weight", float, neckWeight_, OnTreeDirty, 0.5f, AM_DEFAULT);
}

void IKHeadSolver::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
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
    if (target_)
        DrawIKTarget(debug, target_, true);
    if (lookAtTarget_)
        DrawIKTarget(debug, lookAtTarget_, false);
}

bool IKHeadSolver::InitializeNodes(IKNodeCache& nodeCache)
{
    target_ = AddCheckedNode(nodeCache, targetName_);
    lookAtTarget_ = AddCheckedNode(nodeCache, lookAtTargetName_);
    if (!target_ && !lookAtTarget_)
    {
        URHO3D_LOGERROR("IKHeadSolver: Either head or look at target must be specified");
        return false;
    }

    IKNode* neckBone = AddSolverNode(nodeCache, neckBoneName_);
    if (!neckBone)
        return false;

    IKNode* headBone = AddSolverNode(nodeCache, headBoneName_);
    if (!headBone)
        return false;

    SetParentAsFrameOfReference(*neckBone);
    neckChain_.Initialize(neckBone);
    headChain_.Initialize(headBone);
    neckSegment_ = {neckBone, headBone};
    return true;
}

void IKHeadSolver::UpdateChainLengths(const Transform& inverseFrameOfReference)
{
    neckSegment_.UpdateLength();

    const IKNode& neckBone = *neckSegment_.beginNode_;
    const IKNode& headBone = *neckSegment_.endNode_;

    local_.defaultNeckTransform_ = inverseFrameOfReference * Transform{neckBone.position_, neckBone.rotation_};
    local_.defaultHeadTransform_ = inverseFrameOfReference * Transform{headBone.position_, headBone.rotation_};

    const Vector3 eyeDirection = node_->GetWorldRotation() * eyeDirection_;
    const Vector3 eyeOffset = node_->GetWorldRotation() * eyeOffset_;
    neckChain_.SetWorldEyeTransform(eyeOffset, eyeDirection);
    headChain_.SetWorldEyeTransform(eyeOffset, eyeDirection);
}

void IKHeadSolver::EnsureInitialized()
{
    positionWeight_ = Clamp(positionWeight_, 0.0f, 1.0f);
    rotationWeight_ = Clamp(rotationWeight_, 0.0f, 1.0f);
    directionWeight_ = Clamp(directionWeight_, 0.0f, 1.0f);
    lookAtWeight_ = Clamp(lookAtWeight_, 0.0f, 1.0f);
    neckWeight_ = Clamp(neckWeight_, 0.0f, 1.0f);
}

void IKHeadSolver::SolvePosition()
{
    IKNode& neckBone = *neckSegment_.beginNode_;
    IKNode& headBone = *neckSegment_.endNode_;

    const Vector3 targetPosition = target_->GetWorldPosition();
    const Quaternion rotation{headBone.position_ - neckBone.position_, targetPosition - neckBone.position_};
    const Quaternion scaledRotation = Quaternion::IDENTITY.Slerp(rotation, positionWeight_);

    neckBone.RotateAround(neckBone.position_, scaledRotation);
    headBone.RotateAround(neckBone.position_, scaledRotation);

    neckBone.MarkRotationDirty();
    headBone.MarkRotationDirty();
}

void IKHeadSolver::SolveRotation()
{
    IKNode& headBone = *neckSegment_.endNode_;

    const Quaternion rotation = target_->GetWorldRotation();

    headBone.rotation_ = headBone.rotation_.Slerp(rotation, rotationWeight_);

    headBone.MarkRotationDirty();
}

void IKHeadSolver::SolveDirection()
{
    IKNode& headBone = *neckSegment_.endNode_;

    const Vector3 direction = target_->GetWorldDirection();
    const Quaternion rotation = headChain_.SolveLookTo(direction);
    const Quaternion scaledRotation = Quaternion::IDENTITY.Slerp(rotation, directionWeight_);

    headBone.rotation_ = scaledRotation * headBone.rotation_;

    headBone.MarkRotationDirty();
}

void IKHeadSolver::SolveLookAt(const Transform& frameOfReference, const IKSettings& settings)
{
    // Store original rotation
    IKNode& neckBone = *neckSegment_.beginNode_;
    IKNode& headBone = *neckSegment_.endNode_;

    const Quaternion neckBoneRotation = neckBone.rotation_;
    const Quaternion headBoneRotation = headBone.rotation_;

    // Reset transforms before solving
    neckBone.rotation_ = frameOfReference * local_.defaultNeckTransform_.rotation_;
    headBone.position_ = frameOfReference * local_.defaultHeadTransform_.position_;
    headBone.rotation_ = frameOfReference * local_.defaultHeadTransform_.rotation_;
    neckBone.StorePreviousTransform();
    headBone.StorePreviousTransform();

    const Vector3 lookAtTarget = lookAtTarget_->GetWorldPosition();

    const Quaternion neckRotation = neckChain_.SolveLookAt(lookAtTarget, settings);
    const Quaternion neckRotationWeighted = Quaternion::IDENTITY.Slerp(neckRotation, neckWeight_);
    neckBone.rotation_ = neckRotationWeighted * neckBone.rotation_;
    headBone.RotateAround(neckBone.position_, neckRotationWeighted);

    const Quaternion headRotation = headChain_.SolveLookAt(lookAtTarget, settings);
    headBone.rotation_ = headRotation * headBone.rotation_;

    neckBone.MarkRotationDirty();
    headBone.MarkRotationDirty();

    // Interpolate rotation to apply solver weight
    neckBone.rotation_ = neckBoneRotation.Slerp(neckBone.rotation_, lookAtWeight_);
    headBone.rotation_ = headBoneRotation.Slerp(headBone.rotation_, lookAtWeight_);
}

void IKHeadSolver::SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep)
{
    EnsureInitialized();

    if (target_ && positionWeight_ > 0.0f)
        SolvePosition();

    if (target_ && rotationWeight_ > 0.0f)
        SolveRotation();

    if (target_ && directionWeight_ > 0.0f)
        SolveDirection();

    if (lookAtTarget_ && lookAtWeight_ > 0.0f)
        SolveLookAt(frameOfReference, settings);
}

Ray IKHeadSolver::GetEyeRay() const
{
    const IKNode& headBone = *neckSegment_.endNode_;
    const Vector3 origin = headBone.position_ + headBone.rotation_ * headChain_.GetLocalEyeOffset();
    const Vector3 direction = headBone.rotation_ * headChain_.GetLocalEyeDirection();
    return {origin, direction};
}

}
