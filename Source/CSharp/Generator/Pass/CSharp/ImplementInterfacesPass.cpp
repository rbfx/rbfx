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

#include <cppast/cpp_variable.hpp>
#include <cppast/cpp_namespace.hpp>
#include <spdlog/spdlog.h>
#include "GeneratorContext.h"
#include "ImplementInterfacesPass.h"


namespace Urho3D
{

bool DiscoverInterfacesPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (entity->ast_ == nullptr)
        return true;

    if (info.event == info.container_entity_enter)
    {
        if (entity->kind_ == cppast::cpp_entity_kind::class_t)
        {
            const auto* cls = dynamic_cast<const cppast::cpp_class*>(entity->ast_);
            if (cls == nullptr || Count(cls->bases()) < 2)
                return true;

            for (auto it = cls->bases().begin(); it != cls->bases().end(); it++)
            {
                const cppast::cpp_type& base = it->type();
                std::shared_ptr<MetaEntity> metaBase;
                if (auto* metaBase = generator->GetSymbol(Urho3D::GetTypeName(it->type())))
                {
                    if (!(metaBase->flags_ & HintInterface))
                    {
                        // If first class is not an interface then allow it to be consumed as parent class.
                        if (it == cls->bases().begin())
                            continue;

                        metaBase->flags_ |= HintInterface;
                        spdlog::get("console")->info("Interface candidate found: {}", it->name());
                    }
                }
            }

            std::function<void(const cppast::cpp_class*)> putInheritorToBases = [&](const cppast::cpp_class* cls)
            {
                for (auto it = cls->bases().begin(); it != cls->bases().end(); it++)
                {
                    const cppast::cpp_type& base = it->type();
                    std::shared_ptr<MetaEntity> metaBase;
                    if (auto* metaBase = generator->GetSymbol(Urho3D::GetTypeName(it->type())))
                    {
                        if (metaBase->flags_ & HintInterface)
                        {
                            inheritedBy_[metaBase->symbolName_].emplace_back(entity->symbolName_);
                            putInheritorToBases(dynamic_cast<const cppast::cpp_class*>(metaBase->ast_));
                        }
                    }
                };
            };
            putInheritorToBases(cls);
        }
    }
    return true;
}

bool ImplementInterfacesPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (entity->ast_ == nullptr)
        return true;

    if (info.event == info.container_entity_enter)
    {
        if (entity->kind_ == cppast::cpp_entity_kind::class_t)
        {
            const auto* cls = dynamic_cast<const cppast::cpp_class*>(entity->ast_);
            if (cls == nullptr || Count(cls->bases()) < 2)
                return true;

            for (const auto& baseCls : cls->bases())
            {
                const cppast::cpp_type& base = baseCls.type();
                std::shared_ptr<MetaEntity> metaBase;
                auto baseClassName = Urho3D::GetTypeName(baseCls.type());
                if (auto* metaBase = generator->GetSymbol(baseClassName))
                {
                    if (!(metaBase->flags_ & HintInterface))
                        continue;

                    // Bases have to be visited before derived classes, otherwise derived classes will have methods missing.
                    // This is a crude workaround to ensure this requirement as depending on the order of files parsed pass
                    // may fail if stars align incorrectly.
                    Visit(metaBase, info);

                    // This class inherits an interface. Clone non-overriden methods from interface class to current
                    // one. This will cause further passes to generate proper C and C# API for these methods. We can not
                    // just use C api of interfaced class because this API takes instance pointer with interface class
                    // type while subclass would pass instance of it's own type. Basically we have to
                    // dynamic_cast<InterfacedClass*>(instnace) before passing it to C API. It is not possible therefore
                    // we make extra C api for interfaced classes to be generated here.

                    for (const auto& interfaceMethod : metaBase->children_)
                    {
                        if (interfaceMethod->kind_ == cppast::cpp_entity_kind::member_function_t)
                        {
                            // If current class implements method from interface then do not pull it from
                            // interfaced class.
                            auto it = std::find_if(entity->children_.begin(), entity->children_.end(),
                                [&](std::shared_ptr<MetaEntity> child) {
                                    if (child->kind_ != cppast::cpp_entity_kind::member_function_t || child->ast_ == nullptr)
                                        return false;

                                    if (child->sourceName_ != interfaceMethod->sourceName_)
                                        return false;

                                    return child->Ast<cppast::cpp_member_function>().signature() ==
                                        interfaceMethod->Ast<cppast::cpp_member_function>().signature();
                                });

                            if (it != entity->children_.end())
                                continue;

                            interfaceMethod->flags_ |= HintInterface;
                            std::shared_ptr<MetaEntity> newEntity(new MetaEntity(*interfaceMethod));

                            // Avoid C API name collisions
                            str::replace_str(newEntity->cFunctionName_, Sanitize(metaBase->sourceSymbolName_),
                                Sanitize(entity->sourceSymbolName_), 1);

                            // Give new identity
                            str::replace_str(newEntity->uniqueName_, metaBase->symbolName_, entity->symbolName_, 1);

                            // Just to be safe that right method is called
                            str::replace_str(newEntity->sourceSymbolName_, metaBase->sourceSymbolName_,
                                entity->sourceSymbolName_, 1);

                            entity->Add(newEntity.get());
                        }
                    }
                }
                else
                    spdlog::get("console")->warn("Interface base class not found: {}", baseClassName);
            }
        }
    }

    return true;
}
}
