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
        if (spec.name() == "is_flagset")
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

    return true;
}

}
