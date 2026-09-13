// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Project/Project.h"

#include <Urho3D/Utility/AssetTransformer.h>
#include <Urho3D/Utility/GLTFImporter.h>

namespace Urho3D
{

void Assets_ModelImporter(Context* context, Project* project);

class ModelView;

/// Asset transformer that imports GLTF models.
class ModelImporter : public AssetTransformer, private GLTFImporterCallback
{
    URHO3D_OBJECT(ModelImporter, AssetTransformer);

public:
    explicit ModelImporter(Context* context);

    static void RegisterObject(Context* context);

    bool IsApplicable(const AssetTransformerInput& input) override;
    bool Execute(const AssetTransformerInput& input, AssetTransformerOutput& output,
        const AssetTransformerVector& transformers) override;

protected:
    struct ResetRootMotionInfo
    {
        float factor_{};
        Vector3 positionWeight_{};
        float rotationSwingWeight_{};
        float rotationTwistWeight_{};
        float scaleWeight_{};

        void SerializeInBlock(Archive& archive);
    };

    struct TransformerParams : public GLTFImporterSettings
    {
        bool repairLooping_{false};

        bool blenderApplyModifiers_{true};
        bool blenderDeformingBonesOnly_{true};
        bool lightmapUVGenerate_{};
        float lightmapUVTexelsPerUnit_{10.0f};
        unsigned lightmapUVChannel_{1};

        StringVector appendFiles_;
        ea::unordered_map<ea::string, ResetRootMotionInfo> resetRootMotion_;
        ea::unordered_map<ea::string, StringVariantMap> resourceMetadata_;
        ea::vector<ea::string> artificialSkinNodes_;

        void SerializeInBlock(Archive& archive);
    };

    /// Implement GLTFImporterCallback.
    /// @{
    void OnModelLoaded(ModelView& modelView) override;
    void OnAnimationLoaded(Animation& animation) override;
    ea::vector<ea::string> GetArtificialSkinNodes() override;
    /// @}

    /// Tweaks.
    /// @{
    void ResetRootMotion(Animation& animation, const ResetRootMotionInfo& info);
    void AppendResourceMetadata(ResourceWithMetadata& resource) const;
    /// @}

private:
    /// Information about GLTF file that can be imported directly.
    struct GLTFFileInfo
    {
        ea::string fileName_;
    };

    /// Handler of GLTF file. Deletes temporary file on destruction.
    using GLTFFileHandle = ea::shared_ptr<const GLTFFileInfo>;

    bool ImportGLTF(GLTFFileHandle fileHandle, const TransformerParams& params, const AssetTransformerInput& input,
        AssetTransformerOutput& output, const AssetTransformerVector& transformers);

    ea::string GetParametersFileName(const ea::string& fileName) const;
    bool LoadParameters(TransformerParams& params, const ea::string& paramsFileName) const;

    GLTFFileHandle LoadData(
        const ea::string& fileName, const TransformerParams& params, const ea::string& tempPath) const;

    GLTFFileHandle LoadDataNative(const ea::string& fileName, const TransformerParams& params) const;
    GLTFFileHandle LoadDataFromFBX(
        const ea::string& fileName, const TransformerParams& params, const ea::string& tempPath) const;
    GLTFFileHandle LoadDataFromBlend(
        const ea::string& fileName, const TransformerParams& params, const ea::string& tempPath) const;

    ToolManager* GetToolManager() const;

private:
    TransformerParams defaultParams_;

    const TransformerParams* currentParams_{};
};

} // namespace Urho3D
