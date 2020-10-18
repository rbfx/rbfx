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


#include <atomic>
#include <EASTL/hash_set.h>
#include <EASTL/vector.h>

#include <Urho3D/Container/FlagSet.h>
#include <Urho3D/IO/Archive.h>
#include <Urho3D/Scene/Serializable.h>
#include <Toolbox/IO/ContentUtilities.h>
#include <Toolbox/Common/UndoStack.h>

#include "Importers/AssetImporter.h"


namespace Urho3D
{

class Flavor;

class Asset : public Serializable
{
    URHO3D_OBJECT(Asset, Serializable);
    void Inspect();
public:
    using AssetImporterMap = ea::unordered_map<SharedPtr<Flavor>, ea::vector<SharedPtr<AssetImporter>>>;

    /// Construct.
    explicit Asset(Context* context);
    /// Registers object with the engine.
    static void RegisterObject(Context* context);
    /// Returns resource name.
    const ea::string& GetName() const { return name_; }
    /// Set resource name.
    void SetName(const ea::string& name);
    /// Returns absolute path to resource file.
    const ea::string& GetResourcePath() const { return resourcePath_; }
    /// Returns true when source asset is newer than last conversion date.
    bool IsOutOfDate(Flavor* flavor) const;
    /// Returns true when this asset is a settings holder for a directory.
    bool IsMetaAsset() const { return contentType_ == CTYPE_FOLDER; }
    /// Returns content type of this asset.
    ContentType GetContentType() const { return contentType_; }
    /// Delete all byproducts of this asset.
    void ClearCache();
    /// Saves asset data to resourceName.asset file. If asset does not have any settings set - this file will be deleted
    /// if it exists.
    bool Save();
    ///
    bool Load();
    ///
    bool Serialize(Archive& archive) override;
    ///
    const AssetImporterMap& GetImporters() const { return importers_; }
    ///
    const ea::vector<SharedPtr<AssetImporter>>& GetImporters(Flavor* flavor) const;
    ///
    AssetImporter* GetImporter(Flavor* flavor, StringHash type) const;
    /// Returns true when asset importers of any flavor are being executed in worker threads.
    bool IsImporting() const { return importing_; }

protected:
    ///
    void AddFlavor(Flavor* flavor);
    ///
    void ReimportOutOfDateRecursive() const;
    ///
    void OnFlavorAdded(VariantMap& args);
    ///
    void OnFlavorRemoved(VariantMap& args);

    /// Resource name.
    ea::string name_;
    /// Full path to resource. May point to resources or cache directory.
    ea::string resourcePath_;
    /// A content type of this asset.
    ContentType contentType_ = CTYPE_BINARY;
    /// Map a flavor to a list of importers that this asset will be executing.
    AssetImporterMap importers_;
    /// Flag indicating that asset is being imported.
    std::atomic<bool> importing_{false};
    /// Flag indicating that this asset is virtual, and should not be saved.
    bool virtual_ = false;

    friend class Pipeline;
    friend class AssetImporter;
};

}
