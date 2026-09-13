// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <EASTL/span.h>

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
    inline T& operator[](unsigned index) const { return data_[indices_[index]]; }
    T* data_;
    unsigned* indices_;
};

template <typename... Values> struct SpanVariantTuple;


} // namespace Urho3D
