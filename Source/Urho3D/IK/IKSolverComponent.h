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
#include "../Scene/Component.h"

namespace Urho3D
{

using IKNodeCache = ea::unordered_map<Node*, IKNode>;

/// Base component for all IK solvers.
class IKSolverComponent : public Component
{
    URHO3D_OBJECT(IKSolverComponent, Component);

public:
    explicit IKSolverComponent(Context* context);
    ~IKSolverComponent() override;
    static void RegisterObject(Context* context);

    bool Initialize(IKNodeCache& nodeCache);
    void NotifyPositionsReady();
    void Solve(const IKSettings& settings);

    /// Internal. Marks chain tree as dirty.
    void OnTreeDirty();

protected:
    virtual bool InitializeNodes(IKNodeCache& nodeCache) = 0;
    virtual void UpdateChainLengths() = 0;
    virtual void SolveInternal(const IKSettings& settings) = 0;

    void OnNodeSet(Node* previousNode, Node* currentNode) override;

    /// Add node to cache by name. Return null if the node is not found.
    IKNode* AddSolverNode(IKNodeCache& nodeCache, const ea::string& name);
    /// Add node that should be checked for existence before solving.
    Node* AddCheckedNode(IKNodeCache& nodeCache, const ea::string& name) const;

private:
    ea::vector<ea::pair<Node*, IKNode*>> solverNodes_;
};

class IKIdentitySolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKIdentitySolver, IKSolverComponent);

public:
    explicit IKIdentitySolver(Context* context);
    ~IKIdentitySolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    void UpdateProperties();

private:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths() override;
    void SolveInternal(const IKSettings& settings) override;
    /// @}

    void EnsureInitialized();
    void UpdateRotationOffset();

    ea::string boneName_;
    ea::string targetName_;
    Quaternion rotationOffset_{Quaternion::ZERO};

    IKNode* boneNode_{};
    WeakPtr<Node> target_;
};

class IKTrigonometrySolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKTrigonometrySolver, IKSolverComponent);

public:
    explicit IKTrigonometrySolver(Context* context);
    ~IKTrigonometrySolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

private:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths() override;
    void SolveInternal(const IKSettings& settings) override;
    /// @}

    ea::string firstBoneName_;
    ea::string secondBoneName_;
    ea::string thirdBoneName_;

    ea::string targetName_;

    float minAngle_{0.0f};
    float maxAngle_{180.0f};
    Vector3 bendDirection_{Vector3::FORWARD};

    IKTrigonometricChain chain_;
    WeakPtr<Node> target_;
};

class IKLegSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKLegSolver, IKSolverComponent);

public:
    explicit IKLegSolver(Context* context);
    ~IKLegSolver() override;
    static void RegisterObject(Context* context);

    void UpdateProperties();

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

private:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths() override;
    void SolveInternal(const IKSettings& settings) override;
    /// @}

    void EnsureInitialized();
    void UpdateMinHeelAngle();
    Vector3 CalculateCurrentBendDirection(const Vector3& toeTargetPosition) const;
    Vector3 CalculateFootDirectionStraight(
        const Vector3& toeTargetPosition, const Vector3& currentBendDirection) const;
    Vector3 CalculateFootDirectionBent(
        const Vector3& toeTargetPosition, const Vector3& currentBendDirection) const;

    /// Attributes.
    /// @{
    ea::string thighBoneName_;
    ea::string calfBoneName_;
    ea::string heelBoneName_;
    ea::string toeBoneName_;

    ea::string targetName_;

    float minKneeAngle_{0.0f};
    float maxKneeAngle_{180.0f};
    float bendWeight_{};
    Vector3 bendDirection_{Vector3::FORWARD};
    float minHeelAngle_{-1.0f};
    /// @}

    /// IK nodes and effectors.
    /// @{
    IKTrigonometricChain legChain_;
    IKNodeSegment footSegment_;
    WeakPtr<Node> target_;
    /// @}
};

class IKSpineSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKSpineSolver, IKSolverComponent);

public:
    explicit IKSpineSolver(Context* context);
    ~IKSpineSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Return the bone names.
    const StringVector& GetBoneNames() const { return boneNames_; }
    /// Return the target name.
    const ea::string& GetTargetName() const { return targetName_; }

protected:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths() override;
    void SolveInternal(const IKSettings& settings) override;
    /// @}

private:
    StringVector boneNames_;
    ea::string targetName_;

    float maxAngle_{90.0f};

    IKSpineChain chain_;
    WeakPtr<Node> target_;
};

class IKArmSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKArmSolver, IKSolverComponent);

public:
    explicit IKArmSolver(Context* context);
    ~IKArmSolver() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

private:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths() override;
    void SolveInternal(const IKSettings& settings) override;
    /// @}

    void EnsureInitialized();

    /// Attributes.
    /// @{
    ea::string shoulderBoneName_;
    ea::string armBoneName_;
    ea::string forearmBoneName_;
    ea::string handBoneName_;

    ea::string targetName_;

    float minElbowAngle_{0.0f};
    float maxElbowAngle_{180.0f};
    Vector3 bendDirection_{Vector3::FORWARD};
    /// @}

    /// IK nodes and effectors.
    /// @{
    IKTrigonometricChain armChain_;
    IKNodeSegment shoulderSegment_;
    WeakPtr<Node> target_;
    /// @}
};

class IKChainSolver : public IKSolverComponent
{
    URHO3D_OBJECT(IKChainSolver, IKSolverComponent);

public:
    explicit IKChainSolver(Context* context);
    ~IKChainSolver() override;
    static void RegisterObject(Context* context);

    /// Return the bone names.
    const StringVector& GetBoneNames() const { return boneNames_; }
    /// Return the target name.
    const ea::string& GetTargetName() const { return targetName_; }

protected:
    /// Implement IKSolverComponent
    /// @{
    bool InitializeNodes(IKNodeCache& nodeCache) override;
    void UpdateChainLengths() override;
    void SolveInternal(const IKSettings& settings) override;
    /// @}

private:
    StringVector boneNames_;
    ea::string targetName_;

    IKFabrikChain chain_;
    WeakPtr<Node> targetNode_;
};

}
