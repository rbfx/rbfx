// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Graphics/Skeleton.h"
#include "../IO/Log.h"

#include <EASTL/numeric.h>

#include "../DebugNew.h"

namespace Urho3D
{

Skeleton::Skeleton() :
    rootBoneIndex_(M_MAX_UNSIGNED)
{
}

Skeleton::~Skeleton() = default;

bool Skeleton::Load(Deserializer& source)
{
    ClearBones();

    if (source.IsEof())
        return false;

    unsigned bones = source.ReadUInt();
    bones_.reserve(bones);

    for (unsigned i = 0; i < bones; ++i)
    {
        Bone newBone;
        newBone.name_ = source.ReadString();
        newBone.nameHash_ = newBone.name_;
        newBone.parentIndex_ = source.ReadUInt();
        newBone.initialPosition_ = source.ReadVector3();
        newBone.initialRotation_ = source.ReadQuaternion();
        newBone.initialScale_ = source.ReadVector3();
        source.Read(&newBone.offsetMatrix_.m00_, sizeof(Matrix3x4));

        // Read bone collision data
        newBone.collisionMask_ = BoneCollisionShapeFlags(source.ReadUByte());
        if (newBone.collisionMask_ & BONECOLLISION_SPHERE)
            newBone.radius_ = source.ReadFloat();
        if (newBone.collisionMask_ & BONECOLLISION_BOX)
            newBone.boundingBox_ = source.ReadBoundingBox();

        if (newBone.parentIndex_ == i)
            rootBoneIndex_ = i;

        bones_.push_back(newBone);
    }

    UpdateBoneOrder();
    return true;
}

bool Skeleton::Save(Serializer& dest) const
{
    if (!dest.WriteUInt(bones_.size()))
        return false;

    for (unsigned i = 0; i < bones_.size(); ++i)
    {
        const Bone& bone = bones_[i];
        dest.WriteString(bone.name_);
        dest.WriteUInt(bone.parentIndex_);
        dest.WriteVector3(bone.initialPosition_);
        dest.WriteQuaternion(bone.initialRotation_);
        dest.WriteVector3(bone.initialScale_);
        dest.Write(bone.offsetMatrix_.Data(), sizeof(Matrix3x4));

        // Collision info
        dest.WriteUByte(bone.collisionMask_);
        if (bone.collisionMask_ & BONECOLLISION_SPHERE)
            dest.WriteFloat(bone.radius_);
        if (bone.collisionMask_ & BONECOLLISION_BOX)
            dest.WriteBoundingBox(bone.boundingBox_);
    }

    return true;
}

void Skeleton::Define(const Skeleton& src)
{
    ClearBones();

    bones_ = src.bones_;
    // Make sure we clear node references, if they exist
    // (AnimatedModel will create new nodes on its own)
    for (auto i = bones_.begin(); i != bones_.end(); ++i)
        i->node_.Reset();
    rootBoneIndex_ = src.rootBoneIndex_;

    UpdateBoneOrder();
}

void Skeleton::SetRootBoneIndex(unsigned index)
{
    if (index < bones_.size())
        rootBoneIndex_ = index;
    else
        URHO3D_LOGERROR("Root bone index out of bounds");
}

void Skeleton::SetNumBones(unsigned numBones)
{
    bones_.resize(numBones);
}

void Skeleton::ClearBones()
{
    bones_.clear();
    rootBoneIndex_ = M_MAX_UNSIGNED;
}

void Skeleton::UpdateBoneOrder()
{
    const unsigned numBones = bones_.size();
    bonesOrder_.reserve(numBones);
    bonesOrder_.clear();

    // Collect roots first
    for (unsigned boneIndex = 0; boneIndex < numBones; ++boneIndex)
    {
        if (bones_[boneIndex].parentIndex_ == boneIndex)
            bonesOrder_.push_back(boneIndex);
    }

    // Collect layer by layer
    unsigned rangeBegin = 0;
    unsigned rangeEnd = bonesOrder_.size();

    while (bonesOrder_.size() < numBones && rangeBegin != rangeEnd)
    {
        const auto currentParents = ea::span<unsigned>{bonesOrder_}.subspan(rangeBegin, rangeEnd - rangeBegin);
        for (unsigned boneIndex = 0; boneIndex < numBones; ++boneIndex)
        {
            const unsigned parentBoneIndex = bones_[boneIndex].parentIndex_;
            if (parentBoneIndex == boneIndex)
                continue;

            const bool isDirectChild = ea::find(currentParents.begin(), currentParents.end(), parentBoneIndex) != currentParents.end();
            if (isDirectChild)
                bonesOrder_.push_back(boneIndex);
        }

        rangeBegin = rangeEnd;
        rangeEnd = bonesOrder_.size();
    }
}

void Skeleton::Reset()
{
    for (auto i = bones_.begin(); i != bones_.end(); ++i)
    {
        if (i->animated_ && i->node_)
            i->node_->SetTransform(i->initialPosition_, i->initialRotation_, i->initialScale_);
    }
}

void Skeleton::ResetSilent()
{
    for (auto i = bones_.begin(); i != bones_.end(); ++i)
    {
        if (i->animated_ && i->node_)
            i->node_->SetTransformSilent(i->initialPosition_, i->initialRotation_, i->initialScale_);
    }
}


Bone* Skeleton::GetRootBone()
{
    return GetBone(rootBoneIndex_);
}

unsigned Skeleton::GetBoneIndex(const StringHash& boneNameHash) const
{
    const unsigned numBones = bones_.size();
    for (unsigned i = 0; i < numBones; ++i)
    {
        if (bones_[i].nameHash_ == boneNameHash)
            return i;
    }

    return M_MAX_UNSIGNED;
}

unsigned Skeleton::GetBoneIndex(const Bone* bone) const
{
    if (bones_.empty() || bone < &bones_.front() || bone > &bones_.back())
        return M_MAX_UNSIGNED;

    return static_cast<unsigned>(bone - &bones_.front());
}

unsigned Skeleton::GetBoneIndex(const ea::string& boneName) const
{
    return GetBoneIndex(StringHash(boneName));
}

Bone* Skeleton::GetBoneParent(const Bone* bone)
{
    if (GetBoneIndex(bone) == bone->parentIndex_)
        return nullptr;
    else
        return GetBone(bone->parentIndex_);
}

Bone* Skeleton::GetBone(unsigned index)
{
    return index < bones_.size() ? &bones_[index] : nullptr;
}

Bone* Skeleton::GetBone(const ea::string& name)
{
    return GetBone(StringHash(name));
}

Bone* Skeleton::GetBone(const char* name)
{
    return GetBone(StringHash(name));
}

Bone* Skeleton::GetBone(const StringHash& boneNameHash)
{
    const unsigned index = GetBoneIndex(boneNameHash);
    return index < bones_.size() ? &bones_[index] : nullptr;
}

}
