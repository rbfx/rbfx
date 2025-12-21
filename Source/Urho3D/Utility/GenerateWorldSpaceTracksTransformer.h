// Copyright (c) 2022-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Utility/BaseAssetPostTransformer.h"

namespace Urho3D
{

struct GenerateWorldSpaceTracksParams
{
    bool fillRotations_{true};
    bool deltaRotation_{true};
    float sampleRate_{0.0f};

    ea::string targetTrackNameFormat_{"{}_Target"};
    ea::string bendTargetTrackNameFormat_{"{}_BendTarget"};

    ea::unordered_set<ea::string> bones_;
    ea::unordered_map<ea::string, Vector3> bendTargetOffsets_;

    void SerializeInBlock(Archive& archive);
};

/// Single task for GenerateWorldSpaceTracksTransformer.
struct GenerateWorldSpaceTracksTask
{
    SharedPtr<Model> model_;
    SharedPtr<Animation> sourceAnimation_;
    SharedPtr<Animation> targetAnimation_;
    GenerateWorldSpaceTracksParams params_;
};

/// Asset transformer that generates world-space tracks for animation. Useful for IK animation.
class GenerateWorldSpaceTracksTransformer : public BaseAssetPostTransformer
{
    URHO3D_OBJECT(GenerateWorldSpaceTracksTransformer, BaseAssetPostTransformer);

public:
    GenerateWorldSpaceTracksTransformer(Context* context);
    ~GenerateWorldSpaceTracksTransformer() override;
    static void RegisterObject(Context* context);

    void GenerateTracks(const GenerateWorldSpaceTracksTask& task) const;

    /// Implement BaseAssetPostTransformer.
    /// @{
    ea::string_view GetParametersFileName() const override { return "GenerateWorldSpaceTracks.json"; }
    bool Execute(const AssetTransformerInput& input, AssetTransformerOutput& output,
        const AssetTransformerVector& transformers) override;
    /// @}

private:
    struct TaskDescription : public GenerateWorldSpaceTracksParams
    {
        ea::string model_;
        ea::string sourceAnimation_;
        ea::string targetAnimation_;

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
