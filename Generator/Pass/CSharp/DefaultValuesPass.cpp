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

#include <Urho3D/IO/FileSystem.h>
#include <cppast/cpp_variable.hpp>
#include "GeneratorContext.h"
#include "DefaultValuesPass.h"


namespace Urho3D
{

bool DefaultValuesPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (info.event == info.container_entity_exit)
        return true;

    if (entity->name_ == "SetTriangleMesh")
        int a = 2;

    // Ignore preceding default values of parameters whose default values were removed.
    if (entity->kind_ == cppast::cpp_entity_kind::member_function_t ||
        entity->kind_ == cppast::cpp_entity_kind::function_t || entity->kind_ == cppast::cpp_entity_kind::constructor_t)
    {
        bool skip = false;
        for (auto it = entity->children_.rbegin(); it != entity->children_.rend(); it++)
        {
            const auto& param = *it;
            assert(param->kind_ == cppast::cpp_entity_kind::function_parameter_t);
            param->defaultValue_ = GetConvertedDefaultValue(param, false);

            if (skip)
            {
                param->flags_ |= HintIgnoreAstDefaultValue;
                param->defaultValue_ = "";
            }
            else if (param->flags_ & HintIgnoreAstDefaultValue && param->GetDefaultValue().empty())
                skip = true;
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::variable_t ||
        entity->kind_ == cppast::cpp_entity_kind::member_variable_t)
    {
        entity->defaultValue_ = GetConvertedDefaultValue(entity, true);
    }

    return true;
}

std::string DefaultValuesPass::GetConvertedDefaultValue(MetaEntity* param, bool allowComplex)
{
    const cppast::cpp_type* cppType;

    switch (param->kind_)
    {
    case cppast::cpp_entity_kind::function_parameter_t:
        cppType = &param->Ast<cppast::cpp_function_parameter>().type();
        break;
    case cppast::cpp_entity_kind::member_variable_t:
        cppType = &param->Ast<cppast::cpp_member_variable>().type();
        break;
    case cppast::cpp_entity_kind::variable_t:
        cppType = &param->Ast<cppast::cpp_variable>().type();
        break;
    default:
        assert(false);
    }

    auto value = param->GetDefaultValue();
    if (!value.empty())
    {
        auto* typeMap = generator->GetTypeMap(*cppType);
        WeakPtr<MetaEntity> entity;
        if (typeMap != nullptr)
        {
            if (typeMap->csType_ == "string")
            {
                // String literals
                if (value == "String::EMPTY")  // TODO: move to json?
                    value = "\"\"";
            }
            else if (typeMap->isValueType && !allowComplex)
            {
                // C# is rather limited on default values of value types. Ignore them.
                param->flags_ |= HintIgnoreAstDefaultValue;
                return "";
            }
        }

        if (!allowComplex && IsComplexValueType(*cppType) || value == "nullptr")
        {
            // C# may only have default values constructed by default constructor. Because of this such default
            // values are replaced with null. Function body will construct actual default value if parameter is
            // null.
            value = "null";
        }
        else if (generator->symbols_.TryGetValue("Urho3D::" + value, entity))
            value = entity->symbolName_;
        else if (generator->enumValues_.TryGetValue(value, entity))
            value = entity->symbolName_;

        str::replace_str(value, "::", ".");
    }
    return value;
}

}
