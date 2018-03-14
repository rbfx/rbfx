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

#include "GeneratorContext.h"
#include "ConvertToPropertiesPass.h"


namespace Urho3D
{

bool ConvertToPropertiesPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (info.event == info.container_entity_exit)
        return true;

    if (entity->kind_ != cppast::cpp_entity_kind::member_function_t)
        return true;

    if (entity->flags_ & HintProperty)
        return true;

    if (entity->access_ != cppast::cpp_public && !generator->inheritable_.IsIncluded(entity->parent_->symbolName_))
        return true;

    if (std::regex_match(entity->name_, rxGetterName_))
    {
        if (!entity->children_.empty())
            return true;   // Getter can not be with parameters

        auto getterType = cppast::to_string(entity->Ast<cppast::cpp_member_function>().return_type());
        auto propertyName = entity->name_;

        bool skipSetter = false;
        if (propertyName[1] == 's') // "Is" getter
        {
            propertyName = propertyName;
            skipSetter = true;
        }
        else                        // "Get" getter
            propertyName = propertyName.substr(3);

        if (propertyName == entity->parent_->name_)
        {
            URHO3D_LOGWARNINGF("%s was not converted to property because property name would match enclosing parent.",
                entity->sourceSymbolName_.c_str());
            return true;
        }

        auto siblings = entity->parent_->children_;
        for (const auto& sibling : siblings)
        {
            if (sibling != entity && sibling->name_ == propertyName)
            {
                URHO3D_LOGWARNINGF("Could not convert %s to property because %s already exists.",
                    entity->sourceSymbolName_.c_str(), sibling->sourceSymbolName_.c_str());
                return true;
            }
        }

        // Getter
        auto* property = new MetaEntity();
        property->kind_ = cppast::cpp_entity_kind::member_variable_t;
        property->name_ = propertyName;
        property->flags_ = HintProperty;
        property->access_ = entity->access_;
        entity->name_ = "get";
        entity->parent_->Add(property);

        if (!skipSetter)
        {
            auto setterName = "Set" + propertyName;
            if (islower(entity->name_[0]))
                setterName[0] = static_cast<char>(tolower(setterName[0]));

            // Find sibling setter with single parameter of same type as getter
            for (const auto& sibling : siblings)
            {
                if (sibling->kind_ == cppast::cpp_entity_kind::member_function_t && sibling->name_ == setterName &&
                    sibling->children_.size() == 1)
                {
                    auto setterType = cppast::to_string(
                        sibling->children_[0]->Ast<cppast::cpp_function_parameter>().type());
                    if (setterType == getterType)
                    {
                        sibling->name_ = "set";
                        property->Add(sibling);
                        break;
                    }
                }
            }
        }
        property->Add(entity);
    }

    return true;
}

}
