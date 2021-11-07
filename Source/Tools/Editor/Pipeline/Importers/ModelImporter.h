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

#include "Pipeline/Importers/AssetImporter.h"

namespace Urho3D
{

class ModelImporter : public AssetImporter
{
    URHO3D_OBJECT(ModelImporter, AssetImporter);
public:
    explicit ModelImporter(Context* context);
    /// Register object with the engine.
    static void RegisterObject(Context* context);
    ///
    bool Accepts(const ea::string& path) const override;
    ///
    bool Execute(Asset* input, const ea::string& outputPath) override;

protected:
    bool ImportAssetToFolder(Asset* inputAsset,
        const ea::string& outputPath, const ea::string& outputResourceNamePrefix, ea::string& commandOutput);
    bool ExecuteAssimp(const ea::string& inputFileName,
        const ea::string& outputPath, const ea::string& outputResourceNamePrefix, ea::string& commandOutput);
    bool ExecuteImportGLTF(const ea::string& inputFileName,
        const ea::string& outputPath, const ea::string& outputResourceNamePrefix, ea::string& commandOutput);
    bool ExecuteImportFBX(const ea::string& inputFileName,
        const ea::string& outputPath, const ea::string& outputResourceNamePrefix, ea::string& commandOutput);
    bool ExecuteImportBlend(const ea::string& inputFileName,
        const ea::string& outputPath, const ea::string& outputResourceNamePrefix, ea::string& commandOutput);
    ea::string GenerateTemporaryPath() const;

    ///
    bool outputAnimations_ = true;
    ///
    bool outputMaterials_ = true;
    ///
    bool outputMaterialTextures_ = true;
    ///
    bool useMaterialDiffuse_ = true;
    ///
    bool fixInFacingNormals_ = true;
    ///
    int maxBones_ = 64;
    ///
    int animationTick_ = 4800;
    ///
    bool emissiveIsAmbientOcclusion_ = false;
    ///
    bool noFbxPivot_ = false;
};

}
