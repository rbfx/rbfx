// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/IO/Archive.h"
#include "Urho3D/Math/Transform.h"
#include "Urho3D/Utility/AnimationMetadata.h"

#include <EASTL/unique_ptr.h>

namespace tinygltf
{
class Model;
}

namespace Urho3D
{

class Animation;
class ModelView;

struct GLTFImporterSettings
{
    ea::string assetName_{"Asset"};

    bool mirrorX_{};
    float scale_{1.0f};
    Quaternion rotation_;

    bool fadeTransparency_{false};

    bool cleanupBoneNames_{true};
    bool cleanupRootNodes_{true};
    bool combineLODs_{true};
    StringVector skipTags_;
    bool keepNamesOnMerge_{false};
    bool addEmptyNodesToSkeleton_{false};

    float offsetMatrixError_{0.00002f};
    float keyFrameTimeError_{M_EPSILON};

    ea::unordered_map<ea::string, ea::string> nodeRenames_;

    bool gpuResources_{false};

    /// Settings that affect only preview scene.
    struct PreviewSettings
    {
        /// Whether to add directional light source if scene doesn't contain any light sources.
        bool addLights_{true};

        /// Whether to add skybox background. Doesn't affect object reflections!
        bool addSkybox_{true};
        ea::string skyboxMaterial_{"Materials/DefaultSkybox.xml"};

        /// Whether to add cubemap for object reflections.
        bool addReflectionProbe_{true};
        ea::string reflectionProbeCubemap_{"Textures/DefaultSkybox.xml"};

        bool highRenderQuality_{true};
    } preview_;
};

class URHO3D_API GLTFImporterCallback
{
public:
    virtual void OnModelLoaded(ModelView& modelView) {};
    virtual void OnAnimationLoaded(Animation& animation) {};
};

URHO3D_API void SerializeValue(Archive& archive, const char* name, GLTFImporterSettings& value);

/// Utility class to load GLTF file and save it as Urho resources.
/// Temporarily loads resources into resource cache, removes them from the cache on destruction.
/// It's better to use this utility from separate executable.
class URHO3D_API GLTFImporter : public Object
{
    URHO3D_OBJECT(GLTFImporter, Object);

public:
    using ResourceToFileNameMap = ea::unordered_map<ea::string, ea::string>;

    GLTFImporter(Context* context, const GLTFImporterSettings& settings);
    ~GLTFImporter() override;

    /// Load primary GLTF file into memory without any processing.
    bool LoadFile(const ea::string& fileName);
    bool LoadFileBinary(ConstByteSpan data);
    /// Load and merge secondary GLTF file.
    /// Merge functionality is limited, unsupported content of secondary file is ignored.
    bool MergeFile(const ea::string& fileName, const ea::string& assetName);

    /// Process loaded GLTF files and import resources. Injects resources into resource cache!
    bool Process(const ea::string& outputPath, const ea::string& resourceNamePrefix, GLTFImporterCallback* callback);

    /// Save generated resources.
    bool SaveResources();
    /// Return saved resources and their absolute names.
    const ResourceToFileNameMap& GetSavedResources() const;
    /// Convert GLTF transform to the engine format.
    Transform ConvertTransform(const Transform& sourceTransform) const;

private:
    bool LoadFileInternal(const ea::function<tinygltf::Model()> getModel);

    class Impl;
    const GLTFImporterSettings settings_;

    ea::unique_ptr<tinygltf::Model> model_;
    ea::unique_ptr<Impl> impl_;

    GLTFImporterCallback defaultCallback_;
};

} // namespace Urho3D
