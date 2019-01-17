//
// Copyright (c) 2017-2019 Rokas Kupstys.
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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include "Project.h"
#include "GlobResources.h"


namespace Urho3D
{

GlobResources::GlobResources(Context* context)
    : Converter(context)
{
}

void GlobResources::RegisterObject(Context* context)
{
    context->RegisterFactory<GlobResources>();
    URHO3D_COPY_BASE_ATTRIBUTES(Converter);
    URHO3D_ATTRIBUTE_EX("glob", StringVector, glob_, ConvertGlobToRegex, {}, AM_DEFAULT);
}

void GlobResources::Execute(const StringVector& input)
{
    StringVector results = input;
    for (auto it = results.Begin(); it != results.End();)
    {
        bool matched = false;
        for (const std::regex& regex : regex_)
        {
            if (std::regex_match(it->CString(), regex))
            {
                matched = true;
                break;
            }
        }

        if (matched)
            ++it;
        else
            it = results.Erase(it);
    }

    Converter::Execute(results);
}

void GlobResources::ConvertGlobToRegex()
{
    auto globToRegex = [](const String& expression) {
        String regex = expression;
        regex.Replace("^", "\\^");
        regex.Replace("$", "\\$");
        regex.Replace("{", "\\{");
        regex.Replace("}", "\\}");
        regex.Replace("[", "\\[");
        regex.Replace("]", "\\]");
        regex.Replace("(", "\\(");
        regex.Replace(")", "\\)");
        regex.Replace(".", "\\.");
        regex.Replace("+", "\\+");
        regex.Replace("?", "\\?");
        regex.Replace("<", "\\<");
        regex.Replace(">", "\\>");
        regex.Replace("*", "@");
        regex.Replace("@@/", "(.*/)?");
        regex.Replace("@", "[^/]*");
        return std::regex(regex.CString());
    };

    for (const String& glob : glob_)
        regex_.Push(globToRegex(glob));
}

}
