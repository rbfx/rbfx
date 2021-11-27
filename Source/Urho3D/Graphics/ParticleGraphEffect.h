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

#include "../Resource/Resource.h"

namespace Urho3D
{
enum class ParticleGraphContainerType
{
    Span,
    Sparse,
    Scalar,
    Auto
};

enum class ParticleGraphValueType
{
    Auto,
    Float
};

struct ParticleGraphNodePin
{
    /// Container type: span, sparse or scalar.
    ParticleGraphContainerType containerType_{ParticleGraphContainerType::Auto};
    /// Value type (float, vector3, etc).
    ParticleGraphValueType valueType_ { ParticleGraphValueType::Auto };
    /// Buffer offset.
    size_t offset_{};
    /// Buffer size.
    size_t size_{};
};

class ParticleGraphNodeInstance;

class URHO3D_API ParticleGraphNode : public RefCounted
{
public:
    /// Construct.
    explicit ParticleGraphNode();

    /// Destruct.
    ~ParticleGraphNode() override;

    /// Get number of pins.
    virtual unsigned NumPins() const = 0;

    /// Get pin by index.
    virtual ParticleGraphNodePin& GetPin(unsigned index) = 0;

    /// Evaluate size required to place new node instance.
    virtual unsigned EvalueInstanceSize() = 0;

    /// Place new instance at the provided address.
    virtual ParticleGraphNodeInstance* CreateInstanceAt(void* ptr) = 0;
};

/// %Particle graph effect definition.
class URHO3D_API ParticleGraphEffect : public Resource
{
    URHO3D_OBJECT(ParticleGraphEffect, Resource);

public:
    /// Construct.
    explicit ParticleGraphEffect(Context* context);
    /// Destruct.
    ~ParticleGraphEffect() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);
};

}
