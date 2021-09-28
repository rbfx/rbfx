//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include <EASTL/sort.h>

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Animation.h"
#include "../Graphics/AnimationState.h"
#include "../Graphics/Batch.h"
#include "../Graphics/Camera.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/DrawableEvents.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Material.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/SoftwareModelAnimator.h"
#include "../Graphics/VertexBuffer.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ResourceEvents.h"
#include "../Scene/Scene.h"

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* GEOMETRY_CATEGORY;

static const unsigned MAX_ANIMATION_STATES = 256;

AnimatedModel::AnimatedModel(Context* context) :
    StaticModel(context),
    animationLodFrameNumber_(0),
    animationLodBias_(1.0f),
    animationLodTimer_(-1.0f),
    animationLodDistance_(0.0f),
    updateInvisible_(false),
    animationDirty_(false),
    morphsDirty_(false),
    skinningDirty_(true),
    boneBoundingBoxDirty_(true),
    isMaster_(true),
    loading_(false),
    assignBonesPending_(false),
    forceAnimationUpdate_(false)
{
    if (auto renderer = context_->GetSubsystem<Renderer>())
    {
        softwareSkinning_ = !renderer->GetUseHardwareSkinning();
        numSoftwareSkinningBones_ = renderer->GetNumSoftwareSkinningBones();
    }
}

AnimatedModel::~AnimatedModel()
{
    // When being destroyed, remove the bone hierarchy if appropriate (last AnimatedModel in the node)
    Bone* rootBone = skeleton_.GetRootBone();
    if (rootBone && rootBone->node_)
    {
        Node* parent = rootBone->node_->GetParent();
        if (parent && !parent->GetComponent<AnimatedModel>())
            RemoveRootBone();
    }
}

void AnimatedModel::RegisterObject(Context* context)
{
    context->RegisterFactory<AnimatedModel>(GEOMETRY_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Model", GetModelAttr, SetModelAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Material", GetMaterialsAttr, SetMaterialsAttr, ResourceRefList, ResourceRefList(Material::GetTypeStatic()),
        AM_DEFAULT);
    URHO3D_ATTRIBUTE("Is Occluder", bool, occluder_, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Can Be Occluded", IsOccludee, SetOccludee, bool, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Cast Shadows", bool, castShadows_, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Update When Invisible", GetUpdateInvisible, SetUpdateInvisible, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Draw Distance", GetDrawDistance, SetDrawDistance, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Shadow Distance", GetShadowDistance, SetShadowDistance, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("LOD Bias", GetLodBias, SetLodBias, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Animation LOD Bias", GetAnimationLodBias, SetAnimationLodBias, float, 1.0f, AM_DEFAULT);
    URHO3D_COPY_BASE_ATTRIBUTES(Drawable);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Bone Animation Enabled", GetBonesEnabledAttr, SetBonesEnabledAttr, VariantVector,
        Variant::emptyVariantVector, AM_FILE | AM_NOEDIT);
    URHO3D_ACCESSOR_ATTRIBUTE("Morphs", GetMorphsAttr, SetMorphsAttr, ea::vector<unsigned char>, Variant::emptyBuffer,
        AM_DEFAULT | AM_NOEDIT);
}

bool AnimatedModel::Serialize(Archive& archive)
{
    loading_ = true;
    bool success = Component::Serialize(archive);
    loading_ = false;

    return success;
}

bool AnimatedModel::Serialize(Archive& archive, ArchiveBlock& block)
{
    loading_ = true;
    bool success = Serializable::Serialize(archive, block);
    loading_ = false;
    return success;
}

bool AnimatedModel::Load(Deserializer& source)
{
    loading_ = true;
    bool success = Component::Load(source);
    loading_ = false;

    return success;
}

bool AnimatedModel::LoadXML(const XMLElement& source)
{
    loading_ = true;
    bool success = Component::LoadXML(source);
    loading_ = false;

    return success;
}

bool AnimatedModel::LoadJSON(const JSONValue& source)
{
    loading_ = true;
    bool success = Component::LoadJSON(source);
    loading_ = false;

    return success;
}

void AnimatedModel::ApplyAttributes()
{
    if (assignBonesPending_)
        AssignBoneNodes();
}

void AnimatedModel::ProcessRayQuery(const RayOctreeQuery& query, ea::vector<RayQueryResult>& results)
{
    // If no bones or no bone-level testing, use the StaticModel test
    RayQueryLevel level = query.level_;
    if (level < RAY_TRIANGLE || !skeleton_.GetNumBones())
    {
        StaticModel::ProcessRayQuery(query, results);
        return;
    }

    // Check ray hit distance to AABB before proceeding with bone-level tests
    if (query.ray_.HitDistance(GetWorldBoundingBox()) >= query.maxDistance_)
        return;

    const ea::vector<Bone>& bones = skeleton_.GetBones();
    Sphere boneSphere;

    for (unsigned i = 0; i < bones.size(); ++i)
    {
        const Bone& bone = bones[i];
        if (!bone.node_)
            continue;

        float distance;

        // Use hitbox if available
        if (bone.collisionMask_ & BONECOLLISION_BOX)
        {
            // Do an initial crude test using the bone's AABB
            const BoundingBox& box = bone.boundingBox_;
            const Matrix3x4& transform = bone.node_->GetWorldTransform();
            distance = query.ray_.HitDistance(box.Transformed(transform));
            if (distance >= query.maxDistance_)
                continue;
            if (level != RAY_AABB)
            {
                // Follow with an OBB test if required
                Matrix3x4 inverse = transform.Inverse();
                Ray localRay = query.ray_.Transformed(inverse);
                distance = localRay.HitDistance(box);
                if (distance >= query.maxDistance_)
                    continue;
            }
        }
        else if (bone.collisionMask_ & BONECOLLISION_SPHERE)
        {
            boneSphere.center_ = bone.node_->GetWorldPosition();
            boneSphere.radius_ = bone.radius_;
            distance = query.ray_.HitDistance(boneSphere);
            if (distance >= query.maxDistance_)
                continue;
        }
        else
            continue;

        // If the code reaches here then we have a hit
        RayQueryResult result;
        result.position_ = query.ray_.origin_ + distance * query.ray_.direction_;
        result.normal_ = -query.ray_.direction_;
        result.distance_ = distance;
        result.drawable_ = this;
        result.node_ = node_;
        result.subObject_ = i;
        results.push_back(result);
    }
}

void AnimatedModel::Update(const FrameInfo& frame)
{
    // If node was invisible last frame, need to decide animation LOD distance here
    // If headless, retain the current animation distance (should be 0)
    if (frame.camera_ && abs((int)frame.frameNumber_ - (int)viewFrameNumber_) > 1)
    {
        // First check for no update at all when invisible. In that case reset LOD timer to ensure update
        // next time the model is in view
        if (!updateInvisible_)
        {
            if (animationDirty_)
            {
                animationLodTimer_ = -1.0f;
                forceAnimationUpdate_ = true;
            }
            return;
        }
        float distance = frame.camera_->GetDistance(node_->GetWorldPosition());
        // If distance is greater than draw distance, no need to update at all
        if (drawDistance_ > 0.0f && distance > drawDistance_)
            return;
        float scale = GetWorldBoundingBox().Size().DotProduct(DOT_SCALE);
        animationLodDistance_ = frame.camera_->GetLodDistance(distance, scale, lodBias_);
    }

    if (animationDirty_)
        UpdateAnimation(frame);
    else if (boneBoundingBoxDirty_)
        UpdateBoneBoundingBox();
}

void AnimatedModel::UpdateBatches(const FrameInfo& frame)
{
    const Matrix3x4& worldTransform = node_->GetWorldTransform();
    const BoundingBox& worldBoundingBox = GetWorldBoundingBox();
    distance_ = frame.camera_->GetDistance(worldBoundingBox.Center());

    // Note: per-geometry distances do not take skinning into account. Especially in case of a ragdoll they may be
    // much off base if the node's own transform is not updated
    if (batches_.size() == 1)
        batches_[0].distance_ = distance_;
    else
    {
        for (unsigned i = 0; i < batches_.size(); ++i)
            batches_[i].distance_ = frame.camera_->GetDistance(worldTransform * geometryData_[i].center_);
    }

    // Use a transformed version of the model's bounding box instead of world bounding box for LOD scale
    // determination so that animation does not change the scale
    BoundingBox transformedBoundingBox = boundingBox_.Transformed(worldTransform);
    float scale = transformedBoundingBox.Size().DotProduct(DOT_SCALE);
    float newLodDistance = frame.camera_->GetLodDistance(distance_, scale, lodBias_);

    // If model is rendered from several views, use the minimum LOD distance for animation LOD
    if (frame.frameNumber_ != animationLodFrameNumber_)
    {
        animationLodDistance_ = newLodDistance;
        animationLodFrameNumber_ = frame.frameNumber_;
    }
    else
        animationLodDistance_ = Min(animationLodDistance_, newLodDistance);

    if (newLodDistance != lodDistance_)
    {
        lodDistance_ = newLodDistance;
        CalculateLodLevels();
    }
}

void AnimatedModel::UpdateGeometry(const FrameInfo& frame)
{
    // Late update in case the model came into view and animation was dirtied in the meanwhile
    if (forceAnimationUpdate_)
    {
        UpdateAnimation(frame);
        forceAnimationUpdate_ = false;
    }

    if (skinningDirty_)
        UpdateSkinning();

    if (morphsDirty_)
        UpdateMorphs();
}

UpdateGeometryType AnimatedModel::GetUpdateGeometryType()
{
    if (morphsDirty_ || forceAnimationUpdate_ || (skinningDirty_ && softwareSkinning_))
        return UPDATE_MAIN_THREAD;
    else if (skinningDirty_)
        return UPDATE_WORKER_THREAD;
    else
        return UPDATE_NONE;
}

void AnimatedModel::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
    {
        debug->AddBoundingBox(GetWorldBoundingBox(), Color::GREEN, depthTest);
        debug->AddSkeleton(skeleton_, Color(0.75f, 0.75f, 0.75f), depthTest);
    }
}

void AnimatedModel::SetModel(Model* model, bool createBones)
{
    if (model == model_)
        return;

    if (!node_)
    {
        URHO3D_LOGERROR("Can not set model while model component is not attached to a scene node");
        return;
    }

    // Unsubscribe from the reload event of previous model (if any), then subscribe to the new
    if (model_)
        UnsubscribeFromEvent(model_, E_RELOADFINISHED);

    model_ = model;

    if (model)
    {
        SubscribeToEvent(model, E_RELOADFINISHED, URHO3D_HANDLER(AnimatedModel, HandleModelReloadFinished));

        // Copy the subgeometry & LOD level structure
        SetNumGeometries(model->GetNumGeometries());
        const ea::vector<ea::vector<SharedPtr<Geometry> > >& geometries = model->GetGeometries();
        const ea::vector<Vector3>& geometryCenters = model->GetGeometryCenters();
        for (unsigned i = 0; i < geometries.size(); ++i)
        {
            geometries_[i] = geometries[i];
            geometryData_[i].center_ = geometryCenters[i];
        }

        // Copy geometry bone mappings
        const ea::vector<ea::vector<unsigned> >& geometryBoneMappings = model->GetGeometryBoneMappings();
        geometryBoneMappings_.clear();
        geometryBoneMappings_.reserve(geometryBoneMappings.size());
        for (unsigned i = 0; i < geometryBoneMappings.size(); ++i)
            geometryBoneMappings_.push_back(geometryBoneMappings[i]);

        // Copy morphs. Note: morph vertex buffers will be created later on-demand
        modelAnimator_ = nullptr;
        morphs_.clear();
        const ea::vector<ModelMorph>& morphs = model->GetMorphs();
        morphs_.reserve(morphs.size());
        for (unsigned i = 0; i < morphs.size(); ++i)
        {
            ModelMorph newMorph;
            newMorph.name_ = morphs[i].name_;
            newMorph.nameHash_ = morphs[i].nameHash_;
            newMorph.weight_ = 0.0f;
            newMorph.buffers_ = morphs[i].buffers_;
            morphs_.push_back(newMorph);
        }

        // Copy bounding box & skeleton
        SetBoundingBox(model->GetBoundingBox());
        // Initial bone bounding box is just the one stored in the model
        boneBoundingBox_ = boundingBox_;
        boneBoundingBoxDirty_ = true;
        SetSkeleton(model->GetSkeleton(), createBones);
        ResetLodLevels();

        // Reserve space for skinning matrices
        skinMatrices_.resize(skeleton_.GetNumBones());
        SetGeometryBoneMappings();

        // Enable skinning in batches
        for (unsigned i = 0; i < batches_.size(); ++i)
        {
            if (skinMatrices_.size() && !softwareSkinning_)
            {
                batches_[i].geometryType_ = GEOM_SKINNED;
                // Check if model has per-geometry bone mappings
                if (geometrySkinMatrices_.size() && geometrySkinMatrices_[i].size())
                {
                    batches_[i].worldTransform_ = &geometrySkinMatrices_[i][0];
                    batches_[i].numWorldTransforms_ = geometrySkinMatrices_[i].size();
                }
                // If not, use the global skin matrices
                else
                {
                    batches_[i].worldTransform_ = &skinMatrices_[0];
                    batches_[i].numWorldTransforms_ = skinMatrices_.size();
                }
            }
            else if (softwareSkinning_)
            {
                batches_[i].geometryType_ = GEOM_STATIC;
                batches_[i].worldTransform_ = &Matrix3x4::IDENTITY;
                batches_[i].numWorldTransforms_ = 1;
            }
            else
            {
                batches_[i].geometryType_ = GEOM_STATIC;
                batches_[i].worldTransform_ = &node_->GetWorldTransform();
                batches_[i].numWorldTransforms_ = 1;
            }
        }

        // Clone geometries now if software skinning is enabled
        if (softwareSkinning_)
            CloneGeometries();
    }
    else
    {
        RemoveRootBone(); // Remove existing root bone if any
        SetNumGeometries(0);
        geometryBoneMappings_.clear();
        modelAnimator_ = nullptr;
        morphs_.clear();
        SetBoundingBox(BoundingBox());
        SetSkeleton(Skeleton(), false);
    }

    MarkNetworkUpdate();
}

void AnimatedModel::SetAnimationLodBias(float bias)
{
    animationLodBias_ = Max(bias, 0.0f);
    MarkNetworkUpdate();
}

void AnimatedModel::SetUpdateInvisible(bool enable)
{
    updateInvisible_ = enable;
    MarkNetworkUpdate();
}


void AnimatedModel::SetMorphWeight(unsigned index, float weight)
{
    if (index >= morphs_.size())
        return;

    // If morph vertex buffers have not been created yet, create now
    if (weight != 0.0f && !modelAnimator_)
        CloneGeometries();

    if (weight != morphs_[index].weight_)
    {
        morphs_[index].weight_ = weight;

        // For a master model, set the same morph weight on non-master models
        if (isMaster_)
        {
            ea::vector<AnimatedModel*> models;
            GetComponents<AnimatedModel>(models);

            // Indexing might not be the same, so use the name hash instead
            for (unsigned i = 1; i < models.size(); ++i)
            {
                if (!models[i]->isMaster_)
                    models[i]->SetMorphWeight(morphs_[index].nameHash_, weight);
            }
        }

        MarkMorphsDirty();
        MarkNetworkUpdate();
    }
}

void AnimatedModel::SetMorphWeight(const ea::string& name, float weight)
{
    for (unsigned i = 0; i < morphs_.size(); ++i)
    {
        if (morphs_[i].name_ == name)
        {
            SetMorphWeight(i, weight);
            return;
        }
    }
}

void AnimatedModel::SetMorphWeight(StringHash nameHash, float weight)
{
    for (unsigned i = 0; i < morphs_.size(); ++i)
    {
        if (morphs_[i].nameHash_ == nameHash)
        {
            SetMorphWeight(i, weight);
            return;
        }
    }
}

void AnimatedModel::ResetMorphWeights()
{
    for (auto i = morphs_.begin(); i != morphs_.end(); ++i)
        i->weight_ = 0.0f;

    // For a master model, reset weights on non-master models
    if (isMaster_)
    {
        ea::vector<AnimatedModel*> models;
        GetComponents<AnimatedModel>(models);

        for (unsigned i = 1; i < models.size(); ++i)
        {
            if (!models[i]->isMaster_)
                models[i]->ResetMorphWeights();
        }
    }

    MarkMorphsDirty();
    MarkNetworkUpdate();
}

const ea::vector<SharedPtr<VertexBuffer> >& AnimatedModel::GetMorphVertexBuffers() const
{
    static const ea::vector<SharedPtr<VertexBuffer>> empty;
    return modelAnimator_ ? modelAnimator_->GetVertexBuffers() : empty;
}

float AnimatedModel::GetMorphWeight(unsigned index) const
{
    return index < morphs_.size() ? morphs_[index].weight_ : 0.0f;
}

float AnimatedModel::GetMorphWeight(const ea::string& name) const
{
    for (auto i = morphs_.begin(); i != morphs_.end(); ++i)
    {
        if (i->name_ == name)
            return i->weight_;
    }

    return 0.0f;
}

float AnimatedModel::GetMorphWeight(StringHash nameHash) const
{
    for (auto i = morphs_.begin(); i != morphs_.end(); ++i)
    {
        if (i->nameHash_ == nameHash)
            return i->weight_;
    }

    return 0.0f;
}

void AnimatedModel::SetSkeleton(const Skeleton& skeleton, bool createBones)
{
    if (!node_ && createBones)
    {
        URHO3D_LOGERROR("AnimatedModel not attached to a scene node, can not create bone nodes");
        return;
    }

    if (isMaster_)
    {
        // Check if bone structure has stayed compatible (reloading the model). In that case retain the old bones and animations
        if (skeleton_.GetNumBones() == skeleton.GetNumBones())
        {
            ea::vector<Bone>& destBones = skeleton_.GetModifiableBones();
            const ea::vector<Bone>& srcBones = skeleton.GetBones();
            bool compatible = true;

            for (unsigned i = 0; i < destBones.size(); ++i)
            {
                if (destBones[i].node_ && destBones[i].name_ == srcBones[i].name_ && destBones[i].parentIndex_ ==
                                                                                     srcBones[i].parentIndex_)
                {
                    // If compatible, just copy the values and retain the old node and animated status
                    Node* boneNode = destBones[i].node_;
                    bool animated = destBones[i].animated_;
                    destBones[i] = srcBones[i];
                    destBones[i].node_ = boneNode;
                    destBones[i].animated_ = animated;
                }
                else
                {
                    compatible = false;
                    break;
                }
            }
            if (compatible)
                return;
        }

        // Notify animation controller about model change so it can reconnect tracks
        if (animationStateSource_)
            animationStateSource_->MarkAnimationStateTracksDirty();

        // Detach the rootbone of the previous model if any
        if (createBones)
            RemoveRootBone();

        skeleton_.Define(skeleton);

        // Merge bounding boxes from non-master models
        FinalizeBoneBoundingBoxes();

        ea::vector<Bone>& bones = skeleton_.GetModifiableBones();
        // Create scene nodes for the bones
        if (createBones)
        {
            for (auto i = bones.begin(); i != bones.end(); ++i)
            {
                // Create bones as local, as they are never to be directly synchronized over the network
                Node* boneNode = node_->CreateChild(i->name_, LOCAL);
                boneNode->AddListener(this);
                boneNode->SetTransform(i->initialPosition_, i->initialRotation_, i->initialScale_);
                // Copy the model component's temporary status
                boneNode->SetTemporary(IsTemporary());
                i->node_ = boneNode;
            }

            for (unsigned i = 0; i < bones.size(); ++i)
            {
                unsigned parentIndex = bones[i].parentIndex_;
                if (parentIndex != i && parentIndex < bones.size())
                    bones[parentIndex].node_->AddChild(bones[i].node_);
            }
        }

        using namespace BoneHierarchyCreated;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_NODE] = node_;
        node_->SendEvent(E_BONEHIERARCHYCREATED, eventData);
    }
    else
    {
        // For non-master models: use the bone nodes of the master model
        skeleton_.Define(skeleton);

        // Instruct the master model to refresh (merge) its bone bounding boxes
        auto* master = node_->GetComponent<AnimatedModel>();
        if (master && master != this)
            master->FinalizeBoneBoundingBoxes();

        if (createBones)
        {
            ea::vector<Bone>& bones = skeleton_.GetModifiableBones();
            for (auto i = bones.begin(); i != bones.end(); ++i)
            {
                Node* boneNode = node_->GetChild(i->name_, true);
                if (boneNode)
                    boneNode->AddListener(this);
                i->node_ = boneNode;
            }
        }
    }

    assignBonesPending_ = !createBones;
}

void AnimatedModel::SetModelAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    // When loading a scene, set model without creating the bone nodes (will be assigned later during post-load)
    SetModel(cache->GetResource<Model>(value.name_), !loading_);
}

void AnimatedModel::SetBonesEnabledAttr(const VariantVector& value)
{
    ea::vector<Bone>& bones = skeleton_.GetModifiableBones();
    for (unsigned i = 0; i < bones.size() && i < value.size(); ++i)
        bones[i].animated_ = value[i].GetBool();
}

void AnimatedModel::SetMorphsAttr(const ea::vector<unsigned char>& value)
{
    for (unsigned index = 0; index < value.size(); ++index)
        SetMorphWeight(index, (float)value[index] / 255.0f);
}

ResourceRef AnimatedModel::GetModelAttr() const
{
    return GetResourceRef(model_, Model::GetTypeStatic());
}

VariantVector AnimatedModel::GetBonesEnabledAttr() const
{
    VariantVector ret;
    const ea::vector<Bone>& bones = skeleton_.GetBones();
    ret.reserve(bones.size());
    for (auto i = bones.begin(); i != bones.end(); ++i)
        ret.push_back(i->animated_);
    return ret;
}

const ea::vector<unsigned char>& AnimatedModel::GetMorphsAttr() const
{
    attrBuffer_.Clear();
    for (auto i = morphs_.begin(); i != morphs_.end(); ++i)
        attrBuffer_.WriteUByte((unsigned char)(i->weight_ * 255.0f));

    return attrBuffer_.GetBuffer();
}

void AnimatedModel::UpdateBoneBoundingBox()
{
    if (skeleton_.GetNumBones())
    {
        // The bone bounding box is in local space, so need the node's inverse transform
        boneBoundingBox_.Clear();
        Matrix3x4 inverseNodeTransform = node_->GetWorldTransform().Inverse();

        const ea::vector<Bone>& bones = skeleton_.GetBones();
        for (auto i = bones.begin(); i != bones.end(); ++i)
        {
            Node* boneNode = i->node_;
            if (!boneNode)
                continue;

            // Use hitbox if available. If not, use only half of the sphere radius
            /// \todo The sphere radius should be multiplied with bone scale
            if (i->collisionMask_ & BONECOLLISION_BOX)
                boneBoundingBox_.Merge(i->boundingBox_.Transformed(inverseNodeTransform * boneNode->GetWorldTransform()));
            else if (i->collisionMask_ & BONECOLLISION_SPHERE)
                boneBoundingBox_.Merge(Sphere(inverseNodeTransform * boneNode->GetWorldPosition(), i->radius_ * 0.5f));
        }
    }

    boneBoundingBoxDirty_ = false;
    worldBoundingBoxDirty_ = true;
}

void AnimatedModel::OnNodeSet(Node* node)
{
    Drawable::OnNodeSet(node);

    if (node)
    {
        // If this AnimatedModel is the first in the node, it is the master which controls animation & morphs
        isMaster_ = GetComponent<AnimatedModel>() == this;
    }
}

void AnimatedModel::OnMarkedDirty(Node* node)
{
    Drawable::OnMarkedDirty(node);

    // If the scene node or any of the bone nodes move, mark skinning dirty
    if (skeleton_.GetNumBones())
    {
        skinningDirty_ = true;
        // Bone bounding box doesn't need to be marked dirty when only the base scene node moves
        if (node != node_)
            boneBoundingBoxDirty_ = true;
    }
}

void AnimatedModel::OnWorldBoundingBoxUpdate()
{
    if (isMaster_)
    {
        // Note: do not update bone bounding box here, instead do it in either of the threaded updates
        worldBoundingBox_ = boneBoundingBox_.Transformed(node_->GetWorldTransform());
    }
    else
    {
        // Non-master animated models get the bounding box from the master
        /// \todo If it's a skinned attachment that does not cover the whole body, it will have unnecessarily large bounds
        auto* master = node_->GetComponent<AnimatedModel>();
        // Check if we've become the new master model in case the original was deleted
        if (master == this)
            isMaster_ = true;
        if (master)
            worldBoundingBox_ = master->GetWorldBoundingBox();
    }
}

void AnimatedModel::AssignBoneNodes()
{
    assignBonesPending_ = false;

    if (!node_)
        return;

    // Find the bone nodes from the node hierarchy and add listeners
    ea::vector<Bone>& bones = skeleton_.GetModifiableBones();
    bool boneFound = false;
    for (auto i = bones.begin(); i != bones.end(); ++i)
    {
        Node* boneNode = node_->GetChild(i->name_, true);
        if (boneNode)
        {
            boneFound = true;
            boneNode->AddListener(this);
        }
        i->node_ = boneNode;
    }

    // If no bones found, this may be a prefab where the bone information was left out.
    // In that case reassign the skeleton now if possible
    if (!boneFound && model_)
        SetSkeleton(model_->GetSkeleton(), true);

    // Notify AnimationStateSource so it can reconnect to new bone nodes
    if (animationStateSource_)
        animationStateSource_->MarkAnimationStateTracksDirty();
}

void AnimatedModel::FinalizeBoneBoundingBoxes()
{
    ea::vector<Bone>& bones = skeleton_.GetModifiableBones();
    ea::vector<AnimatedModel*> models;
    GetComponents<AnimatedModel>(models);

    if (models.size() > 1)
    {
        // Reset first to the model resource's original bone bounding information if available (should be)
        if (model_)
        {
            const ea::vector<Bone>& modelBones = model_->GetSkeleton().GetBones();
            for (unsigned i = 0; i < bones.size() && i < modelBones.size(); ++i)
            {
                bones[i].collisionMask_ = modelBones[i].collisionMask_;
                bones[i].radius_ = modelBones[i].radius_;
                bones[i].boundingBox_ = modelBones[i].boundingBox_;
            }
        }

        // Get matching bones from all non-master models and merge their bone bounding information
        // to prevent culling errors (master model may not have geometry in all bones, or the bounds are smaller)
        for (auto i = models.begin(); i != models.end(); ++i)
        {
            if ((*i) == this)
                continue;

            Skeleton& otherSkeleton = (*i)->GetSkeleton();
            for (auto j = bones.begin(); j != bones.end(); ++j)
            {
                Bone* otherBone = otherSkeleton.GetBone(j->nameHash_);
                if (otherBone)
                {
                    if (otherBone->collisionMask_ & BONECOLLISION_SPHERE)
                    {
                        j->collisionMask_ |= BONECOLLISION_SPHERE;
                        j->radius_ = Max(j->radius_, otherBone->radius_);
                    }
                    if (otherBone->collisionMask_ & BONECOLLISION_BOX)
                    {
                        j->collisionMask_ |= BONECOLLISION_BOX;
                        if (j->boundingBox_.Defined())
                            j->boundingBox_.Merge(otherBone->boundingBox_);
                        else
                            j->boundingBox_.Define(otherBone->boundingBox_);
                    }
                }
            }
        }
    }

    // Remove collision information from dummy bones that do not affect skinning, to prevent them from being merged
    // to the bounding box and making it artificially large
    for (auto i = bones.begin(); i != bones.end(); ++i)
    {
        if (i->collisionMask_ & BONECOLLISION_BOX && i->boundingBox_.Size().Length() < M_EPSILON)
            i->collisionMask_ &= ~BONECOLLISION_BOX;
        if (i->collisionMask_ & BONECOLLISION_SPHERE && i->radius_ < M_EPSILON)
            i->collisionMask_ &= ~BONECOLLISION_SPHERE;
    }
}

void AnimatedModel::RemoveRootBone()
{
    Bone* rootBone = skeleton_.GetRootBone();
    if (rootBone && rootBone->node_)
        rootBone->node_->Remove();
}

void AnimatedModel::MarkAnimationDirty()
{
    if (isMaster_)
    {
        animationDirty_ = true;
        MarkForUpdate();
    }
}

void AnimatedModel::MarkMorphsDirty()
{
    morphsDirty_ = true;
}

void AnimatedModel::CloneGeometries()
{
    modelAnimator_ = MakeShared<SoftwareModelAnimator>(context_);
    modelAnimator_->Initialize(model_, softwareSkinning_, numSoftwareSkinningBones_);
    geometries_ = modelAnimator_->GetGeometries();

    // Make sure the rendering batches use the new cloned geometries
    ResetLodLevels();
    MarkMorphsDirty();
}

void AnimatedModel::SetGeometryBoneMappings()
{
    geometrySkinMatrices_.clear();
    geometrySkinMatrixPtrs_.clear();

    if (!geometryBoneMappings_.size())
        return;

    // Check if all mappings are empty, then we do not need to use mapped skinning
    bool allEmpty = true;
    for (unsigned i = 0; i < geometryBoneMappings_.size(); ++i)
        if (geometryBoneMappings_[i].size())
            allEmpty = false;

    if (allEmpty)
        return;

    if (softwareSkinning_)
    {
        URHO3D_LOGWARNING("Geometry bone mappings are ignored in software skinning");
        return;
    }

    // Reserve space for per-geometry skinning matrices
    geometrySkinMatrices_.resize(geometryBoneMappings_.size());
    for (unsigned i = 0; i < geometryBoneMappings_.size(); ++i)
        geometrySkinMatrices_[i].resize(geometryBoneMappings_[i].size());

    // Build original-to-skinindex matrix pointer mapping for fast copying
    // Note: at this point layout of geometrySkinMatrices_ cannot be modified or pointers become invalid
    geometrySkinMatrixPtrs_.resize(skeleton_.GetNumBones());
    for (unsigned i = 0; i < geometryBoneMappings_.size(); ++i)
    {
        for (unsigned j = 0; j < geometryBoneMappings_[i].size(); ++j)
            geometrySkinMatrixPtrs_[geometryBoneMappings_[i][j]].push_back(&geometrySkinMatrices_[i][j]);
    }
}

void AnimatedModel::UpdateAnimation(const FrameInfo& frame)
{
    // If using animation LOD, accumulate time and see if it is time to update
    if (animationLodBias_ > 0.0f && animationLodDistance_ > 0.0f)
    {
        // Perform the first update always regardless of LOD timer
        if (animationLodTimer_ >= 0.0f)
        {
            animationLodTimer_ += animationLodBias_ * frame.timeStep_ * ANIMATION_LOD_BASESCALE;
            if (animationLodTimer_ >= animationLodDistance_)
                animationLodTimer_ = fmodf(animationLodTimer_, animationLodDistance_);
            else
                return;
        }
        else
            animationLodTimer_ = 0.0f;
    }

    ApplyAnimation();
}

void AnimatedModel::ApplyAnimation()
{
    // Reset skeleton, apply all animations, calculate bones' bounding box. Make sure this is only done for the master model
    // (first AnimatedModel in a node)
    if (isMaster_)
    {
        skeleton_.ResetSilent();

        // AnimationStateSource is a weak pointer which may or may not be an issue
        if (AnimationStateSource* animationStateSource = animationStateSource_)
        {
            for (AnimationState* state : animationStateSource->GetAnimationStates())
                state->ApplyModelTracks();
        }

        // Skeleton reset and animations apply the node transforms "silently" to avoid repeated marking dirty. Mark dirty now
        node_->MarkDirty();

        // Calculate new bone bounding box
        UpdateBoneBoundingBox();
    }

    animationDirty_ = false;
}

void AnimatedModel::ConnectToAnimationStateSource(AnimationStateSource* source)
{
    animationStateSource_ = source;
}

void AnimatedModel::UpdateSkinning()
{
    // Note: the model's world transform will be baked in the skin matrices
    const ea::vector<Bone>& bones = skeleton_.GetBones();
    // Use model's world transform in case a bone is missing
    const Matrix3x4& worldTransform = node_->GetWorldTransform();

    // Skinning with global matrices only
    if (!geometrySkinMatrices_.size())
    {
        for (unsigned i = 0; i < bones.size(); ++i)
        {
            const Bone& bone = bones[i];
            if (bone.node_)
                skinMatrices_[i] = bone.node_->GetWorldTransform() * bone.offsetMatrix_;
            else
                skinMatrices_[i] = worldTransform;
        }
    }
    // Skinning with per-geometry matrices
    else
    {
        for (unsigned i = 0; i < bones.size(); ++i)
        {
            const Bone& bone = bones[i];
            if (bone.node_)
                skinMatrices_[i] = bone.node_->GetWorldTransform() * bone.offsetMatrix_;
            else
                skinMatrices_[i] = worldTransform;

            // Copy the skin matrix to per-geometry matrices as needed
            for (unsigned j = 0; j < geometrySkinMatrixPtrs_[i].size(); ++j)
                *geometrySkinMatrixPtrs_[i][j] = skinMatrices_[i];
        }
    }

    skinningDirty_ = false;

    // If software skinning is enabled, force update
    if (softwareSkinning_)
        morphsDirty_ = true;
}

void AnimatedModel::UpdateMorphs()
{
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics)
        return;

    if (modelAnimator_)
    {
        modelAnimator_->ResetAnimation();
        modelAnimator_->ApplyMorphs(morphs_);
        if (softwareSkinning_)
            modelAnimator_->ApplySkinning(skinMatrices_);
        modelAnimator_->Commit();
    }

    morphsDirty_ = false;
}

void AnimatedModel::HandleModelReloadFinished(StringHash eventType, VariantMap& eventData)
{
    Model* currentModel = model_;
    model_.Reset(); // Set null to allow to be re-set
    SetModel(currentModel);
}

}
