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

#include <EASTL/span.h>
#include <assert.h>

namespace Urho3D
{

struct UpdateContext;
struct ParticleGraphPinRef;

enum class ParticleGraphContainerType
{
    Span,
    Sparse,
    Scalar,
    Auto
};

template <typename T> struct ScalarSpan
{
    typedef T element_type;
    typedef ea::remove_cv_t<T> value_type;

    ScalarSpan(const ea::span<T>& data)
        : data_(data.data())
    {
    }
    ScalarSpan(T* data)
        : data_(data)
    {
    }
    inline T& operator[](unsigned index) { return *data_; }
    T* data_;
};

template <typename T> struct SparseSpan
{
    typedef T element_type;
    typedef ea::remove_cv_t<T> value_type;

    SparseSpan() = default;
    SparseSpan(const ea::span<T>& data, const ea::span<unsigned>& indices)
        : data_(data.data())
        , indices_(indices.data())
    {
    }
    SparseSpan(T* data, unsigned* indices)
        : data_(data)
        , indices_(indices)
    {
    }
    inline T& operator[](unsigned index) { return data_[indices_[index]]; }
    T* data_;
    unsigned* indices_;
};

template <typename T> struct SpanVariant
{
    typedef T element_type;
    typedef ea::remove_cv_t<T> value_type;

    SpanVariant() = default;

    SpanVariant(ParticleGraphContainerType type, T* data, unsigned* indices)
        : type_(type)
        , data_(data)
        , indices_(indices)
    {
    }

    SpanVariant(UpdateContext& context, ParticleGraphPinRef& pinRef);

    inline T& operator[](unsigned index)
    {
        switch (type_)
        {
        case ParticleGraphContainerType::Span: return data_[index];
        case ParticleGraphContainerType::Sparse: return data_[indices_[index]];
        case ParticleGraphContainerType::Scalar: return *data_;
        default: return *data_;
        }
    }

    T* GetSpan()
    {
        assert(type_ == ParticleGraphContainerType::Span);
        return data_;
    }

    ScalarSpan<T> GetScalar()
    {
        assert(type_ == ParticleGraphContainerType::Scalar);
        return ScalarSpan<T>(data_);
    }

    SparseSpan<T> GetSparse()
    {
        assert(type_ == ParticleGraphContainerType::Sparse);
        return SparseSpan<T>(data_, indices_);
    }

    ParticleGraphContainerType type_{ParticleGraphContainerType::Scalar};
    T* data_;
    unsigned* indices_;
};

template <typename... Values> struct SpanVariantTuple;


} // namespace Urho3D
