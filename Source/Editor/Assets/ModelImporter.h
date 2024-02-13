//
// Copyright (c) 2017-2020 the rbfx project.
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

    struct ModelMetadata
    {
        ea::string metadataFileName_;
        StringVector appendFiles_;
        ea::unordered_map<ea::string, ea::string> nodeRenames_;
        ea::unordered_map<ea::string, ResetRootMotionInfo> resetRootMotion_;
        ea::unordered_map<ea::string, StringVariantMap> resourceMetadata_;

        void SerializeInBlock(Archive& archive);
    };

    /// Implement GLTFImporterCallback.
    /// @{
    void OnModelLoaded(ModelView& modelView) override;
    void OnAnimationLoaded(Animation& animation) override;
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

    bool ImportGLTF(GLTFFileHandle fileHandle, const ModelMetadata& metadata, const AssetTransformerInput& input,
        AssetTransformerOutput& output, const AssetTransformerVector& transformers);

    ModelMetadata LoadMetadata(const ea::string& fileName) const;
    GLTFFileHandle LoadData(const ea::string& fileName, const ea::string& tempPath) const;

    GLTFFileHandle LoadDataNative(const ea::string& fileName) const;
    GLTFFileHandle LoadDataFromFBX(const ea::string& fileName, const ea::string& tempPath) const;
    GLTFFileHandle LoadDataFromBlend(const ea::string& fileName, const ea::string& tempPath) const;

    ToolManager* GetToolManager() const;

    GLTFImporterSettings settings_;

    bool repairLooping_{false};

    bool blenderApplyModifiers_{true};
    bool blenderDeformingBonesOnly_{true};
    bool lightmapUVGenerate_{};
    float lightmapUVTexelsPerUnit_{10.0f};
    unsigned lightmapUVChannel_{1};

    const ModelMetadata* currentMetadata_{};
};

} // namespace Urho3D
