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
#include "Urho3DCustomPass.h"


namespace Urho3D
{

void Urho3DCustomPass::Start()
{
    // C# does not understand octal escape sequences
    std::shared_ptr<MetaEntity> entity;
    if (auto* entity = generator->GetSymbol("SDLK_DELETE"))
        entity->defaultValue_ = "127";

    if (auto* entity = generator->GetSymbol("SDLK_ESCAPE"))
        entity->defaultValue_ = "27";

    // Translate to c# expression, original is "sizeof(void*) * 4" which requires unsafe context.
    if (auto* entity = generator->GetSymbol("Urho3D::VARIANT_VALUE_SIZE"))
    {
        entity->defaultValue_ = "(uint)(IntPtr.Size * 4)";
        entity->flags_ = HintReadOnly;
    }

    if (auto* entity = generator->GetSymbol("Urho3D::MOUSE_POSITION_OFFSCREEN"))
    {
        entity->defaultValue_ = "new Urho3D.IntVector2(int.MinValue, int.MaxValue)";
        entity->flags_ |= HintReadOnly;
    }

    // Fix name to property-compatible as this can be turned to a property.
    if (auto* entity = generator->GetSymbol("Urho3D::Menu::ShowPopup"))
        entity->name_ = "GetShowPopup";

    defaultValueRemap_ = {
        {"M_PI",  "MathDefs.Pi"},
        {"M_MIN_INT", "int.MinValue"},
        {"M_MAX_INT", "int.MaxValue"},
        {"M_MIN_UNSIGNED", "uint.MinValue"},
        {"M_MAX_UNSIGNED", "uint.MaxValue"},
        {"M_INFINITY", "float.PositiveInfinity"},
    };
}

bool Urho3DCustomPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (info.event == info.container_entity_exit)
        return true;

    auto fixDefaultValue = [&](MetaEntity* subEntity) {
        auto value = subEntity->GetDefaultValue();
        if (!value.empty())
        {
            auto it = defaultValueRemap_.find(value);
            if (it != defaultValueRemap_.end())
                // Known default value mappings
                subEntity->defaultValue_ = it->second;
            else
            {
                MetaEntity* constantEntity = generator->GetEntityOfConstant(entity, value);
                if (constantEntity != nullptr && constantEntity->kind_ == cppast::cpp_entity_kind::variable_t)
                {
                    subEntity->defaultValue_ = constantEntity->symbolName_;
                    str::replace_str(subEntity->defaultValue_, "::", ".");
                }
            }
        }
    };

    if (entity->kind_ == cppast::cpp_entity_kind::constructor_t ||
        entity->kind_ == cppast::cpp_entity_kind::function_t ||
        entity->kind_ == cppast::cpp_entity_kind::member_function_t)
    {
        for (auto& param : entity->children_)
            fixDefaultValue(param.get());
    }

    if (entity->kind_ == cppast::cpp_entity_kind::variable_t)
        fixDefaultValue(entity);

    if (entity->kind_ == cppast::cpp_entity_kind::enum_t && entity->name_.empty())
    {
        if (entity->children_.empty())
        {
            entity->Remove();
            return true;
        }

        // Give initial value to first element if there isn't any. This will keep correct enum values when they are
        // merged into mega-enum.
        MetaEntity* firstVar = entity->children_[0].get();
        if (firstVar->GetDefaultValue().empty())
            firstVar->defaultValue_ = "0";

        std::string targetEnum;
        if (firstVar->name_.find("SDL") == 0)
            targetEnum = "SDL";
        else
        {
            spdlog::get("console")->warn("No idea where to put {} and it's siblings.", firstVar->name_);
            entity->Remove();
            return false;
        }

        // Sort out anonymous SDL enums
        auto* toEnum = generator->GetSymbol(targetEnum);
        if (toEnum == nullptr)
        {
            std::shared_ptr<MetaEntity> newEnum(new MetaEntity());
            toEnum = newEnum.get();
            toEnum->name_ = toEnum->uniqueName_ = toEnum->symbolName_ = targetEnum;
            toEnum->kind_ = cppast::cpp_entity_kind::enum_t;
            entity->GetParent()->Add(toEnum);
            generator->symbols_[targetEnum] = toEnum->shared_from_this();
        }

        auto children = entity->children_;
        for (const auto& child : children)
            toEnum->Add(child.get());

        // No longer needed
        entity->Remove();
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::enum_value_t ||
        entity->kind_ == cppast::cpp_entity_kind::variable_t)
    {
        // Global Urho3D constants use anonymous SDL enums, update expressions to point to the named enums
        auto defaultValue = entity->GetDefaultValue();
        if (defaultValue.find("SDL") == 0)
            entity->defaultValue_ = "(int)SDL." + defaultValue;
    }
    else if (entity->name_.find("SDL_") == 0)   // Get rid of anything else belonging to sdl
    {
        entity->Remove();
        spdlog::get("console")->info("Ignore: {}", entity->uniqueName_);
    }

    return true;
}

void Urho3DCustomPass::Stop()
{
    auto removeSymbol = [&](const char* name) {
        if (auto* entity = generator->GetSymbol(name))
            entity->Remove();
    };

    // Provied by C#
    removeSymbol("Urho3D::M_INFINITY");
    removeSymbol("Urho3D::M_MIN_INT");
    removeSymbol("Urho3D::M_MAX_INT");
    removeSymbol("Urho3D::M_MIN_UNSIGNED");
    removeSymbol("Urho3D::M_MAX_UNSIGNED");
    // Provided by OpenTK
    removeSymbol("Urho3D::M_PI");
    removeSymbol("Urho3D::M_HALF_PI");
    removeSymbol("Urho3D::M_DEGTORAD");
    removeSymbol("Urho3D::M_DEGTORAD_2");
    removeSymbol("Urho3D::M_RADTODEG");
}

}
