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

#include "../Core/SettingsManager.h"

#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/Resource/JSONFile.h>

namespace Urho3D
{

namespace
{

ea::pair<ea::string_view, ea::string_view> GetPathAndSection(ea::string_view uniqueName)
{
    const auto colonIndex = uniqueName.find_first_of(':');
    if (colonIndex == ea::string_view::npos)
        return {uniqueName, ea::string_view()};
    return {uniqueName.substr(0, colonIndex), uniqueName.substr(colonIndex + 1)};
}

}

SettingsPage::SettingsPage(Context* context)
    : Object(context)
{
}

SettingsManager::SettingsManager(Context* context)
    : Object(context)
{
}

void SettingsManager::SerializeInBlock(Archive& archive)
{
    for (const auto& [key, page] : sortedPages_)
    {
        if (page->IsSerializable())
            SerializeOptionalValue(archive, key.c_str(), *page, AlwaysSerialize{});
    }
}

void SettingsManager::LoadFile(const ea::string& fileName)
{
    auto jsonFile = MakeShared<JSONFile>(context_);
    if (jsonFile->LoadFile(fileName))
        jsonFile->LoadObject("Settings", *this);
}

void SettingsManager::SaveFile(const ea::string& fileName) const
{
    auto jsonFile = MakeShared<JSONFile>(context_);
    if (jsonFile->SaveObject("Settings", *this))
        jsonFile->SaveFile(fileName);
}

void SettingsManager::AddPage(SharedPtr<SettingsPage> page)
{
    pages_.push_back(page);

    const ea::string& name = page->GetUniqueName();
    const auto [path, section] = GetPathAndSection(name);

    sortedPages_[name] = page;
    InsertPageInGroup(rootGroup_, path, page, section);
}

SettingsPage* SettingsManager::FindPage(const ea::string& key) const
{
    const auto iter = sortedPages_.find(key);
    return iter != sortedPages_.end() ? iter->second : nullptr;
}

void SettingsManager::InsertPageInGroup(SettingsPageGroup& parentGroup, ea::string_view path,
    const SharedPtr<SettingsPage>& page, ea::string_view section)
{
    const auto dotIndex = path.find_first_of('.');
    const bool isLeaf = dotIndex == ea::string_view::npos;
    const ea::string childName{isLeaf ? path : path.substr(0, dotIndex)};

    auto& childGroup = parentGroup.children_[childName];
    if (!childGroup)
        childGroup = ea::make_unique<SettingsPageGroup>();

    if (isLeaf)
        childGroup->pages_.emplace(ea::string(section), page);
    else
        InsertPageInGroup(*childGroup, path.substr(dotIndex + 1), page, section);
}

}
