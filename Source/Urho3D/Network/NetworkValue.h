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

#include <EASTL/optional.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Value stored at multiple points of time in ring buffer.
template <class T>
class NetworkValue
{
public:
    using Container = ea::vector<ea::optional<T>>;

    /// Resize container preserving contents when possible.
    void Resize(unsigned capacity)
    {
        URHO3D_ASSERT(capacity > 0);

        const unsigned oldCapacity = values_.size();
        if (capacity == oldCapacity)
        {
            return;
        }
        else if (oldCapacity == 0)
        {
            // Initialize normally first time
            values_.resize(capacity);
        }
        else if (capacity > oldCapacity)
        {
            // Insert new elements after last one, it will not affect index of last one
            values_.insert(ea::next(values_.begin(), lastIndex_ + 1), capacity - oldCapacity, ea::nullopt);
        }
        else
        {
            // Remove elements starting from the old ones
            values_ = AsVector();
            values_.erase(values_.begin(), ea::next(values_.begin(), oldCapacity - capacity));
            lastIndex_ = values_.size() - 1;
        }
    }

    /// Set value for given frame if not set. No op otherwise.
    void Append(unsigned frame, const T& value) { SetInternal(frame, value, false); }

    /// Set value for given frame. Overwrites previous value.
    void Replace(unsigned frame, const T& value) { SetInternal(frame, value, true); }

    /// Extrapolate value into specified frame if it's uninitialized.
    /// Return true if new value was filled.
    bool ExtrapolateIfEmpty(unsigned frame)
    {
        const auto nearestValidFrame = GetNearestValidFrame(frame);
        if (!nearestValidFrame || *nearestValidFrame == frame)
            return false;

        const auto value = GetValue(*nearestValidFrame);
        URHO3D_ASSERT(value);
        Append(frame, *value);
        return true;
    }

    /// Return as normal vector of values. The last element is the latest appended value.
    Container AsVector() const
    {
        const unsigned capacity = values_.size();

        Container result(capacity);
        for (unsigned i = 0; i < capacity; ++i)
            result[i] = values_[(lastIndex_ + i + 1) % capacity];
        return result;
    }

    /// Return raw value or null if nothing is stored.
    ea::optional<T> GetValue(unsigned frame) const
    {
        const unsigned capacity = values_.size();
        const int behind = static_cast<int>(lastFrame_ - frame);
        if (behind < 0 || behind >= capacity)
            return ea::nullopt;

        const unsigned wrappedIndex = (lastIndex_ + capacity - behind) % capacity;
        return values_[wrappedIndex];
    }

    /// Return specified frame, or nearest frame with valid value prior to specified one.
    ea::optional<unsigned> GetNearestValidFrame(unsigned frame) const
    {
        const unsigned capacity = values_.size();
        const int offset = ea::max(0, static_cast<int>(lastFrame_ - frame));
        if (offset >= capacity)
            return ea::nullopt;

        for (unsigned behind = offset; behind < capacity; ++behind)
        {
            const unsigned wrappedIndex = (lastIndex_ + capacity - behind) % capacity;
            if (values_[wrappedIndex])
                return lastFrame_ - behind;
        }
        return ea::nullopt;
    }

    /// Return nearest future frame with valid value.
    /// Returns last frame if nothing else is found.
    unsigned GetNearestValidFutureFrame(unsigned frame) const
    {
        const unsigned capacity = values_.size();
        const int offset = ea::max(0, static_cast<int>(lastFrame_ - frame));

        if (offset >= 2)
        {
            for (unsigned behind = offset - 1; behind > 0; --behind)
            {
                const unsigned wrappedIndex = (lastIndex_ + capacity - behind) % capacity;
                if (values_[wrappedIndex])
                    return lastFrame_ - behind;
            }
        }
        return lastFrame_;
    }

    /// Return sampled value between specified frame and the next one.
    ea::optional<T> GetBlendedValue(unsigned frame, float blendFactor) const
    {
        const auto value1 = GetValue(frame);
        const auto value2 = GetValue(frame + 1);

        if (value1 && value2)
            return InterpolateOnNetworkClient(*value1, *value2, blendFactor);
        else if (value1)
            return value1;
        else
            return value2;
    }

    /// Return valid sampled value or closest available value.
    T GetBlendedValueOrNearest(unsigned frame, float blendFactor)
    {
        if (auto blendedValue = GetBlendedValue(frame, blendFactor))
            return *blendedValue;

        if (auto previousValidFrame = GetNearestValidFrame(frame))
            return *GetValue(*previousValidFrame);

        const unsigned futureValidFrame = GetNearestValidFutureFrame(frame);
        return *GetValue(futureValidFrame);
    }

private:
    void SetInternal(unsigned frame, const T& value, bool overwrite)
    {
        URHO3D_ASSERT(!values_.empty());

        // Initialize first frame
        if (!initialized_)
        {
            initialized_ = true;
            lastFrame_ = frame;
            lastIndex_ = 0;
            values_[0] = value;
            return;
        }

        const unsigned capacity = values_.size();
        const int offset = static_cast<int>(frame - lastFrame_);

        if (offset > 0)
        {
            // Roll buffer forward on positive delta, resetting intermediate values
            lastFrame_ += offset;
            lastIndex_ = (lastIndex_ + offset) % capacity;
            values_[lastIndex_] = value;

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
                    values_[wrappedIndex] = value;
            }
        }
    }

    bool initialized_{};
    unsigned lastFrame_{};
    unsigned lastIndex_{};
    Container values_;
};

template <class T>
T InterpolateOnNetworkClient(const T& lhs, const T& rhs, float blendFactor)
{
    return Lerp(lhs, rhs, blendFactor);
}

inline Quaternion InterpolateOnNetworkClient(const Quaternion& lhs, const Quaternion& rhs, float blendFactor)
{
    return lhs.Slerp(rhs, blendFactor);
}

}
