// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
