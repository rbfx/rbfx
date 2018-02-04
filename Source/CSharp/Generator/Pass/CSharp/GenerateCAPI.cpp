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

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/File.h>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_member_function.hpp>
#include "GenerateCAPI.h"
#include "GeneratorContext.h"


namespace Urho3D
{

void GenerateCAPI::Start()
{
    URHO3D_LOGDEBUGF("~~~~~ GenerateCAPI ~~~~~");

    printer_ << "#include <Urho3D/Urho3DAll.h>";
    printer_ << "#include \"CSharp.h\"";
    printer_ << "";
    printer_ << "extern \"C\"";
    printer_ << "{";
    printer_ << "";
}

bool GenerateCAPI::Visit(const cppast::cpp_entity& e, cppast::visitor_info info)
{
    auto generator = GetSubsystem<GeneratorContext>();

    if (info.event == cppast::visitor_info::container_entity_exit)
        return true;

    if (e.kind() == cppast::cpp_entity_kind::function_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_function&>(e);
        String symbolName = GetSymbolName(e);
        UserData* data = GetUserData(e);
        data->cFunctionName = GetUniqueName(Sanitize(symbolName));

        printer_ < "URHO3D_EXPORT_API " < generator->MapToCType(func.return_type()) < " " < data->cFunctionName < "(";

        bool isFirst = true;
        for (const auto& param : func.parameters())
        {
            if (isFirst)
                isFirst = false;
            else
                printer_ < ", ";
            printer_ < generator->MapToCType(param.type()) < " " < param.name();
        }

        printer_ < ")";
        printer_.Indent();
        {
            if (!IsVoid(func.return_type()))
                printer_ < "return ToCSharp(";
            printer_ < symbolName < "(";
            if (!func.parameters().empty())
            {
                printer_.Indent("");
                {
                    isFirst = true;
                    for (const auto& param : func.parameters())
                    {
                        if (isFirst)
                            isFirst = false;
                        else
                            printer_ < ", ";
                        printer_ < "FromCSharp(" < param.name() < ")";
                    }
                }
                printer_.Dedent("");
            }
            if (!IsVoid(func.return_type()))
                printer_ < ")";
            printer_ < ");";
        }
        printer_.Dedent();
        printer_.WriteLine();
    }
    else if (e.kind() == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_member_function&>(e);

        // Skip constructors and destructors. RefCounted can be created by factory.
        if (IsConstructor(e) || IsDestructor(e))
            return true;

        String symbolName = GetSymbolName(e);
        UserData* data = GetUserData(e);
        data->cFunctionName = GetUniqueName(Sanitize(symbolName));

        printer_ < "URHO3D_EXPORT_API " < generator->MapToCType(func.return_type()) < " " < data->cFunctionName < "(";
        printer_ < dynamic_cast<const cppast::cpp_class&>(e.parent().value()).name() < "* cls";
        for (const auto& param : func.parameters())
            printer_ < ", " < generator->MapToCType(param.type()) < " " < param.name();

        printer_ < ")";
        printer_.Indent();
        {
            if (!IsVoid(func.return_type()))
                printer_ < "return ToCSharp(";
            printer_ < "cls->" < e.name() < "(";
            if (!func.parameters().empty())
            {
                printer_.Indent("");
                {
                    bool isFirst = true;
                    for (const auto& param : func.parameters())
                    {
                        if (isFirst)
                            isFirst = false;
                        else
                        {
                            printer_ < ", ";
                            printer_.Flush();
                        }
                        printer_ < "FromCSharp(" < param.name() < ")";
                    }
                }
                printer_.Dedent("");
            }
            if (!IsVoid(func.return_type()))
                printer_ < ")";
            printer_ < ");";
        }
        printer_.Dedent();
        printer_.WriteLine();
    }

    return true;
}

void GenerateCAPI::Stop()
{
    // Close extern "C"
    printer_ << "}";

    File file(context_, GetSubsystem<GeneratorContext>()->GetOutputDir() + "CApi.cpp", FILE_WRITE);
    if (!file.IsOpen())
    {
        URHO3D_LOGERROR("Failed saving CApi.cpp");
        return;
    }
    file.WriteLine(printer_.Get());
    file.Close();
}

String GenerateCAPI::GetUniqueName(const String& baseName)
{
    unsigned index = 0;
    String newName;
    for (newName = Sanitize(baseName); usedNames_.Contains(newName); index++)
        newName = baseName + String(index);

    usedNames_.Push(newName);
    return newName;
}

}
