// SPDX-License-Identifier: MIT

#include "AnimationRetargeter.h"

#include <cctype>

namespace Urho3D
{

bool AnimationRetargeter::SetSourceSkeleton(const ea::vector<AnimationRetargetBone>& bones, ea::string* error)
{
    if (!ValidateSkeleton(bones, error))
        return false;
    sourceBones_ = bones;
    mappings_.clear();
    return true;
}

bool AnimationRetargeter::SetTargetSkeleton(const ea::vector<AnimationRetargetBone>& bones, ea::string* error)
{
    if (!ValidateSkeleton(bones, error))
        return false;
    targetBones_ = bones;
    mappings_.clear();
    return true;
}

bool AnimationRetargeter::SetMapping(const ea::string& targetBone, const ea::string& sourceBone)
{
    if (FindTargetBone(targetBone) < 0 || FindSourceBone(sourceBone) < 0)
        return false;
    mappings_[targetBone] = sourceBone;
    return true;
}

bool AnimationRetargeter::RemoveMapping(const ea::string& targetBone)
{
    return mappings_.erase(targetBone) != 0;
}

unsigned AnimationRetargeter::AutoMapByName(bool caseInsensitive)
{
    mappings_.clear();
    unsigned mapped = 0;
    for (const AnimationRetargetBone& target : targetBones_)
    {
        const ea::string targetName = NormalizeName(target.name, caseInsensitive);
        for (const AnimationRetargetBone& source : sourceBones_)
        {
            if (NormalizeName(source.name, caseInsensitive) == targetName)
            {
                mappings_[target.name] = source.name;
                ++mapped;
                break;
            }
        }
    }
    return mapped;
}

const ea::string& AnimationRetargeter::GetMappedSourceBone(const ea::string& targetBone) const
{
    static const ea::string empty;
    const auto iter = mappings_.find(targetBone);
    return iter != mappings_.end() ? iter->second : empty;
}

bool AnimationRetargeter::RetargetPose(const ea::vector<AnimationRetargetTransform>& sourcePose,
    ea::vector<AnimationRetargetTransform>& targetPose) const
{
    if (!IsReady() || sourcePose.size() != sourceBones_.size())
        return false;

    targetPose.clear();
    targetPose.resize(targetBones_.size());
    for (unsigned targetIndex = 0; targetIndex < targetBones_.size(); ++targetIndex)
    {
        const AnimationRetargetBone& targetBind = targetBones_[targetIndex];
        AnimationRetargetTransform& targetTransform = targetPose[targetIndex];
        targetTransform.position = targetBind.position;
        targetTransform.rotation = targetBind.rotation;
        targetTransform.scale = targetBind.scale;

        const auto mapping = mappings_.find(targetBind.name);
        if (mapping == mappings_.end())
            continue;
        const int sourceIndex = FindSourceBone(mapping->second);
        if (sourceIndex < 0 || static_cast<unsigned>(sourceIndex) >= sourcePose.size())
            return false;

        const AnimationRetargetBone& sourceBind = sourceBones_[sourceIndex];
        const AnimationRetargetTransform& sourceTransform = sourcePose[sourceIndex];
        const bool isRoot = targetBind.parentIndex < 0;
        const float positionScale = isRoot ? rootScale_ : 1.0f;
        targetTransform.position = targetBind.position
            + (sourceTransform.position - sourceBind.position) * positionScale;
        targetTransform.rotation = sourceTransform.rotation;
        targetTransform.scale = sourceTransform.scale;
    }
    return true;
}

bool AnimationRetargeter::ValidateSkeleton(const ea::vector<AnimationRetargetBone>& bones, ea::string* error)
{
    for (unsigned i = 0; i < bones.size(); ++i)
    {
        if (bones[i].name.empty())
        {
            if (error)
                *error = "Skeleton bone name cannot be empty.";
            return false;
        }
        if (bones[i].parentIndex >= static_cast<int>(bones.size()) || bones[i].parentIndex == static_cast<int>(i))
        {
            if (error)
                *error = "Skeleton bone parent index is invalid.";
            return false;
        }
        for (unsigned j = 0; j < i; ++j)
        {
            if (bones[j].name == bones[i].name)
            {
                if (error)
                    *error = "Skeleton bone names must be unique.";
                return false;
            }
        }
    }
    return true;
}

ea::string AnimationRetargeter::NormalizeName(const ea::string& name, bool caseInsensitive)
{
    if (!caseInsensitive)
        return name;
    ea::string result = name;
    for (char& character : result)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    return result;
}

int AnimationRetargeter::FindSourceBone(const ea::string& name) const
{
    for (unsigned i = 0; i < sourceBones_.size(); ++i)
    {
        if (sourceBones_[i].name == name)
            return static_cast<int>(i);
    }
    return -1;
}

int AnimationRetargeter::FindTargetBone(const ea::string& name) const
{
    for (unsigned i = 0; i < targetBones_.size(); ++i)
    {
        if (targetBones_[i].name == name)
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace Urho3D
