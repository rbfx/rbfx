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
#include "GeneratorContext.h"
#include "Urho3DCustomPass.h"


namespace Urho3D
{

void Urho3DCustomPass::Start()
{
    // C# does not understand octal escape sequences
    WeakPtr<MetaEntity> entity;
    if (generator->symbols_.TryGetValue("SDLK_DELETE", entity))
        entity->defaultValue_ = "127";

    if (generator->symbols_.TryGetValue("SDLK_ESCAPE", entity))
        entity->defaultValue_ = "27";

    // Translate to c# expression, original is "sizeof(void*) * 4" which requires unsafe context.
    if (generator->symbols_.TryGetValue("Urho3D::VARIANT_VALUE_SIZE", entity))
    {
        entity->defaultValue_ = "(uint)(IntPtr.Size * 4)";
        entity->flags_ = HintReadOnly;
    }

    if (generator->symbols_.TryGetValue("Urho3D::M_INFINITY", entity))
        entity->defaultValue_ = "float.PositiveInfinity";

    if (generator->symbols_.TryGetValue("Urho3D::M_MIN_INT", entity))
        entity->defaultValue_ = "int.MinValue";

    if (generator->symbols_.TryGetValue("Urho3D::M_MAX_INT", entity))
        entity->defaultValue_ = "int.MaxValue";

    if (generator->symbols_.TryGetValue("Urho3D::MOUSE_POSITION_OFFSCREEN", entity))
    {
        entity->defaultValue_ = "new Urho3D.IntVector2(MathDefs.M_MIN_INT, MathDefs.M_MIN_INT)";
        entity->flags_ |= HintReadOnly;
    }

    // Remove default value of up vector due to C# limitations
    if (generator->symbols_.TryGetValue("Urho3D::Node::LookAt(Urho3D::Vector3 const&,Urho3D::Vector3 const&,Urho3D::TransformSpace)", entity))
        entity->children_[1]->flags_ |= HintIgnoreAstDefaultValue;

    // Third parameter is not suitable for default value therefore ignore default value of second parameter as well
    if (generator->symbols_.TryGetValue("Urho3D::UIElement::SetLayout(Urho3D::LayoutMode,int,Urho3D::IntRect const&)", entity))
        entity->children_[1]->flags_ |= HintIgnoreAstDefaultValue;
}

bool Urho3DCustomPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (entity->kind_ == cppast::cpp_entity_kind::enum_t && entity->name_.empty())
    {
        if (entity->children_.empty())
        {
            entity->Remove();
            return true;
        }

        // Give initial value to first element if there isn't any. This will keep correct enum values when they are
        // merged into mega-enum.
        MetaEntity* firstVar = entity->children_[0].Get();
        if (firstVar->GetDefaultValue().empty())
            firstVar->defaultValue_ = "0";

        std::string targetEnum;
        if (firstVar->name_.find("SDL") == 0)
            targetEnum = "SDL";
        else
        {
            URHO3D_LOGWARNINGF("No idea where to put %s and it's siblings.", firstVar->name_.c_str());
            entity->Remove();
            return false;
        }

        // Sort out anonymous SDL enums
        WeakPtr<MetaEntity> toEnum;
        if (!generator->symbols_.TryGetValue(targetEnum, toEnum))
        {
            toEnum = new MetaEntity();
            toEnum->name_ = toEnum->uniqueName_ = toEnum->symbolName_ = targetEnum;
            toEnum->kind_ = cppast::cpp_entity_kind::enum_t;
            entity->parent_->Add(toEnum);
            generator->symbols_[targetEnum] = toEnum;
        }

        auto children = entity->children_;
        for (const auto& child : children)
            toEnum->Add(child);

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
        else if (defaultValue.empty() && entity->parent_->kind_ == cppast::cpp_entity_kind::namespace_t &&
            (entity->name_.find("P_") == 0 || entity->name_.find("E_") == 0) &&
            Urho3D::GetTypeName(entity->Ast<cppast::cpp_variable>().type()) == "Urho3D::StringHash")
        {
            // Give default values to event names and parameters
            entity->defaultValue_ = fmt::format("new Urho3D.StringHash(\"{}\")", entity->name_);
            entity->flags_ |= HintReadOnly;
        }
    }
    else if (entity->name_.find("SDL_") == 0)   // Get rid of anything else belonging to sdl
        entity->Remove();

    return true;
}

}
