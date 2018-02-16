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


#include <map>
#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/Log.h>
#include <cppast/cpp_entity.hpp>
#include <cppast/libclang_parser.hpp>
#include <Urho3D/Resource/XMLFile.h>
#include "Pass/CppPass.h"
#include "TypeMapper.h"


namespace Urho3D
{

/// Maps symbol names to their api declarations
class SymbolTracker
{
public:
    bool Has(const String& symbol)
    {
        return Get(symbol) != nullptr;
    }

    bool Has(const cppast::cpp_type& type)
    {
        return Has(GetTypeName(type));
    }

    void Add(const String& symbol, Declaration* decl)
    {
        URHO3D_LOGINFOF("Known symbol: %s", symbol.CString());
        nameToDeclaration_[symbol] = decl;
    }

    void Remove(const String& symbol)
    {
        nameToDeclaration_.Erase(symbol);
    }

    Declaration* Get(const String& symbol)
    {
        auto it = nameToDeclaration_.Find(symbol);
        if (it == nameToDeclaration_.End())
            return nullptr;
        if (it->second_.Expired())
        {
            nameToDeclaration_.Erase(it);
            return nullptr;
        }
        return it->second_;
    }

protected:
    HashMap<String, WeakPtr<Declaration>> nameToDeclaration_;
};

class GeneratorContext
    : public Object
{
    URHO3D_OBJECT(GeneratorContext, Object);
public:
    explicit GeneratorContext(Context* context);
    bool LoadCompileConfig(const String& pathToFile);

    bool LoadRules(const String& xmlPath);
    bool ParseFiles(const String& sourceDir);
    template<typename T, typename... Args>
    void AddCppPass(Args... args) { cppPasses_.Push(DynamicCast<CppAstPass>(SharedPtr<T>(new T(context_, args...)))); }
    template<typename T, typename... Args>
    void AddApiPass(Args... args) { apiPasses_.Push(DynamicCast<CppApiPass>(SharedPtr<T>(new T(context_, args...)))); }
    void Generate(const String& outputDir);
    bool IsAcceptableType(const cppast::cpp_type& type);

    String sourceDir_;
    String outputDir_;
    SharedPtr<XMLFile> rules_;
    cppast::libclang_compile_config config_;
    std::map<String, std::unique_ptr<cppast::cpp_file>> parsed_;
    Vector<SharedPtr<CppAstPass>> cppPasses_;
    Vector<SharedPtr<CppApiPass>> apiPasses_;
    TypeMapper typeMapper_;
    SharedPtr<Declaration> apiRoot_;
    SymbolTracker symbols_;
};

}
