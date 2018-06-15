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
#include "Urho3DCustomPassEarly.h"


namespace Urho3D
{

inline unsigned SDBMHash(unsigned hash, unsigned char c) { return c + (hash << 6u) + (hash << 16u) - hash; }

unsigned StringHashCalculate(const char* str, unsigned hash=0)
{
    if (!str)
        return hash;

    while (*str)
    {
        // Perform the actual hashing as case-insensitive
        char c = *str;
        hash = SDBMHash(hash, (unsigned char)tolower(c));
        ++str;
    }

    return hash;
}

bool Urho3DCustomPassEarly::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (info.event == info.container_entity_exit)
        return true;

    if (entity->kind_ == cppast::cpp_entity_kind::enum_value_t)
    {
        auto defaultValue = entity->GetDefaultValue();
        if (str::starts_with(defaultValue, "SDL_") || str::starts_with(defaultValue, "SDLK_"))
            entity->defaultValue_ = "SDL." + defaultValue;
    }
    else if (entity->kind_ != cppast::cpp_entity_kind::enum_value_t &&
        entity->kind_ != cppast::cpp_entity_kind::variable_t &&
        str::starts_with(entity->name_, "SDL_"))
    {
        // We only need some enums/constants from SDL. Get rid of anything else.
        entity->Remove();
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::variable_t &&
        entity->GetParent()->kind_ == cppast::cpp_entity_kind::namespace_t && entity->GetDefaultValue().empty() &&
        Urho3D::GetTypeName(entity->Ast<cppast::cpp_variable>().type()) == "Urho3D::StringHash")
    {
        auto defaultValue = entity->GetDefaultValue();
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

                eventNamespace->attributes_.emplace_back(
                    fmt::format("[Event(EventType=0x{:08X})]", StringHashCalculate(eventNamespace->name_.c_str())));
                // Event name is conveyed through attribute therefore this constant is no longer needed.
                entity->Remove();

                // Properly name parameter constants by using their values. Actual constant names have all words mashed
                // into single all-caps word while constant values use CamelCase.
                for (auto& child : eventNamespace->children_)
                {
                    auto val = child->GetDefaultValue();
                    child->name_ = val.substr(1, val.length() - 2);   // Get rid of quotes and prepend P.
                }

                // Constant naming event is always named "Event" and added to same namespace where event parameters are.
                // In C# namespaces can not have constants therefore they must be turned into (static) classes.
                eventNamespace->kind_ = cppast::cpp_entity_kind::class_t;
                eventNamespace->ast_ = nullptr;
                eventNamespace->flags_ |= HintNoStatic;

                // These event classes are moved to ParentNamespace.Events namespace.
                auto* parentSpaceDest = eventNamespace->GetParent();
                std::string parentSpaceSymbol(parentSpaceDest->symbolName_ + "::Events");
                MetaEntity* eventsParentSpace = generator->GetSymbol(parentSpaceSymbol);
                if (eventsParentSpace == nullptr)
                {
                    std::shared_ptr<MetaEntity> eventsParentSpaceStorage(new MetaEntity());
                    eventsParentSpace = eventsParentSpaceStorage.get();
                    eventsParentSpace->name_ = "Events";
                    eventsParentSpace->uniqueName_ = parentSpaceSymbol;
                    eventsParentSpace->kind_ = cppast::cpp_entity_kind::namespace_t;
                    parentSpaceDest->Add(eventsParentSpace);
                    generator->symbols_[parentSpaceSymbol] = eventsParentSpace->shared_from_this();
                }
                eventsParentSpace->Add(eventNamespace);

                // Event name is conveyed through attribute therefore this constant is no longer needed.
                entity->Remove();
            }
        }
    }
    return true;
}

}
