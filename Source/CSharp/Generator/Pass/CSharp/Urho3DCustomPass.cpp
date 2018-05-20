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

#include <fmt/format.h>
#include "GeneratorContext.h"
#include "Urho3DCustomPass.h"


namespace Urho3D
{

void Urho3DCustomPassLate::NamespaceStart()
{
    // Fix name to property-compatible as this can be turned to a property.
    if (auto* entity = generator->GetSymbol("Urho3D::Menu::ShowPopup"))
        entity->name_ = "GetShowPopup";

    // cppast bug workaround
    if (auto* entity = generator->GetSymbol("Urho3D::Localization::Get(Urho3D::String const&,int)"))
        entity->children_[1]->defaultValue_ = "-1";
}

bool Urho3DCustomPassLate::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (info.event == info.container_entity_exit)
        return true;

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

    return true;
}

}
