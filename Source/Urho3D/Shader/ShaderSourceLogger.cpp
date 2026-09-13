// Copyright (c) 2008-2020 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
        sourceFile->Write(source.data(), static_cast<unsigned>(source.size()));
    }
}

} // namespace Urho3D
