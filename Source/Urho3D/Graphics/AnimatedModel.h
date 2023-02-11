//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Graphics/AnimationStateSource.h"
#include "../Graphics/Model.h"
#include "../Graphics/Skeleton.h"
#include "../Graphics/StaticModel.h"

namespace Urho3D
{

class Animation;
class AnimationState;
class SoftwareModelAnimator;

/// Animated model component.
class URHO3D_API AnimatedModel : public StaticModel
{
    URHO3D_OBJECT(AnimatedModel, StaticModel);

    friend class AnimationState;

public:
    /// Construct.
    explicit AnimatedModel(Context* context);
    /// Destruct.
    ~AnimatedModel() override;
    /// Register object factory. Drawable must be registered first.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    void ApplyAttributes() override;

    /// Process raycast with custom transform.
    void ProcessCustomRayQuery(const RayOctreeQuery& query, const BoundingBox& worldBoundingBox,
        const Matrix3x4& worldTransform, ea::span<const Matrix3x4> boneWorldTransforms,
        ea::vector<RayQueryResult>& results);
    /// Process octree raycast. May be called from a worker thread.
    void ProcessRayQuery(const RayOctreeQuery& query, ea::vector<RayQueryResult>& results) override;
    /// Update before octree reinsertion. Is called from a worker thread.
    void Update(const FrameInfo& frame) override;
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    void UpdateBatches(const FrameInfo& frame) override;
    /// Prepare geometry for rendering. Called from a worker thread if possible (no GPU update).
    void UpdateGeometry(const FrameInfo& frame) override;
    /// Return whether a geometry update is necessary, and if it can happen in a worker thread.
    UpdateGeometryType GetUpdateGeometryType() override;
    /// Visualize the component as debug geometry.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Set model.
    void SetModel(Model* model, bool createBones);
    /// Set model and create bone nodes.
    void SetModel(Model* model) override { SetModel(model, true); }
    /// Set animation LOD bias.
    /// @property
    void SetAnimationLodBias(float bias);
    /// Set whether to update animation and the bounding box when not visible. Recommended to enable for physically controlled models like ragdolls.
    /// @property
    void SetUpdateInvisible(bool enable);
    /// Set vertex morph weight by index.
    void SetMorphWeight(unsigned index, float weight);
    /// Set vertex morph weight by name.
    /// @property{set_morphWeights}
    void SetMorphWeight(const ea::string& name, float weight);
    /// Set vertex morph weight by name hash.
    void SetMorphWeight(StringHash nameHash, float weight);
    /// Reset all vertex morphs to zero.
    void ResetMorphWeights();
    /// Reset node transforms of skeleton bones.
    void ResetBones();
    /// Apply all animation states to nodes.
    void ApplyAnimation();
    /// Connect to AnimationStateSource that provides animation states.
    void ConnectToAnimationStateSource(AnimationStateSource* source);

    /// Return skeleton.
    /// @property
    Skeleton& GetSkeleton() { return skeleton_; }

    /// Return animation LOD bias.
    /// @property
    float GetAnimationLodBias() const { return animationLodBias_; }

    /// Return whether to update animation when not visible.
    /// @property
    bool GetUpdateInvisible() const { return updateInvisible_; }

    /// Return all vertex morphs.
    const ea::vector<ModelMorph>& GetMorphs() const { return morphs_; }

    /// Return all morph vertex buffers.
    const ea::vector<SharedPtr<VertexBuffer> >& GetMorphVertexBuffers() const;

    /// Return number of vertex morphs.
    /// @property
    unsigned GetNumMorphs() const { return morphs_.size(); }

    /// Return vertex morph weight by index.
    float GetMorphWeight(unsigned index) const;
    /// Return vertex morph weight by name.
    /// @property{get_morphWeights}
    float GetMorphWeight(const ea::string& name) const;
    /// Return vertex morph weight by name hash.
    float GetMorphWeight(StringHash nameHash) const;

    /// Return whether is the master (first) animated model.
    bool IsMaster() const { return isMaster_; }

    /// Set model attribute.
    void SetModelAttr(const ResourceRef& value);
    /// Set bones' animation enabled attribute.
    void SetBonesEnabledAttr(const VariantVector& value);
    /// Set morphs attribute.
    void SetMorphsAttr(const ea::vector<unsigned char>& value);
    /// Return model attribute.
    ResourceRef GetModelAttr() const;
    /// Return bones' animation enabled attribute.
    VariantVector GetBonesEnabledAttr() const;
    /// Return morphs attribute.
    const ea::vector<unsigned char>& GetMorphsAttr() const;

    /// Return per-geometry bone mappings.
    const ea::vector<ea::vector<unsigned> >& GetGeometryBoneMappings() const { return geometryBoneMappings_; }

    /// Return per-geometry skin matrices. If empty, uses global skinning.
    const ea::vector<ea::vector<Matrix3x4> >& GetGeometrySkinMatrices() const { return geometrySkinMatrices_; }

    /// Recalculate the bone bounding box. Normally called internally, but can also be manually called if up-to-date information before rendering is necessary.
    void UpdateBoneBoundingBox();

protected:
    /// Handle node being assigned.
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    /// Handle node transform being dirtied.
    void OnMarkedDirty(Node* node) override;
    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override;

private:
    /// Assign skeleton and animation bone node references as a postprocess. Called by ApplyAttributes.
    void AssignBoneNodes();
    /// Finalize master model bone bounding boxes by merging from matching non-master bones.. Performed whenever any of the AnimatedModels in the same node changes its model.
    void FinalizeBoneBoundingBoxes();
    /// Remove (old) skeleton root bone.
    void RemoveRootBone();
    /// Mark animation and skinning to require an update.
    void MarkAnimationDirty();
    /// Mark morphs to require an update.
    void MarkMorphsDirty();
    /// Set skeleton.
    void SetSkeleton(const Skeleton& skeleton, bool createBones);
    /// Set mapping of subgeometry bone indices.
    void SetGeometryBoneMappings();
    /// Clone geometries for vertex morphing.
    void CloneGeometries();
    /// Handle model reload finished.
    void HandleModelReloadFinished(StringHash eventType, VariantMap& eventData);
    /// Reconsider whether to use software skinning.
    void UpdateSoftwareSkinningState();

    /// Animation update sequence. Called from Update whenever possible, and from UpdateGeometry in other cases.
    /// @{
    bool PrepareForThreadedUpdate(Camera* camera, unsigned frameNumber);
    bool UpdateAndCheckAnimationTimers(float timeStep);

    void InitializeLocalBoneTransforms(bool reset);
    void CalculateFinalBoneTransforms();
    void CalculateLocalBoundingBox();
    void CalculateAnimations();
    void ApplyBoneTransformsToNodes();

    void UpdateSkinning();
    void UpdateMorphs();
    /// @}

    /// Dirty flags used in animation update sequence.
    /// @{
    bool animationDirty_{};
    bool morphsDirty_{};
    bool skinningDirty_{true};
    bool boneBoundingBoxDirty_{true};
    /// @}

    /// Skeleton.
    Skeleton skeleton_;
    /// Animation data of Skeleton, used only during Update.
    ea::vector<ModelAnimationOutput> skeletonData_;
    /// Component that provides animation states for the model.
    WeakPtr<AnimationStateSource> animationStateSource_;
    /// Software model animator.
    SharedPtr<SoftwareModelAnimator> modelAnimator_;
    /// Vertex morphs.
    ea::vector<ModelMorph> morphs_;
    /// Skinning matrices.
    ea::vector<Matrix3x4> skinMatrices_;
    /// Mapping of subgeometry bone indices, used if more bones than skinning shader can manage.
    ea::vector<ea::vector<unsigned> > geometryBoneMappings_;
    /// Subgeometry skinning matrices, used if more bones than skinning shader can manage.
    ea::vector<ea::vector<Matrix3x4> > geometrySkinMatrices_;
    /// Subgeometry skinning matrix pointers, if more bones than skinning shader can manage.
    ea::vector<ea::vector<Matrix3x4*> > geometrySkinMatrixPtrs_;
    /// Bounding box calculated from bones.
    BoundingBox boneBoundingBox_;
    /// Attribute buffer.
    mutable VectorBuffer attrBuffer_;
    /// The frame number animation LOD distance was last calculated on.
    unsigned animationLodFrameNumber_;
    /// Animation LOD bias.
    float animationLodBias_;
    /// Animation LOD timer.
    float animationLodTimer_;
    /// Animation LOD distance, the minimum of all LOD view distances last frame.
    float animationLodDistance_;
    /// Update animation when invisible flag.
    bool updateInvisible_;
    /// Software skinning flag.
    bool softwareSkinning_{};
    /// Number of bones used for software skinning.
    unsigned numSoftwareSkinningBones_{ 4 };
    /// Master model flag.
    bool isMaster_;
    /// Bone nodes assignment pending flag.
    bool assignBonesPending_;
    /// Force animation update after becoming visible flag.
    bool forceAnimationUpdate_;
};

}
