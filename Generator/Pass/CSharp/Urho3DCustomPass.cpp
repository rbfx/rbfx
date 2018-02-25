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

#include "Declarations/Variable.hpp"
#include "GeneratorContext.h"
#include "Declarations/Enum.hpp"
#include "Urho3DCustomPass.h"


namespace Urho3D
{

void Urho3DCustomPass::Start()
{
    // Translate to c# expression, original is "sizeof(void*) * 4" which requires unsafe context.
    if (auto* var = dynamic_cast<Variable*>(generator->symbols_.Get("Urho3D::VARIANT_VALUE_SIZE")))
    {
        var->defaultValue_ = "(uint)(IntPtr.Size * 4)";
        var->isConstant_ = false;
        var->isReadOnly_ = true;
    }

    // C# does not understand octal escape sequences
    if (auto* var = dynamic_cast<Variable*>(generator->symbols_.Get("SDLK_DELETE")))
        var->defaultValue_ = "127";

    if (auto* var = dynamic_cast<Variable*>(generator->symbols_.Get("SDLK_ESCAPE")))
        var->defaultValue_ = "27";

    if (auto* var = dynamic_cast<Variable*>(generator->symbols_.Get("Urho3D::M_INFINITY")))
        var->defaultValue_ = "float.PositiveInfinity";

    if (auto* var = dynamic_cast<Variable*>(generator->symbols_.Get("Urho3D::M_MIN_INT")))
        var->defaultValue_ = "int.MinValue";

    if (auto* var = dynamic_cast<Variable*>(generator->symbols_.Get("Urho3D::M_MAX_INT")))
        var->defaultValue_ = "int.MaxValue";
}

bool Urho3DCustomPass::Visit(Declaration* decl, Event event)
{
    if (decl->kind_ == Declaration::Kind::Enum)
    {
        Enum* ns = dynamic_cast<Enum*>(decl);
        if (ns->name_.StartsWith("anonymous_"))
        {
            if (ns->children_.Empty())
            {
                ns->Ignore();
                return true;
            }

            // Give initial value to first element if there isn't any. This will keep correct enum values when they are
            // merged into mega-enum.
            Variable* firstVar = dynamic_cast<Variable*>(ns->children_[0].Get());
            if (firstVar->defaultValue_.Empty())
                firstVar->defaultValue_ = "0";

            String targetEnum;
            if (firstVar->name_.StartsWith("SDL"))
                targetEnum = "SDL";
            else
            {
                URHO3D_LOGWARNINGF("No idea where to put %s and it's siblings.", firstVar->name_.CString());
                ns->Ignore();
                return false;
            }

            // Sort out anonymous SDL enums
            Enum* toEnum = dynamic_cast<Enum*>(generator->symbols_.Get(targetEnum));
            if (toEnum == nullptr)
            {
                toEnum = new Enum(nullptr);
                toEnum->name_ = toEnum->symbolName_ = targetEnum;
                toEnum->sourceName_ = "::";
                ns->parent_->Add(toEnum);
                generator->symbols_.Add(targetEnum, toEnum);
            }

            auto children = ns->children_;
            for (auto& child : children)
                toEnum->Add(child);
            ns->Ignore();
        }
    }
    else if (decl->name_.StartsWith("anonymous_"))
        decl->Ignore();
    else if (decl->kind_ == Declaration::Kind::Variable)
    {
        if (decl->isConstant_ && decl->isStatic_)
        {
            Variable* var = dynamic_cast<Variable*>(decl);
            // Global Urho3D constants use anonymous SDL enums, update expressions to point to the named enums
            if (var->defaultValue_.StartsWith("SDL"))
                var->defaultValue_ = "(int)SDL." + var->defaultValue_;
        }
    }
    else if (decl->name_.StartsWith("SDL_") && event != Event::ENTER)
        decl->Ignore();
    return true;
}

}
