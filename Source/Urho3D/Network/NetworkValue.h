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
#include <EASTL/span.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Helper class to manipulate values stored in NetworkValue.
template <class T>
struct NetworkValueTraits
{
    static T Interpolate(const T& lhs, const T& rhs, float blendFactor) { return Lerp(lhs, rhs, blendFactor); }

    static T Extrapolate(const T& first, const T& second, float extrapolationFactor)
    {
        return static_cast<T>(second + (second - first) * extrapolationFactor);
    }
};

template <>
struct NetworkValueTraits<Quaternion>
{
    static Quaternion Interpolate(const Quaternion& lhs, const Quaternion& rhs, float blendFactor)
    {
        return lhs.Slerp(rhs, blendFactor);
    }

    static Quaternion Extrapolate(const Quaternion& first, const Quaternion& second, float extrapolationFactor)
    {
        const Quaternion delta = second * first.Inverse();
        const Quaternion scaledDelta{delta.Angle() * extrapolationFactor, delta.Axis()};
        return scaledDelta * second;
    }
};

/// Extrapolation settings for NetworkValue and NetworkValueVector.
struct NetworkValueExtrapolationSettings
{
    /// Max number of frames beyond valid frame that can be extrapolated.
    /// 0 disables extrapolation completely.
    unsigned maxDistance_{};
    /// Max number of frames that may affect extrapolation. Frames beyond this range are ignored.
    unsigned maxLookback_{16};
    /// Min number of frames required for extrapolation. If there are not enough frames, no extrapolation happens.
    /// Should be at least 2.
    unsigned minFrames_{2};
};

/// Base class for NetworkValue and NetworkValueVector.
class NetworkValueBase
{
public:
    enum class FrameReconstructionMode
    {
        None,
        Interpolate,
        Extrapolate
    };

    struct FrameReconstructionBase
    {
        FrameReconstructionMode mode_{};
        unsigned firstFrame_{};
        unsigned lastFrame_{};
    };

    struct InterpolationBase
    {
        unsigned firstIndex_{};
        unsigned secondIndex_{};
        float blendFactor_{};
    };

    NetworkValueBase() = default;

    bool IsInitialized() const { return initialized_; }
    unsigned GetCapacity() const { return hasFrameByIndex_.size(); }
    unsigned GetFirstFrame() const { return lastFrame_ - GetCapacity() + 1; }
    unsigned GetLastFrame() const { return lastFrame_; }

    /// Intransitive frame comparison
    /// @{
    static int CompareFrames(unsigned lhs, unsigned rhs) { return Sign(static_cast<int>(lhs - rhs)); }
    static bool IsFrameGreaterThan(unsigned lhs, unsigned rhs) { return CompareFrames(lhs, rhs) > 0; }
    static bool IsFrameLessThan(unsigned lhs, unsigned rhs) { return CompareFrames(lhs, rhs) < 0; }
    static unsigned MaxFrame(unsigned lhs, unsigned rhs) { return IsFrameGreaterThan(lhs, rhs) ? lhs : rhs; }
    static unsigned MinFrame(unsigned lhs, unsigned rhs) { return IsFrameLessThan(lhs, rhs) ? lhs : rhs; }
    /// @}

    void Resize(unsigned capacity)
    {
        URHO3D_ASSERT(capacity > 0);

        hasFrameByIndex_.clear();
        hasFrameByIndex_.resize(capacity);
    }

    ea::optional<unsigned> FrameToIndex(unsigned frame) const
    {
        const auto capacity = GetCapacity();
        const auto behind = static_cast<int>(lastFrame_ - frame);
        if (behind >= 0 && behind < capacity)
            return (lastIndex_ + capacity - behind) % capacity;
        return ea::nullopt;
    }

    unsigned FrameToIndexUnchecked(unsigned frame) const
    {
        const auto index = FrameToIndex(frame);
        URHO3D_ASSERT(index);
        return *index;
    }

    ea::optional<unsigned> AllocatedFrameToIndex(unsigned frame) const
    {
        if (auto index = FrameToIndex(frame))
        {
            if (hasFrameByIndex_[*index])
                return *index;
        }
        return ea::nullopt;
    }

    bool AllocateFrame(unsigned frame)
    {
        URHO3D_ASSERT(!hasFrameByIndex_.empty());

        // Initialize first frame if not intialized
        if (!initialized_)
        {
            initialized_ = true;
            lastFrame_ = frame;
            lastIndex_ = 0;

            hasFrameByIndex_[lastIndex_] = true;
            return true;
        }

        // Roll ring buffer forward if frame is greater
        if (IsFrameGreaterThan(frame, lastFrame_))
        {
            const int offset = static_cast<int>(frame - lastFrame_);
            lastFrame_ += offset;
            lastIndex_ = (lastIndex_ + offset) % GetCapacity();

            // Reset skipped frames
            const unsigned firstSkippedFrame = MaxFrame(lastFrame_ - offset + 1, GetFirstFrame());
            for (unsigned skippedFrame = firstSkippedFrame; skippedFrame != lastFrame_; ++skippedFrame)
                hasFrameByIndex_[FrameToIndexUnchecked(skippedFrame)] = false;

            hasFrameByIndex_[lastIndex_] = true;
            return true;
        }

        // Set past value if within buffer
        if (auto index = FrameToIndex(frame))
        {
            // Set singular past value, overwrite is optional
            hasFrameByIndex_[*index] = true;
            return true;
        }

        return false;
    }

    bool HasFrame(unsigned frame) const { return AllocatedFrameToIndex(frame).has_value(); }

    ea::optional<unsigned> FindClosestAllocatedFrame(unsigned frame, bool searchPast, bool searchFuture) const
    {
        if (HasFrame(frame))
            return frame;

        const unsigned firstFrame = GetFirstFrame();

        // Search past values if any
        if (searchPast && IsFrameGreaterThan(frame, firstFrame))
        {
            const unsigned lastCheckedFrame = MinFrame(lastFrame_, frame - 1);
            for (unsigned pastFrame = lastCheckedFrame; pastFrame != firstFrame - 1; --pastFrame)
            {
                if (HasFrame(pastFrame))
                    return pastFrame;
            }
        }

        // Search future values if any
        if (searchFuture && IsFrameLessThan(frame, lastFrame_))
        {
            const unsigned firstCheckedFrame = MaxFrame(firstFrame, frame + 1);
            for (unsigned futureFrame = firstCheckedFrame; futureFrame != lastFrame_ + 1; ++futureFrame)
            {
                if (HasFrame(futureFrame))
                    return futureFrame;
            }
        }

        return ea::nullopt;
    }

    unsigned GetClosestAllocatedFrame(unsigned frame) const
    {
        URHO3D_ASSERT(initialized_);
        if (const auto closestFrame = FindClosestAllocatedFrame(frame, true, true))
            return *closestFrame;
        return lastFrame_;
    }

    FrameReconstructionBase FindReconstructionBase(unsigned frame, const NetworkValueExtrapolationSettings& settings) const
    {
        const auto frameBefore = FindClosestAllocatedFrame(frame, true, false);
        const auto frameAfter = FindClosestAllocatedFrame(frame, false, true);

        if (frameBefore && frameAfter)
        {
            // Frame is present, no reconstruction is needed
            if (*frameBefore == *frameAfter)
                return {FrameReconstructionMode::None, *frameBefore, *frameAfter};

            // Frame is missing but it can be interpolated from past and future
            return {FrameReconstructionMode::Interpolate, *frameBefore, *frameAfter};
        }
        else if (!frameBefore && !frameAfter)
        {
            // Shall never happen for initialized value
            URHO3D_ASSERT(0);
            return {FrameReconstructionMode::None, lastFrame_, lastFrame_};
        }

        // Frame is too far in the past, just take whatever we have
        if (!frameBefore)
            return {FrameReconstructionMode::None, *frameAfter, *frameAfter};

        // Find extrapolation range
        const unsigned firstCheckedFrame = MaxFrame(GetFirstFrame(), *frameBefore - settings.maxLookback_);
        const auto firstValidFrame = FindClosestAllocatedFrame(firstCheckedFrame, false, true);
        URHO3D_ASSERT(*firstValidFrame && !IsFrameGreaterThan(*firstValidFrame, *frameBefore));
        return {FrameReconstructionMode::Extrapolate, *firstValidFrame, *frameBefore};
    }

    InterpolationBase GetValidFrameInterpolation(const NetworkTime& time) const
    {
        const unsigned frame = time.GetFrame();
        const auto thisOrPastFrame = FindClosestAllocatedFrame(frame, true, false);

        // Optimize for exact queries
        if (thisOrPastFrame == frame && time.GetSubFrame() < M_LARGE_EPSILON)
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            return InterpolationBase{index, index, 0.0f};
        }

        const auto nextOrFutureFrame = FindClosestAllocatedFrame(frame + 1, false, true);
        if (thisOrPastFrame && nextOrFutureFrame)
        {
            const unsigned firstIndex = FrameToIndexUnchecked(*thisOrPastFrame);
            const unsigned secondIndex = FrameToIndexUnchecked(*nextOrFutureFrame);
            const auto extraPastFrames = static_cast<int>(frame - *thisOrPastFrame);
            const auto extraFutureFrames = static_cast<int>(*nextOrFutureFrame - frame - 1);
            const float adjustedFactor =
                (extraPastFrames + time.GetSubFrame()) / (extraPastFrames + extraFutureFrames + 1);
            return InterpolationBase{firstIndex, secondIndex, adjustedFactor};
        }

        const unsigned closestFrame = thisOrPastFrame.value_or(nextOrFutureFrame.value_or(lastFrame_));
        const unsigned index = FrameToIndexUnchecked(closestFrame);
        return InterpolationBase{index, index, 0.0f};
    }

    void CollectAllocatedFrames(unsigned firstFrame, unsigned lastFrame, ea::vector<unsigned>& frames) const
    {
        frames.clear();
        for (unsigned frame = firstFrame; frame != lastFrame + 1; ++frame)
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            if (hasFrameByIndex_[index])
                frames.push_back(frame);
        }
    }

    static float GetFrameInterpolationFactor(unsigned lhs, unsigned rhs, unsigned value)
    {
        const auto valueOffset = static_cast<int>(value - lhs);
        const auto maxOffset = static_cast<int>(rhs - lhs);
        return maxOffset >= 0 ? Clamp(static_cast<float>(valueOffset) / maxOffset, 0.0f, 1.0f) : 0.0f;
    }

    static float GetFrameExtrapolationFactor(
        unsigned lhs, unsigned rhs, unsigned value, const NetworkValueExtrapolationSettings& settings)
    {
        const unsigned extrapolationDistance = ea::min(value - rhs, settings.maxDistance_);
        return static_cast<float>(extrapolationDistance) / (rhs - lhs);
    }

private:
    bool initialized_{};
    unsigned lastFrame_{};
    unsigned lastIndex_{};
    ea::vector<bool> hasFrameByIndex_;
};

/// Value stored at multiple points of time in ring buffer.
/// If value was set at least once, it will have at least one valid value forever.
/// On Server, and values are treated as reliable and piecewise-continuous.
/// On Client, values may be extrapolated if frames are missing.
template <class T, class Traits = NetworkValueTraits<T>>
class NetworkValue : private NetworkValueBase
{
public:
    NetworkValue() = default;
    explicit NetworkValue(const Traits& traits)
        : Traits(traits)
    {
    }

    void Resize(unsigned capacity)
    {
        NetworkValueBase::Resize(capacity);

        values_.clear();
        values_.resize(capacity);
    }

    /// Set value for given frame if possible.
    void Set(unsigned frame, const T& value)
    {
        if (AllocateFrame(frame))
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            values_[index] = value;
        }
    }

    /// Return raw value at given frame.
    ea::optional<T> GetRaw(unsigned frame) const
    {
        if (const auto index = AllocatedFrameToIndex(frame))
            return values_[*index];
        return ea::nullopt;
    }

    /// Return closest valid raw value. Prior values take precedence.
    T GetClosestRaw(unsigned frame) const
    {
        const unsigned closestFrame = GetClosestAllocatedFrame(frame);
        return values_[FrameToIndexUnchecked(closestFrame)];
    }

    /// Server-side sampling: interpolate between consequent frames
    /// or return value of the closest valid frame.
    T SampleValid(const NetworkTime& time) const
    {
        const InterpolationBase interpolation = GetValidFrameInterpolation(time);

        if (interpolation.firstIndex_ == interpolation.secondIndex_)
            return values_[interpolation.firstIndex_];

        return Traits::Interpolate(
            values_[interpolation.firstIndex_], values_[interpolation.secondIndex_], interpolation.blendFactor_);
    }

    T SampleValid(unsigned frame) const { return SampleValid(NetworkTime{frame}); }

    /// Client-side sampling: sample value reconstructing missing values.
    ea::optional<T> ReconstructAndSample(const NetworkTime& time, const NetworkValueExtrapolationSettings& settings = {})
    {
        if (!IsInitialized())
            return ea::nullopt;

        const unsigned frame = time.GetFrame();

        if (!reconstruct_ || reconstruct_->frame_ != frame)
        {
            if (!reconstruct_)
                reconstruct_ = ReconstructCache{frame};

            if (reconstruct_->frame_ + 1 == frame)
                reconstruct_->values_[0] = reconstruct_->values_[1];
            else
                reconstruct_->values_[0] = CalculateReconstructedValue(frame, settings);

            reconstruct_->values_[1] = CalculateReconstructedValue(frame + 1, settings);
            reconstruct_->frame_ = frame;
        }

        return Traits::Interpolate(reconstruct_->values_[0], reconstruct_->values_[1], time.GetSubFrame());
    }

private:
    T CalculateReconstructedValue(unsigned frame, const NetworkValueExtrapolationSettings& settings)
    {
        const FrameReconstructionBase base = FindReconstructionBase(frame, settings);
        const T lastValue = values_[FrameToIndexUnchecked(base.lastFrame_)];
        switch (base.mode_)
        {
        case FrameReconstructionMode::Interpolate:
        {
            const T firstValue = values_[FrameToIndexUnchecked(base.firstFrame_)];
            const float factor = GetFrameInterpolationFactor(base.firstFrame_, base.lastFrame_, frame);
            return Traits::Interpolate(firstValue, lastValue, factor);
        }

        case FrameReconstructionMode::Extrapolate:
        {
            CollectAllocatedFrames(base.firstFrame_, base.lastFrame_, extrapolationFrames_);

            // Skip extrapolation if not enough data
            const unsigned numFrames = extrapolationFrames_.size();
            if (numFrames < settings.minFrames_)
                return lastValue;

            // Disable extrapolation immediately if static point detected.
            const unsigned beforeLastFrame = extrapolationFrames_[numFrames - 2];
            const T beforeLastValue = values_[FrameToIndexUnchecked(beforeLastFrame)];
            if (beforeLastValue == lastValue)
                return lastValue;

            // TODO: Add linear regression?
            const float factor = GetFrameExtrapolationFactor(beforeLastFrame, base.lastFrame_, frame, settings);
            return Traits::Extrapolate(beforeLastValue, lastValue, factor);
        }

        case FrameReconstructionMode::None:
        default:
            return lastValue;
        }
    }

    struct ReconstructCache
    {
        unsigned frame_{};
        ea::array<T, 2> values_;
    };

    ea::vector<T> values_;
    ea::optional<ReconstructCache> reconstruct_;

    ea::vector<unsigned> extrapolationFrames_;
};

/// Helper class to interpolate value spans.
template <class T, class Traits = NetworkValueTraits<T>>
class InterpolatedConstSpan
{
public:
    explicit InterpolatedConstSpan(ea::span<const T> valueSpan)
        : first_(valueSpan)
        , second_(valueSpan)
    {
    }

    InterpolatedConstSpan(ea::span<const T> firstSpan, ea::span<const T> secondSpan, float blendFactor)
        : first_(firstSpan)
        , second_(secondSpan)
        , blendFactor_(blendFactor)
    {
    }

    T operator[](unsigned index) const { return Traits::Interpolate(first_[index], second_[index], blendFactor_); }

    unsigned Size() const { return first_.size(); }

private:
    ea::span<const T> first_;
    ea::span<const T> second_;
    float blendFactor_{};
};

/// Similar to NetworkValue, except each frame contains an array of elements.
/// Does not support client-side reconstruction.
template <class T, class Traits = NetworkValueTraits<T>>
class NetworkValueVector : private NetworkValueBase
{
public:
    using ValueSpan = ea::span<const T>;
    using InterpolatedValueSpan = InterpolatedConstSpan<T, Traits>;

    NetworkValueVector() = default;

    void Resize(unsigned size, unsigned capacity)
    {
        NetworkValueBase::Resize(capacity);

        size_ = ea::max(1u, size);
        values_.clear();
        values_.resize(size_ * capacity);
    }

    /// Set value for given frame if possible.
    void Set(unsigned frame, ValueSpan value)
    {
        if (AllocateFrame(frame))
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            const unsigned count = ea::min<unsigned>(value.size(), size_);
            ea::copy_n(value.begin(), count, &values_[index * size_]);
        }
    }

    /// Return raw value at given frame.
    ea::optional<ValueSpan> GetRaw(unsigned frame) const
    {
        if (const auto index = AllocatedFrameToIndex(frame))
            return GetSpanForIndex(*index);
        return ea::nullopt;
    }

    /// Return closest valid raw value, if possible. Prior values take precedence.
    ValueSpan GetClosestRaw(unsigned frame) const
    {
        const unsigned closestFrame = GetClosestAllocatedFrame(frame);
        return GetSpanForIndex(FrameToIndexUnchecked(closestFrame));
    }

    /// Server-side sampling: interpolate between consequent frames
    /// or return value of the closest valid frame.
    InterpolatedValueSpan SampleValid(const NetworkTime& time) const
    {
        const InterpolationBase interpolation = GetValidFrameInterpolation(time);

        if (interpolation.firstIndex_ == interpolation.secondIndex_)
            return InterpolatedValueSpan{GetSpanForIndex(interpolation.firstIndex_)};

        return InterpolatedValueSpan{GetSpanForIndex(interpolation.firstIndex_),
            GetSpanForIndex(interpolation.secondIndex_), interpolation.blendFactor_};
    }

    InterpolatedValueSpan SampleValid(unsigned frame) const { return SampleValid(NetworkTime{frame}); }

private:
    ValueSpan GetSpanForIndex(unsigned index) const
    {
        return ValueSpan(values_).subspan(index * size_, size_);
    }

    unsigned size_{};
    ea::vector<T> values_;
};

}
