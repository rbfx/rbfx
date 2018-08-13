//
// Copyright (c) 2018 Rokas Kupstys
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


#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/FileWatcher.h>
#include <Urho3D/Resource/XMLFile.h>

#include "ImportAsset.h"


namespace Urho3D
{

class AssetConversionParameters;

class AssetConverter : public Object
{
    URHO3D_OBJECT(AssetConverter, Object);
public:
    /// Construct.
    explicit AssetConverter(Context* context);
    /// Destruct.
    ~AssetConverter() override;

    /// Set cache path. Converted assets will be placed there.
    void SetCachePath(const String& cachePath);
    /// Returns asset cache path.
    String GetCachePath() const;
    /// Watch directory for changed assets and automatically convert them.
    void AddAssetDirectory(const String& path);
    /// Stop watching directory for changed assets.
    void RemoveAssetDirectory(const String& path);
    /// Request checking of all assets and convert out of date assets.
    void VerifyCacheAsync();
    /// Request conversion of single asset.
    void ConvertAssetAsync(const String& resourceName);

protected:
    /// Converts asset. Blocks calling thread.
    bool ConvertAsset(const String& resourceName);
    /// Returns true if asset in the cache folder is missing or out of date.
    bool IsCacheOutOfDate(const String& resourceName);
    /// Return a list of converted assets in the cache.
    Vector<String> GetCacheAssets(const String& resourceName);
    /// Watches for changed files and requests asset conversion if needed.
    void DispatchChangedAssets();
    /// Handle console commands.
    void OnConsoleCommand(VariantMap& args);

    /// List of file watchers responsible for watching game data folders for asset changes.
    Vector<SharedPtr<FileWatcher>> watchers_;
    /// Timer used for delaying out of date asset checks.
    Timer checkTimer_;
    /// Absolute path to asset cache.
    String cachePath_;
    /// Registered asset importers.
    Vector<SharedPtr<ImportAsset>> assetImporters_;
};

}
