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
#include "../Resource/Resource.h"
#include "../IO/Archive.h"

namespace Urho3D
{

class ParticleGraphLayer;
class XMLFile;

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
    static void RegisterObject(Context* context);

    /// Set number of layers.
    void SetNumLayers(unsigned numLayers);
    /// Get number of layers.
    unsigned GetNumLayers() const;

    /// Get layer by index.
    SharedPtr<ParticleGraphLayer> GetLayer(unsigned layerIndex) const;

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;

    /// Save resource. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Serialize from/to archive. Return true if successful.
    void SerializeInBlock(Archive& archive) override;

private:
    /// Reset to defaults.
    void ResetToDefaults();

private:
    /// Effect layers.
    ea::vector<SharedPtr<ParticleGraphLayer>> layers_;
};

}
