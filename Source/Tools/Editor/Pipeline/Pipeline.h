//
// Copyright (c) 2017-2019 Rokas Kupstys.
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


#include <Urho3D/IO/FileWatcher.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Serializable.h>
#include <Toolbox/IO/ContentUtilities.h>

#include "Converter.h"


namespace Urho3D
{

class Converter;
class Pipeline;

struct ResourcePathLock
{
    explicit ResourcePathLock(Pipeline* pipeline, const String& resourcePath);

    ~ResourcePathLock();

    operator bool() { return true; }

    ResourcePathLock(ResourcePathLock&& other) noexcept
    {
        Swap(pipeline_, other.pipeline_);
        Swap(resourcePath_, other.resourcePath_);
    }

protected:
    ///
    Pipeline* pipeline_{};
    ///
    String resourcePath_{};
};

class Pipeline : public Serializable
{
    URHO3D_OBJECT(Pipeline, Serializable);
public:
    ///
    explicit Pipeline(Context* context);
    ///
    ~Pipeline() override;
    ///
    bool LoadJSON(const JSONValue& source) override;
    /// Watch directory for changed assets and automatically convert them.
    void EnableWatcher();
    /// Execute asset converters specified in `converterKinds` in worker threads. Returns immediately.
    void BuildCache(ConverterKinds converterKinds, const StringVector& files={}, bool complete=false);
    /// Returns true when assets in the cache are older than source asset.
    bool IsCacheOutOfDate(const String& resourceName) const;
    /// Remove any cached assets belonging to specified resource.
    void ClearCache(const String& resourceName);
    /// Register converted asset with the pipeline.
    void AddCacheEntry(const String& resourceName, const String& cacheResourceName);
    /// Register converted asset with the pipeline.
    void AddCacheEntry(const String& resourceName, const StringVector& cacheResourceNames);
    /// Acquire lock on resource path. Returns object whose lifetime is controls lifetime of the lock. Subsequent calls
    /// when same `resourcePath` is specified will block until result of this function is destroyed.
    /// This "lock" is used to prevent multiple pipeline converters from writing to same folder at the same time. Reason
    /// is that converter process can be anything and it likely does not output any information of written files.
    /// Pipeline needs to track converted files however and this is done by diffing file trees before and after conversion.
    /// Should be called from workers only.
    ResourcePathLock LockResourcePath(const String& resourcePath);
    ///
    void SetSkipUpToDateAssets(bool skip) { skipUpToDateAssets_ = skip; }
    ///
    void Reschedule(const String& resourceName);
    ///
    void CreatePaksAsync();

protected:
    friend class ResourcePathLock;
    friend class Project;

    /// Watches for changed files and requests asset conversion if needed.
    void DispatchChangedAssets();
    ///
    void SaveCacheInfo();
    ///
    void StartWorkItems(ConverterKinds converterKinds, const StringVector& resourcePaths);
    ///
    void HandleEndFrame(StringHash, VariantMap&);

    struct CacheEntry
    {
        /// Modification time of source file.
        unsigned mtime_;
        /// List of files that
        HashSet<String> files_;
    };

    /// Collection of top level converters defined in pipeline.
    Vector<SharedPtr<Converter>> converters_;
    /// List of file watchers responsible for watching game data folders for asset changes.
    FileWatcher watcher_;
    ///
    HashMap<String, CacheEntry> cacheInfo_;
    ///
    Mutex lock_{};
    ///
    StringVector lockedPaths_{};
    ///
    std::atomic<bool> cacheInfoOutOfDate_{false};
    ///
    bool skipUpToDateAssets_ = false;
    ///
    StringVector reschedule_;
};

}
