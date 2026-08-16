#include "../Precompiled.h"

#include "AssetPipeline.h"

#include "../Core/StringUtils.h"
#include "../IO/Deserializer.h"
#include "../IO/Serializer.h"
#include "../Resource/JSONFile.h"

#include <EASTL/sort.h>
#include <EASTL/unordered_set.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

void SetError(ea::string* error, const ea::string& message)
{
    if (error)
        *error = message;
}

JSONValue VariantToJSON(const Variant& value)
{
    switch (value.GetType())
    {
    case VAR_INT: return JSONValue(value.GetInt());
    case VAR_BOOL: return JSONValue(value.GetBool());
    case VAR_FLOAT: return JSONValue(value.GetFloat());
    case VAR_DOUBLE: return JSONValue(value.GetDouble());
    case VAR_STRING: return JSONValue(value.GetString());
    default: return JSONValue(value.ToString());
    }
}

Variant VariantFromJSON(const JSONValue& value, VariantType type)
{
    switch (type)
    {
    case VAR_INT: return Variant(value.GetInt());
    case VAR_BOOL: return Variant(value.GetBool());
    case VAR_FLOAT: return Variant(value.GetFloat());
    case VAR_DOUBLE: return Variant(value.GetDouble());
    case VAR_STRING: return Variant(value.GetString());
    default: return Variant(value.IsString() ? value.GetString() : ea::string());
    }
}

} // namespace

AssetImportSettingsResource::AssetImportSettingsResource(Context* context)
    : Resource(context)
{
}

void AssetImportSettingsResource::RegisterObject(Context* context)
{
    context->RegisterFactory<AssetImportSettingsResource>();
}

bool AssetImportSettingsResource::BeginLoad(Deserializer& source)
{
    const unsigned size = source.GetSize();
    if (size == 0)
        return false;

    ea::string json;
    json.resize(size);
    if (source.Read(json.data(), size) != size)
        return false;

    JSONValue root;
    if (!JSONFile::ParseJSON(json, root, false))
        return false;

    ea::string error;
    if (!settings_.FromJSON(root, &error))
        return false;
    SetMemoryUse(size);
    return true;
}

bool AssetImportSettingsResource::Save(Serializer& dest) const
{
    JSONFile file(context_);
    file.GetRoot() = settings_.ToJSON();
    const ea::string json = file.ToString("  ");
    return dest.Write(json.data(), json.size()) == json.size();
}

JSONValue AssetImportSettings::ToJSON() const
{
    JSONValue root(JSON_OBJECT);
    root.Set("version", version);
    root.Set("importer", importer);

    JSONValue values(JSON_OBJECT);
    ea::vector<ea::string> names;
    names.reserve(properties.size());
    for (const auto& property : properties)
        names.push_back(property.first);
    ea::sort(names.begin(), names.end());

    for (const ea::string& name : names)
    {
        const Variant& value = properties.at(name);
        JSONValue item(JSON_OBJECT);
        item.Set("type", static_cast<unsigned>(value.GetType()));
        item.Set("value", VariantToJSON(value));
        values.Set(name, ea::move(item));
    }
    root.Set("properties", ea::move(values));
    return root;
}

bool AssetImportSettings::FromJSON(const JSONValue& value, ea::string* error)
{
    if (!value.IsObject())
    {
        SetError(error, "Asset import settings root must be an object.");
        return false;
    }

    AssetImportSettings parsed;
    parsed.version = value.Contains("version") ? value["version"].GetUInt() : 1u;
    parsed.importer = value.Contains("importer") ? value["importer"].GetString() : "Generic";
    if (value.Contains("properties"))
    {
        if (!value["properties"].IsObject())
        {
            SetError(error, "Asset import settings properties must be an object.");
            return false;
        }
        for (const auto& item : value["properties"].GetObject())
        {
            if (!item.second.IsObject() || !item.second.Contains("type") || !item.second.Contains("value"))
            {
                SetError(error, Format("Invalid asset import property '{}'.", item.first));
                return false;
            }
            const unsigned type = item.second["type"].GetUInt();
            if (type >= MAX_VAR_TYPES)
            {
                SetError(error, Format("Unsupported Variant type {} in asset import property '{}'.", type, item.first));
                return false;
            }
            parsed.properties[item.first] = VariantFromJSON(item.second["value"], static_cast<VariantType>(type));
        }
    }

    *this = ea::move(parsed);
    return true;
}

unsigned AssetImportSettings::CalculateHash() const
{
    unsigned result = 0;
    CombineHash(result, version);
    CombineHash(result, MakeHash(importer));

    ea::vector<ea::string> names;
    names.reserve(properties.size());
    for (const auto& property : properties)
        names.push_back(property.first);
    ea::sort(names.begin(), names.end());
    for (const ea::string& name : names)
    {
        const Variant& value = properties.at(name);
        CombineHash(result, MakeHash(name));
        CombineHash(result, static_cast<unsigned>(value.GetType()));
        CombineHash(result, MakeHash(value.ToString()));
    }
    return result;
}

ea::string AssetCache::MakeKey(const ea::string& assetId, unsigned sourceHash, unsigned settingsHash) const
{
    return Format("{}:{}:{}", assetId, sourceHash, settingsHash);
}

void AssetCache::Store(const AssetCacheEntry& entry)
{
    entries_[MakeKey(entry.assetId, entry.sourceHash, entry.settingsHash)] = entry;
}

bool AssetCache::Find(const ea::string& assetId, unsigned sourceHash, unsigned settingsHash, AssetCacheEntry& entry) const
{
    const auto iter = entries_.find(MakeKey(assetId, sourceHash, settingsHash));
    if (iter == entries_.end())
        return false;
    entry = iter->second;
    return true;
}

void AssetCache::Invalidate(const ea::string& assetId)
{
    for (auto iter = entries_.begin(); iter != entries_.end();)
    {
        if (iter->second.assetId == assetId)
            iter = entries_.erase(iter);
        else
            ++iter;
    }
}

void AssetCache::InvalidateDependency(const ea::string& dependencyId)
{
    for (auto iter = entries_.begin(); iter != entries_.end();)
    {
        const auto& dependencies = iter->second.dependencies;
        if (ea::find(dependencies.begin(), dependencies.end(), dependencyId) != dependencies.end())
            iter = entries_.erase(iter);
        else
            ++iter;
    }
}

void AssetCache::Clear()
{
    entries_.clear();
}

bool AssetDependencyGraph::WouldCreateCycle(const ea::string& assetId,
    const ea::vector<ea::string>& dependencies) const
{
    ea::vector<ea::string> pending = dependencies;
    ea::unordered_set<ea::string> visited;
    while (!pending.empty())
    {
        const ea::string current = pending.back();
        pending.pop_back();
        if (current == assetId)
            return true;
        if (!visited.insert(current).second)
            continue;
        const auto iter = dependencies_.find(current);
        if (iter != dependencies_.end())
            pending.insert(pending.end(), iter->second.begin(), iter->second.end());
    }
    return false;
}

bool AssetDependencyGraph::SetDependencies(const ea::string& assetId,
    const ea::vector<ea::string>& dependencies, ea::string* error)
{
    if (assetId.empty())
    {
        SetError(error, "Asset dependency graph requires a non-empty asset id.");
        return false;
    }
    if (ea::find(dependencies.begin(), dependencies.end(), assetId) != dependencies.end())
    {
        SetError(error, "An asset cannot depend on itself.");
        return false;
    }

    const auto previous = dependencies_.find(assetId);
    const ea::vector<ea::string> oldDependencies = previous != dependencies_.end() ? previous->second : ea::vector<ea::string>();
    dependencies_[assetId] = dependencies;
    if (WouldCreateCycle(assetId, dependencies))
    {
        if (oldDependencies.empty())
            dependencies_.erase(assetId);
        else
            dependencies_[assetId] = oldDependencies;
        SetError(error, Format("Adding dependencies for '{}' would create a cycle.", assetId));
        return false;
    }
    return true;
}

void AssetDependencyGraph::Remove(const ea::string& assetId)
{
    dependencies_.erase(assetId);
    for (auto& item : dependencies_)
    {
        auto& values = item.second;
        values.erase(ea::remove(values.begin(), values.end(), assetId), values.end());
    }
}

const ea::vector<ea::string>& AssetDependencyGraph::GetDependencies(const ea::string& assetId) const
{
    static const ea::vector<ea::string> empty;
    const auto iter = dependencies_.find(assetId);
    return iter != dependencies_.end() ? iter->second : empty;
}

ea::vector<ea::string> AssetDependencyGraph::GetDependents(const ea::string& assetId) const
{
    ea::vector<ea::string> result;
    for (const auto& item : dependencies_)
    {
        if (ea::find(item.second.begin(), item.second.end(), assetId) != item.second.end())
            result.push_back(item.first);
    }
    ea::sort(result.begin(), result.end());
    return result;
}

ea::vector<ea::string> AssetDependencyGraph::CollectDirty(const ea::vector<ea::string>& roots) const
{
    ea::vector<ea::string> result;
    ea::vector<ea::string> pending = roots;
    ea::unordered_set<ea::string> visited;
    while (!pending.empty())
    {
        const ea::string current = pending.back();
        pending.pop_back();
        if (!visited.insert(current).second)
            continue;
        result.push_back(current);
        const ea::vector<ea::string> dependents = GetDependents(current);
        pending.insert(pending.end(), dependents.begin(), dependents.end());
    }
    ea::sort(result.begin(), result.end());
    return result;
}

bool AssetDependencyGraph::Visit(const ea::string& assetId,
    ea::unordered_map<ea::string, unsigned char>& marks, ea::string* error) const
{
    const unsigned char mark = marks[assetId];
    if (mark == 1)
    {
        SetError(error, Format("Asset dependency cycle detected at '{}'.", assetId));
        return false;
    }
    if (mark == 2)
        return true;
    marks[assetId] = 1;
    const auto iter = dependencies_.find(assetId);
    if (iter != dependencies_.end())
    {
        for (const ea::string& dependency : iter->second)
        {
            if (!Visit(dependency, marks, error))
                return false;
        }
    }
    marks[assetId] = 2;
    return true;
}

bool AssetDependencyGraph::Validate(ea::string* error) const
{
    ea::unordered_map<ea::string, unsigned char> marks;
    for (const auto& item : dependencies_)
    {
        if (!Visit(item.first, marks, error))
            return false;
    }
    return true;
}

void AssetImporter::RegisterRule(const AssetImporterRule& rule)
{
    if (!rule.extension.empty())
        rules_[rule.extension] = rule;
}

void AssetImporter::UnregisterRule(const ea::string& extension)
{
    rules_.erase(extension);
}

const AssetImporterRule* AssetImporter::FindRule(const ea::string& assetId) const
{
    const auto dot = assetId.find_last_of('.');
    if (dot == ea::string::npos)
        return nullptr;
    const ea::string extension = assetId.substr(dot + 1);
    const auto iter = rules_.find(extension);
    return iter != rules_.end() ? &iter->second : nullptr;
}

unsigned AssetImporter::CalculateSettingsHash(const AssetImporterRule& rule,
    const AssetImportSettings& settings) const
{
    unsigned result = settings.CalculateHash();
    CombineHash(result, MakeHash(rule.name));
    CombineHash(result, rule.version);
    return result;
}

AssetImportResult AssetImporter::Import(const ea::string& assetId, const ea::string& sourceData,
    const AssetImportSettings& settings, const ea::string& outputPath)
{
    AssetImportResult result;
    result.assetId = assetId;
    result.outputPath = outputPath;
    result.sourceHash = MakeHash(sourceData);

    const AssetImporterRule* rule = FindRule(assetId);
    if (!rule)
    {
        result.error = Format("No importer registered for asset '{}'.", assetId);
        return result;
    }
    result.settingsHash = CalculateSettingsHash(*rule, settings);

    AssetCacheEntry cached;
    if (cache_.Find(assetId, result.sourceHash, result.settingsHash, cached))
    {
        result.success = true;
        result.fromCache = true;
        result.outputPath = cached.outputPath;
        result.dependencies = cached.dependencies;
        return result;
    }

    ea::vector<ea::string> dependencies;
    ea::string error;
    const bool imported = rule->callback
        ? rule->callback(assetId, sourceData, settings, outputPath, dependencies, error)
        : true;
    if (!imported)
    {
        result.error = error.empty() ? "Asset importer callback failed." : error;
        return result;
    }
    if (!dependencies_.SetDependencies(assetId, dependencies, &error))
    {
        result.error = error;
        return result;
    }

    AssetCacheEntry entry;
    entry.assetId = assetId;
    entry.outputPath = outputPath;
    entry.sourceHash = result.sourceHash;
    entry.settingsHash = result.settingsHash;
    entry.dependencies = dependencies;
    cache_.Store(entry);

    result.success = true;
    result.dependencies = ea::move(dependencies);
    return result;
}

ea::vector<ea::string> AssetImporter::MarkDirty(const ea::string& assetId)
{
    const ea::vector<ea::string> dirty = dependencies_.CollectDirty({assetId});
    for (const ea::string& id : dirty)
        cache_.Invalidate(id);
    for (const ea::string& id : dirty)
        cache_.InvalidateDependency(id);
    return dirty;
}

} // namespace Urho3D
