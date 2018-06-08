//
// Copyright (c) 2008-2018 the Urho3D project.
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

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include "GeneratorContext.h"
#include "FixDefaultValuesPass.h"


namespace Urho3D
{

bool FixDefaultValuesPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (info.event == info.container_entity_exit)
        return true;

    MetaEntity* defaultValueEntity = nullptr;
    switch (entity->kind_)
    {
    case cppast::cpp_entity_kind::constructor_t:
    case cppast::cpp_entity_kind::function_t:
    case cppast::cpp_entity_kind::member_function_t:
    {
        for (auto& param : entity->children_)
        {
            auto value = param->GetDefaultValue();
            if (!value.empty())
            {
                if (generator->GetSymbolOfConstant(entity, value, param->defaultValue_, &defaultValueEntity))
                    param->defaultValueEntity_.reset(defaultValueEntity);
            }
        }
        break;
    }
    case cppast::cpp_entity_kind::variable_t:
    {
        auto value = entity->GetDefaultValue();
        if (!value.empty())
        {
            if (generator->GetSymbolOfConstant(entity, value, entity->defaultValue_, &defaultValueEntity))
                entity->defaultValueEntity_.reset(defaultValueEntity);
        }
        break;
    }
    default:
        break;
    }

    return true;
}

}
