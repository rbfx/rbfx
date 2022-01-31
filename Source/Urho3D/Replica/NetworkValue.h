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
#include "../Replica/NetworkTime.h"

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

    static T Extrapolate(const T& value, float extrapolationFactor) { return value; }
};

template <>
struct NetworkValueTraits<Quaternion>
{
    static Quaternion Interpolate(const Quaternion& lhs, const Quaternion& rhs, float blendFactor)
    {
        return lhs.Slerp(rhs, blendFactor);
    }

    static Quaternion Extrapolate(const Quaternion& value, float extrapolationFactor) { return value; }
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

    FrameReconstructionBase FindReconstructionBase(unsigned frame) const
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

        // Extrapolate from prior frame
        return {FrameReconstructionMode::Extrapolate, *frameBefore, *frameBefore};
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

    static float GetFrameExtrapolationFactor(unsigned baseFrame, unsigned extrapolatedFrame, unsigned maxExtrapolation)
    {
        return static_cast<float>(ea::min(extrapolatedFrame - baseFrame, maxExtrapolation));
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
    using NetworkValueBase::IsInitialized;

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

    /// Interpolate between two frames or return value of the closest valid frame.
    T SampleValid(const NetworkTime& time) const
    {
        const InterpolationBase interpolation = GetValidFrameInterpolation(time);

        if (interpolation.firstIndex_ == interpolation.secondIndex_)
            return values_[interpolation.firstIndex_];

        return Traits::Interpolate(
            values_[interpolation.firstIndex_], values_[interpolation.secondIndex_], interpolation.blendFactor_);
    }

    T SampleValid(unsigned frame) const { return SampleValid(NetworkTime{frame}); }

    /// Interpolate between two frames or extrapolate for the latest frame without any filtering.
    T CalculateReconstructedValue(unsigned frame, unsigned maxExtrapolation) const
    {
        const FrameReconstructionBase base = FindReconstructionBase(frame);
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
            // Skip extrapolation if unwanted
            if (maxExtrapolation == 0)
                return lastValue;

            const float factor = GetFrameExtrapolationFactor(base.lastFrame_, frame, maxExtrapolation);
            return Traits::Extrapolate(lastValue, factor);
        }

        case FrameReconstructionMode::None:
        default:
            return lastValue;
        }
    }

private:
    ea::vector<T> values_;
};

/// Helper class that manages continuous sampling of NetworkValue on the client side.
template <class T, class Traits = NetworkValueTraits<T>>
class NetworkValueSampler
{
public:
    /// Client-side sampling: sample value reconstructing missing values.
    ea::optional<T> ReconstructAndSample(const NetworkValue<T, Traits>& value, const NetworkTime& time, unsigned maxExtrapolation = 0)
    {
        if (!value.IsInitialized())
            return ea::nullopt;

        const unsigned frame = time.GetFrame();

        if (!reconstruct_ || reconstruct_->frame_ != frame)
        {
            if (!reconstruct_)
                reconstruct_ = ReconstructCache{frame};

            if (reconstruct_->frame_ + 1 == frame)
                reconstruct_->values_[0] = reconstruct_->values_[1];
            else
                reconstruct_->values_[0] = value.CalculateReconstructedValue(frame, maxExtrapolation);

            reconstruct_->values_[1] = value.CalculateReconstructedValue(frame + 1, maxExtrapolation);
            reconstruct_->frame_ = frame;
        }

        return Traits::Interpolate(reconstruct_->values_[0], reconstruct_->values_[1], time.GetSubFrame());
    }

private:
    struct ReconstructCache
    {
        unsigned frame_{};
        ea::array<T, 2> values_;
    };

    ea::optional<ReconstructCache> reconstruct_;
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
