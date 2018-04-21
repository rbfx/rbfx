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

#include <cppast/cpp_template.hpp>
#include <fmt/format.h>
#include "GeneratorContext.h"
#include "RenameMembersPass.h"


namespace Urho3D
{

bool RenameMembersPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    switch (entity->kind_)
    {
    case cppast::cpp_entity_kind::variable_t:
        if (entity->name_.find("E_") == 0 || entity->name_.find("P_") == 0)
            // Events and parameters
            return true;
    case cppast::cpp_entity_kind::member_variable_t:
    case cppast::cpp_entity_kind::member_function_t:
    case cppast::cpp_entity_kind::function_t:
    // case cppast::cpp_entity_kind::enum_t:
    // case cppast::cpp_entity_kind::enum_value_t:
    // Enums appear in default parameters as well. Reason for skipping them is same as constants above.
    {
        auto parts = str::SplitName(entity->name_);
        if (parts.size() > 1 && entity->name_.length() > 2 && isupper(entity->name_[0]) && entity->name_[1] == '_')
            // Prefixes like M_
            parts.erase(parts.begin());

        entity->name_ = str::join(parts, "");
        entity->symbolName_ = entity->GetParent()->symbolName_ + "::" + entity->name_;
        break;
    }
    default:
        break;
    }
    return true;
}

}
