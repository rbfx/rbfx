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

#pragma once

#include <vector>
#include "../../../Primitives/interface/BasicTypes.h"

namespace Diligent
{

enum class EFileAccessMode
{
    Read,
    Overwrite,
    Append
};

enum class FilePosOrigin
{
    Start,
    Curr,
    End
};

struct FileOpenAttribs
{
    const Char*     strFilePath;
    EFileAccessMode AccessMode;
    FileOpenAttribs(const Char*     Path   = nullptr,
                    EFileAccessMode Access = EFileAccessMode::Read) :
        strFilePath{Path},
        AccessMode{Access}
    {}
};

class BasicFile
{
public:
    BasicFile(const FileOpenAttribs& OpenAttribs, Char SlashSymbol);
    virtual ~BasicFile();

    const String& GetPath() { return m_Path; }

protected:
    String GetOpenModeStr();

    const String          m_Path;
    const FileOpenAttribs m_OpenAttribs;
};

struct FindFileData
{
    virtual const Char* Name() const        = 0;
    virtual bool        IsDirectory() const = 0;

    virtual ~FindFileData() {}
};

struct BasicFileSystem
{
public:
    static BasicFile* OpenFile(FileOpenAttribs& OpenAttribs);
    static void       ReleaseFile(BasicFile*);

    static bool FileExists(const Char* strFilePath);

    static void SetWorkingDirectory(const Char* strWorkingDir) { m_strWorkingDirectory = strWorkingDir; }

    static const String& GetWorkingDirectory() { return m_strWorkingDirectory; }

    static Char GetSlashSymbol();

    static void CorrectSlashes(String& Path, Char SlashSymbol);

    static void SplitFilePath(const String& FullName,
                              String*       Path,
                              String*       Name);

    static bool IsPathAbsolute(const Char* strPath);


    /// Simplifies the path.

    /// The function performs the following path simplifications:
    /// - Normalizes slashes using the given slash symbol (a\b/c -> a/b/c)
    /// - Removes redundant slashes (a///b -> a/b)
    /// - Removes redundant . (a/./b -> a/b)
    /// - Collapses .. (a/b/../c -> a/c)
    /// - Removes leading and trailing slashes (/a/b/c/ -> a/b/c)
    static std::string SimplifyPath(const Char* Path, Char SlashSymbol);


    /// Splits a list of paths separated by a given separator and calls a user callback for every individual path.
    /// Empty paths are skipped.
    template <typename CallbackType>
    static void SplitPathList(const Char* PathList, CallbackType Callback, const char Separator = ';')
    {
        const auto* Pos = PathList;
        while (*Pos != '\0')
        {
            while (*Pos != '\0' && *Pos == Separator)
                ++Pos;
            if (*Pos == '\0')
                break;

            const auto* End = Pos + 1;
            while (*End != '\0' && *End != Separator)
                ++End;

            if (!Callback(Pos, static_cast<size_t>(End - Pos)))
                break;

            Pos = End;
        }
    }

protected:
    static String m_strWorkingDirectory;
};

} // namespace Diligent
