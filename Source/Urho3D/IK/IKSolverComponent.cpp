// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IKSolverComponent.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IK/IKSolver.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Scene/Node.h"

namespace Urho3D
{

namespace
{

void DrawNode(DebugRenderer* debug, bool oriented, const Vector3& position, const Quaternion& rotation,
    const Color& color, float radius)
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

} // namespace

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

void IKSolverComponent::DrawIKTarget(
    DebugRenderer* debug, const Vector3& position, const Quaternion& rotation, bool oriented) const
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

ea::pair<Vector3, Vector3> IKSolverComponent::CalculateBendDirectionsInternal(
    const Transform& frameOfReference, const BendCalculationParams& params)
{
    const float bendTargetWeight = params.bendTarget_ ? params.bendTargetWeight_ : 0.0f;
    const Vector3 bendTargetPosition = params.bendTarget_ ? params.bendTarget_->GetWorldPosition() : Vector3::ZERO;
    const Vector3 bendTargetDirection = bendTargetPosition - Lerp(params.startPosition_, params.targetPosition_, 0.5f);

    const Quaternion chainRotation{frameOfReference.rotation_ * params.targetDirectionInLocalSpace_,
        params.targetPosition_ - params.startPosition_};
    const Vector3 originalDirection = params.parentNodeRotation_ * params.bendDirectionInNodeSpace_;
    const Vector3 currentDirection0 = chainRotation * frameOfReference.rotation_ * params.bendDirectionInLocalSpace_;
    const Vector3 currentDirection1 = bendTargetDirection.Normalized();
    const Vector3 currentDirection = Lerp(currentDirection0, currentDirection1, bendTargetWeight);

    return {originalDirection, currentDirection};
}

float IKSolverComponent::GetMaxDistance(const IKTrigonometricChain& chain, float maxAngle)
{
    const float a = chain.GetFirstLength();
    const float b = chain.GetSecondLength();
    return Sqrt(a * a + b * b - 2 * a * b * Cos(maxAngle));
}

} // namespace Urho3D
