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

#include <EASTL/sort.h>

namespace Urho3D
{

namespace
{

using ObjectTypeVector = ea::vector<StringHash>;
using SortedCategoriesVector = ea::vector<ea::pair<ea::string, const ObjectTypeVector*>>;
using SortedTypesVector = ea::vector<const ObjectReflection*>;

const SortedCategoriesVector& GetSortedCategories(Context* context, ea::string_view prefix)
{
    const auto& typesByCategory = context->GetObjectCategories();

    static thread_local SortedCategoriesVector sortedCategories;
    sortedCategories.clear();

    ea::transform(typesByCategory.begin(), typesByCategory.end(), std::back_inserter(sortedCategories),
        [](const auto& pair) { return ea::make_pair(pair.first, &pair.second); });

    ea::erase_if(sortedCategories, [&](const auto& pair) { return !pair.first.starts_with(prefix); });
    ea::erase_if(sortedCategories, [&](const auto& pair) { return pair.second->empty(); });

    ea::sort(sortedCategories.begin(), sortedCategories.end());

    return sortedCategories;
}

const SortedTypesVector& GetSortedTypes(Context* context, const ObjectTypeVector& types)
{
    static thread_local SortedTypesVector sortedTypes;
    sortedTypes.clear();

    ea::transform(types.begin(), types.end(), std::back_inserter(sortedTypes),
        [context](const StringHash type) { return context->GetReflection(type); });

    ea::erase(sortedTypes, nullptr);
    ea::erase_if(sortedTypes, [](const ObjectReflection* reflection) { return !reflection->HasObjectFactory(); });

    const auto compareNames = [](const ObjectReflection* lhs, const ObjectReflection* rhs)
    { return lhs->GetTypeName() < rhs->GetTypeName(); };
    ea::sort(sortedTypes.begin(), sortedTypes.end(), compareNames);

    return sortedTypes;
}
}

ea::optional<StringHash> RenderCreateComponentMenu(Context* context)
{
    static const ea::string prefix = "Component/";
    const auto& sortedCategories = GetSortedCategories(context, prefix);

    ea::optional<StringHash> result;
    for (const auto& [categoryName, types] : sortedCategories)
    {
        const ea::string title = categoryName.substr(prefix.length());
        if (ui::BeginMenu(title.c_str()))
        {
            const auto& sortedTypes = GetSortedTypes(context, *types);
            for (const ObjectReflection* reflection : sortedTypes)
            {
                if (ui::MenuItem(reflection->GetTypeName().c_str()))
                    result = reflection->GetTypeNameHash();
            }
            ui::EndMenu();
        }
    }
    return result;
}

}
