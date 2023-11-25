// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/InverseKinematics.h"
#include "Urho3D/Math/Ray.h"
#include "Urho3D/Math/Transform.h"
#include "Urho3D/Scene/Component.h"

namespace Urho3D
{

using IKNodeCache = ea::unordered_map<WeakPtr<Node>, IKNode>;

/// Base component for all IK solvers.
class URHO3D_API IKSolverComponent : public Component
{
    URHO3D_OBJECT(IKSolverComponent, Component);

public:
    explicit IKSolverComponent(Context* context);
    ~IKSolverComponent() override;
    static void RegisterObject(Context* context);

    bool Initialize(IKNodeCache& nodeCache);
    void NotifyPositionsReady();
    void Solve(const IKSettings& settings, float timeStep);

    /// Internal. Marks chain tree as dirty.
    void OnTreeDirty();

protected:
    virtual bool InitializeNodes(IKNodeCache& nodeCache) = 0;
    virtual void UpdateChainLengths(const Transform& inverseFrameOfReference) = 0;
    virtual void SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep) = 0;

    void OnNodeSet(Node* previousNode, Node* currentNode) override;

    /// Add node to cache by name. Return null if the node is not found.
    IKNode* AddSolverNode(IKNodeCache& nodeCache, const ea::string& name);
    /// Add node that should be checked for existence before solving.
    Node* AddCheckedNode(IKNodeCache& nodeCache, const ea::string& name) const;
    /// Find Node corresponding to solver IKNode. Suboptimal, prefer to call it on initialization only!
    Node* FindNode(const IKNode& node) const;
    /// Set frame of reference Node used for calculations.
    void SetFrameOfReference(Node* node);
    void SetFrameOfReference(const IKNode& node);
    /// Same as SetFrameOfReference, except it accepts first child of the node.
    void SetParentAsFrameOfReference(const IKNode& childNode);

    /// Draw IK node in DebugRenderer.
    void DrawIKNode(DebugRenderer* debug, const IKNode& node, bool oriented) const;
    /// Draw IK segment line in DebugRenderer.
    void DrawIKSegment(DebugRenderer* debug, const IKNode& beginNode, const IKNode& endNode) const;
    /// Draw IK target in DebugRenderer.
    void DrawIKTarget(DebugRenderer* debug, const Vector3& position, const Quaternion& rotation, bool oriented) const;
    void DrawIKTarget(DebugRenderer* debug, const Node* node, bool oriented) const;
    /// Draw direction in DebugRenderer.
    void DrawDirection(DebugRenderer* debug, const Vector3& position, const Vector3& direction, bool markBegin = false,
        bool markEnd = true) const;

protected:
    struct BendCalculationParams
    {
        Quaternion parentNodeRotation_;
        Vector3 startPosition_;
        Vector3 targetPosition_;

        Vector3 targetDirectionInLocalSpace_;
        Vector3 bendDirectionInNodeSpace_;
        Vector3 bendDirectionInLocalSpace_;

        Node* bendTarget_{};
        float bendTargetWeight_{};
    };

    static ea::pair<Vector3, Vector3> CalculateBendDirectionsInternal(
        const Transform& frameOfReference, const BendCalculationParams& params);

    static float GetMaxDistance(const IKTrigonometricChain& chain, float maxAngle);

private:
    Transform GetFrameOfReferenceTransform() const;

    ea::vector<ea::pair<Node*, IKNode*>> solverNodes_;
    WeakPtr<Node> frameOfReferenceNode_;
};

} // namespace Urho3D
