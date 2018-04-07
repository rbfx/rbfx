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
#include "Urho3DEventsPass.h"


namespace Urho3D
{

bool Urho3DEventsPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (info.event == info.container_entity_exit)
        return true;

    if (entity->kind_ == cppast::cpp_entity_kind::variable_t)
    {
        auto defaultValue = entity->GetDefaultValue();
        if (defaultValue.empty() && entity->GetParent()->kind_ == cppast::cpp_entity_kind::namespace_t &&
            Urho3D::GetTypeName(entity->Ast<cppast::cpp_variable>().type()) == "Urho3D::StringHash")
        {
            // Give default values to event names
            if (entity->name_.find("E_") == 0)
            {
                const auto& siblings = entity->GetParent()->children_;
                auto it = std::find(siblings.begin(), siblings.end(), entity->shared_from_this());
                assert(it != siblings.end());

                // Next sibling which is supposed to be namespace containing event parameters. Name of namespace is
                // event name.
                it++;

                if (it != siblings.end())
                {
                    auto* eventNamespace = it->get();
                    assert(eventNamespace->kind_ == cppast::cpp_entity_kind::namespace_t);
                    entity->defaultValue_ = fmt::format("\"{}\"", eventNamespace->name_);
                    entity->flags_ |= HintReadOnly;
                }
            }
        }
    }

    return true;
}

}
