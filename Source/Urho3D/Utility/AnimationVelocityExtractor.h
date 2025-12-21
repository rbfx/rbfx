// Copyright (c) 2022-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Utility/BaseAssetPostTransformer.h"

#include <EASTL/optional.h>

namespace Urho3D
{

struct CalculateAnimationVelocityParams
{
    StringVector targetBones_;
    float groundThreshold_{0.005f};
    float velocityThreshold_{M_LARGE_EPSILON};
    float sampleRate_{0.0f};

    void SerializeInBlock(Archive& archive);
};

/// Single task for CalculateAnimationVelocityTransformer.
struct CalculateAnimationVelocityTask
{
    SharedPtr<Model> model_;
    SharedPtr<Animation> animation_;
    CalculateAnimationVelocityParams params_;
};

/// Asset transformer that extracts velocity from movement animations.
class AnimationVelocityExtractor : public BaseAssetPostTransformer
{
    URHO3D_OBJECT(AnimationVelocityExtractor, BaseAssetPostTransformer);

public:
    static constexpr float DefaultThreshold = 0.005f;

    AnimationVelocityExtractor(Context* context);
    ~AnimationVelocityExtractor() override;
    static void RegisterObject(Context* context);

    ea::optional<Vector3> CalculateVelocity(const CalculateAnimationVelocityTask& task) const;

    /// Implement BaseAssetPostTransformer.
    /// @{
    ea::string_view GetParametersFileName() const override { return "CalculateAnimationVelocity.json"; }
    bool Execute(const AssetTransformerInput& input, AssetTransformerOutput& output,
        const AssetTransformerVector& transformers) override;
    /// @}

private:
    struct TaskDescription : public CalculateAnimationVelocityParams
    {
        ea::string model_;
        ea::string animation_;

        void SerializeInBlock(Archive& archive);
    };

    struct TransformerParams
    {
        ea::vector<TaskDescription> tasks_;
        ea::vector<TaskDescription> taskTemplates_;

        void SerializeInBlock(Archive& archive);
    };
};

} // namespace Urho3D
