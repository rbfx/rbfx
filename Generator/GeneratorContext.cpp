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
#include "GeneratorContext.h"
#include "Utilities.h"


namespace Urho3D
{

GeneratorContext::GeneratorContext(Urho3D::Context* context)
    : Object(context)
    , typeMapper_(context)
{
    apiRoot_ = new MetaEntity();
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
    if (!rules_->LoadFile(jsonPath))
        return false;

    const auto& final = rules_->GetRoot().Get("final").GetArray();
    for (auto i = 0; i < final.Size(); i++)
        final_.Push(final[i].GetCString());

    return true;
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

    std::function<void(CppApiPass*, MetaEntity*)> visitOverlayEntity = [&](CppApiPass* pass, MetaEntity* entity)
    {
        cppast::visitor_info info{};
        info.access = entity->access_;
        info.event = entity->children_.empty() ? cppast::visitor_info::leaf_entity :
                     cppast::visitor_info::container_entity_enter;

        if (pass->Visit(entity, info) && !entity->children_.empty())
        {
            std::vector<SharedPtr<MetaEntity>> childrenCopy = entity->children_;
            for (const auto& childEntity : childrenCopy)
                visitOverlayEntity(pass, childEntity.Get());

            info.event = cppast::visitor_info::container_entity_exit;
            pass->Visit(entity, info);
        }
    };
    for (const auto& pass : apiPasses_)
    {
        URHO3D_LOGINFOF("#### Run pass: %s", pass->GetTypeName().CString());
        pass->Start();
        visitOverlayEntity(pass, apiRoot_);
        pass->Stop();
    }
}

bool GeneratorContext::IsAcceptableType(const cppast::cpp_type& type)
{
    // Builtins map directly to c# types
    if (type.kind() == cppast::cpp_type_kind::builtin_t)
        return true;

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

    // Manually handled types
    if (typeMapper_.GetTypeMap(type) != nullptr)
        return true;

    // Known symbols will be classes that are being wrapped
    return symbols_.Contains(cppast::to_string(GetBaseType(type)));
}

}
