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

/// Asset transformer that imports GLTF models.
class ModelImporter : public AssetTransformer
{
    URHO3D_OBJECT(ModelImporter, AssetTransformer);

public:
    explicit ModelImporter(Context* context);

    static void RegisterObject(Context* context);

    bool IsApplicable(const AssetTransformerInput& input) override;
    bool Execute(const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers) override;

private:
    bool ImportGLTF(const ea::string& fileName, const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers);
    bool ImportFBX(const ea::string& fileName, const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers);
    bool ImportBlend(const ea::string& fileName, const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers);

    ToolManager* GetToolManager() const;

    GLTFImporterSettings settings_;
};

}
