//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Core/Variant.h"

#include <EASTL/vector.h>
#include <EASTL/sort.h>

namespace Urho3D
{

/// Sorted array of keyframes.
/// T: must be structure with member `float time_` which is used for ordering.
template <class T>
struct KeyFrameSet
{
    using KeyFrame = T;

    /// Sort keyframes by time.
    void SortKeyFrames()
    {
        static const auto compare = [](const KeyFrame& lhs, const KeyFrame& rhs) { return lhs.time_ < rhs.time_; };
        ea::sort(keyFrames_.begin(), keyFrames_.end(), compare);
    }

    /// Append keyframe preserving container order.
    void AddKeyFrame(const KeyFrame& keyFrame)
    {
        const bool needSort = keyFrames_.size() > 0 && keyFrames_.back().time_ > keyFrame.time_;
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
    /// @property
    unsigned GetNumKeyFrames() const { return keyFrames_.size(); }

    /// Return keyframes for interpolation.
    void GetKeyFrames(float time, float duration, bool isLooped,
        unsigned& frameIndex, unsigned& nextFrameIndex, float& blendFactor) const
    {
        GetKeyFrameIndex(time, frameIndex);

        const unsigned numFrames = keyFrames_.size();
        nextFrameIndex = isLooped
            ? (frameIndex + 1) % numFrames  // Wrap around if looped
            : ea::min(frameIndex + 1, numFrames - 1);  // Trim if not looped

        if (frameIndex != nextFrameIndex)
        {
            const float frameTime = keyFrames_[frameIndex].time_;
            const float nextFrameTime = keyFrames_[nextFrameIndex].time_;

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
        while (index && time < keyFrames_[index].time_)
            --index;

        // Check for being too far behind
        while (index < keyFrames_.size() - 1 && time >= keyFrames_[index + 1].time_)
            ++index;

        return true;
    }

    ea::vector<KeyFrame> keyFrames_;
};


}
