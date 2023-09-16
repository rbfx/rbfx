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

#pragma once

#include "../Math/InverseKinematics.h"
#include "../Math/Ray.h"
#include "../Math/Transform.h"
#include "../Scene/Component.h"

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
    void DrawDirection(DebugRenderer* debug, const Vector3& position, const Vector3& direction,
        bool markBegin = false, bool markEnd = true) const;

private:
    Transform GetFrameOfReferenceTransform() const;

    ea::vector<ea::pair<Node*, IKNode*>> solverNodes_;
    WeakPtr<Node> frameOfReferenceNode_;
};

class URHO3D_API IKIdentitySolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKIdentitySolver, IKSolverComponent);

public:
    explicit IKIdentitySolver(Context* context);
    ~IKIdentitySolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    void UpdateProperties();

    /// Attributes.
    /// @{
    void SetBoneName(const ea::string& name) { boneName_ = name; OnTreeDirty(); }
    const ea::string& GetBoneName() const { return boneName_; }
    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }

    void SetRotationOffset(const Quaternion& rotation) { rotationOffset_ = rotation; }
    const Quaternion& GetRotationOffset() const { return rotationOffset_; }
    /// @}

private:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths(const Transform& inverseFrameOfReference) override;
    void SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep) override;
    /// @}

    void EnsureInitialized();
    void UpdateRotationOffset();

    ea::string boneName_;
    ea::string targetName_;
    Quaternion rotationOffset_{Quaternion::ZERO};

    IKNode* boneNode_{};
    WeakPtr<Node> target_;
};

class URHO3D_API IKLimbSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKLimbSolver, IKSolverComponent);

public:
    explicit IKLimbSolver(Context* context);
    ~IKLimbSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    void SetFirstBoneName(const ea::string& name) { firstBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetFirstBoneName() const { return firstBoneName_; }
    void SetSecondBoneName(const ea::string& name) { secondBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetSecondBoneName() const { return secondBoneName_; }
    void SetThirdBoneName(const ea::string& name) { thirdBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetThirdBoneName() const { return thirdBoneName_; }

    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }
    void SetBendTargetName(const ea::string& name) { bendTargetName_ = name; OnTreeDirty(); }
    const ea::string& GetBendTargetName() const { return bendTargetName_; }

    void SetPositionWeight(float weight) { positionWeight_ = weight; }
    float GetPositionWeight() const { return positionWeight_; }
    void SetRotationWeight(float weight) { rotationWeight_ = weight; }
    float GetRotationWeight() const { return rotationWeight_; }
    void SetBendWeight(float weight) { bendWeight_ = weight; }
    float GetBendWeight() const { return bendWeight_; }
    void SetMinAngle(float angle) { minAngle_ = angle; }
    float GetMinAngle() const { return minAngle_; }
    void SetMaxAngle(float angle) { maxAngle_ = angle; }
    float GetMaxAngle() const { return maxAngle_; }
    void SetBendDirection(const Vector3& direction) { bendDirection_ = direction; }
    const Vector3& GetBendDirection() const { return bendDirection_; }
    /// @}

private:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths(const Transform& inverseFrameOfReference) override;
    void SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep) override;
    /// @}

    void EnsureInitialized();
    Vector3 GetTargetPosition() const;
    ea::pair<Vector3, Vector3> CalculateBendDirections(
        const Transform& frameOfReference, const Vector3& targetPosition) const;

    ea::string firstBoneName_;
    ea::string secondBoneName_;
    ea::string thirdBoneName_;

    ea::string targetName_;
    ea::string bendTargetName_;

    float positionWeight_{1.0f};
    float rotationWeight_{0.0f};
    float bendWeight_{1.0f};
    float minAngle_{0.0f};
    float maxAngle_{180.0f};
    Vector3 bendDirection_{Vector3::FORWARD};

    IKTrigonometricChain chain_;
    WeakPtr<Node> target_;
    WeakPtr<Node> bendTarget_;

    struct LocalCache
    {
        Vector3 bendDirection_;
        Vector3 targetDirection_;
    } local_;

    Vector3 latestTargetPosition_;
};

class URHO3D_API IKLegSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKLegSolver, IKSolverComponent);

public:
    explicit IKLegSolver(Context* context);
    ~IKLegSolver() override;
    static void RegisterObject(Context* context);

    void UpdateProperties();

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    void SetThighBoneName(const ea::string& name) { thighBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetThighBoneName() const { return thighBoneName_; }
    void SetCalfBoneName(const ea::string& name) { calfBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetCalfBoneName() const { return calfBoneName_; }
    void SetHeelBoneName(const ea::string& name) { heelBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetHeelBoneName() const { return heelBoneName_; }
    void SetToeBoneName(const ea::string& name) { toeBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetToeBoneName() const { return toeBoneName_; }

    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }
    void SetBendTargetName(const ea::string& name) { bendTargetName_ = name; OnTreeDirty(); }
    const ea::string& GetBendTargetName() const { return bendTargetName_; }
    void SetGroundTargetName(const ea::string& name) { groundTargetName_ = name; OnTreeDirty(); }
    const ea::string& GetGroundTargetName() const { return groundTargetName_; }

    void SetPositionWeight(float weight) { positionWeight_ = weight; }
    float GetPositionWeight() const { return positionWeight_; }
    void SetRotationWeight(float weight) { rotationWeight_ = weight; }
    float GetRotationWeight() const { return rotationWeight_; }
    void SetBendWeight(float weight) { bendWeight_ = weight; }
    float GetBendWeight() const { return bendWeight_; }
    void SetMinAngle(float angle) { minKneeAngle_ = angle; }
    float GetMinAngle() const { return minKneeAngle_; }
    void SetMaxAngle(float angle) { maxKneeAngle_ = angle; }
    float GetMaxAngle() const { return maxKneeAngle_; }
    void SetBaseTiptoe(const Vector2& value) { baseTiptoe_ = value; }
    const Vector2& GetBaseTiptoe() const { return baseTiptoe_; }
    void SetGroundTiptoeTweaks(const Vector4& value) { groundTiptoeTweaks_ = value; }
    const Vector4& GetGroundTiptoeTweaks() const { return groundTiptoeTweaks_; }
    void SetBendDirection(const Vector3& direction) { bendDirection_ = direction; }
    const Vector3& GetBendDirection() const { return bendDirection_; }

    void SetHeelGroundOffset(float offset) { heelGroundOffset_ = offset; }
    float GetHeelGroundOffset() const { return heelGroundOffset_; }
    /// @}

private:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths(const Transform& inverseFrameOfReference) override;
    void SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep) override;
    /// @}

    void EnsureInitialized();
    void UpdateHeelGroundOffset();

    void RotateFoot(const Vector3& toeToHeel);

    Vector3 GetTargetPosition() const;
    Plane GetGroundPlane() const;
    Vector2 ProjectOnGround(const Vector3& position) const;
    float GetHeelReachDistance() const;
    float GetToeReachDistance() const;
    Vector3 RecoverFromGroundPenetration(const Vector3& toeToHeel, const Vector3& toePosition) const;
    Vector3 SnapToReachablePosition(const Vector3& toeToHeel, const Vector3& toePosition) const;

    ea::pair<Vector3, Vector3> CalculateBendDirections(
        const Transform& frameOfReference, const Vector3& toeTargetPosition) const;
    Quaternion CalculateLegRotation(
        const Vector3& toeTargetPosition, const Vector3& originalDirection, const Vector3& currentDirection) const;
    Vector2 CalculateLegOffset(
        const Transform& frameOfReference, const Vector3& toeTargetPosition, const Vector3& bendDirection);
    float CalculateTiptoeFactor(const Vector3& toeTargetPosition) const;

    Vector3 CalculateToeToHeel(const Transform& frameOfReference, float tiptoeFactor,
        const Vector3& toeTargetPosition, const Vector3& originalDirection, const Vector3& currentDirection) const;
    Vector3 CalculateToeToHeelBent(
        const Vector3& toeTargetPosition, const Vector3& approximateBendDirection) const;

    /// Attributes.
    /// @{
    ea::string thighBoneName_;
    ea::string calfBoneName_;
    ea::string heelBoneName_;
    ea::string toeBoneName_;

    ea::string targetName_;
    ea::string bendTargetName_;
    ea::string groundTargetName_;

    float positionWeight_{1.0f};
    float rotationWeight_{0.0f};
    float bendWeight_{1.0f};
    float minKneeAngle_{0.0f};
    float maxKneeAngle_{180.0f};
    Vector2 baseTiptoe_{0.5f, 0.0f};
    Vector4 groundTiptoeTweaks_{0.2f, 0.2f, 0.2f, 0.2f};
    Vector3 bendDirection_{Vector3::FORWARD};

    float heelGroundOffset_{-1.0f};
    /// @}

    /// IK nodes and effectors.
    /// @{
    IKTrigonometricChain legChain_;
    IKNodeSegment footSegment_;
    WeakPtr<Node> target_;
    WeakPtr<Node> bendTarget_;
    WeakPtr<Node> groundTarget_;
    /// @}

    struct LocalCache
    {
        Vector3 toeToHeel_;
        float defaultThighToToeDistance_{};
        float tiptoeTweakOffset_{};

        Vector3 bendDirection_;
        Vector3 targetDirection_;
        Quaternion defaultFootRotation_;
        Vector3 defaultToeOffset_;
        Quaternion defaultToeRotation_;
    } local_;

    Vector3 latestTargetPosition_;
    float latestTiptoeFactor_{};
};

class URHO3D_API IKSpineSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKSpineSolver, IKSolverComponent);

public:
    explicit IKSpineSolver(Context* context);
    ~IKSpineSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    void UpdateProperties();

    /// Attributes.
    /// @{
    void SetBoneNames(const StringVector& names) { boneNames_ = names; OnTreeDirty(); }
    const StringVector& GetBoneNames() const { return boneNames_; }
    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }
    void SetTwistTargetName(const ea::string& name) { twistTargetName_ = name; OnTreeDirty(); }
    const ea::string& GetTwistTargetName() const { return twistTargetName_; }

    void SetPositionWeight(float weight) { positionWeight_ = weight; }
    float GetPositionWeight() const { return positionWeight_; }
    void SetRotationWeight(float weight) { rotationWeight_ = weight; }
    float GetRotationWeight() const { return rotationWeight_; }
    void SetTwistWeight(float weight) { twistWeight_ = weight; }
    float GetTwistWeight() const { return twistWeight_; }
    void SetMaxAngle(float angle) { maxAngle_ = angle; }
    float GetMaxAngle() const { return maxAngle_; }
    void SetBendTweak(float tweak) { bendTweak_ = tweak; }
    float GetBendTweak() const { return bendTweak_; }
    /// @}

protected:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths(const Transform& inverseFrameOfReference) override;
    void SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep) override;
    /// @}

private:
    void EnsureInitialized();
    void UpdateTwistRotationOffset();

    float WeightFunction(float x) const;

    void SetOriginalTransforms(const Transform& frameOfReference);
    float GetTwistAngle(const Transform& frameOfReference, const IKNodeSegment& segment, Node* targetNode) const;

    StringVector boneNames_;
    ea::string twistTargetName_;
    ea::string targetName_;

    float positionWeight_{1.0f};
    float rotationWeight_{0.0f};
    float twistWeight_{1.0f};
    float maxAngle_{90.0f};
    float bendTweak_{0.0f};
    /// This orientation of twist bone in object space is equivalent to having no twist.
    Quaternion twistRotationOffset_{Quaternion::ZERO};

    IKSpineChain chain_;
    WeakPtr<Node> target_;
    WeakPtr<Node> twistTarget_;

    // TODO: Rename "local cache"
    struct LocalCache
    {
        ea::vector<Transform> defaultTransforms_;
        Vector3 baseDirection_;
        Quaternion zeroTwistRotation_;
    } local_;

    ea::vector<Quaternion> originalBoneRotations_;
};

class URHO3D_API IKArmSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKArmSolver, IKSolverComponent);

public:
    explicit IKArmSolver(Context* context);
    ~IKArmSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    void SetShoulderBoneName(const ea::string& name) { shoulderBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetShoulderBoneName() const { return shoulderBoneName_; }
    void SetArmBoneName(const ea::string& name) { armBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetArmBoneName() const { return armBoneName_; }
    void SetForearmBoneName(const ea::string& name) { forearmBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetForearmBoneName() const { return forearmBoneName_; }
    void SetHandBoneName(const ea::string& name) { handBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetHandBoneName() const { return handBoneName_; }

    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }
    void SetBendTargetName(const ea::string& name) { bendTargetName_ = name; OnTreeDirty(); }
    const ea::string& GetBendTargetName() const { return bendTargetName_; }

    void SetPositionWeight(float weight) { positionWeight_ = weight; }
    float GetPositionWeight() const { return positionWeight_; }
    void SetRotationWeight(float weight) { rotationWeight_ = weight; }
    float GetRotationWeight() const { return rotationWeight_; }
    void SetBendWeight(float weight) { bendWeight_ = weight; }
    float GetBendWeight() const { return bendWeight_; }
    void SetMinAngle(float angle) { minElbowAngle_ = angle; }
    float GetMinAngle() const { return minElbowAngle_; }
    void SetMaxAngle(float angle) { maxElbowAngle_ = angle; }
    float GetMaxAngle() const { return maxElbowAngle_; }
    void SetShoulderWeight(const Vector2& weight) { shoulderWeight_ = weight; }
    const Vector2& GetShoulderWeight() const { return shoulderWeight_; }
    void SetBendDirection(const Vector3& direction) { bendDirection_ = direction; }
    const Vector3& GetBendDirection() const { return bendDirection_; }
    void SetUpDirection(const Vector3& direction) { upDirection_ = direction; }
    const Vector3& GetUpDirection() const { return upDirection_; }
    /// @}

private:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths(const Transform& inverseFrameOfReference) override;
    void SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep) override;
    /// @}

    void EnsureInitialized();

    void RotateShoulder(const Quaternion& rotation);

    ea::pair<Vector3, Vector3> CalculateBendDirections(
        const Transform& frameOfReference, const Vector3& handTargetPosition) const;
    Quaternion CalculateMaxShoulderRotation(const Vector3& handTargetPosition) const;

    /// Attributes.
    /// @{
    ea::string shoulderBoneName_;
    ea::string armBoneName_;
    ea::string forearmBoneName_;
    ea::string handBoneName_;

    ea::string targetName_;
    ea::string bendTargetName_;

    float positionWeight_{1.0f};
    float rotationWeight_{0.0f};
    float bendWeight_{1.0f};
    float minElbowAngle_{0.0f};
    float maxElbowAngle_{180.0f};
    Vector2 shoulderWeight_{};
    Vector3 bendDirection_{Vector3::FORWARD};
    Vector3 upDirection_{Vector3::UP};
    /// @}

    /// IK nodes and effectors.
    /// @{
    IKTrigonometricChain armChain_;
    IKNodeSegment shoulderSegment_;
    WeakPtr<Node> target_;
    WeakPtr<Node> bendTarget_;
    /// @}

    struct LocalCache
    {
        Vector3 bendDirection_;
        Vector3 up_;
        Vector3 targetDirection_;

        Quaternion shoulderRotation_;
        Vector3 armOffset_;
        Quaternion armRotation_;
    } local_;
};

class URHO3D_API IKChainSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKChainSolver, IKSolverComponent);

public:
    explicit IKChainSolver(Context* context);
    ~IKChainSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    void SetBoneNames(const StringVector& names) { boneNames_ = names; OnTreeDirty(); }
    const StringVector& GetBoneNames() const { return boneNames_; }
    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }
    /// @}

protected:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths(const Transform& inverseFrameOfReference) override;
    void SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep) override;
    /// @}

private:
    StringVector boneNames_;
    ea::string targetName_;

    IKFabrikChain chain_;
    WeakPtr<Node> target_;
};

class URHO3D_API IKHeadSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKHeadSolver, IKSolverComponent);

public:
    explicit IKHeadSolver(Context* context);
    ~IKHeadSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    void SetNeckBoneName(const ea::string& name) { neckBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetNeckBoneName() const { return neckBoneName_; }
    void SetHeadBoneName(const ea::string& name) { headBoneName_ = name; OnTreeDirty(); }
    const ea::string& GetHeadBoneName() const { return headBoneName_; }

    void SetTargetName(const ea::string& name) { targetName_ = name; OnTreeDirty(); }
    const ea::string& GetTargetName() const { return targetName_; }
    void SetLookAtTargetName(const ea::string& name) { lookAtTargetName_ = name; OnTreeDirty(); }
    const ea::string& GetLookAtTargetName() const { return lookAtTargetName_; }

    void SetPositionWeight(float weight) { positionWeight_ = weight; }
    float GetPositionWeight() const { return positionWeight_; }
    void SetRotationWeight(float weight) { rotationWeight_ = weight; }
    float GetRotationWeight() const { return rotationWeight_; }
    void SetDirectionWeight(float weight) { directionWeight_ = weight; }
    float GetDirectionWeight() const { return directionWeight_; }
    void SetLookAtWeight(float weight) { lookAtWeight_ = weight; }
    float GetLookAtWeight() const { return lookAtWeight_; }
    void SetEyeDirection(const Vector3& direction) { eyeDirection_ = direction; }
    const Vector3& GetEyeDirection() const { return eyeDirection_; }
    void SetEyeOffset(const Vector3& offset) { eyeOffset_ = offset; }
    const Vector3& GetEyeOffset() const { return eyeOffset_; }
    void SetNeckWeight(float weight) { neckWeight_ = weight; }
    float GetNeckWeight() const { return neckWeight_; }
    /// @}

protected:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths(const Transform& inverseFrameOfReference) override;
    void SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep) override;
    /// @}

private:
    void EnsureInitialized();

    void SolvePosition();
    void SolveRotation();
    void SolveDirection();
    void SolveLookAt(const Transform& frameOfReference, const IKSettings& settings);

    Ray GetEyeRay() const;

    ea::string neckBoneName_;
    ea::string headBoneName_;
    ea::string targetName_;
    ea::string lookAtTargetName_;

    float positionWeight_{1.0f};
    float rotationWeight_{0.0f};
    float directionWeight_{1.0f};
    float lookAtWeight_{0.0f};
    Vector3 eyeDirection_{Vector3::FORWARD};
    Vector3 eyeOffset_;
    float neckWeight_{0.5f};

    IKNodeSegment neckSegment_;
    IKEyeChain neckChain_;
    IKEyeChain headChain_;

    WeakPtr<Node> target_;
    WeakPtr<Node> lookAtTarget_;

    struct LocalCache
    {
        Transform defaultNeckTransform_;
        Transform defaultHeadTransform_;
    } local_;
};

class URHO3D_API IKStickTargets : public IKSolverComponent
{
    URHO3D_OBJECT(IKStickTargets, IKSolverComponent);

public:
    static constexpr float DefaultPositionThreshold = 0.3f;
    static constexpr float DefaultRotationThreshold = 45.0f;
    static constexpr float DefaultTimeThreshold = 0.8f;
    static constexpr float DefaultRecoverTime = 0.2f;

    explicit IKStickTargets(Context* context);
    ~IKStickTargets() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Attributes.
    /// @{
    void SetTargetNames(const StringVector& names) { targetNames_ = names; OnTreeDirty(); }
    const StringVector& GetTargetNames() const { return targetNames_; }
    void SetPositionSticky(bool value) { isPositionSticky_ = value; }
    bool IsPositionSticky() const { return isPositionSticky_; }
    void SetRotationSticky(bool value) { isRotationSticky_ = value; }
    bool IsRotationSticky() const { return isRotationSticky_; }
    void SetPositionThreshold(float threshold) { positionThreshold_ = threshold; }
    float GetPositionThreshold() const { return positionThreshold_; }
    void SetRotationThreshold(float threshold) { rotationThreshold_ = threshold; }
    float GetRotationThreshold() const { return rotationThreshold_; }
    void SetTimeThreshold(float threshold) { timeThreshold_ = threshold; }
    float GetTimeThreshold() const { return timeThreshold_; }
    void SetRecoverTime(float time) { recoverTime_ = time; }
    float GetRecoverTime() const { return recoverTime_; }
    void SetMinTargetDistance(float distance) { minTargetDistance_ = distance; }
    float GetMinTargetDistance() const { return minTargetDistance_; }
    void SetMaxSimultaneousRecoveries(unsigned max) { maxSimultaneousRecoveries_ = max; }
    unsigned GetMaxSimultaneousRecoveries() const { return maxSimultaneousRecoveries_; }
    void SetBaseWorldVelocity(const Vector3& velocity) { baseWorldVelocity_ = velocity; }
    const Vector3& GetBaseWorldVelocity() const { return baseWorldVelocity_; }
    /// @}

protected:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths(const Transform& inverseFrameOfReference) override;
    void SolveInternal(const Transform& frameOfReference, const IKSettings& settings, float timeStep) override;
    /// @}

private:
    enum class TargetState
    {
        Inactive,
        Stuck,
        Recovering,
    };

    struct TargetInfo
    {
        WeakPtr<Node> node_;

        TargetState state_{};
        Transform desiredWorldTransform_;

        ea::optional<Transform> overrideWorldTransform_;
        float overrideWeight_{};
        float stuckTimer_{};

        void Stick()
        {
            state_ = TargetState::Stuck;
            stuckTimer_ = 0.0f;
            OverrideToCurrent();
        }

        void OverrideToCurrent()
        {
            overrideWorldTransform_ = GetCurrentTransform();
            overrideWeight_ = 1.0f;
        }

        void SubtractWeight(float delta) { overrideWeight_ = ea::max(overrideWeight_ - delta, 0.0f); }

        bool IsEffectivelyInactive() const { return state_ == TargetState::Inactive && overrideWeight_ == 0.0f; }

        Transform GetCurrentTransform() const
        {
            if (!overrideWorldTransform_ || overrideWeight_ == 0.0f)
                return desiredWorldTransform_;
            return desiredWorldTransform_.Lerp(*overrideWorldTransform_, overrideWeight_);
        }

        float GetStuckPositionError() const
        {
            if (!overrideWorldTransform_ || state_ != TargetState::Stuck)
                return 0.0f;

            return (overrideWorldTransform_->position_ - desiredWorldTransform_.position_).Length();
        }

        float GetStuckRotationError() const
        {
            if (!overrideWorldTransform_ || state_ != TargetState::Stuck)
                return 0.0f;

            return (overrideWorldTransform_->rotation_ * desiredWorldTransform_.rotation_.Inverse()).Angle();
        }

        float GetStuckTime() const
        {
            if (!overrideWorldTransform_ || state_ != TargetState::Stuck)
                return 0.0f;

            return stuckTimer_;
        }
    };

    void CollectDesiredWorldTransforms();
    void ApplyWorldMovement(float timeStep);
    void UpdateOverrideWeights(float timeStep);
    void UpdateStuckTimers(float timeStep);
    void ApplyDeactivation();
    void ApplyActivation();
    void UpdateRecovery();
    void CommitWorldTransforms();

    bool IsActive() const { return isPositionSticky_ || isRotationSticky_; }
    float GetDistanceToNearestStuckTarget(const Vector3& worldPosition) const;

    StringVector targetNames_;
    bool isPositionSticky_{true};
    bool isRotationSticky_{true};
    float positionThreshold_{DefaultPositionThreshold};
    float rotationThreshold_{DefaultRotationThreshold};
    float timeThreshold_{DefaultTimeThreshold};
    float recoverTime_{DefaultRecoverTime};
    float minTargetDistance_{};
    unsigned maxSimultaneousRecoveries_{};
    Vector3 baseWorldVelocity_;

    ea::vector<TargetInfo> targets_;
    unsigned recoveryStartIndex_{};
};

}
