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

#include "../ParticleGraphNode.h"
#include "../ParticleGraphNodeInstance.h"
#include "../ParticleGraphPin.h"

namespace Urho3D
{
class ParticleGraphSystem;

namespace ParticleGraphNodes
{

/// Operation on attribute
class URHO3D_API Random : public ParticleGraphNode
{
    URHO3D_OBJECT(Random, ParticleGraphNode)
public:
    /// Construct.
    explicit Random(Context* context);
    /// Register particle node factory.
    static void RegisterObject(ParticleGraphSystem* context);

protected:
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(Random* node);
        void Update(UpdateContext& context) override;

    protected:
        Random* node_;
    };

public:
    /// Get number of pins.
    unsigned GetNumPins() const override { return static_cast<unsigned>(ea::size(pins_)); }

    /// Get pin by index.
    ParticleGraphPin& GetPin(unsigned index) override { return pins_[index]; }

    /// Evaluate size required to place new node instance.
    unsigned EvaluateInstanceSize() const override { return sizeof(Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override
    {
        return new (ptr) Instance(this);
    }

    const Variant& GetMin() const { return min_; }
    void SetMin(const Variant& val) { min_ = val; }
    const Variant& GetMax() const { return max_; }
    void SetMax(const Variant& val) { max_ = val; }

protected:
    /// Pins
    ParticleGraphPin pins_[1];

    /// Min value.
    Variant min_{0.0f};
    /// Max value.
    Variant max_{1.0f};
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
