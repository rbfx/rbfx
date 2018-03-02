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
#include <cppast/cpp_namespace.hpp>
#include "GeneratorContext.h"
#include "MoveGlobalsPass.h"


namespace Urho3D
{

bool MoveGlobalsPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (info.event == info.container_entity_exit)
        return true;

    if (entity->kind_ == cppast::cpp_entity_kind::namespace_t)
    {
        // Convert non-top-level namespaces to classes if they have any functions or variables.
        if (!entity->parent_->name_.empty())
        {
            const auto& ns = entity->Ast<cppast::cpp_namespace>();
            for (const auto& child : ns)
            {
                if (child.kind() == cppast::cpp_entity_kind::function_t ||
                    child.kind() == cppast::cpp_entity_kind::variable_t)
                {
                    entity->kind_ = cppast::cpp_entity_kind::class_t;
                    return true;
                }
            }
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::variable_t)
    {
        const auto& var = entity->Ast<cppast::cpp_variable>();
        auto& ns = *entity->parent_;
        if (ns.kind_ != cppast::cpp_entity_kind::class_t)
        {
            std::string className = ns.name_;
            std::string classSymbol = ns.uniqueName_ + "::" + className;

            if (ns.uniqueName_ == className && entity->ast_ != nullptr)
            {
                className = GetFileName(*entity->ast_);
                classSymbol = ns.uniqueName_ + "::" + className;

                WeakPtr<MetaEntity> toClass;
                if (!generator->symbols_.TryGetValue(classSymbol, toClass) || toClass.Expired())
                {
                    toClass = new MetaEntity();
                    toClass->name_ = className;
                    toClass->sourceName_ = ns.sourceName_;
                    toClass->uniqueName_ = toClass->symbolName_ = classSymbol;
                    toClass->kind_ = cppast::cpp_entity_kind::class_t;
                    ns.Add(toClass);
                    generator->symbols_[classSymbol] = toClass;
                }

                entity->symbolName_ = toClass->uniqueName_ + "::" + entity->name_;
                toClass->Add(entity);
            }
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
