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
#include <Declarations/Variable.hpp>
#include <GeneratorContext.h>
#include <Declarations/Class.hpp>
#include "MoveGlobalsPass.h"


namespace Urho3D
{

bool MoveGlobalsPass::Visit(Declaration* decl, Event event)
{
    if (event == Event::EXIT)
        return true;

    if (decl->kind_ == Declaration::Kind::Variable)
    {
        Variable* var = dynamic_cast<Variable*>(decl);
        Namespace* ns = dynamic_cast<Namespace*>(var->parent_.Get());
        if (var->isStatic_ && ns->kind_ == Declaration::Kind::Namespace)
        {
            GeneratorContext* generator = GetSubsystem<GeneratorContext>();
            std::string className = ns->name_;
            std::string classSymbol = ns->symbolName_ + "::" + className;

            if (ns->symbolName_ == className && decl->source_ != nullptr)
            {
                className = GetFileName(*decl->source_);
                classSymbol = ns->symbolName_ + "::" + className;
            }

            SharedPtr<Class> toClass(reinterpret_cast<Class*>(generator->symbols_.Get(classSymbol)));
            if (toClass == nullptr)
            {
                toClass = SharedPtr<Class>(new Class(nullptr));
                toClass->name_ = className;
                toClass->sourceName_ = ns->sourceName_;
                toClass->symbolName_ = classSymbol;
                ns->Add(toClass);
                generator->symbols_.Add(classSymbol, toClass.Get());
            }
            toClass->Add(var);
        }
    }

    return true;
}

std::string MoveGlobalsPass::GetFileName(const cppast::cpp_entity& entity)
{
    const cppast::cpp_entity* e = &entity;
    while (e->kind() != cppast::cpp_entity_kind::file_t)
        e = &e->parent().value();

    return Urho3D::GetFileName(e->name()).CString();
}

}
