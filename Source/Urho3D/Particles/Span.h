//
// Copyright (c) 2021 the rbfx project.
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

#include <EASTL/span.h>

namespace Urho3D
{

template <typename T> struct ScalarSpan
{
    typedef T element_type;
    typedef ea::remove_cv_t<T> value_type;

    ScalarSpan(const ea::span<T>& data)
        : data_(data)
    {
    }

    inline T& operator[](unsigned index) { return data_.front(); }
    ea::span<T> data_;
};

template <typename T> struct SparseSpan
{
    typedef T element_type;
    typedef ea::remove_cv_t<T> value_type;

    SparseSpan() = default;
    SparseSpan(const ea::span<T>& data, const ea::span<unsigned>& indices)
        : data_(data)
        , indices_(indices)
    {
    }
    inline T& operator[](unsigned index) { return data_[indices_[index]]; }
    ea::span<T> data_;
    ea::span<unsigned> indices_;
};

} // namespace Urho3D
