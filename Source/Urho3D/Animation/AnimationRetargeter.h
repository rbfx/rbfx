// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/Vector3.h>

#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Bind-pose description of one skeleton bone.
struct URHO3D_API AnimationRetargetBone
{
    ea::string name;
    int parentIndex{-1};
    Vector3 position{Vector3::ZERO};
    Quaternion rotation{Quaternion::IDENTITY};
    Vector3 scale{Vector3::ONE};
};

/// Runtime transform used as a source or target retarget pose.
struct URHO3D_API AnimationRetargetTransform
{
    Vector3 position{Vector3::ZERO};
    Quaternion rotation{Quaternion::IDENTITY};
    Vector3 scale{Vector3::ONE};
};

/// Skeleton-to-skeleton retargeter using bind-pose offsets and explicit bone mappings.
class URHO3D_API AnimationRetargeter
{
public:
    bool SetSourceSkeleton(const ea::vector<AnimationRetargetBone>& bones, ea::string* error = nullptr);
    bool SetTargetSkeleton(const ea::vector<AnimationRetargetBone>& bones, ea::string* error = nullptr);
    const ea::vector<AnimationRetargetBone>& GetSourceSkeleton() const { return sourceBones_; }
    const ea::vector<AnimationRetargetBone>& GetTargetSkeleton() const { return targetBones_; }

    bool SetMapping(const ea::string& targetBone, const ea::string& sourceBone);
    bool RemoveMapping(const ea::string& targetBone);
    void ClearMappings() { mappings_.clear(); }
    unsigned AutoMapByName(bool caseInsensitive = true);
    const ea::string& GetMappedSourceBone(const ea::string& targetBone) const;
    unsigned GetMappingCount() const { return mappings_.size(); }

    void SetRootScale(float scale) { rootScale_ = Max(scale, 0.0f); }
    float GetRootScale() const { return rootScale_; }
    bool IsReady() const { return !sourceBones_.empty() && !targetBones_.empty(); }

    bool RetargetPose(const ea::vector<AnimationRetargetTransform>& sourcePose,
        ea::vector<AnimationRetargetTransform>& targetPose) const;

private:
    static bool ValidateSkeleton(const ea::vector<AnimationRetargetBone>& bones, ea::string* error);
    static ea::string NormalizeName(const ea::string& name, bool caseInsensitive);
    int FindSourceBone(const ea::string& name) const;
    int FindTargetBone(const ea::string& name) const;

    ea::vector<AnimationRetargetBone> sourceBones_;
    ea::vector<AnimationRetargetBone> targetBones_;
    ea::unordered_map<ea::string, ea::string> mappings_;
    float rootScale_{1.0f};
};

} // namespace Urho3D
