//
// Copyright (c) 2018 Rokas Kupstys
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

#include <cppast/cpp_class_template.hpp>
#include <cppast/cpp_namespace.hpp>
#include <cppast/cpp_type_alias.hpp>
#include "GeneratorContext.h"
#include "FindFlagEnumsPass.h"


namespace Urho3D
{

bool FindFlagEnumsPass::Visit(const cppast::cpp_entity& e, cppast::visitor_info info)
{
    if (info.event == info.container_entity_exit)
        return true;

    if (e.kind() == cppast::cpp_entity_kind::class_template_specialization_t)
    {
        const auto& spec = dynamic_cast<const cppast::cpp_class_template_specialization&>(e);
        if (spec.name() == "IsFlagSet")
        {
            const auto& enumName = spec.unexposed_arguments().as_string();
            std::string _;
            const cppast::cpp_entity* enumEntity = nullptr;
            if (generator->GetSymbolOfConstant(e.parent().value(), enumName, _, &enumEntity))
            {
                ((MetaEntity*)enumEntity->user_data())->attributes_.emplace_back("Flags");
            }
        }
    }
    else if (e.kind() == cppast::cpp_entity_kind::type_alias_t)
    {
        const auto& alias = dynamic_cast<const cppast::cpp_type_alias&>(e);
        if (auto* entity = (MetaEntity*)e.user_data())
        {
            auto sourceType = entity->GetParent()->uniqueName_ + "::" + alias.name();
            auto targetType = GetTypeName(alias.underlying_type());
            if (str::starts_with(targetType, "FlagSet<"))
            {
                targetType = targetType.substr(8, targetType.length() - 9);
                std::string _;
                const cppast::cpp_entity* enumEntity = nullptr;
                if (generator->GetSymbolOfConstant(e.parent().value(), targetType, _, &enumEntity))
                {
                    auto* enumSymbol = (MetaEntity*)enumEntity->user_data();
                    if (std::find(enumSymbol->attributes_.begin(), enumSymbol->attributes_.end(), "Flags") !=
                        enumSymbol->attributes_.end())
                    {
                        generator->typeAliases_[sourceType] = &alias.underlying_type();
                        spdlog::get("console")->info("Type Alias: {} -> {}", sourceType, targetType);
                    }
                }
            }
        }
    }

    return true;
}

}
