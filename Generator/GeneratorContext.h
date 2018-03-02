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


#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Container/Hash.h>
#include <string>
#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/Log.h>
#include <cppast/cpp_entity.hpp>
#include <cppast/libclang_parser.hpp>
#include <Urho3D/Resource/JSONFile.h>
#include "Pass/CppPass.h"
#include "TypeMapper.h"


namespace Urho3D
{

/// C string hash function.
template <> inline unsigned MakeHash(const char* value)
{
    unsigned hash = 0;
    while (*value)
    {
        hash = SDBMHash(hash, static_cast<unsigned char>(*value));
        ++value;
    }
    return hash;
}

/// std::String hash function.
template <> inline unsigned MakeHash(const std::string& value)
{
    return MakeHash(value.c_str());
}

class GeneratorContext
    : public Object
{
    URHO3D_OBJECT(GeneratorContext, Object);
public:
    explicit GeneratorContext(Context* context);
    void LoadCompileConfig(const std::vector<std::string>& includes, std::vector<std::string>& defines,
        const std::vector<std::string>& options);

    bool LoadRules(const String& jsonPath);
    bool ParseFiles(const String& sourceDir);
    template<typename T, typename... Args>
    void AddCppPass(Args... args) { cppPasses_.Push(DynamicCast<CppAstPass>(SharedPtr<T>(new T(context_, args...)))); }
    template<typename T, typename... Args>
    void AddApiPass(Args... args) { apiPasses_.Push(DynamicCast<CppApiPass>(SharedPtr<T>(new T(context_, args...)))); }
    void Generate(const String& outputDirCpp, const String& outputDirCs);
    bool IsAcceptableType(const cppast::cpp_type& type);

    String sourceDir_;
    String outputDirCpp_;
    String outputDirCs_;
    SharedPtr<JSONFile> rules_;
    cppast::libclang_compile_config config_;
    std::map<String, std::unique_ptr<cppast::cpp_file>> parsed_;
    Vector<SharedPtr<CppAstPass>> cppPasses_;
    Vector<SharedPtr<CppApiPass>> apiPasses_;
    TypeMapper typeMapper_;
    SharedPtr<MetaEntity> apiRoot_;
    cppast::cpp_entity_index index_;
    std::string defaultNamespace_ = "Urho3D";
    HashMap<std::string, WeakPtr<MetaEntity>> symbols_;
    Vector<std::string> final_;
    HashMap<std::string, WeakPtr<MetaEntity>> enumValues_;
};

extern GeneratorContext* generator;

}
