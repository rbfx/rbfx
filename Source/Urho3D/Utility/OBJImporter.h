// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

/// \file

#pragma once

#include <Urho3D/Core/Object.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Material.h>

namespace Urho3D
{
class ModelView;

class URHO3D_API ObjImporterCallback
{
public:
    virtual void OnModelLoaded(ModelView& modelView){};
};

/// Utility class to load OBJ file and save it as Urho resources.
/// Temporarily loads resources into resource cache, removes them from the cache on destruction.
/// It's better to use this utility from separate executable.
class URHO3D_API OBJImporter : public Object
{
    URHO3D_OBJECT(OBJImporter, Object);

public:
    OBJImporter(Context* context);
    ~OBJImporter() override;

    /// Load OBJ file into memory.
    bool LoadFileToResourceCache(const ea::string& fileName, const ea::string& resourcePrefix);
    /// Save generated resources.
    bool SaveResources(const ea::string& folderPrefix);

private:
    bool SaveResource(Resource* resource, const ea::string& folderPrefix);

    ea::vector<SharedPtr<Material>> materialsToSave_;
    ea::vector<SharedPtr<Model>> modelsToSave_;
};

} // namespace Urho3D
