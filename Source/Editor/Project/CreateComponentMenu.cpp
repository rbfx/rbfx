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

#include "../Project/CreateComponentMenu.h"

#include <Urho3D/SystemUI/SystemUI.h>

#include <EASTL/map.h>
#include <EASTL/sort.h>

namespace Urho3D
{

namespace
{

using TypeNameAndReflection = ea::pair<ea::string, WeakPtr<ObjectReflection>>;

struct CategoryGroup
{
    ea::vector<TypeNameAndReflection> types_;
    ea::map<ea::string, CategoryGroup> children_;
    ea::string ungroupedGroupName_;

    CategoryGroup& GetGroup(ea::span<const ea::string> groups)
    {
        if (groups.empty())
            return *this;

        return children_[groups.front()].GetGroup(groups.subspan(1));
    }

    bool IsEmpty() const { return types_.empty() && children_.empty(); }

    void Finalize()
    {
        ea::sort(types_.begin(), types_.end());

        for (auto& [_, child] : children_)
            child.Finalize();

        ea::erase_if(children_, [](const auto& item) { return item.second.IsEmpty(); });
    }

    ObjectReflection* Render() const
    {
        ObjectReflection* result = nullptr;

        const auto renderTypes = [&]
        {
            for (const auto& [typeName, reflection] : types_)
            {
                if (ui::MenuItem(typeName.c_str()))
                    result = reflection;
            }
        };

        if (!ungroupedGroupName_.empty() && !types_.empty() && ui::BeginMenu(ungroupedGroupName_.c_str()))
        {
            renderTypes();
            ui::EndMenu();
        }

        for (const auto& [groupName, group] : children_)
        {
            if (ui::BeginMenu(groupName.c_str()))
            {
                if (ObjectReflection* childResult = group.Render())
                    result = childResult;
                ui::EndMenu();
            }
        }

        if (ungroupedGroupName_.empty())
            renderTypes();

        return result;
    }
};

void ExtractSpecialGroups(
    ea::vector<CategoryGroup>& groups, ea::string_view prefix, ea::span<const ConstString> specialCategories)
{
    for (const ea::string& categoryName : specialCategories)
    {
        const ea::string groupName = categoryName.substr(prefix.size());

        CategoryGroup& commonGroups = groups.front();
        const auto iter = commonGroups.children_.find(groupName);
        if (iter == commonGroups.children_.end())
            continue;

        auto specialGroup = ea::move(iter->second);
        commonGroups.children_.erase(iter);

        specialGroup.ungroupedGroupName_ = Format("[{}]", groupName);
        groups.push_back(ea::move(specialGroup));
    }
}

ea::vector<CategoryGroup> CreateCategoryGroups(
    Context* context, ea::string_view prefix, ea::span<const ConstString> specialCategories)
{
    ea::vector<CategoryGroup> result;
    CategoryGroup& commonGroups = result.emplace_back();

    const auto& typesByCategory = context->GetObjectCategories();
    for (const auto& [category, typeList] : typesByCategory)
    {
        if (!category.starts_with(prefix))
            continue;

        const auto groups = category.substr(prefix.size()).split('/');
        CategoryGroup& group = commonGroups.GetGroup(groups);

        for (const StringHash typeHash : typeList)
        {
            const WeakPtr<ObjectReflection> reflection{context->GetReflection(typeHash)};
            if (reflection->HasObjectFactory())
                group.types_.emplace_back(reflection->GetTypeName(), reflection);
        }
    }

    commonGroups.Finalize();

    ExtractSpecialGroups(result, prefix, specialCategories);
    return result;
}

const ea::vector<CategoryGroup>& GetOrCreateCategoryGroups(
    Context* context, ea::string_view prefix, ea::span<const ConstString> specialCategories)
{
    static struct Cache
    {
        unsigned hash_{};
        ea::vector<CategoryGroup> groups_;
    } cache;

    const auto& typesByCategory = context->GetObjectCategories();
    const unsigned currentHash = ea::max(1u, MakeHash(typesByCategory));

    if (currentHash != cache.hash_)
    {
        cache.hash_ = currentHash;
        cache.groups_ = CreateCategoryGroups(context, prefix, specialCategories);
    }

    return cache.groups_;
}

} // namespace

ObjectReflection* RenderCreateComponentMenu(Context* context)
{
    static const ea::string prefix = "Component/";
    static const ConstString specialCategories[] = {Category_Plugin, Category_User};

    const auto& groups = GetOrCreateCategoryGroups(context, prefix, specialCategories);

    ObjectReflection* result = nullptr;
    for (const CategoryGroup& group : groups)
    {
        if (&group != &groups.front())
            ui::Separator();

        if (ObjectReflection* groupResult = group.Render())
            result = groupResult;
    }

    return result;
}

} // namespace Urho3D
