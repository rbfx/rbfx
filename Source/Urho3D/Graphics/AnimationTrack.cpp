// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2020-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Graphics/AnimationTrack.h"

#include "Urho3D/IO/ArchiveSerialization.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

template <class T> void AppendKeys(ea::vector<float>& keys, ea::span<const T> keyFrames)
{
    for (const auto& keyFrame : keyFrames)
        keys.push_back(GetKeyFrameTime(keyFrame));
}

void EraseEquivalentKeys(ea::vector<float>& keys, float epsilon)
{
    unsigned lastValidIndex = 0;
    for (unsigned i = 1; i < keys.size(); ++i)
    {
        if (keys[i] - keys[lastValidIndex] < epsilon)
            keys[i] = -M_LARGE_VALUE;
        else
            lastValidIndex = i;
    }

    ea::erase_if(keys, [](float time) { return time < 0.0f; });
}

Vector3 LerpValue(const Vector3& lhs, const Vector3& rhs, float factor)
{
    return Lerp(lhs, rhs, factor);
}

Quaternion LerpValue(const Quaternion& lhs, const Quaternion& rhs, float factor)
{
    return lhs.Slerp(rhs, factor);
}

template <class T>
ea::vector<T> RemapKeyFrameValues(const ea::vector<float>& destKeys, ea::span<const ea::pair<float, T>> sourceKeyFrames)
{
    if (sourceKeyFrames.empty())
        return ea::vector<T>(destKeys.size());

    ea::vector<T> result(destKeys.size());
    for (unsigned i = 0; i < destKeys.size(); ++i)
    {
        const float destKey = destKeys[i];
        const auto compareTime = [](const auto& keyFrame, float time) { return keyFrame.first < time; };
        const auto iter = ea::lower_bound(sourceKeyFrames.begin(), sourceKeyFrames.end(), destKey, compareTime);
        const unsigned secondIndex =
            ea::min<unsigned>(sourceKeyFrames.size() - 1, static_cast<unsigned>(iter - sourceKeyFrames.begin()));
        const unsigned firstIndex = iter == sourceKeyFrames.end() ? secondIndex : ea::max(1u, secondIndex) - 1;
        const auto& firstFrame = sourceKeyFrames[firstIndex];
        const auto& secondFrame = sourceKeyFrames[secondIndex];

        if (firstIndex == secondIndex)
            result[i] = firstFrame.second;
        else
        {
            const float factor = InverseLerp(firstFrame.first, secondFrame.first, destKey);
            result[i] = LerpValue(firstFrame.second, secondFrame.second, factor);
        }
    }
    return result;
}

} // namespace

void AnimationTrack::Sample(float time, float duration, bool isLooped, unsigned& frameIndex, Transform& value) const
{
    float blendFactor{};
    unsigned nextFrameIndex{};
    GetKeyFrames(time, duration, isLooped, frameIndex, nextFrameIndex, blendFactor);

    const AnimationKeyFrame& keyFrame = keyFrames_[frameIndex];
    const AnimationKeyFrame& nextKeyFrame = keyFrames_[nextFrameIndex];

    if (blendFactor >= M_EPSILON)
    {
        if (channelMask_ & CHANNEL_POSITION)
            value.position_ = keyFrame.position_.Lerp(nextKeyFrame.position_, blendFactor);
        if (channelMask_ & CHANNEL_ROTATION)
            value.rotation_ = keyFrame.rotation_.Slerp(nextKeyFrame.rotation_, blendFactor);
        if (channelMask_ & CHANNEL_SCALE)
            value.scale_ = keyFrame.scale_.Lerp(nextKeyFrame.scale_, blendFactor);
    }
    else
    {
        if (channelMask_ & CHANNEL_POSITION)
            value.position_ = keyFrame.position_;
        if (channelMask_ & CHANNEL_ROTATION)
            value.rotation_ = keyFrame.rotation_;
        if (channelMask_ & CHANNEL_SCALE)
            value.scale_ = keyFrame.scale_;
    }
}

bool AnimationTrack::IsLooped(float positionThreshold, float rotationThreshold, float scaleThreshold) const
{
    if (keyFrames_.empty())
        return true;

    const Transform& firstTransform = keyFrames_.front();
    const Transform& lastTransform = keyFrames_.back();

    if (channelMask_.Test(CHANNEL_POSITION)
        && !firstTransform.position_.Equals(lastTransform.position_, positionThreshold))
        return false;
    if (channelMask_.Test(CHANNEL_ROTATION)
        && !firstTransform.rotation_.Equals(lastTransform.rotation_, rotationThreshold))
        return false;
    if (channelMask_.Test(CHANNEL_SCALE) && !firstTransform.scale_.Equals(lastTransform.scale_, scaleThreshold))
        return false;

    return true;
}

void AnimationTrack::CreateMerged(AnimationChannelFlags channels,
    ea::span<const ea::pair<float, Vector3>> positionTrack, ea::span<const ea::pair<float, Quaternion>> rotationTrack,
    ea::span<const ea::pair<float, Vector3>> scaleTrack, float epsilon)
{
    channelMask_ = channels;
    keyFrames_.clear();

    const bool hasPositions = channels.IsAnyOf(CHANNEL_POSITION);
    const bool hasRotations = channels.IsAnyOf(CHANNEL_ROTATION);
    const bool hasScales = channels.IsAnyOf(CHANNEL_SCALE);

    ea::vector<float> keys;
    if (hasPositions)
        AppendKeys(keys, positionTrack);
    if (hasRotations)
        AppendKeys(keys, rotationTrack);
    if (hasScales)
        AppendKeys(keys, scaleTrack);
    ea::sort(keys.begin(), keys.end());
    EraseEquivalentKeys(keys, epsilon);

    const unsigned numKeyFrames = keys.size();
    keyFrames_.resize(numKeyFrames);
    for (unsigned i = 0; i < numKeyFrames; ++i)
        keyFrames_[i].time_ = keys[i];

    if (hasPositions)
    {
        const auto remappedPositions = RemapKeyFrameValues(keys, positionTrack);
        for (unsigned i = 0; i < numKeyFrames; ++i)
            keyFrames_[i].position_ = remappedPositions[i];
    }

    if (hasRotations)
    {
        const auto remappedRotations = RemapKeyFrameValues(keys, rotationTrack);
        for (unsigned i = 0; i < numKeyFrames; ++i)
            keyFrames_[i].rotation_ = remappedRotations[i];
    }

    if (hasScales)
    {
        const auto remappedScales = RemapKeyFrameValues(keys, scaleTrack);
        for (unsigned i = 0; i < numKeyFrames; ++i)
            keyFrames_[i].scale_ = remappedScales[i];
    }
}

bool VariantAnimationTrack::IsLooped() const
{
    if (keyFrames_.empty())
        return true;

    const Variant& firstValue = keyFrames_.front().value_;
    const Variant& lastValue = keyFrames_.back().value_;

    return firstValue == lastValue;
}

} // namespace Urho3D
