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

namespace Detail
{

/// Return squared distance between points.
/// @{
inline float GetDistanceSquared(float lhs, float rhs) { return (lhs - rhs) * (lhs - rhs); }
inline float GetDistanceSquared(const Vector2& lhs, const Vector2& rhs) { return (lhs - rhs).LengthSquared(); }
inline float GetDistanceSquared(const Vector3& lhs, const Vector3& rhs) { return (lhs - rhs).LengthSquared(); }
inline float GetDistanceSquared(const Quaternion& lhs, const Quaternion& rhs) { return 1.0f - Abs(lhs.DotProduct(rhs)); }
template <class T> inline float GetDistanceSquared(const T& lhs, const T& rhs) { return 0.0f; }
/// @}

/// Base class for NetworkValue and NetworkValueVector.
class NetworkValueBase
{
public:
    struct InterpolationBase
    {
        NetworkFrame firstFrame_{};
        unsigned firstIndex_{};

        NetworkFrame secondFrame_{};
        unsigned secondIndex_{};

        float blendFactor_{};
    };

    NetworkValueBase() = default;

    bool IsInitialized() const { return initialized_; }
    unsigned GetCapacity() const { return hasFrameByIndex_.size(); }
    NetworkFrame GetFirstFrame() const { return lastFrame_ - GetCapacity() + 1; }
    NetworkFrame GetLastFrame() const { return lastFrame_; }

    void Resize(unsigned capacity)
    {
        URHO3D_ASSERT(capacity > 0);

        hasFrameByIndex_.clear();
        hasFrameByIndex_.resize(capacity);
    }

    ea::optional<unsigned> FrameToIndex(NetworkFrame frame) const
    {
        const auto capacity = GetCapacity();
        const auto behind = lastFrame_ - frame;
        if (behind >= 0 && behind < capacity)
            return (lastIndex_ + capacity - behind) % capacity;
        return ea::nullopt;
    }

    unsigned FrameToIndexUnchecked(NetworkFrame frame) const
    {
        const auto index = FrameToIndex(frame);
        URHO3D_ASSERT(index);
        return *index;
    }

    ea::optional<unsigned> AllocatedFrameToIndex(NetworkFrame frame) const
    {
        if (auto index = FrameToIndex(frame))
        {
            if (hasFrameByIndex_[*index])
                return *index;
        }
        return ea::nullopt;
    }

    bool AllocateFrame(NetworkFrame frame)
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
        if (frame > lastFrame_)
        {
            const int offset = static_cast<int>(frame - lastFrame_);
            lastFrame_ = frame;
            lastIndex_ = (lastIndex_ + offset) % GetCapacity();

            // Reset skipped frames
            const NetworkFrame firstSkippedFrame = ea::max(lastFrame_ - offset + 1, GetFirstFrame());
            for (NetworkFrame skippedFrame = firstSkippedFrame; skippedFrame != lastFrame_; ++skippedFrame)
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

    bool HasFrame(NetworkFrame frame) const { return AllocatedFrameToIndex(frame).has_value(); }

    ea::optional<NetworkFrame> FindClosestAllocatedFrame(NetworkFrame frame, bool searchPast, bool searchFuture) const
    {
        if (HasFrame(frame))
            return frame;

        const NetworkFrame firstFrame = GetFirstFrame();

        // Search past values if any
        if (searchPast && (frame > firstFrame))
        {
            const NetworkFrame lastCheckedFrame = ea::min(lastFrame_, frame - 1);
            for (NetworkFrame pastFrame = lastCheckedFrame; pastFrame != firstFrame - 1; --pastFrame)
            {
                if (HasFrame(pastFrame))
                    return pastFrame;
            }
        }

        // Search future values if any
        if (searchFuture && (frame < lastFrame_))
        {
            const NetworkFrame firstCheckedFrame = ea::max(firstFrame, frame + 1);
            for (NetworkFrame futureFrame = firstCheckedFrame; futureFrame != lastFrame_ + 1; ++futureFrame)
            {
                if (HasFrame(futureFrame))
                    return futureFrame;
            }
        }

        return ea::nullopt;
    }

    NetworkFrame GetClosestAllocatedFrame(NetworkFrame frame) const
    {
        URHO3D_ASSERT(initialized_);
        if (const auto closestFrame = FindClosestAllocatedFrame(frame, true, true))
            return *closestFrame;
        return lastFrame_;
    }

    InterpolationBase GetValidFrameInterpolation(const NetworkTime& time) const
    {
        const NetworkFrame frame = time.Frame();
        const auto thisOrPastFrame = FindClosestAllocatedFrame(frame, true, false);

        // Optimize for exact queries
        if (thisOrPastFrame == frame && time.Fraction() < M_LARGE_EPSILON)
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            return InterpolationBase{frame, index, frame, index, 0.0f};
        }

        const auto nextOrFutureFrame = FindClosestAllocatedFrame(frame + 1, false, true);
        if (thisOrPastFrame && nextOrFutureFrame)
        {
            const unsigned firstIndex = FrameToIndexUnchecked(*thisOrPastFrame);
            const unsigned secondIndex = FrameToIndexUnchecked(*nextOrFutureFrame);
            const auto extraPastFrames = static_cast<int>(frame - *thisOrPastFrame);
            const auto extraFutureFrames = static_cast<int>(*nextOrFutureFrame - frame - 1);
            const float adjustedFactor =
                (extraPastFrames + time.Fraction()) / (extraPastFrames + extraFutureFrames + 1);
            return InterpolationBase{*thisOrPastFrame, firstIndex, *nextOrFutureFrame, secondIndex, adjustedFactor};
        }

        const NetworkFrame closestFrame = thisOrPastFrame.value_or(nextOrFutureFrame.value_or(lastFrame_));
        const unsigned index = FrameToIndexUnchecked(closestFrame);
        return InterpolationBase{closestFrame, index, closestFrame, index, 0.0f};
    }

private:
    bool initialized_{};
    NetworkFrame lastFrame_{};
    unsigned lastIndex_{};
    ea::vector<bool> hasFrameByIndex_;
};

/// Helper class to interpolate value spans.
template <class T, class Traits>
class InterpolatedConstSpan
{
public:
    explicit InterpolatedConstSpan(ea::span<const T> valueSpan)
        : first_(valueSpan)
        , second_(valueSpan)
        , snapThreshold_(M_LARGE_VALUE)
    {
    }

    InterpolatedConstSpan(ea::span<const T> firstSpan, ea::span<const T> secondSpan, float blendFactor, float snapThreshold)
        : first_(firstSpan)
        , second_(secondSpan)
        , blendFactor_(blendFactor)
        , snapThreshold_(snapThreshold)
    {
    }

    T operator[](unsigned index) const { return Traits::Interpolate(first_[index], second_[index], blendFactor_, snapThreshold_); }

    unsigned Size() const { return first_.size(); }

private:
    ea::span<const T> first_;
    ea::span<const T> second_;
    float blendFactor_{};
    float snapThreshold_{};
};

}

/// Value with derivative, can be extrapolated.
template <class T>
struct ValueWithDerivative
{
    T value_{};
    T derivative_{};
};

template <class T> inline bool operator==(const ValueWithDerivative<T>& lhs, const T& rhs) { return lhs.value_ == rhs; }
template <class T> inline bool operator==(const T& lhs, const ValueWithDerivative<T>& rhs) { return lhs == rhs.value_; }

/// Derivative of a quaternion is angular velocity vector.
template <>
struct ValueWithDerivative<Quaternion>
{
    Quaternion value_{};
    Vector3 derivative_{};
};

/// Utility to manipulate values stored in NetworkValue.
/// @{
template <class T>
struct NetworkValueTraits
{
    using InternalType = T;
    using ReturnType = T;

    static InternalType Interpolate(
        const InternalType& lhs, const InternalType& rhs, float blendFactor, float snapThreshold)
    {
        if (Detail::GetDistanceSquared(lhs, rhs) >= snapThreshold * snapThreshold)
            return blendFactor < 0.5f ? lhs : rhs;
        return Lerp(lhs, rhs, blendFactor);
    }

    static ReturnType Extract(const InternalType& value) { return value; }

    static ReturnType Extrapolate(const InternalType& value, float extrapolationFactor) { return value; }

    static void UpdateCorrection(
        ReturnType& inverseCorrection, const ReturnType& correctValue, const ReturnType& oldValue)
    {
        inverseCorrection -= correctValue - oldValue;
    }

    static void SmoothCorrection(ReturnType& inverseCorrection, float blendFactor)
    {
        inverseCorrection = Lerp(inverseCorrection, ReturnType{}, blendFactor);
    }

    static void ApplyCorrection(const ReturnType& inverseCorrection, ReturnType& value)
    {
        value += inverseCorrection;
    }
};

template <>
struct NetworkValueTraits<Quaternion>
{
    using InternalType = Quaternion;
    using ReturnType = Quaternion;

    static Quaternion Extract(const Quaternion& value) { return value; }

    static Quaternion Interpolate(const Quaternion& lhs, const Quaternion& rhs, float blendFactor, float snapThreshold)
    {
        return lhs.Slerp(rhs, blendFactor);
    }

    static Quaternion Extrapolate(const Quaternion& value, float extrapolationFactor) { return value; }

    static void UpdateCorrection(Quaternion& inverseCorrection, const Quaternion& correctValue, const Quaternion& oldValue)
    {
        inverseCorrection = oldValue * correctValue.Inverse() * inverseCorrection;
    }

    static void SmoothCorrection(Quaternion& inverseCorrection, float blendFactor)
    {
        inverseCorrection = inverseCorrection.Slerp(Quaternion::IDENTITY, blendFactor);
    }

    static void ApplyCorrection(const Quaternion& inverseCorrection, Quaternion& value)
    {
        value = inverseCorrection * value;
    }
};

template <class T>
struct NetworkValueTraits<ValueWithDerivative<T>>
{
    using InternalType = ValueWithDerivative<T>;
    using ReturnType = T;

    static InternalType Interpolate(
        const InternalType& lhs, const InternalType& rhs, float blendFactor, float snapThreshold)
    {
        if (Detail::GetDistanceSquared(lhs.value_, rhs.value_) >= snapThreshold * snapThreshold)
            return blendFactor < 0.5f ? lhs : rhs;

        const auto interpolatedValue = Lerp(lhs.value_, rhs.value_, blendFactor);
        const auto interpolatedDerivative = Lerp(lhs.derivative_, rhs.derivative_, blendFactor);
        return InternalType{interpolatedValue, interpolatedDerivative};
    }

    static ReturnType Extract(const InternalType& value) { return value.value_; }

    static ReturnType Extrapolate(const InternalType& value, float extrapolationFactor)
    {
        return value.value_ + value.derivative_ * extrapolationFactor;
    }

    static void UpdateCorrection(
        ReturnType& inverseCorrection, const ReturnType& correctValue, const ReturnType& oldValue)
    {
        NetworkValueTraits<ReturnType>::UpdateCorrection(inverseCorrection, correctValue, oldValue);
    }

    static void SmoothCorrection(ReturnType& inverseCorrection, float blendFactor)
    {
        NetworkValueTraits<ReturnType>::SmoothCorrection(inverseCorrection, blendFactor);
    }

    static void ApplyCorrection(const ReturnType& inverseCorrection, ReturnType& value)
    {
        NetworkValueTraits<ReturnType>::ApplyCorrection(inverseCorrection, value);
    }
};

template <>
struct NetworkValueTraits<ValueWithDerivative<Quaternion>>
{
    using InternalType = ValueWithDerivative<Quaternion>;
    using ReturnType = Quaternion;

    static InternalType Interpolate(
        const InternalType& lhs, const InternalType& rhs, float blendFactor, float snapThreshold)
    {
        const auto interpolatedValue = lhs.value_.Slerp(rhs.value_, blendFactor);
        const auto interpolatedDerivative = Lerp(lhs.derivative_, rhs.derivative_, blendFactor);
        return InternalType{interpolatedValue, interpolatedDerivative};
    }

    static ReturnType Extract(const InternalType& value) { return value.value_; }

    static ReturnType Extrapolate(const InternalType& value, float extrapolationFactor)
    {
        return Quaternion::FromAngularVelocity(value.derivative_ * extrapolationFactor) * value.value_;
    }

    static void UpdateCorrection(
        ReturnType& inverseCorrection, const ReturnType& correctValue, const ReturnType& oldValue)
    {
        NetworkValueTraits<ReturnType>::UpdateCorrection(inverseCorrection, correctValue, oldValue);
    }

    static void SmoothCorrection(ReturnType& inverseCorrection, float blendFactor)
    {
        NetworkValueTraits<ReturnType>::SmoothCorrection(inverseCorrection, blendFactor);
    }

    static void ApplyCorrection(const ReturnType& inverseCorrection, ReturnType& value)
    {
        NetworkValueTraits<ReturnType>::ApplyCorrection(inverseCorrection, value);
    }
};
/// @}

/// Value stored at multiple points of time in ring buffer.
/// If value was set at least once, it will have at least one valid value forever.
/// Value can be sampled raw or interpolated. No extrapolation is performed.
template <class T, class Traits = NetworkValueTraits<T>>
class NetworkValue : private Detail::NetworkValueBase
{
public:
    using InternalType = typename Traits::InternalType;
    using ReturnType = typename Traits::ReturnType;

    using NetworkValueBase::IsInitialized;
    using NetworkValueBase::GetLastFrame;
    using NetworkValueBase::FindClosestAllocatedFrame;

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
    void Set(NetworkFrame frame, const InternalType& value)
    {
        if (AllocateFrame(frame))
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            values_[index] = value;
        }
    }

    /// Return whether the frame is present.
    bool Has(NetworkFrame frame) const { return AllocatedFrameToIndex(frame).has_value(); }

    /// Return raw value at given frame.
    ea::optional<InternalType> GetRaw(NetworkFrame frame) const
    {
        if (const auto index = AllocatedFrameToIndex(frame))
            return values_[*index];
        return ea::nullopt;
    }

    /// Return raw valid value at given frame.
    const InternalType& GetRawUnchecked(NetworkFrame frame) const
    {
        return values_[FrameToIndexUnchecked(frame)];
    }

    /// Return raw value at the given or prior frame.
    ea::optional<ea::pair<InternalType, NetworkFrame>> GetRawOrPrior(NetworkFrame frame) const
    {
        if (const auto closestFrame = FindClosestAllocatedFrame(frame, true, false))
            return ea::make_pair(values_[FrameToIndexUnchecked(*closestFrame)], *closestFrame);
        return ea::nullopt;
    }

    /// Return closest valid raw value. Prior values take precedence.
    InternalType GetClosestRaw(NetworkFrame frame) const
    {
        const NetworkFrame closestFrame = GetClosestAllocatedFrame(frame);
        return values_[FrameToIndexUnchecked(closestFrame)];
    }

    /// Interpolate between two frames or return value of the closest valid frame.
    InternalType SampleValid(const NetworkTime& time, float snapThreshold = M_LARGE_VALUE) const
    {
        return CalculateInterpolatedValue(time, snapThreshold).first;
    }

    /// Interpolate between two valid frames if possible.
    ea::optional<InternalType> SamplePrecise(const NetworkTime& time, float snapThreshold = M_LARGE_VALUE) const
    {
        const auto [value, isPrecise] = CalculateInterpolatedValue(time, snapThreshold);
        if (!isPrecise)
            return ea::nullopt;

        return value;
    }

private:
    /// Calculate exact, interpolated or nearest valid value. Return whether the result is precise.
    ea::pair<InternalType, bool> CalculateInterpolatedValue(const NetworkTime& time, float snapThreshold) const
    {
        const InterpolationBase interpolation = GetValidFrameInterpolation(time);

        const InternalType value = interpolation.firstIndex_ == interpolation.secondIndex_
            ? values_[interpolation.firstIndex_]
            : Traits::Interpolate(values_[interpolation.firstIndex_], values_[interpolation.secondIndex_],
                interpolation.blendFactor_, snapThreshold);

        const NetworkFrame frame = time.Frame();
        // Consider too old frames "precise" because we are not going to get any new data for them anyway.
        const bool isPrecise = /*interpolation.firstFrame_ <= frame &&*/ frame <= interpolation.secondFrame_;

        return {value, isPrecise};
    }

    ea::vector<InternalType> values_;
};

/// Helper class that manages continuous sampling of NetworkValue on the client side.
/// Performs extrapolation and error smoothing.
template <class T, class Traits = NetworkValueTraits<T>>
class NetworkValueSampler
{
public:
    using NetworkValueType = NetworkValue<T, Traits>;
    using InternalType = typename Traits::InternalType;
    using ReturnType = typename Traits::ReturnType;

    /// Update sampler settings.
    void Setup(unsigned maxExtrapolation, float smoothingConstant, float snapThreshold)
    {
        maxExtrapolation_ = maxExtrapolation;
        smoothingConstant_ = smoothingConstant;
        snapThreshold_ = snapThreshold;
    }

    /// Update sampler state for new time and return current value.
    ea::optional<ReturnType> UpdateAndSample(
        const NetworkValueType& value, const NetworkTime& time, float timeStep)
    {
        if (!value.IsInitialized())
            return ea::nullopt;

        UpdateCorrection(value, timeStep);
        UpdateCache(value, time.Frame());

        ReturnType sampledValue = CalculateValueFromCache(value, time);
        previousValue_ = TimeAndValue{time, sampledValue};

        Traits::ApplyCorrection(valueCorrection_, sampledValue);
        return sampledValue;
    }

private:
    float GetExtrapolationFactor(const NetworkTime& time, NetworkFrame baseFrame, unsigned maxExtrapolation) const
    {
        const float factor = (time.Frame() - baseFrame) + time.Fraction();
        return ea::min(factor, static_cast<float>(maxExtrapolation));
    }

    void UpdateCorrection(const NetworkValueType& value, float timeStep)
    {
        if (!previousValue_)
            return;

        Traits::SmoothCorrection(valueCorrection_, ExpSmoothingInv(smoothingConstant_, timeStep));

        UpdateCache(value, previousValue_->time_.Frame());
        const ReturnType newPreviousValue = CalculateValueFromCache(value, previousValue_->time_);
        Traits::UpdateCorrection(valueCorrection_, newPreviousValue, previousValue_->value_);
    }

    void UpdateCache(const NetworkValueType& value, NetworkFrame frame)
    {
        // Nothing to do if cache is valid
        if (interpolationCache_ && interpolationCache_->baseFrame_ == frame)
            return;

        if (const auto nextValue = value.SamplePrecise(NetworkTime{frame + 1}, snapThreshold_))
        {
            // Update interpolation if has enough data for it.
            // Get base value from cache if possible, or just take previous frame.
            const InternalType baseValue = (interpolationCache_ && interpolationCache_->baseFrame_ + 1 == frame)
                ? interpolationCache_->nextValue_
                : value.SampleValid(NetworkTime{frame}, snapThreshold_);

            interpolationCache_ = InterpolationCache{frame, baseValue, *nextValue};
            extrapolationFrame_ = frame + 1;
        }
        else
        {
            // Update frame used for extrapolation.
            extrapolationFrame_ = value.GetLastFrame();
            URHO3D_ASSERT(extrapolationFrame_ && *extrapolationFrame_ < frame + 1);
        }
    }

    ReturnType CalculateValueFromCache(const NetworkValueType& value, const NetworkTime& time)
    {
        if (interpolationCache_ && interpolationCache_->baseFrame_ == time.Frame())
        {
            const InternalType value = Traits::Interpolate(
                interpolationCache_->baseValue_, interpolationCache_->nextValue_, time.Fraction(), snapThreshold_);
            return Traits::Extract(value);
        }

        URHO3D_ASSERT(extrapolationFrame_);

        const InternalType baseValue = *value.GetRaw(*extrapolationFrame_);
        const float factor = GetExtrapolationFactor(time, *extrapolationFrame_, maxExtrapolation_);
        return Traits::Extrapolate(baseValue, factor);
    }

    unsigned maxExtrapolation_{};
    float smoothingConstant_{};
    float snapThreshold_{M_LARGE_VALUE};

    struct InterpolationCache
    {
        NetworkFrame baseFrame_{};
        InternalType baseValue_{};
        InternalType nextValue_{};
    };

    struct TimeAndValue
    {
        NetworkTime time_;
        ReturnType value_{};
    };

    ea::optional<InterpolationCache> interpolationCache_;
    ea::optional<TimeAndValue> previousValue_;
    ea::optional<NetworkFrame> extrapolationFrame_;

    ReturnType valueCorrection_{};
};

/// Similar to NetworkValue, except each frame contains an array of elements.
template <class T, class Traits = NetworkValueTraits<T>>
class NetworkValueVector : private Detail::NetworkValueBase
{
public:
    using ValueSpan = ea::span<const T>;
    using InterpolatedValueSpan = Detail::InterpolatedConstSpan<T, Traits>;

    NetworkValueVector() = default;

    void Resize(unsigned size, unsigned capacity)
    {
        NetworkValueBase::Resize(capacity);

        size_ = ea::max(1u, size);
        values_.clear();
        values_.resize(size_ * capacity);
    }

    /// Return dynamic size of the vector.
    unsigned Size() const { return size_; }

    /// Set value and return uninitialized buffer to be filled.
    ea::span<T> SetUninitialized(NetworkFrame frame)
    {
        if (AllocateFrame(frame))
        {
            const unsigned index = FrameToIndexUnchecked(frame);
            return {&values_[index * size_], size_};
        }
        return {};
    }

    /// Set value for given frame if possible.
    void Set(NetworkFrame frame, ValueSpan value)
    {
        const auto dest = SetUninitialized(frame);
        if (!dest.empty())
        {
            const unsigned count = ea::min<unsigned>(value.size(), size_);
            ea::copy_n(value.begin(), count, dest.begin());
        }
    }

    /// Return raw value at given frame.
    ea::optional<ValueSpan> GetRaw(NetworkFrame frame) const
    {
        if (const auto index = AllocatedFrameToIndex(frame))
            return GetSpanForIndex(*index);
        return ea::nullopt;
    }

    /// Return closest valid raw value, if possible. Prior values take precedence.
    ValueSpan GetClosestRaw(NetworkFrame frame) const
    {
        const NetworkFrame closestFrame = GetClosestAllocatedFrame(frame);
        return GetSpanForIndex(FrameToIndexUnchecked(closestFrame));
    }

    /// Server-side sampling: interpolate between consequent frames
    /// or return value of the closest valid frame.
    InterpolatedValueSpan SampleValid(const NetworkTime& time, float snapThreshold = M_LARGE_VALUE) const
    {
        const InterpolationBase interpolation = GetValidFrameInterpolation(time);

        if (interpolation.firstIndex_ == interpolation.secondIndex_)
            return InterpolatedValueSpan{GetSpanForIndex(interpolation.firstIndex_)};

        return InterpolatedValueSpan{GetSpanForIndex(interpolation.firstIndex_),
            GetSpanForIndex(interpolation.secondIndex_), interpolation.blendFactor_, snapThreshold};
    }

private:
    ValueSpan GetSpanForIndex(unsigned index) const
    {
        return ValueSpan(values_).subspan(index * size_, size_);
    }

    unsigned size_{};
    ea::vector<T> values_;
};

}
