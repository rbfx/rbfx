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

/// Base class for NetworkValue and NetworkValueVector.
class NetworkValueBase
{
public:
    NetworkValueBase() = default;

    unsigned GetCapacity() const { return elements_.size(); }
    unsigned GetFirstFrame() const { return lastFrame_ - GetCapacity(); }
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

        elements_.clear();
        elements_.resize(capacity);
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
            if (elements_[*index])
                return index;
        }
        return ea::nullopt;
    }

    bool AllocateFrame(unsigned frame, unsigned penalty, bool overwrite)
    {
        URHO3D_ASSERT(!elements_.empty());

        // Initialize first frame if not intialized
        if (!initialized_)
        {
            initialized_ = true;
            lastFrame_ = frame;
            lastIndex_ = 0;

            elements_[lastIndex_] = penalty;
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
                elements_[FrameToIndexUnchecked(skippedFrame)] = ea::nullopt;

            elements_[lastIndex_] = penalty;
            return true;
        }

        // Set past value if within buffer
        if (auto index = FrameToIndex(frame))
        {
            // Set singular past value, overwrite is optional
            if (overwrite || !elements_[*index])
            {
                elements_[*index] = penalty;
                return true;
            }
        }

        return false;
    }

    bool HasFrame(unsigned frame) const { return AllocatedFrameToIndex(frame).has_value(); }

    ea::optional<unsigned> FindClosestAllocatedFrame(unsigned frame, bool searchFuture) const
    {
        if (HasFrame(frame))
            return frame;

        // Search past values if any
        const unsigned firstFrame = GetFirstFrame();
        if (IsFrameGreaterThan(frame, firstFrame))
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

    template <class Callback>
    void RepairMissingFramesUpTo(unsigned frame, const Callback& callback)
    {
        if (!initialized_)
            return;

        // Skip repair if frame is good or if cannot repair
        const auto baseFrame = FindClosestAllocatedFrame(frame, false);
        if (!baseFrame || *baseFrame == frame)
            return;

        // Invoke callback for all missing frames
        unsigned penalty = *elements_[FrameToIndexUnchecked(*baseFrame)];
        for (unsigned frameToRepair = *baseFrame + 1; frameToRepair != frame + 1; ++frameToRepair)
        {
            if (penalty != M_MAX_UNSIGNED)
                ++penalty;

            const bool added = AllocateFrame(frameToRepair, penalty, true);
            URHO3D_ASSERT(added);
            callback(frameToRepair, penalty);
        }
    }

private:
    bool initialized_{};
    unsigned lastFrame_{};
    unsigned lastIndex_{};
    ea::vector<ea::optional<unsigned>> elements_;
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

    /// Set value for given frame if not set. No op otherwise.
    void Append(unsigned frame, const T& value) { SetInternal(frame, value, false); }

    /// Set value for given frame. Overwrites previous value.
    void Replace(unsigned frame, const T& value) { SetInternal(frame, value, true); }

    /// Return raw value at given frame.
    ea::optional<T> GetRaw(unsigned frame) const
    {
        if (const auto index = AllocatedFrameToIndex(frame))
            return values_[*index];
        return ea::nullopt;
    }

    /// Return closest valid raw value, if possible. Prior values take precedence.
    T GetClosestRaw(unsigned frame) const
    {
        if (const auto closestFrame = FindClosestAllocatedFrame(frame, true))
            return values_[FrameToIndexUnchecked(*closestFrame)];

        URHO3D_ASSERT(0);
        return *GetRaw(GetLastFrame());
    }

    /// Client-side sampling: repair missing values if necessary and sample value.
    ea::optional<T> RepairAndSample(const NetworkTime& time, unsigned maxExtrapolationPenalty = 0)
    {
        const auto callback = [&](unsigned frame, unsigned penalty)
        {
            const auto previousIndex = AllocatedFrameToIndex(frame - 2);
            const unsigned baseIndex = FrameToIndexUnchecked(frame - 1);
            const unsigned repairedIndex = FrameToIndexUnchecked(frame);
            const auto nextIndex = AllocatedFrameToIndex(frame + 1);

            const bool isLowPenalty = penalty < maxExtrapolationPenalty || maxExtrapolationPenalty == M_MAX_UNSIGNED;
            if (nextIndex)
                values_[repairedIndex] = Traits::Interpolate(values_[baseIndex], values_[*nextIndex], 0.5f);
            else if (isLowPenalty && previousIndex)
                values_[repairedIndex] = Traits::Extrapolate(values_[*previousIndex], values_[baseIndex]);
            else
                values_[repairedIndex] = values_[baseIndex];
        };

        const unsigned frame = time.GetFrame();
        RepairMissingFramesUpTo(frame, callback);
        RepairMissingFramesUpTo(frame + 1, callback);

        const auto index = AllocatedFrameToIndex(frame);
        const auto nextIndex = AllocatedFrameToIndex(frame + 1);
        if (index && nextIndex)
            return Traits::Interpolate(values_[*index], values_[*nextIndex], time.GetSubFrame());
        return ea::nullopt;
    }

    /// Server-side sampling: interpolate between consequent frames
    /// or return value of the closest valid frame.
    T SampleValid(const NetworkTime& time) const
    {
        const unsigned frame = time.GetFrame();
        const auto index = AllocatedFrameToIndex(frame);

        if (index && time.GetSubFrame() < M_LARGE_EPSILON)
            return values_[*index];

        const auto nextIndex = AllocatedFrameToIndex(frame + 1);
        if (index && nextIndex)
            return Traits::Interpolate(values_[*index], values_[*nextIndex], time.GetSubFrame());

        return GetClosestRaw(frame);
    }

    T SampleValid(unsigned frame) const { return SampleValid(NetworkTime{frame}); }

private:
    void SetInternal(unsigned frame, const T& value, bool overwrite)
    {
        if (AllocateFrame(frame, 0, overwrite))
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            values_[index] = value;
        }
    }

    ea::vector<T> values_;
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

    /// Set value for given frame if not set. No op otherwise.
    void Append(unsigned frame, ValueSpan value) { SetInternal(frame, value, false); }

    /// Set value for given frame. Overwrites previous value.
    void Replace(unsigned frame, ValueSpan value) { SetInternal(frame, value, true); }

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
        if (const auto closestFrame = FindClosestAllocatedFrame(frame, true))
            return GetSpanForIndex(FrameToIndexUnchecked(*closestFrame));

        URHO3D_ASSERT(0);
        return *GetRaw(GetLastFrame());
    }

    /// Client-side sampling: repair missing values if necessary and sample value.
    ea::optional<InterpolatedValueSpan> RepairAndSample(const NetworkTime& time, unsigned maxExtrapolationPenalty = 0)
    {
        const auto callback = [&](unsigned frame, unsigned penalty)
        {
            const auto previousIndex = AllocatedFrameToIndex(frame - 2);
            const unsigned baseIndex = FrameToIndexUnchecked(frame - 1);
            const unsigned repairedIndex = FrameToIndexUnchecked(frame);
            const auto nextIndex = AllocatedFrameToIndex(frame + 1);

            const bool isLowPenalty = penalty < maxExtrapolationPenalty || maxExtrapolationPenalty == M_MAX_UNSIGNED;

            for (unsigned i = 0; i < size_; ++i)
            {
                const unsigned repairedSubIndex = GetElementIndex(repairedIndex, i);
                const unsigned baseSubIndex = GetElementIndex(baseIndex, i);

                if (nextIndex)
                    values_[repairedSubIndex] = Traits::Interpolate(values_[baseSubIndex], values_[GetElementIndex(*nextIndex, i)], 0.5f);
                else if (isLowPenalty && previousIndex)
                    values_[repairedSubIndex] = Traits::Extrapolate(values_[GetElementIndex(*previousIndex, i)], values_[baseSubIndex]);
                else
                    values_[repairedSubIndex] = values_[baseSubIndex];
            }
        };

        const unsigned frame = time.GetFrame();
        RepairMissingFramesUpTo(frame, callback);
        RepairMissingFramesUpTo(frame + 1, callback);

        const auto index = AllocatedFrameToIndex(frame);
        const auto nextIndex = AllocatedFrameToIndex(frame + 1);
        if (index && nextIndex)
            return InterpolatedValueSpan{GetSpanForIndex(*index), GetSpanForIndex(*nextIndex), time.GetSubFrame()};
        return ea::nullopt;
    }

    /// Server-side sampling: interpolate between consequent frames
    /// or return value of the closest valid frame.
    InterpolatedValueSpan SampleValid(const NetworkTime& time) const
    {
        const unsigned frame = time.GetFrame();
        const auto index = AllocatedFrameToIndex(frame);

        if (index && time.GetSubFrame() < M_LARGE_EPSILON)
            return InterpolatedValueSpan{GetSpanForIndex(*index)};

        const auto nextIndex = AllocatedFrameToIndex(frame + 1);
        if (index && nextIndex)
            return InterpolatedValueSpan{GetSpanForIndex(*index), GetSpanForIndex(*nextIndex), time.GetSubFrame()};

        return InterpolatedValueSpan{GetClosestRaw(frame)};
    }

    InterpolatedValueSpan SampleValid(unsigned frame) const { return SampleValid(NetworkTime{frame}); }

private:
    unsigned GetElementIndex(unsigned index, unsigned subIndex) const { return index * size_ + subIndex; }

    ValueSpan GetSpanForIndex(unsigned index) const
    {
        return ValueSpan(values_).subspan(index * size_, size_);
    }

    void SetInternal(unsigned frame, ea::span<const T> value, bool overwrite)
    {
        if (AllocateFrame(frame, 0, overwrite))
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            const unsigned count = ea::min<unsigned>(value.size(), size_);
            ea::copy_n(value.begin(), count, &values_[index * size_]);
        }
    }

    unsigned size_{};
    ea::vector<T> values_;
};

}
