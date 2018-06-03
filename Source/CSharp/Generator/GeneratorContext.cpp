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

#include <regex>
#if __GNUC__
#include <cxxabi.h>
#endif
#include <cppast/libclang_parser.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <thread>
#include <chrono>
#include "GeneratorContext.h"
#include "Utilities.h"


namespace Urho3D
{

GeneratorContext::GeneratorContext()
{
}

bool GeneratorContext::AddModule(const std::string& sourceDir, const std::string& outputDir,
                                 const std::vector<std::string>& includes, const std::vector<std::string>& defines,
                                 const std::vector<std::string>& options, const std::string& rulesFile)
{
    modules_.resize(modules_.size() + 1);
    Module& m = modules_.back();

    m.sourceDir_ = str::AddTrailingSlash(sourceDir);
    m.outputDir_ = str::AddTrailingSlash(outputDir);
    m.outputDirCs_ = m.outputDir_ + "CSharp/";
    m.outputDirCpp_ = m.outputDir_ + "Native/";
    m.rulesFile_ = rulesFile;

    Urho3D::CreateDirsRecursive(m.outputDirCpp_);
    Urho3D::CreateDirsRecursive(m.outputDirCs_);

    // Load compiler config
    {
        for (const auto& item : includes)
            m.config_.add_include_dir(item);

        for (const auto& item : defines)
        {
            auto parts = str::split(item, "=");
            if (std::find(parts.begin(), parts.end(), "=") != parts.end())
            {
                assert(parts.size() == 2);
                m.config_.define_macro(parts[0], parts[1]);
            }
            else
                m.config_.define_macro(item, "");
        }

#if _WIN32
        m.config_.set_flags(cppast::cpp_standard::cpp_14, {
            cppast::compile_flag::ms_compatibility | cppast::compile_flag::ms_extensions
        });
#else
        m.config_.set_flags(cppast::cpp_standard::cpp_11, {cppast::compile_flag::gnu_extensions});
#endif
    }

    // Load module rules
    {
        std::ifstream fp(rulesFile);
        std::stringstream buffer;
        buffer << fp.rdbuf();
        auto json = buffer.str();
        rapidjson::Document jsonRules;
        jsonRules.Parse<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag>(json.c_str(), json.length());

        if (!jsonRules.IsObject())
            return false;

        // Module options
        {
            m.moduleName_ = jsonRules["module"].GetString();

            if (jsonRules.HasMember("default-values"))
            {
                const auto& defaults = jsonRules["default-values"];
                for (auto it = defaults.MemberBegin(); it != defaults.MemberEnd(); it++)
                {
                    std::string value;
                    if (it->value.IsObject())
                    {
                        value = it->value["value"].GetString();
                        if (it->value.HasMember("const") && it->value["const"].GetBool())
                            forceCompileTimeConstants_.emplace_back(value);
                    }
                    else
                        value = it->value.GetString();
                    defaultValueRemaps_[it->name.GetString()] = value;
                }
            }
        }

        // Namespace rules
        auto& namespaces = jsonRules["namespaces"];
        for (auto it = namespaces.MemberBegin(); it != namespaces.MemberEnd(); it++)
        {
            NamespaceRules parserRules{};
            auto& nsRules = it->value;

            parserRules.defaultNamespace_ = it->name.GetString();
            if (nsRules.HasMember("inheritable"))
                parserRules.inheritable_.Load(nsRules["inheritable"]);

            const auto& parse = nsRules["parse"];
            assert(parse.IsObject());

            for (auto jt = parse.MemberBegin(); jt != parse.MemberEnd(); jt++)
            {
                NamespaceRules::ParsePath parsePath{};
                parsePath.path_ = str::AddTrailingSlash(m.sourceDir_ + jt->name.GetString());
                parsePath.checker_.Load(jt->value);
                parserRules.parsePaths_.emplace_back(std::move(parsePath));
            }

            if (nsRules.HasMember("symbols"))
            {
                parserRules.symbolChecker_.Load(nsRules["symbols"]);
            }

            if (nsRules.HasMember("include"))
            {
                const auto& includes = nsRules["include"];
                for (auto jt = includes.Begin(); jt != includes.End(); jt++)
                    parserRules.includes_.emplace_back(jt->GetString());
            }

            m.rules_.emplace_back(std::move(parserRules));
        }

        // typemaps
        if (jsonRules.HasMember("typemaps"))
        {
            auto& typeMaps = jsonRules["typemaps"];

            // Typemap const char* strings
            {
                rapidjson::Value stringMap(rapidjson::kObjectType);
                stringMap.AddMember("type", "char const*", jsonRules.GetAllocator());
                stringMap.AddMember("ptype", "string", jsonRules.GetAllocator());
                stringMap.AddMember("cstype", "string", jsonRules.GetAllocator());
                stringMap.AddMember("cpp_to_c", "{value}", jsonRules.GetAllocator());
                stringMap.AddMember("is_value_type", true, jsonRules.GetAllocator());
                typeMaps.PushBack(stringMap, jsonRules.GetAllocator());
            }

            std::function<void(rapidjson::Value&, const std::string&)> parseTypemaps = [&](
                rapidjson::Value& typeMaps, const std::string& jsonPath) {
                for (auto jt = typeMaps.Begin(); jt != typeMaps.End(); ++jt)
                {
                    const auto& typeMap = *jt;

                    if (typeMap.GetType() == rapidjson::kStringType)
                    {
                        // String contains a path relative to config json file. This file defines array of shared typemaps
                        std::string newJsonPath;
                        auto slashPos = jsonPath.rfind("/");
                        if (slashPos != std::string::npos)
                            newJsonPath = jsonPath.substr(0, slashPos + 1) + typeMap.GetString();
                        else
                            newJsonPath = typeMap.GetString();

                        std::ifstream newFp(newJsonPath);
                        std::stringstream newBuffer;
                        newBuffer << newFp.rdbuf();
                        auto newJson = newBuffer.str();
                        rapidjson::Document newJsonRules;
                        newJsonRules.Parse<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag>(
                            newJson.c_str(), newJson.length());

                        parseTypemaps(newJsonRules, newJsonPath);
                    }
                    else if (typeMap.GetType() == rapidjson::kObjectType)
                    {
                        // Object denotes typemaps themselves
                        TypeMap map;
                        map.cppType_ = typeMap["type"].GetString();
                        if (typeMap.HasMember("ctype"))
                            map.cType_ = typeMap["ctype"].GetString();
                        if (typeMap.HasMember("cstype"))
                            map.csType_ = typeMap["cstype"].GetString();
                        map.pInvokeType_ = typeMap["ptype"].GetString();
                        map.isValueType_ = typeMap.HasMember("is_value_type") && typeMap["is_value_type"].GetBool();

                        if (map.cType_.empty())
                            map.cType_ = map.cppType_;

                        if (map.csType_.empty())
                            map.csType_ = map.pInvokeType_;

                        if (typeMap.HasMember("cpp_to_c"))
                            map.cppToCTemplate_ = typeMap["cpp_to_c"].GetString();

                        if (typeMap.HasMember("c_to_cpp"))
                            map.cToCppTemplate_ = typeMap["c_to_cpp"].GetString();

                        if (typeMap.HasMember("pinvoke_to_cs"))
                            map.pInvokeToCSTemplate_ = typeMap["pinvoke_to_cs"].GetString();

                        if (typeMap.HasMember("cs_to_pinvoke"))
                            map.csToPInvokeTemplate_ = typeMap["cs_to_pinvoke"].GetString();

                        if (typeMap.HasMember("marshaller"))
                            map.customMarshaller_ = typeMap["marshaller"].GetString();

                        // Doctor string typemaps with some internal details.
                        if (map.csType_ == "string" && map.customMarshaller_.empty())
                        {
                            map.customMarshaller_ = "StringUtf8";
                        }

                        typeMaps_[map.cppType_] = map;
                    }
                    else
                        assert(false);
                }
            };

            parseTypemaps(typeMaps, rulesFile);
        }
    }

    // Gather files that need parsing
    {
        for (auto& nsRules : m.rules_)
        {
            for (const auto& parsePath : nsRules.parsePaths_)
            {
                std::vector<std::pair<std::string, std::string>> sourceFiles;
                std::vector<std::string> scannedFiles;
                if (!ScanDirectory(parsePath.path_, scannedFiles,
                                   ScanDirectoryFlags::IncludeFiles | ScanDirectoryFlags::Recurse, parsePath.path_))
                {
                    spdlog::get("console")->error("Failed to scan directory {}", parsePath.path_);
                    return false;
                }

                // Remove excluded files
                for (auto it = scannedFiles.begin(); it != scannedFiles.end(); it++)
                {
                    if (parsePath.checker_.IsIncluded(*it))
                        sourceFiles.emplace_back(std::pair<std::string, std::string>{parsePath.path_, *it});
                }
                nsRules.sourceFiles_.insert(nsRules.sourceFiles_.end(), sourceFiles.begin(), sourceFiles.end());
            }
        }
    }
    return true;
}

void GeneratorContext::Generate()
{
    auto getNiceName = [](const char* name) -> std::string
    {
#if __GNUC__
        int status;
        return abi::__cxa_demangle(name, 0, 0, &status);
#else
        return name;
#endif
    };

    for (auto& m : modules_)
    {
        currentModule_ = &m;
        for (const auto& pass : cppPasses_)
            pass->Start();

        for (const auto& pass : apiPasses_)
            pass->Start();

        for (auto& nsRules : m.rules_)
        {
            // Parse module headers
            {
                std::mutex lock;
                auto workItem = [&]
                {
                    for (;;)
                    {
                        std::pair<std::string, std::string> filePath;
                        // Initialize
                        {
                            std::unique_lock<std::mutex> scopedLock(lock);
                            if (!nsRules.sourceFiles_.empty())
                            {
                                filePath = nsRules.sourceFiles_.back();
                                nsRules.sourceFiles_.pop_back();
                            }
                            else
                                break;
                        }

                        spdlog::get("console")->debug("Parse: {}", filePath.second);

                        cppast::stderr_diagnostic_logger logger;
                        // the parser is used to parse the entity
                        // there can be multiple parser implementations
                        cppast::libclang_parser parser(type_safe::ref(logger));

                        auto absPath = filePath.first + filePath.second;
                        auto file = parser.parse(index_, absPath, m.config_);
                        if (parser.error())
                        {
                            spdlog::get("console")->error("Failed parsing {}", filePath.second);
                            parser.reset_error();
                        }
                        else
                        {
                            std::unique_lock<std::mutex> scopedLock(lock);
                            nsRules.parsed_[absPath].reset(file.release());
                        }
                    }
                };

                std::vector<std::thread> pool;
                for (auto i = 0; i < std::thread::hardware_concurrency(); i++)
                    pool.emplace_back(std::thread(workItem));

                for (auto& t : pool)
                    t.join();
            }

            nsRules.apiRoot_ = std::shared_ptr<MetaEntity>(new MetaEntity());
            currentNamespace_ = &nsRules;

            for (const auto& pass : cppPasses_)
                pass->NamespaceStart();

            for (const auto& pass : cppPasses_)
            {
                auto& passRef = *pass;
                spdlog::get("console")->info("#### Run pass: {}", getNiceName(typeid(passRef).name()));
                for (const auto& pair : nsRules.parsed_)
                {
                    pass->StartFile(pair.first);
                    cppast::visit(*pair.second, [&](const cppast::cpp_entity& e, cppast::visitor_info info)
                    {
                        if (e.kind() == cppast::cpp_entity_kind::file_t || cppast::is_templated(e) ||
                            cppast::is_friended(e))
                            // no need to do anything for a file,
                            // templated and friended entities are just proxies, so skip those as well
                            // return true to continue visit for children
                            return true;
                        return pass->Visit(e, info);
                    });
                    pass->StopFile(pair.first);
                }
            }

            for (const auto& pass : cppPasses_)
                pass->NamespaceStop();

            std::function<void(CppApiPass*, MetaEntity*)> visitOverlayEntity = [&](CppApiPass* pass, MetaEntity* entity)
            {
                cppast::visitor_info info{ };
                info.access = entity->access_;

                switch (entity->kind_)
                {
                case cppast::cpp_entity_kind::file_t:
                case cppast::cpp_entity_kind::language_linkage_t:
                case cppast::cpp_entity_kind::namespace_t:
                case cppast::cpp_entity_kind::enum_t:
                case cppast::cpp_entity_kind::class_t:
                case cppast::cpp_entity_kind::function_template_t:
                case cppast::cpp_entity_kind::class_template_t:
                    info.event = info.container_entity_enter;
                    break;
                case cppast::cpp_entity_kind::macro_definition_t:
                case cppast::cpp_entity_kind::include_directive_t:
                case cppast::cpp_entity_kind::namespace_alias_t:
                case cppast::cpp_entity_kind::using_directive_t:
                case cppast::cpp_entity_kind::using_declaration_t:
                case cppast::cpp_entity_kind::type_alias_t:
                case cppast::cpp_entity_kind::enum_value_t:
                case cppast::cpp_entity_kind::access_specifier_t: // ?
                case cppast::cpp_entity_kind::base_class_t:
                case cppast::cpp_entity_kind::variable_t:
                case cppast::cpp_entity_kind::member_variable_t:
                case cppast::cpp_entity_kind::bitfield_t:
                case cppast::cpp_entity_kind::function_parameter_t:
                case cppast::cpp_entity_kind::function_t:
                case cppast::cpp_entity_kind::member_function_t:
                case cppast::cpp_entity_kind::conversion_op_t:
                case cppast::cpp_entity_kind::constructor_t:
                case cppast::cpp_entity_kind::destructor_t:
                case cppast::cpp_entity_kind::friend_t:
                case cppast::cpp_entity_kind::template_type_parameter_t:
                case cppast::cpp_entity_kind::non_type_template_parameter_t:
                case cppast::cpp_entity_kind::template_template_parameter_t:
                case cppast::cpp_entity_kind::alias_template_t:
                case cppast::cpp_entity_kind::variable_template_t:
                case cppast::cpp_entity_kind::function_template_specialization_t:
                case cppast::cpp_entity_kind::class_template_specialization_t:
                case cppast::cpp_entity_kind::static_assert_t:
                case cppast::cpp_entity_kind::unexposed_t:
                case cppast::cpp_entity_kind::count:
                    info.event = info.leaf_entity;
                    break;
                }

                if (pass->Visit(entity, info) && info.event == info.container_entity_enter)
                {
                    auto childrenCopy = entity->children_;
                    for (const auto& childEntity : childrenCopy)
                        visitOverlayEntity(pass, childEntity.get());

                    info.event = cppast::visitor_info::container_entity_exit;
                    pass->Visit(entity, info);
                }
            };

            for (const auto& pass : apiPasses_)
                pass->NamespaceStart();

            for (const auto& pass : apiPasses_)
            {
                auto& passRef = *pass;
                spdlog::get("console")->info("#### Run pass: {}", getNiceName(typeid(passRef).name()));
                visitOverlayEntity(pass.get(), nsRules.apiRoot_.get());
            }

            for (const auto& pass : apiPasses_)
                pass->NamespaceStop();
        }

        for (const auto& pass : cppPasses_)
            pass->Stop();

        for (const auto& pass : apiPasses_)
            pass->Stop();

        SetLastModifiedTime(m.outputDir_, 0);   // Touch
        currentNamespace_ = nullptr;
    }
}

bool GeneratorContext::IsAcceptableType(const cppast::cpp_type& type)
{
    // Builtins map directly to c# types
    if (type.kind() == cppast::cpp_type_kind::builtin_t)
        return true;

    // Manually handled types
    if (GetTypeMap(type) != nullptr)
        return true;

    if (type.kind() == cppast::cpp_type_kind::template_instantiation_t)
        return container::contains(symbols_, GetTemplateSubtype(type));

    std::function<bool(const cppast::cpp_type&)> isPInvokable = [&](const cppast::cpp_type& type)
    {
        switch (type.kind())
        {
        case cppast::cpp_type_kind::builtin_t:
        {
            const auto& builtin = dynamic_cast<const cppast::cpp_builtin_type&>(type);
            switch (builtin.builtin_type_kind())
            {
            case cppast::cpp_void:
            case cppast::cpp_bool:
            case cppast::cpp_uchar:
            case cppast::cpp_ushort:
            case cppast::cpp_uint:
            case cppast::cpp_ulong:
            case cppast::cpp_ulonglong:
            case cppast::cpp_schar:
            case cppast::cpp_short:
            case cppast::cpp_int:
            case cppast::cpp_long:
            case cppast::cpp_longlong:
            case cppast::cpp_float:
            case cppast::cpp_double:
            case cppast::cpp_char:
            case cppast::cpp_nullptr:
                return true;
            default:
                break;
            }
            break;
        }
        case cppast::cpp_type_kind::cv_qualified_t:
            return isPInvokable(dynamic_cast<const cppast::cpp_cv_qualified_type&>(type).type());
        case cppast::cpp_type_kind::pointer_t:
            return isPInvokable(dynamic_cast<const cppast::cpp_pointer_type&>(type).pointee());
        case cppast::cpp_type_kind::reference_t:
            return isPInvokable(dynamic_cast<const cppast::cpp_reference_type&>(type).referee());
        default:
            break;
        }
        return false;
    };

    // Some non-builtin types also map to c# types (like some pointers)
    if (isPInvokable(type))
        return true;

    // Known symbols will be classes that are being wrapped
    return container::contains(symbols_, Urho3D::GetTypeName(type));
}

const TypeMap* GeneratorContext::GetTypeMap(const cppast::cpp_type& type, bool strict)
{
    if (auto* map = GetTypeMap(cppast::to_string(type)))
        return map;

    if (!strict)
    {
        if (auto* map = GetTypeMap(cppast::to_string(GetBaseType(type))))
            return map;
    }

    return nullptr;
}

const TypeMap* GeneratorContext::GetTypeMap(const std::string& typeName)
{
    auto it = typeMaps_.find(typeName);
    if (it != typeMaps_.end())
        return &it->second;

    return nullptr;
}

bool GeneratorContext::GetSymbolOfConstant(MetaEntity* user, const std::string& constant, std::string& result,
                                           MetaEntity** constantEntity)
{
    std::string symbol = constant;
    do
    {
        // Remaps are a priority
        auto it = defaultValueRemaps_.find(symbol);
        if (it != defaultValueRemaps_.end())
        {
            result = it->second;
            return true;
        }

        // Existing entities
        if (MetaEntity* entity = generator->GetSymbol(symbol))
        {
            result = entity->symbolName_;
            if (constantEntity != nullptr)
                *constantEntity = entity;
            return true;
        }
        symbol.clear();

        // Get next possible symbol
        while (user != nullptr)
        {
            if (user->kind_ != cppast::cpp_entity_kind::class_t || user->kind_ != cppast::cpp_entity_kind::namespace_t)
            {
                symbol = user->symbolName_ + "::" + constant;
                user = user->GetParent();
                // Check next symbol
                break;
            }
            else
                // Unable to make symbol, try again with parent node
                user = user->GetParent();
        }
        // If making symbol fails loop terminates
    } while (!symbol.empty());

    return false;
}

MetaEntity* GeneratorContext::GetSymbol(const std::string& symbolName)
{
    auto it = symbols_.find(symbolName);
    if (it == symbols_.end())
        return nullptr;
    if (it->second.expired())
        return nullptr;
    return it->second.lock().get();
}

bool GeneratorContext::IsInheritable(const std::string& symbolName) const
{
    assert(currentNamespace_ != nullptr);
    return currentNamespace_->inheritable_.IsIncluded(symbolName);
}

bool GeneratorContext::IsOutOfDate(const std::string& generatorExe)
{
    for (const auto& m : modules_)
    {
        if (GetFileSize(m.outputDirCpp_ + m.moduleName_ + "CApi.cpp") == 0)
            // Missing file, needed for the first time generator runs
            return true;

        auto outputTime = GetLastModifiedTime(m.outputDir_);
        auto exeTime = GetLastModifiedTime(generatorExe);
        auto rulesTime = GetLastModifiedTime(m.rulesFile_);
        if (outputTime == 0 || exeTime == 0 || rulesTime == 0)
            return true;

        if (exeTime > outputTime || rulesTime > outputTime)
            return true;

        for (const auto& nsRules : m.rules_)
        {
            for (const auto& path : nsRules.sourceFiles_)
            {
                auto fileTime = GetLastModifiedTime(path.first + path.second);
                if (fileTime == 0 || fileTime > outputTime)
                    return true;
            }
        }
    }

    return false;
}

}
