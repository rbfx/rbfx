// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Core/Signal.h>
#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Scene/Scene.h>

namespace Urho3D
{

class BinaryFile;
class JSONFile;
class XMLFile;

/// Scene resource.
/// Can be used to load and save Scene, since Scene is not a Resource.
/// Be careful when using cached SceneResource, since the underlying Scene may be active.
class URHO3D_API SceneResource : public Resource
{
    URHO3D_OBJECT(SceneResource, Resource)

public:
    Signal<void(bool& cancelReload)> OnReloadBegin;
    Signal<void(bool success)> OnReloadEnd;

    explicit SceneResource(Context* context);
    ~SceneResource() override;

    static void RegisterObject(Context* context);

    void SetPrefab(bool value) { isPrefab_ = value; }

    void SetSaveFormatHint(InternalResourceFormat format);
    bool Save(Serializer& dest, InternalResourceFormat format, bool asPrefab = false) const;
    bool SaveFile(const FileIdentifier& fileName, InternalResourceFormat format, bool asPrefab = false) const;

    /// Implement Resource.
    /// @{
    bool BeginLoad(Deserializer& source) override;
    bool EndLoad() override;
    bool Save(Serializer& dest) const override;
    bool SaveFile(const FileIdentifier& fileName) const override;
    /// @}

    /// Return scene. It may be modified.
    Scene* GetScene() const { return scene_; }

    /// Get name of XML root element.
    static const char* GetXmlRootName();

private:
    SharedPtr<Scene> scene_;

    ea::optional<InternalResourceFormat> loadFormat_;
    ea::optional<InternalResourceFormat> saveFormat_;
    bool isPrefab_{};

    SharedPtr<BinaryFile> loadBinaryFile_;
    SharedPtr<JSONFile> loadJsonFile_;
    SharedPtr<XMLFile> loadXmlFile_;
};

} // namespace Urho3D
