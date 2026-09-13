// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "ParticleGraphEmitter.h"
#include "ParticleGraphPin.h"

namespace Urho3D
{

struct ParticleGraphPinRef;

struct UpdateContext
{
    /// Current frame time step.
    float timeStep_{};
    /// Time since emitter start.
    float time_{};
    ea::span<unsigned> indices_;
    ea::span<uint8_t> attributes_;
    ea::span<uint8_t> tempBuffer_;
    ParticleGraphLayerInstance* layer_;

    template <typename ValueType> SparseSpan<ValueType> GetSpan(const ParticleGraphPinRef& pin) const;
};

template <typename ValueType> SparseSpan<ValueType> UpdateContext::GetSpan(const ParticleGraphPinRef& pin) const
{
    switch (pin.type_)
    {
    case ParticleGraphContainerType::Span: return layer_->GetSpan<ValueType>(pin.index_);
    case ParticleGraphContainerType::Scalar: return layer_->GetScalar<ValueType>(pin.index_);
    case ParticleGraphContainerType::Sparse: return layer_->GetSparse<ValueType>(pin.index_, indices_);
    default: assert(!"Invalid pin container type"); return layer_->GetSparse<ValueType>(pin.index_, indices_);
    }
}

} // namespace Urho3D
