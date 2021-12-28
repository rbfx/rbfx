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

#include "../Core/Assert.h"
#include "../Math/MathDefs.h"
#include "../Math/Quaternion.h"
#include "../Network/NetworkTime.h"

#include <EASTL/optional.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Helper class to manipulate values stored in NetworkValue.
template <class T>
struct NetworkValueTraits
{
    static T Interpolate(const T& lhs, const T& rhs, float blendFactor) { return Lerp(lhs, rhs, blendFactor); }

    static T Extrapolate(const T& first, const T& second) { return second + (second - first); }
};

template <>
struct NetworkValueTraits<Quaternion>
{
    static Quaternion Interpolate(const Quaternion& lhs, const Quaternion& rhs, float blendFactor)
    {
        return lhs.Slerp(rhs, blendFactor);
    }

    static Quaternion Extrapolate(const Quaternion& first, const Quaternion& second)
    {
        return (second * first.Inverse()) * second;
    }
};

/// Value stored at multiple points of time in ring buffer.
/// If value was set at least once, it will have at least one valid value forever.
/// On Server, and values are treated as reliable and piecewise-continuous.
/// On Client, values may be extrapolated if frames are missing.
template <class T, class Traits = NetworkValueTraits<T>>
class NetworkValue : private Traits
{
public:
    NetworkValue() = default;
    explicit NetworkValue(unsigned capacity)
        : values_(capacity)
    {
    }
    explicit NetworkValue(const Traits& traits, unsigned capacity)
        : Traits(traits)
        , values_(capacity)
    {
    }

    unsigned GetCapacity() const { return values_.size(); }
    unsigned GetFirstFrame() const { return lastFrame_ - GetCapacity(); }
    unsigned GetLastFrame() const { return lastFrame_; }

    /// Resize container and reset contents except latest frame.
    void Resize(unsigned capacity)
    {
        URHO3D_ASSERT(capacity > 0);

        NetworkValue<T> result(capacity);
        if (initialized_)
            result.Replace(lastFrame_, values_[lastIndex_]->value_);

        *this = ea::move(result);
    }

    /// Set value for given frame if not set. No op otherwise.
    void Append(unsigned frame, const T& value) { SetInternal(frame, value, false, DefaultPenalty); }

    /// Set value for given frame. Overwrites previous value.
    void Replace(unsigned frame, const T& value) { SetInternal(frame, value, true, DefaultPenalty); }

    /// Return whether the frame is contained in the buffer (it may still have no value).
    bool Contains(unsigned frame) const
    {
        const int behind = static_cast<int>(lastFrame_ - frame);
        return behind >= 0 && behind < values_.size();
    }

    /// Return raw value at given frame.
    ea::optional<T> GetRaw(unsigned frame) const
    {
        if (Contains(frame))
        {
            if (const auto& frameValue = values_[FrameToIndex(frame)])
                return frameValue->value_;
        }
        return ea::nullopt;
    }

    /// Return closest valid raw value, if possible. Prior values take precedence.
    ea::optional<T> GetClosestRaw(unsigned frame) const
    {
        if (const auto value = GetRaw(frame))
            return *value;

        const int behind = static_cast<int>(lastFrame_ - frame);
        if (behind > 0)
        {
            const unsigned capacity = values_.size();
            const int clampedBehind = Clamp<int>(behind, 0, capacity - 1);

            // Check frames before `frame`
            const unsigned numFramesBefore = capacity - (clampedBehind + 1);
            for (unsigned i = 1; i <= numFramesBefore; ++i)
            {
                if (const auto pastValue = GetRaw(frame - i))
                    return *pastValue;
            }

            // Check frames after `frame`
            const unsigned numFramesAfter = clampedBehind;
            for (unsigned i = 1; i <= numFramesAfter; ++i)
            {
                if (const auto futureValue = GetRaw(frame + i))
                    return *futureValue;
            }
        }

        URHO3D_ASSERT(behind <= 0);
        return *GetRaw(lastFrame_);
    }

    /// Client-side sampling: repair missing values if necessary and sample value.
    ea::optional<T> RepairAndSample(const NetworkTime& time, unsigned maxExtrapolationPenalty = 0)
    {
        if (initialized_)
        {
            const unsigned frame = time.GetFrame();
            RepairFrame(frame + 1, maxExtrapolationPenalty);

            const auto frameValue = GetRaw(frame);
            const auto nextFrameValue = GetRaw(frame + 1);
            if (frameValue && nextFrameValue)
                return this->Interpolate(*frameValue, *nextFrameValue, time.GetSubFrame());
        }
        return ea::nullopt;
    }

    /// Server-side sampling: interpolate between consequent frames
    /// or return value of the closest valid frame.
    T SampleValid(const NetworkTime& time) const
    {
        const unsigned frame = time.GetFrame();

        const auto frameValue = GetRaw(frame);
        if (frameValue && time.GetSubFrame() < M_LARGE_EPSILON)
            return *frameValue;

        const auto nextFrameValue = GetRaw(frame + 1);
        if (frameValue && nextFrameValue)
            return this->Interpolate(*frameValue, *nextFrameValue, time.GetSubFrame());

        return GetClosestRaw(frame);
    }

    T SampleValid(unsigned frame) const { return SampleValid(NetworkTime{frame}); }

private:
    static constexpr unsigned DefaultPenalty = 0;

    struct FrameValue
    {
        T value_{};
        unsigned extrapolationPenalty_{};
    };

    void SetInternal(unsigned frame, const T& value, bool overwrite, unsigned penalty)
    {
        URHO3D_ASSERT(!values_.empty());

        // Initialize first frame
        if (!initialized_)
        {
            initialized_ = true;
            lastFrame_ = frame;
            lastIndex_ = 0;
            values_[0] = FrameValue{value, penalty};
            return;
        }

        const unsigned capacity = values_.size();
        const int offset = static_cast<int>(frame - lastFrame_);

        if (offset > 0)
        {
            // Roll buffer forward on positive delta, resetting intermediate values
            lastFrame_ += offset;
            lastIndex_ = (lastIndex_ + offset) % capacity;
            values_[lastIndex_] = FrameValue{value, penalty};

            const unsigned uninitializedBeginBehind = 1;
            const unsigned uninitializedEndBehind = ea::min(static_cast<unsigned>(offset), capacity);
            for (unsigned behind = uninitializedBeginBehind; behind < uninitializedEndBehind; ++behind)
            {
                const unsigned wrappedIndex = (lastIndex_ + capacity - behind) % capacity;
                values_[wrappedIndex] = ea::nullopt;
            }
        }
        else
        {
            // Set singular past value, no overwrite
            const unsigned behind = -offset;
            if (behind < capacity)
            {
                const unsigned wrappedIndex = (lastIndex_ + capacity - behind) % capacity;
                if (overwrite || !values_[wrappedIndex])
                    values_[wrappedIndex] = FrameValue{value, penalty};
            }
        }
    }

    ea::optional<unsigned> GetLatestValidFrame(unsigned frame) const
    {
        const int behind = static_cast<int>(lastFrame_ - frame);
        if (behind < 0)
            return lastFrame_;
        if (behind >= values_.size())
            return ea::nullopt;

        const unsigned firstFrame = GetFirstFrame();
        for (unsigned validFrame = frame; validFrame != firstFrame - 1; --validFrame)
        {
            if (values_[FrameToIndex(validFrame)])
                return validFrame;
        }

        return ea::nullopt;
    }

    void RepairFrame(unsigned frame, unsigned maxExtrapolationPenalty)
    {
        const auto latestValidFrame = GetLatestValidFrame(frame);
        if (!latestValidFrame)
            return;

        for (unsigned baseFrame = *latestValidFrame; baseFrame != frame; ++baseFrame)
        {
            const auto nextFrame = ExtrapolateNextFrameValue(baseFrame, maxExtrapolationPenalty);
            SetInternal(baseFrame + 1, nextFrame.value_, true, nextFrame.extrapolationPenalty_);
        }
    }

    FrameValue ExtrapolateNextFrameValue(unsigned frame, unsigned maxExtrapolationPenalty)
    {
        const unsigned index = FrameToIndex(frame);
        const unsigned penalty = values_[index]->extrapolationPenalty_;

        T nextValue = values_[index]->value_;
        if (penalty < maxExtrapolationPenalty || maxExtrapolationPenalty == M_MAX_UNSIGNED)
        {
            if (const auto prevValue = GetRaw(frame - 1))
                nextValue = this->Extrapolate(*prevValue, nextValue);
        }

        const unsigned nextPenalty = penalty != M_MAX_UNSIGNED ? penalty + 1 : penalty;
        return {nextValue, nextPenalty};
    }

    unsigned FrameToIndex(unsigned frame) const { return (lastIndex_ + (frame - lastFrame_ + values_.size())) % values_.size(); }

    bool initialized_{};
    unsigned lastFrame_{};
    unsigned lastIndex_{};
    ea::vector<ea::optional<FrameValue>> values_;
};

}
