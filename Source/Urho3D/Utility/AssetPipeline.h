// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Utility/AssetTransformer.h"

namespace Urho3D
{

/// Resource containing an array of AssetTransformer-s.
class URHO3D_API AssetPipeline : public Resource
{
    URHO3D_OBJECT(AssetPipeline, Resource);

public:
    explicit AssetPipeline(Context* context);
    static bool CheckExtension(const ea::string& fileName);

    void AddTransformer(AssetTransformer* transformer);
    void RemoveTransformer(AssetTransformer* transformer);
    void ReorderTransformer(AssetTransformer* transformer, unsigned index);

    /// Implement Resource.
    /// @{
    void SerializeInBlock(Archive& archive) override;
    bool BeginLoad(Deserializer& source) override;
    bool Save(Serializer& dest) const override;
    /// @}

    const ea::vector<SharedPtr<AssetTransformer>>& GetTransformers() const { return transformers_; }
    const ea::vector<AssetTransformerDependency>& GetDependencies() const { return dependencies_; }

private:
    ea::vector<SharedPtr<AssetTransformer>> transformers_;
    ea::vector<AssetTransformerDependency> dependencies_;
};

}
