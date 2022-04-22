/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include "BasicFileSystem.hpp"
#include "DebugUtilities.hpp"
#include <algorithm>

namespace Diligent
{

String BasicFileSystem::m_strWorkingDirectory;

BasicFile::BasicFile(const FileOpenAttribs& OpenAttribs, Char SlashSymbol) :
    m_Path //
    {
        [](const Char* strPath, Char SlashSymbol) //
        {
            String Path = strPath != nullptr ? strPath : "";
            BasicFileSystem::CorrectSlashes(Path, SlashSymbol);
            return Path;
        }(OpenAttribs.strFilePath, SlashSymbol) //
    },
    m_OpenAttribs{m_Path.c_str(), OpenAttribs.AccessMode}
{
}

BasicFile::~BasicFile()
{
}

String BasicFile::GetOpenModeStr()
{
    std::string OpenModeStr;
    switch (m_OpenAttribs.AccessMode)
    {
        // clang-format off
        case EFileAccessMode::Read:      OpenModeStr += 'r'; break;
        case EFileAccessMode::Overwrite: OpenModeStr += 'w'; break;
        case EFileAccessMode::Append:    OpenModeStr += 'a'; break;
        // clang-format on
        default: break;
    }

    // Always open file in binary mode. Text mode is platform-specific
    OpenModeStr += 'b';

    return OpenModeStr;
}

BasicFile* BasicFileSystem::OpenFile(FileOpenAttribs& OpenAttribs)
{
    return nullptr;
}

void BasicFileSystem::ReleaseFile(BasicFile* pFile)
{
    if (pFile)
        delete pFile;
}

bool BasicFileSystem::FileExists(const Char* strFilePath)
{
    return false;
}

Char BasicFileSystem::GetSlashSymbol()
{
    UNSUPPORTED("Unsupported");
    return 0;
}

void BasicFileSystem::CorrectSlashes(String& Path, Char SlashSymbol)
{
    VERIFY(SlashSymbol == '\\' || SlashSymbol == '/',
           "Incorrect slash symbol");
    Char RevSlashSym = (SlashSymbol == '\\') ? '/' : '\\';
    std::replace(Path.begin(), Path.end(), RevSlashSym, SlashSymbol);
}

void BasicFileSystem::SplitFilePath(const String& FullName,
                                    String*       Path,
                                    String*       Name)
{
    auto LastSlashPos = FullName.find_last_of("/\\");
    if (Path)
    {
        if (LastSlashPos != String::npos)
            *Path = FullName.substr(0, LastSlashPos);
        else
            *Path = "";
    }

    if (Name)
    {
        if (LastSlashPos != String::npos)
            *Name = FullName.substr(LastSlashPos + 1);
        else
            *Name = FullName;
    }
}

bool BasicFileSystem::IsPathAbsolute(const Char* strPath)
{
    if (strPath == nullptr || strPath[0] == 0)
        return false;

#if PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS
    return ((strPath[1] == ':' && (strPath[2] == '\\' || strPath[2] == '/')) || // c:\Path or c:/Path
            (strPath[0] == '\\' && strPath[1] == '\\'));                        // \\?\Path
#elif PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS || PLATFORM_TVOS || PLATFORM_ANDROID || PLATFORM_EMSCRIPTEN
    return strPath[0] == '/';
#else
#    error Unknown platform.
#endif
}

std::string BasicFileSystem::SimplifyPath(const Char* Path, Char SlashSymbol)
{
    if (Path == nullptr)
        return "";

    auto IsSlash = [](const Char c) {
        return c == '/' || c == '\\';
    };

    std::vector<std::string> PathComponents;

    const auto* c = Path;
    while (*c != '\0')
    {
        while (*c != '\0' && IsSlash(*c))
            ++c;

        if (*c == '\0')
        {
            // a/
            break;
        }

        std::string PathCmp;
        while (*c != '\0' && !IsSlash(*c))
            PathCmp.push_back(*(c++));

        if (PathCmp == ".")
        {
            // Skip /.
            continue;
        }
        else if (PathCmp == ".." && !PathComponents.empty() && PathComponents.back() != "..")
        {
            // Pop previous subdirectory if /.. is found, but only if there is no .. already
            PathComponents.pop_back();
        }
        else
        {
            PathComponents.emplace_back(std::move(PathCmp));
        }
    }

    std::string SimplifiedPath;
    for (const auto& Cmp : PathComponents)
    {
        if (!SimplifiedPath.empty())
            SimplifiedPath.push_back(SlashSymbol);
        SimplifiedPath.append(Cmp);
    }
    return SimplifiedPath;
}

} // namespace Diligent
