// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/Hash.h"
#include "Urho3D/Math/MathDefs.h"

namespace Urho3D
{

namespace Detail
{

/// Common traits for all vectors.
template <class T, int N> struct VectorTraits
{
    /// Number of compo
    static constexpr int NumComponents = N;
    /// Underlying scalar type.
    using ScalarType = T;
};

/// Create vector from components, casted.
template <class VectorType, class T1, class T2, class T3, class T4>
constexpr VectorType CreateVectorCast(const T1& x, const T2& y, const T3& z, const T4& w)
{
    using ScalarType = typename VectorType::ScalarType;
    constexpr int NumComponents = VectorType::NumComponents;
    static_assert(NumComponents >= 2 && NumComponents <= 4, "Cannot cast to this type");

    if constexpr (NumComponents == 2)
    {
        return VectorType{
            static_cast<ScalarType>(x),
            static_cast<ScalarType>(y),
        };
    }
    else if constexpr (NumComponents == 3)
    {
        return VectorType{
            static_cast<ScalarType>(x),
            static_cast<ScalarType>(y),
            static_cast<ScalarType>(z),
        };
    }
    else if constexpr (NumComponents == 4)
    {
        return VectorType{
            static_cast<ScalarType>(x),
            static_cast<ScalarType>(y),
            static_cast<ScalarType>(z),
            static_cast<ScalarType>(w),
        };
    }
}

/// Calculate vector hash.
template <class VectorType> unsigned CalculateVectorHash(const VectorType& v)
{
    unsigned hash{};
    for (int i = 0; i < VectorType::NumComponents; ++i)
        CombineHash(hash, MakeHash(v.Data()[i]));
    return hash;
}

} // namespace Detail

} // namespace Urho3D
