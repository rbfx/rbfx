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

#include "ParticleGraphEmitter.h"
#include "ParticleGraphPin.h"

namespace Urho3D
{

struct UpdateContext
{
    float timeStep_{};
    ea::span<unsigned> indices_;
    ea::span<uint8_t> attributes_;
    ea::span<uint8_t> tempBuffer_;
    ParticleGraphLayerInstance* layer_;

    template <typename ValueType> ea::span<ValueType> GetSpan(const ParticleGraphPinRef& pin);
    template <typename ValueType> ScalarSpan<ValueType> GetScalar(const ParticleGraphPinRef& pin);
    template <typename ValueType> SparseSpan<ValueType> GetSparse(const ParticleGraphPinRef& pin);
};

class URHO3D_API ParticleGraphNodeInstance
{
public:
    /// Construct.
    ParticleGraphNodeInstance();

    /// Destruct.
    virtual ~ParticleGraphNodeInstance();

    /// Update.
    virtual void Update(UpdateContext& context) = 0;
};

} // namespace Urho3D
