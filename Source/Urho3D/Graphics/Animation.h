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

/// \file

#pragma once

#include "../Graphics/AnimationTrack.h"
#include "../Container/Ptr.h"
#include "../Resource/Resource.h"

namespace Urho3D
{

/// %Animation trigger point.
struct AnimationTriggerPoint
{
    /// Trigger time.
    float time_{};
    /// Trigger data.
    Variant data_;
};

/// Skeletal animation resource.
/// Don't use bone tracks and generic variant tracks with the same names.
class URHO3D_API Animation : public ResourceWithMetadata
{
    URHO3D_OBJECT(Animation, ResourceWithMetadata);

public:
    /// Construct.
    explicit Animation(Context* context);
    /// Destruct.
    ~Animation() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Reset resource state to default.
    void ResetToDefault();
    /// Load resource from XML file.
    bool LoadXML(const XMLElement& source);
    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Save resource. Return true if successful.
    bool Save(Serializer& dest) const override;

    /// Set animation name.
    /// @property
    void SetAnimationName(const ea::string& name);
    /// Set animation length.
    /// @property
    void SetLength(float length);
    /// Create and return a track by name. If track by same name already exists, returns the existing.
    AnimationTrack* CreateTrack(const ea::string& name);
    /// Create and return generic variant track by name. If variant track by same name already exists, returns the existing.
    VariantAnimationTrack* CreateVariantTrack(const ea::string& name);
    /// Remove a track by name. Return true if was found and removed successfully. This is unsafe if the animation is currently used in playback.
    bool RemoveTrack(const ea::string& name);
    /// Remove all tracks. This is unsafe if the animation is currently used in playback.
    void RemoveAllTracks();
    /// Set a trigger point at index.
    /// @property{set_triggers}
    void SetTrigger(unsigned index, const AnimationTriggerPoint& trigger);
    /// Add a trigger point.
    void AddTrigger(const AnimationTriggerPoint& trigger);
    /// Add a trigger point.
    void AddTrigger(float time, bool timeIsNormalized, const Variant& data);
    /// Remove a trigger point by index.
    void RemoveTrigger(unsigned index);
    /// Remove all trigger points.
    void RemoveAllTriggers();
    /// Resize trigger point vector.
    /// @property
    void SetNumTriggers(unsigned num);
    /// Clone the animation.
    SharedPtr<Animation> Clone(const ea::string& cloneName = EMPTY_STRING) const;

    /// Return animation name.
    /// @property
    const ea::string& GetAnimationName() const { return animationName_; }

    /// Return animation name hash.
    StringHash GetAnimationNameHash() const { return animationNameHash_; }

    /// Return animation length.
    /// @property
    float GetLength() const { return length_; }

    /// Return all animation tracks.
    const ea::unordered_map<StringHash, AnimationTrack>& GetTracks() const { return tracks_; }

    /// Return number of animation tracks.
    /// @property
    unsigned GetNumTracks() const { return tracks_.size(); }

    /// Return animation track by index.
    AnimationTrack *GetTrack(unsigned index);

    /// Return animation track by name.
    /// @property{get_tracks}
    AnimationTrack* GetTrack(const ea::string& name);
    /// Return animation track by name hash.
    AnimationTrack* GetTrack(StringHash nameHash);

    /// Return generic variant animation tracks.
    /// @{
    const ea::unordered_map<StringHash, VariantAnimationTrack>& GetVariantTracks() const { return variantTracks_; }
    unsigned GetNumVariantTracks() const { return variantTracks_.size(); }
    VariantAnimationTrack* GetVariantTrack(unsigned index);
    VariantAnimationTrack* GetVariantTrack(const ea::string& name);
    VariantAnimationTrack* GetVariantTrack(StringHash nameHash);
    /// @}

    /// Return animation trigger points.
    const ea::vector<AnimationTriggerPoint>& GetTriggers() const { return triggers_; }

    /// Return number of animation trigger points.
    /// @property
    unsigned GetNumTriggers() const { return triggers_.size(); }

    /// Return a trigger point by index.
    AnimationTriggerPoint* GetTrigger(unsigned index);

    /// Set all animation tracks.
    void SetTracks(const ea::vector<AnimationTrack>& tracks);

private:
    void LoadTriggersFromXML(const XMLElement& source);

    /// Class versions (used for serialization)
    /// @{
    static const unsigned legacyVersion = 1; // Fake version for legacy unversioned UANI file
    static const unsigned variantTrackVersion = 2; // VariantAnimationTrack support added here

    static const unsigned currentVersion = variantTrackVersion;
    /// @}

    /// Animation name.
    ea::string animationName_;
    /// Animation name hash.
    StringHash animationNameHash_;
    /// Animation length.
    float length_;
    /// Animation tracks.
    ea::unordered_map<StringHash, AnimationTrack> tracks_;
    /// Generic variant animation tracks.
    ea::unordered_map<StringHash, VariantAnimationTrack> variantTracks_;
    /// Animation trigger points.
    ea::vector<AnimationTriggerPoint> triggers_;
};

}
