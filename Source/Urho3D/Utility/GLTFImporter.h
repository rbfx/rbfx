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

/// \file

#pragma once

#include "../Core/Object.h"
#include "../IO/Archive.h"

#include <EASTL/unique_ptr.h>

namespace Urho3D
{

struct GLTFImporterSettings
{
    ea::string assetName_{"Asset"};

    bool mirrorX_{};
    float scale_{1.0f};
    Quaternion rotation_;

    bool cleanupBoneNames_{true};
    bool cleanupRootNodes_{true};
    bool combineLODs_{true};
    bool repairLooping_{false};

    float offsetMatrixError_{ 0.00002f };
    float keyFrameTimeError_{ M_EPSILON };

    /// Settings that affect only preview scene.
    struct PreviewSettings
    {
        /// Whether to add directional light source if scene doesn't contain any light sources.
        bool addLights_{ true };

        /// Whether to add skybox background. Doesn't affect object reflections!
        bool addSkybox_{ true };
        ea::string skyboxMaterial_{ "Materials/DefaultSkybox.xml" };

        /// Whether to add cubemap for object reflections.
        bool addReflectionProbe_{ true };
        ea::string reflectionProbeCubemap_{ "Textures/DefaultSkybox.xml" };

        bool highRenderQuality_{ true };
    } preview_;
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

    /// Load GLTF files and import resources. Injects resources into resource cache!
    bool LoadFile(const ea::string& fileName,
        const ea::string& outputPath, const ea::string& resourceNamePrefix);
    /// Save generated resources.
    bool SaveResources();
    /// Return saved resources and their absolute names.
    const ResourceToFileNameMap& GetSavedResources() const;

private:
    class Impl;
    const GLTFImporterSettings settings_;
    ea::unique_ptr<Impl> impl_;
};

}
