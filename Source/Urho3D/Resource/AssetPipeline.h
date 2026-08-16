#pragma once

#include <EASTL/functional.h>
#include <EASTL/map.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

#include "../Core/Variant.h"
#include "../Resource/JSONValue.h"
#include "../Resource/Resource.h"

namespace Urho3D
{

/// Versioned, deterministic settings used to import one source asset.
struct URHO3D_API AssetImportSettings
{
    unsigned version{1};
    ea::string importer{"Generic"};
    StringVariantMap properties;

    JSONValue ToJSON() const;
    bool FromJSON(const JSONValue& value, ea::string* error = nullptr);
    unsigned CalculateHash() const;
};

/// Resource wrapper for versioned asset import settings files.
class URHO3D_API AssetImportSettingsResource : public Resource
{
    URHO3D_OBJECT(AssetImportSettingsResource, Resource)

public:
    explicit AssetImportSettingsResource(Context* context);

    static void RegisterObject(Context* context);
    bool BeginLoad(Deserializer& source) override;
    bool Save(Serializer& dest) const override;

    AssetImportSettings& GetSettings() { return settings_; }
    const AssetImportSettings& GetSettings() const { return settings_; }
    void SetSettings(const AssetImportSettings& settings) { settings_ = settings; }

private:
    AssetImportSettings settings_;
};

/// One cached cooked asset entry. The key is source content hash plus settings hash.
struct URHO3D_API AssetCacheEntry
{
    ea::string assetId;
    ea::string outputPath;
    unsigned sourceHash{};
    unsigned settingsHash{};
    ea::vector<ea::string> dependencies;
};

/// Content-addressed cache for cooked assets.
class URHO3D_API AssetCache
{
public:
    /// Store or replace an entry.
    void Store(const AssetCacheEntry& entry);
    /// Find an entry matching the source and settings hashes.
    bool Find(const ea::string& assetId, unsigned sourceHash, unsigned settingsHash, AssetCacheEntry& entry) const;
    /// Invalidate all entries produced for an asset.
    void Invalidate(const ea::string& assetId);
    /// Invalidate all entries whose dependency is the given asset.
    void InvalidateDependency(const ea::string& dependencyId);
    /// Remove every entry.
    void Clear();
    /// Return number of cached variants.
    unsigned GetEntryCount() const { return entries_.size(); }

private:
    ea::string MakeKey(const ea::string& assetId, unsigned sourceHash, unsigned settingsHash) const;
    ea::unordered_map<ea::string, AssetCacheEntry> entries_;
};

/// Directed dependency graph for source and cooked assets.
class URHO3D_API AssetDependencyGraph
{
public:
    /// Replace the dependencies of an asset and reject self/cyclic edges.
    bool SetDependencies(const ea::string& assetId, const ea::vector<ea::string>& dependencies,
        ea::string* error = nullptr);
    /// Remove an asset and all references to it.
    void Remove(const ea::string& assetId);
    /// Return direct dependencies.
    const ea::vector<ea::string>& GetDependencies(const ea::string& assetId) const;
    /// Return direct reverse dependencies.
    ea::vector<ea::string> GetDependents(const ea::string& assetId) const;
    /// Propagate dirtiness through reverse edges, including the roots.
    ea::vector<ea::string> CollectDirty(const ea::vector<ea::string>& roots) const;
    /// Check whether the graph is acyclic.
    bool Validate(ea::string* error = nullptr) const;
    /// Return number of graph nodes.
    unsigned GetNodeCount() const { return dependencies_.size(); }

private:
    bool WouldCreateCycle(const ea::string& assetId, const ea::vector<ea::string>& dependencies) const;
    bool Visit(const ea::string& assetId, ea::unordered_map<ea::string, unsigned char>& marks,
        ea::string* error) const;

    ea::unordered_map<ea::string, ea::vector<ea::string>> dependencies_;
};

/// Result of one import attempt.
struct URHO3D_API AssetImportResult
{
    bool success{};
    bool fromCache{};
    ea::string assetId;
    ea::string outputPath;
    ea::string error;
    unsigned sourceHash{};
    unsigned settingsHash{};
    ea::vector<ea::string> dependencies;
};

/// Import callback implemented by a format-specific importer.
using AssetImportCallback = ea::function<bool(const ea::string& sourceId, const ea::string& sourceData,
    const AssetImportSettings& settings, const ea::string& outputPath, ea::vector<ea::string>& dependencies,
    ea::string& error)>;

/// Named asset importer rule with explicit versioning and cache participation.
struct URHO3D_API AssetImporterRule
{
    ea::string extension;
    ea::string name;
    unsigned version{1};
    AssetImportCallback callback;
};

/// Extensible asset import coordinator. The actual format conversion is supplied by callbacks.
class URHO3D_API AssetImporter
{
public:
    /// Register or replace an importer rule for an extension.
    void RegisterRule(const AssetImporterRule& rule);
    /// Remove an importer rule.
    void UnregisterRule(const ea::string& extension);
    /// Import source data, reusing a matching cooked cache entry when possible.
    AssetImportResult Import(const ea::string& assetId, const ea::string& sourceData,
        const AssetImportSettings& settings, const ea::string& outputPath);
    /// Mark an asset and its reverse dependents dirty.
    ea::vector<ea::string> MarkDirty(const ea::string& assetId);
    /// Access the cache for build tools and diagnostics.
    AssetCache& GetCache() { return cache_; }
    const AssetCache& GetCache() const { return cache_; }
    /// Access the dependency graph.
    AssetDependencyGraph& GetDependencyGraph() { return dependencies_; }
    const AssetDependencyGraph& GetDependencyGraph() const { return dependencies_; }

private:
    const AssetImporterRule* FindRule(const ea::string& assetId) const;
    unsigned CalculateSettingsHash(const AssetImporterRule& rule, const AssetImportSettings& settings) const;

    ea::unordered_map<ea::string, AssetImporterRule> rules_;
    AssetCache cache_;
    AssetDependencyGraph dependencies_;
};

} // namespace Urho3D
