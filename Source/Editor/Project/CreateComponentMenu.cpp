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

namespace Urho3D
{

ea::optional<StringHash> RenderCreateComponentMenu(Context* context)
{
    static const ea::string prefix = "Component/";
    const auto& typesByCategory = context->GetObjectCategories();

    static StringVector sortedCategories;
    sortedCategories.clear();
    ea::transform(typesByCategory.begin(), typesByCategory.end(), std::back_inserter(sortedCategories),
        [](const auto& pair) { return pair.first; });
    std::sort(sortedCategories.begin(), sortedCategories.end());

    ea::optional<StringHash> result;
    for (const auto& category : sortedCategories)
    {
        const auto iter = typesByCategory.find(category);
        if (iter == typesByCategory.end())
            continue;
        if (!category.starts_with(prefix))
            continue;

        const auto& types = iter->second;
        const ea::string title = category.substr(prefix.length());
        if (ui::BeginMenu(title.c_str()))
        {
            for (const StringHash type : types)
            {
                const ObjectReflection* reflection = context->GetReflection(type);
                if (!reflection->HasObjectFactory())
                    continue;
                if (ui::MenuItem(reflection->GetTypeName().c_str()))
                    result = type;
            }
            ui::EndMenu();
        }
    }
    return result;
}

}
