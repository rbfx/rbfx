// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
