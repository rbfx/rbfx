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

#include "OverrideConstantsPass.h"
#include "GeneratorContext.h"

namespace Urho3D
{

bool OverrideConstantsPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (entity->kind_ == cppast::cpp_entity_kind::variable_t || entity->kind_ == cppast::cpp_entity_kind::enum_value_t)
    {
        auto it = generator->defaultValueRemaps_.find(entity->symbolName_);
        if (it != generator->defaultValueRemaps_.end())
        {
            entity->defaultValue_ = it->second;

            // Try to guess if new default value is not a compile time constant.
            const auto& defaultValue = entity->GetDefaultValue();
            if (entity->kind_ == cppast::cpp_entity_kind::enum_value_t ||
                IsConst(entity->Ast<cppast::cpp_variable>().type()))
            {
                if (!container::contains(generator->forceCompileTimeConstants_, entity->symbolName_) &&
                    !str::is_numeric(defaultValue) && !str::is_hex(defaultValue) &&
                    !str::starts_with(defaultValue, "\"") && !str::ends_with(defaultValue, "\""))
                    entity->flags_ |= HintReadOnly;
            }
        }
    }
    return true;
}

}
