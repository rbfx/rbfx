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
#include "../../../Primitives/interface/FlagEnum.h"

namespace Diligent
{

enum class EFileAccessMode
{
    Read,
    Overwrite,
    Append,
    ReadUpdate,
    OverwriteUpdate,
    AppendUpdate
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
    BasicFile(const FileOpenAttribs& OpenAttribs);
    virtual ~BasicFile();

    const String& GetPath() { return m_Path; }

protected:
    String GetOpenModeStr();

    const String          m_Path;
    const FileOpenAttribs m_OpenAttribs;
};


enum FILE_DIALOG_FLAGS : Uint32
{
    FILE_DIALOG_FLAG_NONE = 0x000,

    /// Prevents the system from adding a link to the selected file in the file system
    /// directory that contains the user's most recently used documents.
    FILE_DIALOG_FLAG_DONT_ADD_TO_RECENT = 0x001,

    /// Only existing files can be opened
    FILE_DIALOG_FLAG_FILE_MUST_EXIST = 0x002,

    /// Restores the current directory to its original value if the user changed the
    /// directory while searching for files.
    FILE_DIALOG_FLAG_NO_CHANGE_DIR = 0x004,

    /// Causes the Save As dialog box to show a message box if the selected file already exists.
    FILE_DIALOG_FLAG_OVERWRITE_PROMPT = 0x008
};
DEFINE_FLAG_ENUM_OPERATORS(FILE_DIALOG_FLAGS);

enum FILE_DIALOG_TYPE : Uint32
{
    FILE_DIALOG_TYPE_OPEN,
    FILE_DIALOG_TYPE_SAVE
};

struct FileDialogAttribs
{
    FILE_DIALOG_TYPE  Type  = FILE_DIALOG_TYPE_OPEN;
    FILE_DIALOG_FLAGS Flags = FILE_DIALOG_FLAG_NONE;

    const char* Title  = nullptr;
    const char* Filter = nullptr;

    FileDialogAttribs() noexcept {}

    explicit FileDialogAttribs(FILE_DIALOG_TYPE _Type) noexcept :
        Type{_Type}
    {
        switch (Type)
        {
            case FILE_DIALOG_TYPE_OPEN:
                Flags = FILE_DIALOG_FLAG_DONT_ADD_TO_RECENT | FILE_DIALOG_FLAG_FILE_MUST_EXIST | FILE_DIALOG_FLAG_NO_CHANGE_DIR;
                break;

            case FILE_DIALOG_TYPE_SAVE:
                Flags = FILE_DIALOG_FLAG_DONT_ADD_TO_RECENT | FILE_DIALOG_FLAG_OVERWRITE_PROMPT | FILE_DIALOG_FLAG_NO_CHANGE_DIR;
                break;
        }
    }
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
#if PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS
    static constexpr Char SlashSymbol = '\\';
#else
    static constexpr Char SlashSymbol = '/';
#endif

    static BasicFile* OpenFile(FileOpenAttribs& OpenAttribs);
    static void       ReleaseFile(BasicFile*);

    static bool FileExists(const Char* strFilePath);

    static void SetWorkingDirectory(const Char* strWorkingDir) { m_strWorkingDirectory = strWorkingDir; }

    static const String& GetWorkingDirectory() { return m_strWorkingDirectory; }

    static bool IsSlash(Char c)
    {
        return c == '/' || c == '\\';
    }

    static void CorrectSlashes(String& Path, Char Slash = 0);

    static void GetPathComponents(const String& Path,
                                  String*       Directory,
                                  String*       FileName);

    static bool IsPathAbsolute(const Char* Path);

    /// Splits path into individual components optionally simplifying it.
    ///
    /// If Simplify is true:
    ///     - Removes redundant slashes (a///b -> a/b)
    ///     - Removes redundant . (a/./b -> a/b)
    ///     - Collapses .. (a/b/../c -> a/c)
    static std::vector<String> SplitPath(const Char* Path, bool Simplify);

    /// Builds a path from the given components.
    static std::string BuildPathFromComponents(const std::vector<String>& Components, Char Slash = 0);

    /// Simplifies the path.

    /// The function performs the following path simplifications:
    /// - Normalizes slashes using the given slash symbol (a\b/c -> a/b/c)
    /// - Removes redundant slashes (a///b -> a/b)
    /// - Removes redundant . (a/./b -> a/b)
    /// - Collapses .. (a/b/../c -> a/c)
    /// - Removes trailing slashes (/a/b/c/ -> /a/b/c)
    /// - When 'Slash' is Windows slash ('\'), removes leading slashes (\a\b\c -> a\b\c)
    static std::string SimplifyPath(const Char* Path, Char Slash = 0);


    /// Splits a list of paths separated by a given separator and calls a user callback for every individual path.
    /// Empty paths are skipped.
    template <typename CallbackType>
    static void SplitPathList(const Char* PathList, CallbackType Callback, const char Separator = ';')
    {
        if (PathList == nullptr)
            return;

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

    /// Returns a relative path from one file or folder to another.

    /// \param [in]  PathFrom        - Path that defines the start of the relative path.
    ///                                Must not be null.
    /// \param [in]  IsFromDirectory - Indicates if PathFrom is a directory.
    /// \param [in]  PathTo          - Path that defines the endpoint of the relative path.
    ///                                Must not be null.
    /// \param [in]  IsToDirectory   - Indicates if PathTo is a directory.
    ///
    /// \return                        Relative path from PathFrom to PathTo.
    ///                                If no relative path exists, PathFrom is returned.
    static std::string GetRelativePath(const Char* PathFrom,
                                       bool        IsFromDirectory,
                                       const Char* PathTo,
                                       bool        IsToDirectory);

    static std::string FileDialog(const FileDialogAttribs& DialogAttribs);
    static std::string OpenFolderDialog(const char* Title);

protected:
    static String m_strWorkingDirectory;
};

} // namespace Diligent
