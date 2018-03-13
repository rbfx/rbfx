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

#pragma once


#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Utils.h>
#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_member_variable.hpp>
#include <cppast/cpp_variable.hpp>
#include <cppast/cpp_enum.hpp>
#include <cppast/visitor.hpp>
#include "Utilities.h"

namespace Urho3D
{

enum CppEntityHints
{
    HintNone = 0,
    HintReadOnly = 1,
    HintIgnoreAstDefaultValue = 2,
    HintInterface = 4,
};

URHO3D_TO_FLAGS_ENUM(CppEntityHints);

/// Wrapper over cppast::cpp_entity. Overlay-AST is assembled from these entities. This allows freely modifying AST
/// structure while maintianing original information of AST entities.
struct MetaEntity : public RefCounted
{
    MetaEntity() = default;
    MetaEntity(const cppast::cpp_entity& source, cppast::cpp_access_specifier_kind access)
        : ast_(&source)
        , access_(access)
    {
        sourceName_ = name_ = source.name();
        kind_ = source.kind();
        uniqueName_ = GetUniqueName(source);
        sourceSymbolName_ = symbolName_ = GetSymbolName(source);
        ast_->set_user_data((void*)this);
    }

    MetaEntity(const MetaEntity& other)
    {

        kind_ = other.kind_;
        ast_ = other.ast_;
        access_ = other.access_;
        parent_ = nullptr;

        symbolName_ = other.symbolName_;
        sourceSymbolName_ = other.sourceSymbolName_;
        uniqueName_ = other.uniqueName_;
        sourceName_ = other.sourceName_;
        name_ = other.name_;
        defaultValue_ = other.defaultValue_;
        flags_ = other.flags_;
        cFunctionName_ = other.cFunctionName_;

        for (const auto& child : other.children_)
            children_.emplace_back(new MetaEntity(*child));
    }

    template<typename T>
    const T& Ast() const { return dynamic_cast<const T&>(*ast_); }

    void Remove()
    {
        if (parent_ != nullptr)
        {
            Unregister();
            SharedPtr<MetaEntity> ref(this);
            auto& children = parent_->children_;
            children.erase(std::find(children.begin(), children.end(), this));
            parent_ = nullptr;
        }
    }

    void Add(MetaEntity* entity)
    {
        if (std::find(children_.begin(), children_.end(), this) != children_.end())
            return;

        SharedPtr<MetaEntity> ref(entity);
        ref->Remove();
        ref->parent_ = this;
        children_.emplace_back(ref);
        entity->Register();
    }

    std::string GetDefaultValue() const
    {
        if (!defaultValue_.empty())
            return defaultValue_;

        if ((flags_ & HintIgnoreAstDefaultValue) == 0)
            return GetNativeDefaultValue();

        return "";
    }

    std::string GetNativeDefaultValue() const
    {
        switch (kind_)
        {
        case cppast::cpp_entity_kind::enum_value_t:
        {
            const auto& val = Ast<cppast::cpp_enum_value>().value();
            if (val.has_value())
                return ToString(val.value());
            break;
        }
        case cppast::cpp_entity_kind::variable_t:
        {
            const auto& val = Ast<cppast::cpp_variable>().default_value();
            if (val.has_value())
                return ToString(val.value());
            break;
        }
        case cppast::cpp_entity_kind::member_variable_t:
        {
            const auto& val = Ast<cppast::cpp_member_variable>().default_value();
            if (val.has_value())
                return ToString(val.value());
            break;
        }
        case cppast::cpp_entity_kind::bitfield_t:
            break;
        case cppast::cpp_entity_kind::function_parameter_t:
        {
            const auto& val = Ast<cppast::cpp_function_parameter>().default_value();
            if (val.has_value())
                return ToString(val.value());
            break;
        }
        default:
            break;
        }
        return "";
    }

    cppast::cpp_entity_kind kind_ = cppast::cpp_entity_kind::file_t;
    /// Source ast entity.
    const cppast::cpp_entity* ast_ = nullptr;
    /// Source ast info.
    cppast::cpp_access_specifier_kind access_ = cppast::cpp_access_specifier_kind::cpp_public;
    /// Parent of this entity.
    WeakPtr<MetaEntity> parent_ = nullptr;
    /// Children of overlay ast entity.
    std::vector<SharedPtr<MetaEntity>> children_;
    /// A full name of c++ symbol.
    std::string symbolName_;
    /// A original full name of c++ symbol.
    std::string sourceSymbolName_;
    /// Unique name identifying this entity. It is symbolName + methodSignature or just symbolName.
    std::string uniqueName_;
    /// Name which will be used to access symbol in C API.
    std::string sourceName_;
    /// Name of the symbol used in wrapping API. Used for renaming entities in target language API.
    std::string name_;
    /// Override default value of this entity. This can an expression in target language.
    std::string defaultValue_;
    /// Various hints about this entity.
    CppEntityHints flags_ = HintNone;
    /// Name of a function wrapping this entity in a C wrapper.
    std::string cFunctionName_;

protected:
    void Register();
    void Unregister();
};

class CppAstPass : public Object
{
    URHO3D_OBJECT(CppAstPass, Object)
public:
    explicit CppAstPass(Context* context) : Object(context) { };

    virtual void Start() { }
    virtual void StartFile(const String& filePath) { }
    virtual bool Visit(const cppast::cpp_entity& e, cppast::visitor_info info) = 0;
    virtual void StopFile(const String& filePath) { }
    virtual void Stop() { }
};

class CppApiPass : public Object
{
URHO3D_OBJECT(CppApiPass, Object)
public:
    explicit CppApiPass(Context* context) : Object(context) { };

    virtual void Start() { }
    virtual bool Visit(MetaEntity* entity, cppast::visitor_info info) = 0;
    virtual void Stop() { }
};

}
