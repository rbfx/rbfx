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
#include <cppast/cpp_entity.hpp>
#include <cppast/libclang_parser.hpp>
#include <Urho3D/Resource/XMLFile.h>
#include "Pass/ParserPass.h"


namespace Urho3D
{

struct UserData
{
    /// Setting this to false ignores node.
    bool generated = true;
    /// Wrapper classes faciliate protected member access and virtual method overriding.
    bool hasWrapperClass = false;
    /// Full name for methods, base name for getters/setters, prepend get_ or set_ for full name.
    String cFunctionName;
    /// Cache access info so that we have this info when iterating over children nodes.
    cppast::cpp_access_specifier_kind access = cppast::cpp_access_specifier_kind::cpp_public;
};

struct TypeMap
{
    String cType;
    String cppType;
    String csType;
    String pInvokeAttribute;
};

class GeneratorContext
    : public Object
{
    URHO3D_OBJECT(GeneratorContext, Object);
public:
    explicit GeneratorContext(Context* context);
    bool LoadCompileConfig(const String& pathToFile);
    cppast::libclang_compile_config& GetCompilerConfig() { return config_; }
    String GetSourceDir() const { return sourceDir_; }
    String GetOutputDir() const { return outputDir_; }
    XMLFile* GetRules() const { return rules_; }

    bool LoadRules(const String& xmlPath);
    bool ParseFiles(const String& sourceDir);
    template<typename T, typename... Args>
    void AddPass(Args... args) { passes_.Push(DynamicCast<ParserPass>(SharedPtr<T>(new T(context_, args...)))); }
    void Generate(const String& outputDir);

    void RegisterKnownType(const String& name, const cppast::cpp_entity& e);
    const cppast::cpp_entity* GetKnownType(const String& name);
    bool IsKnownType(const cppast::cpp_type& type);
    bool IsKnownType(const String& name);
    TypeMap GetTypeMap(const cppast::cpp_type& type);
    String GetCSType(const cppast::cpp_type& type, bool pInvoke=false);
    bool IsSubclassOf(const cppast::cpp_class& cls, const String& baseName);

protected:
    String sourceDir_;
    String outputDir_;
    SharedPtr<XMLFile> rules_;
    cppast::libclang_compile_config config_;
    HashMap<String, const cppast::cpp_entity*> types_;
    std::map<String, std::unique_ptr<cppast::cpp_file>> parsed_;
    Vector<SharedPtr<ParserPass>> passes_;
    Vector<TypeMap> typeMaps_;
    Vector<String> manualTypes_;
};

}
