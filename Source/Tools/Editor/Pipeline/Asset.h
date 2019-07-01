//
// Copyright (c) 2017-2019 the rbfx project.
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


#include <atomic>
#include <EASTL/hash_set.h>
#include <EASTL/vector.h>

#include <Urho3D/Container/FlagSet.h>
#include <Urho3D/Scene/Serializable.h>
#include <Inspector/ModelInspector.h>
#include <Inspector/MaterialInspector.h>

#include "Importers/AssetImporter.h"


namespace Urho3D
{

class Asset : public Serializable, public IInspectorProvider
{
    URHO3D_OBJECT(Asset, Serializable);
public:
    using AssetImporterMap = ea::unordered_map<ea::string, ea::vector<SharedPtr<AssetImporter>>>;

    ///
    explicit Asset(Context* context);
    ///
    static void RegisterObject(Context* context);
    /// Returns resource name.
    const ea::string& GetName() const { return name_; }
    ///
    const ea::string& GetResourcePath() const { return resourcePath_; }
    /// Returns true when source asset is newer than last conversion date.
    bool IsOutOfDate() const;
    /// Returns true when this asset is a settings holder for a directory.
    bool IsMetaAsset() const { return name_.ends_with("/"); }
    /// Returns content type of this asset.
    ContentType GetContentType() const { return contentType_; }
    /// Delete all byproducts of this asset.
    void ClearCache();
    ///
    void RenderInspector(const char* filter) override;
    /// Saves asset data to resourceName.asset file. If asset does not have any settings set - this file will be deleted
    /// if it exists.
    bool Save();
    ///
    bool Load();
    ///
    bool SaveJSON(JSONValue& dest) const override;
    ///
    bool LoadJSON(const JSONValue& source) override;
    ///
    const AssetImporterMap& GetImporters() { return importers_; }

protected:
    /// Set resource name. Also loads resource configuration. Used only by Pipeline class.
    void Initialize(const ea::string& resourceName);
    ///
    void AddFlavor(const ea::string& name);
    ///
    void RemoveFlavor(const ea::string& name);
    ///
    void RenameFlavor(const ea::string& oldName, const ea::string& newName);
    ///
    void UpdateExtraInspectors(const ea::string& resourceName);

    /// Resource name.
    ea::string name_;
    /// Full path to resource. May point to resources or cache directory.
    ea::string resourcePath_;
    /// A content type of this asset.
    ContentType contentType_ = CTYPE_UNKNOWN;
    /// Map a flavor to a list of importers that this asset will be executing.
    AssetImporterMap importers_;
    /// Flag indicating that asset is being imported.
    std::atomic<bool> importing_{false};
    ///
    ea::unordered_map<ea::string, SharedPtr<ResourceInspector>> extraInspectors_{};
    ///
    WeakPtr<ResourceInspector> currentExtraInspectorProvider_{};

    friend class Pipeline;
    friend class AssetImporter;
};

}
