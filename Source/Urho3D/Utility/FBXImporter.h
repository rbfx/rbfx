// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#pragma once

#include "Urho3D/Utility/GLTFImporter.h"

namespace Urho3D
{

using FBXImporterSettings = GLTFImporterSettings;
using FBXImporterCallback = GLTFImporterCallback;

/// Utility class to load FBX files through ufbx and cook them directly into engine resources.
class URHO3D_API FBXImporter : public Object
{
    URHO3D_OBJECT(FBXImporter, Object);

public:
    using ResourceToFileNameMap = GLTFImporter::ResourceToFileNameMap;

    FBXImporter(Context* context, const FBXImporterSettings& settings);
    ~FBXImporter() override;

    /// Load primary FBX file into memory without any processing.
    bool LoadFile(const ea::string& fileName);
    /// Load primary FBX file from memory without any processing.
    bool LoadFileBinary(ConstByteSpan data);
    /// Load and merge secondary FBX file.
    bool MergeFile(const ea::string& fileName, const ea::string& assetName);
    /// Return whether the source contains animations but no renderable models.
    bool IsAnimationOnly(unsigned sourceIndex = 0) const;
    /// Return number of animation stacks in the source.
    unsigned GetNumAnimations(unsigned sourceIndex = 0) const;

    /// Prepare imported resources without creating GPU resources or scene objects. May be called from a worker thread.
    bool BeginProcess(
        const ea::string& outputPath, const ea::string& resourceNamePrefix, FBXImporterCallback* callback);
    /// Finish resource creation and create the preview prefab. Must be called from the main thread.
    bool EndProcess();
    /// Process loaded FBX files and import resources. Injects resources into resource cache.
    bool Process(const ea::string& outputPath, const ea::string& resourceNamePrefix, FBXImporterCallback* callback);
    /// Save generated resources.
    bool SaveResources();
    /// Return saved resources and their absolute names.
    const ResourceToFileNameMap& GetSavedResources() const;
    /// Convert source transform to the engine format.
    Transform ConvertTransform(const Transform& sourceTransform) const;

private:
    class Impl;
    const FBXImporterSettings settings_;
    ea::unique_ptr<Impl> impl_;
    FBXImporterCallback defaultCallback_;
};

} // namespace Urho3D
