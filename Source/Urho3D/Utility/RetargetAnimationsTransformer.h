// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Utility/BaseAssetPostTransformer.h"

namespace Urho3D
{

/// Single task for RetargetAnimationsTransformer.
struct RetargetAnimationTask
{
    SharedPtr<Model> sourceModel_;
    SharedPtr<Animation> sourceAnimation_;
    SharedPtr<Model> targetModel_;
    ea::string targetAnimationName_;
    ea::unordered_map<ea::string, ea::string> sourceToTargetBones_;
    ea::unordered_map<ea::string, ea::string> targetToSourceBones_;
    ea::vector<ea::vector<ea::string>> ikChains_;
};

/// Asset transformer that re-targets animation from one model to another.
class URHO3D_API RetargetAnimationsTransformer : public BaseAssetPostTransformer
{
    URHO3D_OBJECT(RetargetAnimationsTransformer, BaseAssetPostTransformer);

public:
    explicit RetargetAnimationsTransformer(Context* context);

    static void RegisterObject(Context* context);

    SharedPtr<Animation> RetargetAnimation(const RetargetAnimationTask& task) const;

    /// Implement BaseAssetPostTransformer.
    /// @{
    ea::string_view GetParametersFileName() const override { return "RetargetAnimations.json"; }
    bool Execute(const AssetTransformerInput& input, AssetTransformerOutput& output,
        const AssetTransformerVector& transformers) override;
    /// @}

private:
    struct TaskDescription
    {
        ea::string sourceModel_;
        ea::string sourceAnimation_;
        ea::string targetModel_;
        ea::string targetAnimation_;
        ea::unordered_map<ea::string, ea::string> boneMapping_;
        ea::vector<ea::vector<ea::string>> ikChains_;

        void SerializeInBlock(Archive& archive);
    };

    struct TransformerParams
    {
        ea::vector<TaskDescription> tasks_;

        void SerializeInBlock(Archive& archive);
    };
};

} // namespace Urho3D
