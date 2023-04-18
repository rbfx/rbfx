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

#include "../Container/Functors.h"

#include <EASTL/span.h>

#include <iterator>

namespace Urho3D
{

/// Helper class that transforms span into different type using unary predicate.
template <class SourceType, class DestinationType, class Function>
class TransformedSpan
{
public:
    using UndelyingSpan = ea::span<SourceType>;
    using UnderlyingIterator = typename UndelyingSpan::const_iterator;

    class Iterator
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = DestinationType;
        using difference_type = eastl_size_t;
        using pointer = const DestinationType*;
        using reference = const DestinationType&;

        Iterator() = default;
        explicit Iterator(UnderlyingIterator iter) : iter_{iter} {}

        value_type operator*() const { return Function{}(*iter_); }

        Iterator& operator++()
        {
            ++iter_;
            return *this;
        }

        Iterator operator++(int)
        {
            const Iterator result = *this;
            ++iter_;
            return result;
        }

        Iterator& operator--()
        {
            --iter_;
            return *this;
        }

        Iterator operator--(int)
        {
            const Iterator result = *this;
            --iter_;
            return result;
        }

        Iterator& operator+=(difference_type n)
        {
            iter_ += n;
            return *this;
        }

        Iterator& operator-=(difference_type n)
        {
            iter_ -= n;
            return *this;
        }

        Iterator operator+(difference_type n) const
        {
            Iterator result = *this;
            result.iter_ += n;
            return result;
        }

        Iterator operator-(difference_type n) const
        {
            Iterator result = *this;
            result.iter_ += n;
            return result;
        }

        bool operator==(const Iterator& rhs) const { return iter_ == rhs.iter_; }
        bool operator!=(const Iterator& rhs) const { return iter_ != rhs.iter_; }
        bool operator<(const Iterator& rhs) const { return iter_ < rhs.iter_; }
        bool operator>(const Iterator& rhs) const { return iter_ > rhs.iter_; }
        bool operator<=(const Iterator& rhs) const { return iter_ <= rhs.iter_; }
        bool operator>=(const Iterator& rhs) const { return iter_ >= rhs.iter_; }

        difference_type operator-(const Iterator& rhs) const { return static_cast<difference_type>(iter_ - rhs.iter_); }

    private:
        UnderlyingIterator iter_{};
    };

    TransformedSpan() = default;
    explicit TransformedSpan(UndelyingSpan span) : span_(span) {}

    Iterator Begin() const { return Iterator{span_.begin()}; }
    Iterator End() const { return Iterator{span_.end()}; }
    eastl_size_t Size() const { return span_.size(); }

private:
    UndelyingSpan span_;
};

/// Perform static cast on the span.
template <class T, class SourceType>
auto StaticCastSpan(ea::span<SourceType> value)
{
    return TransformedSpan<SourceType, T, StaticCaster<T>>{value};
}

template <class SourceType, class DestinationType, class Function>
auto begin(const TransformedSpan<SourceType, DestinationType, Function>& value) { return value.Begin(); }

template <class SourceType, class DestinationType, class Function>
auto end(const TransformedSpan<SourceType, DestinationType, Function>& value) { return value.End(); }

}
