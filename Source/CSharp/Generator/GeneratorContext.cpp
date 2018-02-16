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
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/JSONValue.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Declarations/Namespace.hpp>
#include "GeneratorContext.h"
#include "Utilities.h"


namespace Urho3D
{

GeneratorContext::GeneratorContext(Urho3D::Context* context)
    : Object(context)
    , typeMapper_(context)
    , apiRoot_(new Namespace(nullptr))
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
    typeMapper_.Load(rules_);

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

    return true;
}

void GeneratorContext::Generate(const String& outputDir)
{
    outputDir_ = outputDir;

    for (const auto& pass : cppPasses_)
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

    std::function<void(CppApiPass*, Declaration*)> visitDeclaration = [&](CppApiPass* pass, Declaration* decl) {
        if (decl->IsNamespaceLike())
        {
            Namespace* ns = dynamic_cast<Namespace*>(decl);
            pass->Visit(decl, CppApiPass::ENTER);
            // Passes may alter API structure while they are running. Copying children list ensures that we are
            // iterating over container that is guaranteed to not be modified.
            auto childrenCopy = ns->children_;
            for (const auto& child : childrenCopy)
                visitDeclaration(pass, child);
            pass->Visit(decl, CppApiPass::EXIT);
        }
        else
            pass->Visit(decl, CppApiPass::LEAF);
    };

    for (const auto& pass : apiPasses_)
    {
        URHO3D_LOGINFOF("#### Run pass: %s", pass->GetTypeName().CString());
        pass->Start();
        visitDeclaration(pass, apiRoot_);
        pass->Stop();
    }
}

bool GeneratorContext::IsAcceptableType(const cppast::cpp_type& type)
{
    if (type.kind() == cppast::cpp_type_kind::builtin_t)
        return true;

    if (types_.Has(type))
        return true;

    return typeMapper_.GetTypeMap(type) != nullptr;

}

}
