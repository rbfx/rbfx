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

#include <regex>
#include <cppast/libclang_parser.hpp>
#include <Urho3D/IO/File.h>
#include <Urho3D/Resource/JSONValue.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Urho3DAll.h>
#include "GeneratorContext.h"
#include "Utilities.h"
#include "CppTypeInfo.h"


namespace Urho3D
{

GeneratorContext::GeneratorContext(Urho3D::Context* context)
    : Object(context)
{

}

bool GeneratorContext::LoadCompileConfig(const String& pathToFile)
{
    File file(context_);
    if (!file.Open(pathToFile))
        return false;

    JSONValue value;
    if (!JSONFile::ParseJSON(file.ReadString(), value))
        return false;

    if (!value.IsArray())
        return false;

    JSONArray array = value.GetArray();

    if (array.Size() < 1)
        return false;

    JSONValue entry = value.GetArray().At(0);

    if (!entry.IsObject())
        return false;

    JSONValue command;
    if (!entry.GetObject().TryGetValue("command", command))
        return false;

    if (!command.IsString())
        return false;

    std::string commandStr = command.GetCString();
    std::regex re(R"( +(?=(?:[^"]*"[^"]*")*[^"]*$))");
    for (std::sregex_token_iterator it(commandStr.begin(), commandStr.end(), re, -1), end ; it != end; ++it)
    {
        String parameter = std::string(*it);
        if (parameter.StartsWith("-D"))
        {
            String name, value;
            unsigned eqIndex = parameter.Find("=");
            if (eqIndex == String::NPOS)
                name = parameter.Substring(2);
            else
            {
                name = parameter.Substring(2, eqIndex - 2);
                value = parameter.Substring(eqIndex + 1);
            }
            config_.define_macro(name.CString(), value.CString());
        }
        else if (parameter.StartsWith("-I"))
            config_.add_include_dir(parameter.Substring(2).CString());
    }

    return true;
}

bool GeneratorContext::LoadRules(const String& xmlPath)
{
    rules_ = new XMLFile(context_);
    return rules_->LoadFile(xmlPath);
}

bool GeneratorContext::ParseFiles(const String& sourceDir)
{
    sourceDir_ = AddTrailingSlash(sourceDir);
    Vector<String> sourceFiles;
    GetFileSystem()->ScanDir(sourceFiles, sourceDir_, "", SCAN_FILES, true);

    cppast::stderr_diagnostic_logger logger;
    // the entity index is used to resolve cross references in the AST
    // we don't need that, so it will not be needed afterwards
    cppast::cpp_entity_index idx;
    // the parser is used to parse the entity
    // there can be multiple parser implementations
    cppast::libclang_parser parser(type_safe::ref(logger));

    IncludedChecker checker(rules_->GetRoot().GetChild("headers"));
    for (const auto& filePath : sourceFiles)
    {
        if (!checker.IsIncluded(filePath))
            continue;

        auto file = parser.parse(idx, (sourceDir_ + filePath).CString(), config_);
        if (parser.error())
        {
            URHO3D_LOGERRORF("Failed parsing %s", filePath.CString());
            parser.reset_error();
        }
        else
            parsed_[filePath] = std::move(file);
    }

    // Load typemaps
    XMLElement typeMaps = rules_->GetRoot().GetChild("typemaps");
    for (auto typeMap = typeMaps.GetChild("typemap"); typeMap.NotNull(); typeMap = typeMap.GetNext("typemap"))
    {
        TypeMap map;
        map.cType = typeMap.GetChild("c").GetValue(),
        map.cppType = typeMap.GetChild("cpp").GetValue(),
        map.csType = map.csPInvokeType = typeMap.GetChild("cs").GetValue(),
        map.pInvokeAttribute = typeMap.GetChild("cs").GetAttribute("pinvoke");
        typeMaps_.Push(map);
    }

    XMLElement types = rules_->GetRoot().GetChild("types");
    for (auto manual = types.GetChild("manual"); manual.NotNull(); manual = manual.GetNext("manual"))
        manualTypes_.Push(manual.GetValue());

    return true;
}

void GeneratorContext::Generate(const String& outputDir)
{
    outputDir_ = outputDir;

    for (const auto& pass : passes_)
    {
        URHO3D_LOGINFOF("#### Run pass: %s", pass->GetTypeName().CString());
        pass->Start();
        for (const auto& pair : parsed_)
        {
            pass->StartFile(pair.first);
            cppast::visit(*pair.second, [&](const cppast::cpp_entity& e, cppast::visitor_info info) {
                if (e.kind() == cppast::cpp_entity_kind::file_t || cppast::is_templated(e) || cppast::is_friended(e))
                    // no need to do anything for a file,
                    // templated and friended entities are just proxies, so skip those as well
                    // return true to continue visit for children
                    return true;

                if (GetUserData(e)->generated)
                    return pass->Visit(e, info);
                else if (info.event == cppast::visitor_info::container_entity_enter)
                    // Ignore children of ignored nodes.
                    return false;
                return true;
            });
            pass->StopFile(pair.first);
        }
        pass->Stop();
    }

}

void GeneratorContext::RegisterKnownType(const String& name, const cppast::cpp_entity& e)
{
    if (!types_.Contains(name))
    {
        types_[name] = &e;
        URHO3D_LOGDEBUGF("Known type: %s", GetSymbolName(e).CString());
    }
}

const cppast::cpp_entity* GeneratorContext::GetKnownType(const String& name)
{
    const cppast::cpp_entity* result = nullptr;
    if (types_.TryGetValue(name, result))
        return result;
    return nullptr;
}

bool GeneratorContext::IsKnownType(const cppast::cpp_type& type)
{
    if (type.kind() == cppast::cpp_type_kind::builtin_t)
        return true;

    CppTypeInfo info(type);
    if (info)
        return IsKnownType(info.name_);

    return false;
}

bool GeneratorContext::IsKnownType(const String& name)
{
    String convertedName = name.Replaced(".", "::");

    if (types_.Contains(convertedName))
        return true;

    if (manualTypes_.Contains(convertedName))
        return true;

    return false;
}

TypeMap GeneratorContext::GetTypeMap(const cppast::cpp_type& type)
{
    CppTypeInfo info(type);

    for (const auto& map : typeMaps_)
    {
        if (map.cppType == info.name_ || map.cppType == info.fullName_)
            return map;
    }

    return TypeMap{type};
}

bool GeneratorContext::IsSubclassOf(const cppast::cpp_class& cls, const String& baseName)
{
    if (GetSymbolName(cls) == baseName)
        return true;

    for (const auto& base : cls.bases())
    {
        auto baseName2 = cppast::to_string(base.type());
        if (const auto* baseCls = dynamic_cast<const cppast::cpp_class*>(GetKnownType(baseName2)))
        {
            if (IsSubclassOf(*baseCls, baseName))
                return true;
        }
    }
    return false;
}

TypeMap::TypeMap(const cppast::cpp_type& type)
{
    const CppTypeInfo info(type);
    cType = info.fullName_;
    cppType = info.fullName_;

    static HashMap<String, String> cppToCS = {
        {"char", "char"},
        {"unsigned char", "byte"},
        {"short", "short"},
        {"unsigned short", "ushort"},
        {"int", "int"},
        {"unsigned int", "uint"},
        {"long long", "long"},
        {"unsigned long long", "ulong"},
        {"void", "void"},
        {"void*", "IntPtr"},
        {"bool", "bool"},
        {"float", "float"},
        {"double", "double"},
    };

    if (csType.Empty() && type.kind() == cppast::cpp_type_kind::builtin_t)
    {
        auto it = cppToCS.Find(info.name_);
        assert(it != cppToCS.End());
        assert(!info.pointer_);         // Handle later if needed
        csType = csPInvokeType = it->second_;
    }
    else
        csType = info.name_.Replaced("::", ".");
}

String TypeMap::GetPInvokeType(bool forReturn)
{
    if (pInvokeAttribute.Empty())
        return csPInvokeType;
    else if (forReturn)
        return csType;
    else
        return "[param: MarshalAs(" + pInvokeAttribute + ")]" + csType;
}

}
