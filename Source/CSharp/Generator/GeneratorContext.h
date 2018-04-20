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


#include <string>
#include <map>
#include <unordered_map>
#include <cppast/cpp_entity.hpp>
#include <cppast/libclang_parser.hpp>
#include <rapidjson/document.h>
#include "Pass/CppPass.h"


namespace Urho3D
{

struct TypeMap
{
    std::string cppType_ = "void*";
    std::string cType_ = "";
    std::string csType_ = "";
    std::string pInvokeType_ = "IntPtr";
    std::string cToCppTemplate_ = "{value}";
    std::string cppToCTemplate_ = "{value}";
    std::string csToPInvokeTemplate_ = "{value}";
    std::string pInvokeToCSTemplate_ = "{value}";
    std::string marshalAttribute_;
    bool isValueType_ = false;
};

class GeneratorContext
{
public:
    explicit GeneratorContext();
    void LoadCompileConfig(const std::vector<std::string>& includes, std::vector<std::string>& defines,
        const std::vector<std::string>& options);

    bool LoadRules(const std::string& jsonPath);
    bool ParseFiles(const std::string& sourceDir);
    template<typename T, typename... Args>
    void AddCppPass(Args... args) { cppPasses_.emplace_back(std::move(std::unique_ptr<CppAstPass>(dynamic_cast<CppAstPass*>(new T(args...))))); }
    template<typename T, typename... Args>
    void AddApiPass(Args... args) { apiPasses_.emplace_back(std::move(std::unique_ptr<CppApiPass>(dynamic_cast<CppApiPass*>(new T(args...))))); }
    void Generate(const std::string& outputDirCpp, const std::string& outputDirCs);
    bool IsAcceptableType(const cppast::cpp_type& type);
    const TypeMap* GetTypeMap(const cppast::cpp_type& type, bool strict=false);
    const TypeMap* GetTypeMap(const std::string& typeName);
    template<typename T>
    T* GetPass()
    {
        auto& id = typeid(T);
        for (const auto& pass : apiPasses_)
        {
            auto& passId = typeid(*pass.get());
            if (id == passId)
                return (T*)pass.get();
        }
        return nullptr;
    }
    MetaEntity* GetEntityOfConstant(MetaEntity* user, const std::string& constant);
    MetaEntity* GetSymbol(const char* symbolName) { return GetSymbol(std::string(symbolName)); }
    MetaEntity* GetSymbol(const std::string& symbolName);

    std::string sourceDir_;
    std::string outputDirCpp_;
    std::string outputDirCs_;
    rapidjson::Document rules_;
    cppast::libclang_compile_config config_;
    std::map<std::string, std::unique_ptr<cppast::cpp_file>> parsed_;
    std::vector<std::unique_ptr<CppAstPass>> cppPasses_;
    std::vector<std::unique_ptr<CppApiPass>> apiPasses_;
    std::shared_ptr<MetaEntity> apiRoot_;
    cppast::cpp_entity_index index_;
    std::string defaultNamespace_ = "Urho3D";
    std::unordered_map<std::string, std::weak_ptr<MetaEntity>> symbols_;
    std::unordered_map<std::string, std::weak_ptr<MetaEntity>> enumValues_;
    std::unordered_map<std::string, TypeMap> typeMaps_;
    IncludedChecker inheritable_;
    bool isStatic_ = false;
    std::vector<std::string> extraMonoCallInitializers_;
};

extern GeneratorContext* generator;

}
