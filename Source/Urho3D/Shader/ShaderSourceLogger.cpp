//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "Urho3D/Precompiled.h"

#include "Urho3D/Shader/ShaderSourceLogger.h"

#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/IO/VirtualFileSystem.h"

namespace Urho3D
{

void LogShaderSource(const FileIdentifier& fileName, ea::string_view defines, ea::string_view source)
{
    auto context = Context::GetInstance();
    auto graphics = context->GetSubsystem<Graphics>();
    if (!graphics->GetSettings().logShaderSources_)
        return;

    auto vfs = context->GetSubsystem<VirtualFileSystem>();
    if (auto sourceFile = vfs->OpenFile(fileName, FILE_WRITE))
    {
        const ea::string header = Format("// {}\n", defines);
        sourceFile->Write(header.data(), header.size());
        sourceFile->Write(source.data(), source.size());
    }
}

} // namespace Urho3D
