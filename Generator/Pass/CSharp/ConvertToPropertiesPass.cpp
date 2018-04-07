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

    if (entity->access_ != cppast::cpp_public)
        return true;

    // Virtual getters/setters of inheritable classes can not be turned to properties in order to allow overriding.
    if (entity->Ast<cppast::cpp_member_function>().is_virtual() &&
        !generator->inheritable_.IsIncluded(entity->GetParent()->symbolName_))
        return true;

    // If method is part of interface then getters/setters must appear as methods.
    if (entity->flags_ & HintInterface)
        return true;

    if (std::regex_match(entity->name_, rxGetterName_))
    {
        MetaEntity* getter = entity;
        MetaEntity* setter = entity;
        if (!getter->children_.empty())
            // Getter can not be with parameters
            return true;

        if (getter->cFunctionName_.empty())
            // Getter has no C API
            return true;

        auto getterType = cppast::to_string(getter->Ast<cppast::cpp_member_function>().return_type());
        auto propertyName = getter->name_;

        std::string setterName;
        if (propertyName[1] == 's') // "Is" getter
        {
            propertyName = propertyName;
            setterName = "Set" + propertyName.substr(2);
        }
        else                        // "Get" getter
        {
            propertyName = propertyName.substr(3);
            setterName = "Set" + propertyName;
        }

        if (propertyName == getter->GetParent()->name_)
        {
            spdlog::get("console")->warn("{} was not converted to property because property name would match enclosing "
                                         "parent.", getter->sourceSymbolName_);
            return true;
        }

        auto siblings = getter->GetParent()->children_;
        // Find setter
        for (const auto& sibling : siblings)
        {
            if (sibling->kind_ == cppast::cpp_entity_kind::member_function_t && sibling->name_ == setterName &&
                sibling->children_.size() == 1 && sibling->access_ == getter->access_)
            {
                auto setterType = cppast::to_string(
                    sibling->children_[0]->Ast<cppast::cpp_function_parameter>().type());
                if (setterType == getterType && !sibling->cFunctionName_.empty() &&
                    getter->access_ == sibling->access_ && !(sibling->flags_ & HintInterface))
                {
                    setter = sibling.get();
                    break;
                }
            }
        }

        // Find if sibling with matching name exists, it may interfere with property generation.
        for (const auto& sibling : siblings)
        {
            if (sibling.get() != getter && sibling->name_ == propertyName)
            {
                if (setter != nullptr && sibling->kind_ == cppast::cpp_entity_kind::member_variable_t)
                {
                    // Both getter and setter were found and their access matches, hence getter/setter property may
                    // replace a variable for which this property is generated.
                    sibling->Remove();
                }
                else
                {
                    spdlog::get("console")->warn("Could not convert {} to property because {} already exists.",
                        getter->sourceSymbolName_, sibling->sourceSymbolName_);
                    return true;
                }
            }
        }

        // Getter
        std::shared_ptr<MetaEntity> property(new MetaEntity());
        property->kind_ = cppast::cpp_entity_kind::member_variable_t;
        property->name_ = propertyName;
        property->flags_ = HintProperty;
        property->access_ = getter->access_;
        getter->GetParent()->Add(property.get());

        if (islower(getter->name_[0]))
            setterName[0] = static_cast<char>(tolower(setterName[0]));

        setter->name_ = "set";
        property->Add(setter);

        getter->name_ = "get";
        property->Add(getter);
    }

    return true;
}

}
