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

#include "../Math/Rect.h"
#include "../Math/Vector2.h"

#include <EASTL/vector.h>

namespace Urho3D
{

/// 2D indexing utilities for row-major 2D array.
class ArrayDimensions2D
{
public:
    /// Construct default.
    ArrayDimensions2D() = default;
    /// Construct with given size.
    ArrayDimensions2D(int width, int height) : width_(width), height_(height) {}
    /// Construct with given size as vector (implicitly convertible).
    ArrayDimensions2D(const IntVector2& size) : width_(size.x_), height_(size.y_) {}

    /// Return width.
    int GetWidth() const { return width_; }
    /// Return height.
    int GetHeight() const { return height_; }
    /// Return array size as int vector.
    IntVector2 GetSize() const { return { width_, height_ }; }
    /// Return array dimensions as IntRect.
    IntRect GetRect() const { return { IntVector2::ZERO, GetSize() }; }
    /// Return total number of elements.
    unsigned GetCapacity() const { return static_cast<unsigned>(width_ * height_); }

    /// Return whether the index is contained in the array.
    bool Contains(const IntVector2& index) const
    {
        return index.x_ >= 0 && index.y_ >= 0 && index.x_ < width_ && index.y_ < height_;
    }

    /// Return index clamped to array boundaries.
    IntVector2 ClampIndex(const IntVector2& index) const
    {
        return VectorClamp(index, IntVector2::ZERO, GetSize() - IntVector2::ONE);
    }

    /// Return index wrapped to array boundaries.
    IntVector2 WrapIndex(const IntVector2& index) const
    {
        return { AbsMod(index.x_, width_), AbsMod(index.y_, height_) };
    }

    /// Convert 2D array index to linear array index.
    unsigned IndexToOffset(const IntVector2& index) const
    {
        assert(Contains(index));
        return static_cast<unsigned>(index.y_ * width_ + index.x_);
    }

    /// Convert linear array index to 2D array index.
    IntVector2 OffsetToIndex(unsigned index) const
    {
        assert(index < GetCapacity());
        return { static_cast<int>(index % width_), static_cast<int>(index / width_) };
    }

protected:
    /// Set dimensions.
    void SetSize(int width, int height)
    {
        width_ = width;
        height_ = height;
    }

    /// Swap with other array dimensions.
    void Swap(ArrayDimensions2D& other)
    {
        ea::swap(width_, other.width_);
        ea::swap(height_, other.height_);
    }

private:
    /// Array width.
    int width_{};
    /// Array height.
    int height_{};
};

/// 2D array template.
template <class T, class Container = ea::vector<T>>
class Array2D : public ArrayDimensions2D
{
public:
    /// Construct default.
    Array2D() = default;

    /// Construct with dimensions and default value.
    explicit Array2D(const ArrayDimensions2D& dim)
        : ArrayDimensions2D(dim)
        , data_(GetCapacity())
    {}

    /// Construct with dimensions and initial value.
    explicit Array2D(const ArrayDimensions2D& dim, const T& value)
        : ArrayDimensions2D(dim)
        , data_(GetCapacity(), value)
    {}

    /// Construct with given size and default value.
    Array2D(int width, int height)
        : ArrayDimensions2D(width, height)
        , data_(GetCapacity())
    {}

    /// Construct with given size and initial value.
    Array2D(int width, int height, const T& value)
        : ArrayDimensions2D(width, height)
        , data_(GetCapacity(), value)
    {}

    /// Reset array to empty.
    void Reset()
    {
        Reset(0, 0);
    }

    /// Resize array. All elements are reset to default value.
    void Reset(const ArrayDimensions2D& dim)
    {
        Reset(dim.GetWidth(), dim.GetHeight());
    }

    /// Resize array. All elements are reset to specified default value.
    void Reset(const ArrayDimensions2D& dim, const T& value)
    {
        Reset(dim.GetWidth(), dim.GetHeight(), value);
    }

    /// Resize array. All elements are reset to default value.
    void Reset(int width, int height)
    {
        ArrayDimensions2D::SetSize(width, height);
        data_.clear();
        data_.resize(GetCapacity());
    }

    /// Resize array. All elements are reset to specified default value.
    void Reset(int width, int height, const T& value)
    {
        ArrayDimensions2D::SetSize(width, height);
        data_.clear();
        data_.resize(GetCapacity(), value);
    }

    /// Fill array with given value.
    void Fill(const T& value)
    {
        for (T& elem : data_)
            elem = value;
    }

    /// Return array dimensions.
    const ArrayDimensions2D& Dim() const { return *this; }
    /// Return underlying container.
    Container& GetContainer() { return data_; }
    /// Return constant underlying container.
    const Container& GetContainer() const { return data_; }

    /// Return element by index.
    T& Get(const IntVector2& index) { return data_[IndexToOffset(index)]; }
    /// Return element by index or null if out of bounds.
    T* GetOptional(const IntVector2& index) { return Contains(index) ? &data_[IndexToOffset(index)] : nullptr; }
    /// Return element by wrapped index.
    T& GetWrapped(const IntVector2& index) { return data_[IndexToOffset(WrapIndex(index))]; }
    /// Return element by clamped index.
    T& GetClamped(const IntVector2& index) { return data_[IndexToOffset(ClampIndex(index))]; }
    /// Return constant element by index.
    const T& Get(const IntVector2& index) const { return data_[IndexToOffset(index)]; }
    /// Return constant element by index or null if out of bounds.
    const T* GetOptional(const IntVector2& index) const { return Contains(index) ? &data_[IndexToOffset(index)] : nullptr; }
    /// Return constant element by wrapped index.
    const T& GetWrapped(const IntVector2& index) const { return data_[IndexToOffset(WrapIndex(index))]; }
    /// Return constant element by clamped index.
    const T& GetClamped(const IntVector2& index) const { return data_[IndexToOffset(ClampIndex(index))]; }

    /// Return element by index.
    T& operator [](const IntVector2& index) { return Get(index); }
    /// Return constant element by index.
    const T& operator [](const IntVector2& index) const { return Get(index); }

    /// Swap with other array.
    void Swap(Array2D<T>& other)
    {
        ArrayDimensions2D::Swap(other);
        ea::swap(data_, other.data_);
    }

    /// Return array begin.
    auto Begin() { return data_.begin(); }
    /// Return array end.
    auto End() { return data_.end(); }
    /// Return array begin (const).
    auto Begin() const { return data_.cbegin(); }
    /// Return array end (const).
    auto End() const { return data_.cend(); }

private:
    /// Array data.
    Container data_;
};

template <class T> auto begin(Array2D<T>& array) { return array.Begin(); }
template <class T> auto end(Array2D<T>& array) { return array.End(); }
template <class T> auto begin(const Array2D<T>& array) { return array.Begin(); }
template <class T> auto end(const Array2D<T>& array) { return array.End(); }

template <class T> void swap(Array2D<T>& lhs, Array2D<T>& rhs) { lhs.Swap(rhs); }

}
