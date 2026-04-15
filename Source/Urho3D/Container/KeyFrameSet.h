// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Assert.h"
#include "Urho3D/Core/Variant.h"

#include <EASTL/sort.h>
#include <EASTL/vector.h>

namespace Urho3D
{

template <class T> float GetKeyFrameTime(const T& keyFrame)
{
    return keyFrame.time_;
}

template <class T, class U> float GetKeyFrameTime(const ea::pair<T, U>& keyFrame)
{
    return keyFrame.first;
}

/// Sorted array of keyframes.
/// T: GetKeyFrameTime(T) must return `float` time to be used for ordering.
template <class T> struct KeyFrameSet
{
    using KeyFrame = T;

    /// Sort keyframes by time.
    void SortKeyFrames()
    {
        static const auto compare = [](const KeyFrame& lhs, const KeyFrame& rhs)
        {
            return GetKeyFrameTime(lhs) < GetKeyFrameTime(rhs);
            //
        };
        ea::sort(keyFrames_.begin(), keyFrames_.end(), compare);
    }

    /// Append keyframe preserving container order.
    void AddKeyFrame(const KeyFrame& keyFrame)
    {
        const bool needSort = keyFrames_.size() > 0 && GetKeyFrameTime(keyFrames_.back()) > GetKeyFrameTime(keyFrame);
        keyFrames_.push_back(keyFrame);
        if (needSort)
            SortKeyFrames();
    }

    /// Remove a keyframe at index.
    void RemoveKeyFrame(unsigned index) { keyFrames_.erase_at(index); }

    /// Remove all keyframes.
    void RemoveAllKeyFrames() { keyFrames_.clear(); }

    /// Return keyframe at index, or null if not found.
    KeyFrame* GetKeyFrame(unsigned index) { return index < keyFrames_.size() ? &keyFrames_[index] : nullptr; }

    /// Return number of keyframes.
    unsigned GetNumKeyFrames() const { return keyFrames_.size(); }

    /// Return whether the set is empty.
    bool IsEmpty() const { return keyFrames_.empty(); }

    /// Return keyframes for interpolation.
    void GetKeyFrames(float time, float duration, bool isLooped, unsigned& frameIndex, unsigned& nextFrameIndex,
        float& blendFactor) const
    {
        GetKeyFrameIndex(time, frameIndex);

        const unsigned numFrames = keyFrames_.size();
        nextFrameIndex = isLooped //
            ? (frameIndex + 1) % numFrames // Wrap around if looped
            : ea::min(frameIndex + 1, numFrames - 1); // Trim if not looped

        if (frameIndex != nextFrameIndex)
        {
            const float frameTime = GetKeyFrameTime(keyFrames_[frameIndex]);
            const float nextFrameTime = GetKeyFrameTime(keyFrames_[nextFrameIndex]);

            float timeInterval = nextFrameTime - frameTime;
            if (timeInterval < 0.0f)
                timeInterval += duration;
            blendFactor = timeInterval > 0.0f ? (time - frameTime) / timeInterval : 1.0f;
        }
        else
        {
            blendFactor = 0.0f;
        }
    }

    /// Return keyframe index based on time and previous index as hint. Return false if animation is empty.
    bool GetKeyFrameIndex(float time, unsigned& index) const
    {
        if (keyFrames_.empty())
            return false;

        if (time < 0.0f)
            time = 0.0f;

        if (index >= keyFrames_.size())
            index = keyFrames_.size() - 1;

        // Check for being too far ahead
        while (index && time < GetKeyFrameTime(keyFrames_[index]))
            --index;

        // Check for being too far behind
        while (index < keyFrames_.size() - 1 && time >= GetKeyFrameTime(keyFrames_[index + 1]))
            ++index;

        return true;
    }

    ea::vector<KeyFrame> keyFrames_;
};

} // namespace Urho3D
