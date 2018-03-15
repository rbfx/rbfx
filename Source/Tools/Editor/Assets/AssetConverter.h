//
// Copyright (c) 2008-2017 the Urho3D project.
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


namespace Urho3D
{

class AssetConversionParameters;

class AssetConverter : public Object
{
    URHO3D_OBJECT(AssetConverter, Object);
public:
    /// rulesFile is resource name or full path to xml file defining asset conversion rules.
    explicit AssetConverter(Context* context);

    /// Watch directory for changed assets and automatically convert them.
    void AddAssetDirectory(const String& path);
    /// Stop watching directory for changed assets.
    void RemoveAssetDirectory(const String& path);
    /// Returns asset cache path.
    String GetCachePath();
    /// Request checking of all assets and convert out of date assets.
    void VerifyCacheAsync();
    /// Request conversion of single asset.
    void ConvertAssetAsync(const String& resourceName);

protected:
    /// Converts asset. Blocks calling thread.
    bool ConvertAsset(const String& resourceName, const SharedPtr<XMLFile>& rules);
    /// Executes external application.
    int ExecuteConverterPOpen(const XMLElement& popen, const String& resourceName);
    /// Returns true if asset in the cache folder is missing or out of date.
    bool IsCacheOutOfDate(const String& resourceName);
    /// Return a list of converted assets in the cache.
    Vector<String> GetCacheAssets(const String& resourceName);
    /// Watches for changed files and requests asset conversion if needed.
    void DispatchChangedAssets();
    /// Inserts various variables to values specified in rules file.
    void InsertVariables(const String& resourceName, String& value);

    /// List of file watchers responsible for watching game data folders for asset changes.
    Vector<SharedPtr<FileWatcher>> watchers_;
    /// Timer used for delaying out of date asset checks.
    Timer checkTimer_;
};

}
