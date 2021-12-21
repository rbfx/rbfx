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
    /// Whether to add directional light source if scene doesn't contain any light sources.
    bool addLights_{ true };

    /// Whether to add skybox background. Doesn't affect object reflections!
    bool addSkybox_{ true };
    ea::string skyboxMaterial_{ "Materials/Skybox.xml" };

    /// Whether to add cubemap for reflections
    bool addReflectionProbe_{ true };
    ea::string reflectionProbeCubemap_{ "Textures/Skybox.xml" };

    bool highRenderQuality_{ true };
    float offsetMatrixError_{ 0.00002f };
    float keyFrameTimeError_{ M_EPSILON };
};

URHO3D_API void SerializeValue(Archive& archive, const char* name, GLTFImporterSettings& value);

/// Utility class to load GLTF file and save it as Urho resources.
/// It may modify Context singletons, so it's better to use this utility from separate executable.
/// TODO: Remove imported resources from cache on destruction?
class URHO3D_API GLTFImporter : public Object
{
    URHO3D_OBJECT(GLTFImporter, Object);

public:
    GLTFImporter(Context* context, const GLTFImporterSettings& settings);
    ~GLTFImporter() override;

    /// Load GLTF files and import resources. Injects resources into resource cache!
    bool LoadFile(const ea::string& fileName,
        const ea::string& outputPath, const ea::string& resourceNamePrefix);
    /// Save generated resources.
    bool SaveResources();

private:
    class Impl;
    const GLTFImporterSettings settings_;
    ea::unique_ptr<Impl> impl_;
};

}
