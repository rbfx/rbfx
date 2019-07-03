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

#include <Urho3D/IO/FileSystem.h>

#include "Project.h"
#include "Pipeline/Importers/AssetImporter.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/Asset.h"

namespace Urho3D
{

AssetImporter::AssetImporter(Context* context)
    : Serializable(context)
{
    SubscribeToEvent(&inspector_, E_ATTRIBUTEINSPECTVALUEMODIFIED, [this](StringHash, VariantMap& args) {
        using namespace AttributeInspectorValueModified;
        const AttributeInfo& attr = *reinterpret_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr());
        attributesModified_ = true;
        // Set to true when modified and to false when reset
        AttributeInspectorModifiedFlags flags(args[P_MODIFIED].GetUInt());

        if (flags & AttributeInspectorModified::SET_DEFAULT)
        {
            // Resetting value to default. Set as modified still if it is not same as inherited.
            Variant inherited = GetInstanceDefault(attr.name_);
            if (!inherited.IsEmpty())
                isAttributeSet_[attr.name_] = inherited != attr.defaultValue_;
            else
                isAttributeSet_[attr.name_] = false;

            if (isAttributeSet_[attr.name_])
                SetInheritedDefaults(attr, GetAttribute(attr.name_));
            else
                SetInheritedDefaults(attr, GetInstanceDefault(attr.name_));
        }
        else if (flags & AttributeInspectorModified::SET_INHERITED)
        {
            isAttributeSet_[attr.name_] = false;
            SetInheritedDefaults(attr, GetInstanceDefault(attr.name_));
        }

    });
    SubscribeToEvent(&inspector_, E_ATTRIBUTEINSPECTOATTRIBUTE, [this](StringHash, VariantMap& args) {
        using namespace AttributeInspectorAttribute;
        if (isAttributeSet_[reinterpret_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr())->name_])
            args[P_COLOR] = Color::WHITE;
    });
}

void AssetImporter::RenderInspector(const char* filter)
{
    RenderAttributes(this, filter, &inspector_);
}

bool AssetImporter::SaveJSON(JSONValue& dest) const
{
    if (!Serializable::SaveJSON(dest))
        return false;

    JSONValue& byproductsDest = dest["byproducts"];
    byproductsDest.SetType(JSON_ARRAY);
    for (const ea::string& byproduct : byproducts_)
        byproductsDest.Push(byproduct);

    const_cast<AssetImporter*>(this)->attributesModified_ = false;   // TODO: sucks
    return true;
}

bool AssetImporter::LoadJSON(const JSONValue& source)
{
    if (!Serializable::LoadJSON(source))
        return false;

    const JSONValue& byproductsSrc = source["byproducts"];
    if (byproductsSrc.IsArray())
    {
        auto* project = GetSubsystem<Project>();
        auto* fs = GetFileSystem();

        for (int i = 0; i < byproductsSrc.Size(); i++)
        {
            ea::string resourceName = byproductsSrc[i].GetString();
            if (fs->FileExists(project->GetCachePath() + resourceName))
                byproducts_.push_back(resourceName);
        }
    }

    return true;
}

bool AssetImporter::IsModified()
{
    if (!byproducts_.empty())
        return true;

    bool isModified = false;
    if (const ea::vector<AttributeInfo>* attributes = GetAttributes())
    {
        for (const AttributeInfo& info : *attributes)
        {
            Variant value;
            // Side-step attribute inheritance.
            Serializable::OnGetAttribute(info, value);
            isModified |= value != GetAttributeDefault(info.name_);
        }
    }
    return isModified;
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
    SetInheritedDefaults(attr, src);
    if (!asset_->IsMetaAsset() && flavor_ == DEFAULT_PIPELINE_FLAVOR)
        GetSubsystem<Project>()->GetPipeline().ScheduleImport(asset_);
}

void AssetImporter::SetInheritedDefaults(const AttributeInfo& attr, const Variant& src)
{
    bool isDefaultFlavor = flavor_ == DEFAULT_PIPELINE_FLAVOR;
    if (isDefaultFlavor)
    {
        // Set defaults to sibling flavors
        for (const auto& flavor : asset_->GetImporters())
        {
            if (flavor.first == flavor_)
                continue;

            for (const auto& importer : flavor.second)
                importer->SetInstanceDefault(attr.name_, src);
        }
    }

    bool modified = false;
    for (const auto& flavor : asset_->GetImporters())
    {
        if (!isDefaultFlavor)
        {
            // default = set all, otherwise set only that flavor
            if (flavor.first != flavor_)
                continue;
        }

        for (const auto& importer : flavor.second)
            modified |= importer->SetInheritedDefaultsForImporter(attr, src);
    }
}

void AssetImporter::SetInheritedDefaultsIfSet()
{
    if (const auto* attributes = GetAttributes())
    {
        for (const AttributeInfo& attr : *attributes)
        {
            if (isAttributeSet_[attr.name_])
                SetInheritedDefaults(attr, GetAttribute(attr.name_));
        }
    }
}

bool AssetImporter::SetInheritedDefaultsForImporter(const AttributeInfo& attr, const Variant& src)
{
    auto* fs = GetFileSystem();
    auto* project = GetSubsystem<Project>();

    // If this is not a meta-asset - it's attibutes cant be inherited.
    if (!asset_->IsMetaAsset())
        return false;

    // Set downstream defaults.
    StringVector resourceNames;
    fs->ScanDir(resourceNames, asset_->GetResourcePath(), "", SCAN_FILES|SCAN_DIRS, false);
    resourceNames.erase_first(".");
    resourceNames.erase_first("..");

    bool modified = false;
    for (const ea::string& resourceName : resourceNames)
    {
        if (Asset* asset = project->GetPipeline().GetAsset(asset_->GetName() + resourceName))
        {
            auto it = asset->GetImporters().find(flavor_);
            for (const auto& importer : it->second)
            {
                if (importer->GetInstanceDefault(attr.name_) != src)
                {
                    importer->SetInstanceDefault(attr.name_, src);
                    modified = true;
                }
                if (!importer->isAttributeSet_[attr.name_])
                    modified |= importer->SetInheritedDefaultsForImporter(attr, src);

                if (modified && flavor_ == DEFAULT_PIPELINE_FLAVOR)
                    GetSubsystem<Project>()->GetPipeline().ScheduleImport(asset);
            }
        }
    }
    return modified;
}

void AssetImporter::Initialize(Asset* asset, const ea::string& flavor)
{
    asset_ = asset;
    flavor_ = flavor;
}

void AssetImporter::ClearByproducts()
{
    auto* fs = GetFileSystem();
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

bool AssetImporter::SaveDefaultAttributes(const AttributeInfo& attr) const
{
    auto it = isAttributeSet_.find(attr.name_);
    return it != isAttributeSet_.end() && it->second;
}

}
