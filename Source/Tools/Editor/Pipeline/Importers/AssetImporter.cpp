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

#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/FileSystem.h>

#include "EditorEvents.h"
#include "Project.h"
#include "Pipeline/Importers/AssetImporter.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/Asset.h"

namespace Urho3D
{

AssetImporter::AssetImporter(Context* context)
    : Serializable(context)
{
    SubscribeToEvent(this, E_ATTRIBUTEINSPECTVALUEMODIFIED, &AssetImporter::OnInspectorModified);
    SubscribeToEvent(this, E_ATTRIBUTEINSPECTOATTRIBUTE, &AssetImporter::OnRenderInspectorAttribute);
}

void AssetImporter::OnInspectorModified(StringHash, VariantMap& args)
{
    using namespace AttributeInspectorValueModified;
    const AttributeInfo& attr = *reinterpret_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr());
    auto reason = static_cast<AttributeInspectorModified>(args[P_REASON].GetUInt());
    // Resetting value to default. Set as modified still if it is not same as inherited.
    if (reason & AttributeInspectorModified::SET_DEFAULT)
    {
        Variant inherited = GetInstanceDefault(attr.name_);
        if (!inherited.IsEmpty())
            isAttributeSet_[attr.name_] = inherited != attr.defaultValue_;
        else
            isAttributeSet_[attr.name_] = false;
    }
    else if (reason & AttributeInspectorModified::SET_INHERITED)
        isAttributeSet_[attr.name_] = false;

    if (flavor_->IsDefault())
        asset_->ReimportOutOfDateRecursive();
}

void AssetImporter::OnRenderInspectorAttribute(StringHash, VariantMap& args)
{
    using namespace AttributeInspectorAttribute;
    if (isAttributeSet_[reinterpret_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr())->name_])
        args[P_VALUE_KIND] = (int)AttributeValueKind::ATTRIBUTE_VALUE_CUSTOM;
}

bool AssetImporter::Execute(Urho3D::Asset* input, const ea::string& outputPath)
{
    lastAttributeHash_ = HashEffectiveAttributeValues();
    ClearByproducts();
    return true;
}

bool AssetImporter::Serialize(Archive& archive, ArchiveBlock& block)
{
    if (!BaseClassName::Serialize(archive, block))
        return false;

    if (!SerializeVector(archive, "byproducts", "resourceName", byproducts_))
        return false;

    lastAttributeHash_ = HashEffectiveAttributeValues();
    return true;
}

bool AssetImporter::IsModified() const
{
    for (const auto& nameStatus : isAttributeSet_)
    {
        if (nameStatus.second)
            return true;
    }
    return false;
}

bool AssetImporter::IsOutOfDate() const
{
    if (!Accepts(asset_->GetResourcePath()))
        return false;

    if (byproducts_.empty())
        return true;

    if (lastAttributeHash_ != HashEffectiveAttributeValues())
        return true;

    auto* fs = context_->GetSubsystem<FileSystem>();
    auto* project = GetSubsystem<Project>();

    unsigned mtime = fs->GetLastModifiedTime(asset_->GetResourcePath());
    for (const ea::string& byproduct : byproducts_)
    {
        ea::string byproductPath = project->GetCachePath() + byproduct;
        if (!fs->FileExists(byproductPath))
            return true;

        if (fs->GetLastModifiedTime(byproductPath) < mtime)
            return true;
    }

    return false;
}

void AssetImporter::OnGetAttribute(const AttributeInfo& attr, Variant& dest) const
{
    auto it = isAttributeSet_.find(attr.name_);
    if (it == isAttributeSet_.end() || !it->second)
        dest = GetAttributeDefault(attr.name_);
    else
        Serializable::OnGetAttribute(attr, dest);
}

void AssetImporter::OnSetAttribute(const AttributeInfo& attr, const Variant& src)
{
    isAttributeSet_[attr.name_] = true;
    Serializable::OnSetAttribute(attr, src);

    using namespace EditorImporterAttributeModified;
    VariantMap& args = GetEventDataMap();
    args[P_ASSET] = asset_;
    args[P_IMPORTER] = this;
    args[P_ATTRINFO] = (void*)&attr;
    args[P_NEWVALUE] = src;
    SendEvent(E_EDITORIMPORTERATTRIBUTEMODIFIED, args);
}

void AssetImporter::Initialize(Asset* asset, Flavor* flavor)
{
    auto undo = GetSubsystem<UndoStack>();
    undo->Connect(this);
    asset_ = asset;
    flavor_ = WeakPtr(flavor);
}

void AssetImporter::ClearByproducts()
{
    auto* fs = context_->GetSubsystem<FileSystem>();
    auto* project = GetSubsystem<Project>();
    for (const ea::string& byproduct : byproducts_)
        fs->Delete(project->GetCachePath() + byproduct);
    byproducts_.clear();
}

void AssetImporter::AddByproduct(const ea::string& byproduct)
{
    auto* project = GetSubsystem<Project>();
    if (byproduct.starts_with(project->GetCachePath()))
        // Byproducts should contain resource names. Trim if this is a full path.
        byproducts_.emplace_back(byproduct.substr(project->GetCachePath().size()));
    else
    {
        assert(!IsAbsolutePath(byproduct));
        byproducts_.push_back(byproduct);
    }
}

void AssetImporter::RemoveByproduct(const ea::string& byproduct)
{
    auto* project = GetSubsystem<Project>();
    if (byproduct.starts_with(project->GetCachePath()))
        // Byproducts should contain resource names. Trim if this is a full path.
        byproducts_.erase_first(byproduct.substr(project->GetCachePath().size()));
    else
    {
        assert(!IsAbsolutePath(byproduct));
        byproducts_.erase_first(byproduct);
    }
}

bool AssetImporter::SaveDefaultAttributes(const AttributeInfo& attr) const
{
    auto it = isAttributeSet_.find(attr.name_);
    return it != isAttributeSet_.end() && it->second;
}

Variant AssetImporter::GetInstanceDefault(const ea::string& name) const
{
    Pipeline* pipeline = GetSubsystem<Pipeline>();
    if (!flavor_->IsDefault())
    {
        // Attempt inheriting value from a sibling default flavor.
        Flavor* defaultFlavor = pipeline->GetDefaultFlavor();
        if (AssetImporter* importer = asset_->GetImporter(defaultFlavor, GetType()))
        {
            if (importer->isAttributeSet_[name])
                return importer->GetAttribute(name);
        }
    }

    ea::string resourceName = asset_->GetName();

    if (resourceName.ends_with("/"))
        resourceName.resize(resourceName.length() - 1);         // Meta assets always end with /, remove it so we can "cd .."

    if (!resourceName.contains('/'))                            // Top level asset
        return Variant::EMPTY;

    resourceName.resize(resourceName.find_last_of('/') + 1);    // cd ..

    // Get value from same importer at meta asset of parent path.
    if (Asset* asset = pipeline->GetAsset(resourceName))
    {
        if (AssetImporter* importer = asset->GetImporter(flavor_, GetType()))
        {
            auto it = importer->isAttributeSet_.find(name);
            if (it != importer->isAttributeSet_.cend() && it->second)
                return importer->GetAttribute(name);
            return importer->GetInstanceDefault(name);
        }
    }

    return Variant::EMPTY;
}

unsigned AssetImporter::HashEffectiveAttributeValues() const
{
    unsigned hash = 16777619;
    Variant value;

    if (const ea::vector<AttributeInfo>* attributes = GetAttributes())
    {
        for (const AttributeInfo& attr : *attributes)
        {
            if (IsAttributeSet(attr.name_))
                OnGetAttribute(attr, value);
            else
            {
                value = GetInstanceDefault(attr.name_);
                if (value.IsEmpty())
                    value = attr.defaultValue_;
            }

            hash = hash * 31 + value.ToHash();
        }
    }

    return hash;
}

bool AssetImporter::IsAttributeSet(const eastl::string& name) const
{
    auto it = isAttributeSet_.find(name);
    return it != isAttributeSet_.end() && it->second;
}

}
