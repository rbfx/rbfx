//
// Copyright (c) 2022-2022 the rbfx project.
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
    bool isPrefab_{};

    SharedPtr<BinaryFile> loadBinaryFile_;
    SharedPtr<JSONFile> loadJsonFile_;
    SharedPtr<XMLFile> loadXmlFile_;
};

} // namespace Urho3D
