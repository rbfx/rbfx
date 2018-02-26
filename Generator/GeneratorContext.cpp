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
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Core/Thread.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/JSONValue.h>
#include "Declarations/Namespace.hpp"
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

void GeneratorContext::LoadCompileConfig(const std::vector<std::string>& includes, std::vector<std::string>& defines,
    const std::vector<std::string>& options)
{
    for (const auto& item : includes)
        config_.add_include_dir(item);

    for (const String item : defines)
    {
        auto parts = item.Split('=');
        if (parts.Contains("="))
        {
            assert(parts.Size() == 2);
            config_.define_macro(parts[0].CString(), parts[1].CString());
        }
        else
            config_.define_macro(item.CString(), "");
    }
}

bool GeneratorContext::LoadRules(const String& jsonPath)
{
    rules_ = new JSONFile(context_);
    return rules_->LoadFile(jsonPath);
}

bool GeneratorContext::ParseFiles(const String& sourceDir)
{
    typeMapper_.Load(rules_->GetRoot());

    sourceDir_ = AddTrailingSlash(sourceDir);

    IncludedChecker checker(rules_->GetRoot().Get("headers"));

    auto parseFiles = [&](const String& subdir)
    {
        Vector<String> sourceFiles;
        GetFileSystem()->ScanDir(sourceFiles, sourceDir_ + subdir, "", SCAN_FILES, true);
        Mutex m;

        auto workItem = [&](String absPath, String filePath) {
            URHO3D_LOGDEBUGF("Parse: %s", filePath.CString());

            cppast::stderr_diagnostic_logger logger;
            // the parser is used to parse the entity
            // there can be multiple parser implementations
            cppast::libclang_parser parser(type_safe::ref(logger));

            auto file = parser.parse(index_, absPath.CString(), config_);
            if (parser.error())
            {
                URHO3D_LOGERRORF("Failed parsing %s", filePath.CString());
                parser.reset_error();
            }
            else
            {
                MutexLock scoped(m);
                parsed_[absPath] = std::move(file);
            }

            // Ensures log messages are displayed.
            if (Thread::IsMainThread())
                SendEvent(E_ENDFRAME);
        };

        for (const auto& filePath : sourceFiles)
        {
            if (!checker.IsIncluded(filePath))
                continue;

            String absPath = sourceDir_ + subdir + filePath;
            GetWorkQueue()->AddWorkItem(std::bind(workItem, absPath, filePath));
        }

        GetWorkQueue()->Complete(0);
        SendEvent(E_ENDFRAME);            // Ensures log messages are displayed.
    };

    parseFiles("../ThirdParty/");
    parseFiles("");

    return true;
}

void GeneratorContext::Generate(const String& outputDirCpp, const String& outputDirCs)
{
    outputDirCpp_ = outputDirCpp;
    outputDirCs_ = outputDirCs;

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
                return pass->Visit(e, info);
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
    // Builtins map directly to c# types
    if (type.kind() == cppast::cpp_type_kind::builtin_t)
        return true;

    // Some non-builtin types also map to c# types (like some pointers)
    if (!typeMapper_.ToPInvokeType(cppast::to_string(type), "").empty())
        return true;

    if (!typeMapper_.ToPInvokeType(Urho3D::GetTypeName(type), "").empty())
        return true;

    // Known symbols will be classes that are being wrapped
    if (symbols_.Has(type))
        return true;

    // Any other manually handled types
    return typeMapper_.GetTypeMap(type) != nullptr;

}

}
