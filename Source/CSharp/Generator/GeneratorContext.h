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

#pragma once


#include <string>
#include <map>
#include <memory>
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
    std::string customMarshaller_;
    bool isValueType_ = false;
};

struct NamespaceRules
{
    struct ParsePath
    {
        std::string path_;
        IncludedChecker checker_;
    };

    std::string defaultNamespace_;
    std::vector<ParsePath> parsePaths_;
    IncludedChecker inheritable_;
    IncludedChecker symbolChecker_;
    std::map<std::string, std::shared_ptr<cppast::cpp_file>> parsed_;
    std::shared_ptr<MetaEntity> apiRoot_;
    std::vector<std::string> includes_;
    std::vector<std::pair<std::string, std::string>> sourceFiles_;
};

class GeneratorContext
{
public:
    explicit GeneratorContext();

    bool AddModule(const std::string& sourceDir, const std::string& outputDir, const std::vector<std::string>& includes,
                   const std::vector<std::string>& defines, const std::vector<std::string>& options,
                   const std::string& rulesFile);
    bool IsOutOfDate(const std::string& generatorExe);

    template<typename T, typename... Args>
    void AddCppPass(Args... args) { cppPasses_.emplace_back(std::move(std::unique_ptr<CppAstPass>(dynamic_cast<CppAstPass*>(new T(args...))))); }
    template<typename T, typename... Args>
    void AddApiPass(Args... args) { apiPasses_.emplace_back(std::move(std::unique_ptr<CppApiPass>(dynamic_cast<CppApiPass*>(new T(args...))))); }
    void Generate();
    bool IsAcceptableType(const cppast::cpp_type& type);
    const TypeMap* GetTypeMap(const cppast::cpp_type& type, bool strict=false);
    const TypeMap* GetTypeMap(const std::string& typeName);
    template<typename T>
    T* GetPass()
    {
        auto& id = typeid(T);
        for (const auto& pass : apiPasses_)
        {
            auto& passRef = *pass;
            auto& passId = typeid(passRef);
            if (id == passId)
                return (T*)pass.get();
        }
        return nullptr;
    }
    bool GetSymbolOfConstant(MetaEntity* user, const std::string& constant, std::string& result,
                             MetaEntity** constantEntity=nullptr);
    MetaEntity* GetSymbol(const char* symbolName) { return GetSymbol(std::string(symbolName)); }
    MetaEntity* GetSymbol(const std::string& symbolName);
    bool IsInheritable(const std::string& symbolName) const;

    struct Module
    {
        std::string sourceDir_;
        std::string outputDir_;
        std::string outputDirCpp_;
        std::string outputDirCs_;
        std::vector<NamespaceRules> rules_;
        std::string moduleName_;
        std::vector<std::string> extraMonoCallInitializers_;
        cppast::libclang_compile_config config_;
        std::string rulesFile_;
    };
    bool isStatic_ = false;
    std::vector<Module> modules_;
    NamespaceRules* currentNamespace_ = nullptr;
    Module* currentModule_ = nullptr;
    cppast::cpp_entity_index index_;
    std::vector<std::unique_ptr<CppAstPass>> cppPasses_;
    std::vector<std::unique_ptr<CppApiPass>> apiPasses_;
    std::unordered_map<std::string, std::weak_ptr<MetaEntity>> enumValues_;
    std::unordered_map<std::string, std::weak_ptr<MetaEntity>> symbols_;
    std::unordered_map<std::string, std::string> defaultValueRemaps_;
    std::vector<std::string> forceCompileTimeConstants_;
    std::unordered_map<std::string, TypeMap> typeMaps_;
};

extern GeneratorContext* generator;

}
