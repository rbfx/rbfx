/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

#include "Win32FileSystem.hpp"
#include "Errors.hpp"
#include "../../Common/interface/StringTools.hpp"
#include "../../Basic/include/SearchRecursive.inl"

// We can't use namespace Diligent before #including <Windows.h> because Diligent::INTERFACE_ID will conflict with windows InterfaceID
//using namespace Diligent;

// Windows.h defines CreateDirectory and DeleteFile as macros.
// So we need to do some tricks to avoid name mess.

namespace Diligent
{

static bool CreateDirectoryImpl(const Diligent::Char* strPath);

bool WindowsFileSystem::CreateDirectory(const Diligent::Char* strPath)
{
    return CreateDirectoryImpl(strPath);
}

static void DeleteFileImpl(const Diligent::Char* strPath);

void WindowsFileSystem::DeleteFile(const Diligent::Char* strPath)
{
    DeleteFileImpl(strPath);
}

static std::string GetCurrentDirectoryImpl();

std::string WindowsFileSystem::GetCurrentDirectory()
{
    return GetCurrentDirectoryImpl();
}

static std::string GetLocalAppDataDirectoryImpl(const char* AppName, bool Create);

std::string WindowsFileSystem::GetLocalAppDataDirectory(const char* AppName, bool Create)
{
    return GetLocalAppDataDirectoryImpl(AppName, Create);
}

} // namespace Diligent

#include <Windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <shlobj.h>

#if !(defined(__MINGW64__) || defined(__MINGW32__))
#    include <atlbase.h>
#endif

namespace Diligent
{

class WindowsPathHelper
{
public:
    explicit WindowsPathHelper(const char* Path)
    {
        // NOTE: the MAX_PATH limitation apparently applies to the total path length.
        //       For a relative path, it also counts the implicit current directory part.
        //       As there is no reliable way to check if we will exceed the limit,
        //       always use the long path method.

        if (!WindowsFileSystem::IsPathAbsolute(Path))
        {
            m_Path = GetCurrentDirectory_();
            m_Path.push_back(WindowsFileSystem::SlashSymbol);
        }
        m_Path += Path;
        m_Path = WindowsFileSystem::SimplifyPath(m_Path.c_str());

        m_LongPathW = WidenString(m_Path);

        constexpr auto* LongPathPrefix = LR"(\\?\)";
        if (m_LongPathW.compare(0, 4, LongPathPrefix) != 0)
            m_LongPathW.insert(0, LongPathPrefix);
    }

    explicit WindowsPathHelper(const std::string& Path) :
        WindowsPathHelper{Path.c_str()}
    {}

#define CALL_WIN_FUNC(WinFunc, ...) (WinFunc##W(m_LongPathW.c_str(), ##__VA_ARGS__))

    bool PathFileExists_() const
    {
        return CALL_WIN_FUNC(PathFileExists) != FALSE;
    }

    DWORD GetFileAttributes_() const
    {
        return CALL_WIN_FUNC(GetFileAttributes);
    }

    bool SetFileAttributes_(DWORD dwAttributes) const
    {
        return CALL_WIN_FUNC(SetFileAttributes, dwAttributes) != FALSE;
    }

    bool CreateDirectory_() const
    {
        return CALL_WIN_FUNC(CreateDirectory, NULL) != FALSE;
    }

    bool DeleteFile_() const
    {
        return CALL_WIN_FUNC(DeleteFile) != FALSE;
    }

    bool RemoveDirectory_() const
    {
        return CALL_WIN_FUNC(RemoveDirectory) != FALSE;
    }
#undef CALL_WIN_FUNC

    static std::string GetCurrentDirectory_()
    {
        std::string CurrDir;
        // If the function succeeds, the return value specifies the number of characters that
        // are written to the buffer, NOT including the terminating null character.
        // HOWEVER, if the buffer that is pointed to by lpBuffer is not large enough,
        // the return value specifies the required size of the buffer, in characters,
        // INCLUDING the null-terminating character.
        // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getcurrentdirectory
        auto BufferSize = GetCurrentDirectoryA(0, nullptr);

        if (BufferSize > 1)
        {
            // Note that std::string::resize(n) resizes the string to a length of n characters.
            CurrDir.resize(BufferSize - 1);

            // BufferSize must include room for a terminating null character.
            auto NumChars = GetCurrentDirectoryA(BufferSize, &CurrDir[0]);
            VERIFY_EXPR(CurrDir.length() == NumChars);
        }
        return CurrDir;
    }

    static bool SetWorkingDirectory(const char* Path)
    {
        BasicFileSystem::SetWorkingDirectory(Path);
        // If slash is missing at the end, it will be added
        if (strlen(Path) < MAX_PATH - 2)
        {
            return SetCurrentDirectoryA(Path) != FALSE;
        }
        else
        {
            std::wstring PathW = WidenString(Path);
            return SetCurrentDirectoryW(PathW.c_str()) != FALSE;
        }
    }

    errno_t fopen(FILE** ppFile, const char* Mode) const
    {
        return _wfopen_s(ppFile, m_LongPathW.c_str(), WidenString(Mode).c_str());
    }

    std::string operator/(const char* Path) const
    {
        const auto WndSlash = WindowsFileSystem::SlashSymbol;

        auto Res = m_Path;
        if (Res.back() != WndSlash)
            Res.push_back(WndSlash);
        Res.append(Path);

        return Res;
    }

    std::wstring operator/(const wchar_t* Path) const
    {
        auto Res = m_LongPathW;
        if (Res.back() != L'\\')
            Res.push_back(L'\\');
        Res.append(Path);

        return Res;
    }

    std::string operator/(const std::string& Path) const
    {
        return *this / Path.c_str();
    }
    std::wstring operator/(const std::wstring& Path) const
    {
        return *this / Path.c_str();
    }

private:
    std::string  m_Path;
    std::wstring m_LongPathW;
};




WindowsFile::WindowsFile(const FileOpenAttribs& OpenAttribs) :
    StandardFile{OpenAttribs}
{
    VERIFY_EXPR(m_pFile == nullptr);
    const auto ModeStr = GetOpenModeStr();

    const WindowsPathHelper WndPath{m_OpenAttribs.strFilePath};
    for (;;)
    {
        const auto err = WndPath.fopen(&m_pFile, ModeStr.c_str());
        if (err == 0)
        {
            break;
        }
        else if (err == ENFILE || // Too many files open in system
                 err == EMFILE)   // Too many open files
        {
            // No more file descriptors are available: we have to wait
            Sleep(50);
            continue;
        }
        else
        {
            char errstr[128];
            strerror_s(errstr, _countof(errstr), err);
            LOG_ERROR_AND_THROW("Failed to open file ", m_OpenAttribs.strFilePath,
                                "\nThe following error occurred: ", errstr);
        }
    }
}

WindowsFile* WindowsFileSystem::OpenFile(const FileOpenAttribs& OpenAttribs)
{
    WindowsFile* pFile = nullptr;
    try
    {
        pFile = new WindowsFile{OpenAttribs};
    }
    catch (const std::runtime_error& /*err*/)
    {
    }

    return pFile;
}

bool WindowsFileSystem::FileExists(const Char* strFilePath)
{
    const WindowsPathHelper WndPath{strFilePath};

    if (!WndPath.PathFileExists_())
        return false;

    auto FileAttribs = WndPath.GetFileAttributes_();
    if (FileAttribs == INVALID_FILE_ATTRIBUTES)
        return false;

    return (FileAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static bool CreateDirectoryImpl(const Char* strPath)
{
    if (strPath == nullptr || strPath[0] == '\0')
    {
        UNEXPECTED("Path must not be null or empty");
        return false;
    }

    // Test all parent directories
    std::string            DirectoryPath = strPath;
    std::string::size_type SlashPos      = std::wstring::npos;
    const auto             SlashSym      = WindowsFileSystem::SlashSymbol;
    WindowsFileSystem::CorrectSlashes(DirectoryPath);

    do
    {
        SlashPos = DirectoryPath.find(SlashSym, (SlashPos != std::string::npos) ? SlashPos + 1 : 0);

        std::string ParentDirPath = (SlashPos != std::wstring::npos) ? DirectoryPath.substr(0, SlashPos) : DirectoryPath;
        if (ParentDirPath.back() == ':')
            continue; // Skip drive letters such as "C:"

        const WindowsPathHelper ParentDir{ParentDirPath};
        if (!ParentDir.PathFileExists_())
        {
            // If there is no directory, create it
            if (!ParentDir.CreateDirectory_())
            {
                // If multiple threads are trying to create the same directory, it is possible
                // that the directory has been created by another thread, which is OK.
                if (GetLastError() != ERROR_ALREADY_EXISTS)
                    return false;

                if ((ParentDir.GetFileAttributes_() & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    return false;
            }
        }
    } while (SlashPos != std::string::npos);

    return true;
}

template <typename CharType>
inline bool IsDot(const CharType Name[])
{
    return Name[0] == CharType{'.'} && Name[1] == 0;
}
template <typename CharType>
inline bool IsDblDot(const CharType Name[])
{
    return Name[0] == CharType{'.'} && Name[1] == CharType{'.'} && Name[2] == 0;
}

void WindowsFileSystem::ClearDirectory(const Char* strPath, bool Recursive)
{
    // Find the first file in the directory.
    WindowsPathHelper Directory{strPath};

    const auto SearchPattern = Directory / L"*";

    WIN32_FIND_DATAW ffd   = {};
    const auto       hFind = FindFirstFileW(SearchPattern.c_str(), &ffd);

    if (INVALID_HANDLE_VALUE == hFind)
    {
        LOG_ERROR_MESSAGE("FindFirstFile failed with error code ", GetLastError());
        return;
    }

    // List all the files in the directory
    do
    {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (Recursive)
            {
                // Skip '.' and '..'
                if (!IsDot(ffd.cFileName) && !IsDblDot(ffd.cFileName))
                {
                    auto SubDirName = Directory / NarrowString(ffd.cFileName);
                    ClearDirectory(SubDirName.c_str(), Recursive);

                    if (!WindowsPathHelper{SubDirName}.RemoveDirectory_())
                    {
                        LOG_ERROR_MESSAGE("Failed to remove directory '", SubDirName, "'. Error code: ", GetLastError());
                    }
                }
            }
        }
        else
        {
            auto FileName = Directory / NarrowString(ffd.cFileName);
            DeleteFileImpl(FileName.c_str());
        }
    } while (FindNextFileW(hFind, &ffd) != 0);

    FindClose(hFind);
}


static void DeleteFileImpl(const Char* strPath)
{
    const WindowsPathHelper WndPath{strPath};
    if (!WndPath.SetFileAttributes_(FILE_ATTRIBUTE_NORMAL))
    {
        LOG_WARNING_MESSAGE("Failed to set FILE_ATTRIBUTE_NORMAL for file '", strPath, "'. Error code: ", GetLastError());
    }

    if (!WndPath.DeleteFile_())
    {
        LOG_ERROR_MESSAGE("Failed to delete file '", strPath, "'. Error code: ", GetLastError());
    }
}

void WindowsFileSystem::DeleteDirectory(const Char* strPath)
{
    ClearDirectory(strPath, true);

    const WindowsPathHelper WndPath{strPath};
    if (!WndPath.RemoveDirectory_())
    {
        LOG_ERROR_MESSAGE("Failed to remove directory '", strPath, "'. Error code: ", GetLastError());
    }
}


bool WindowsFileSystem::PathExists(const Char* strPath)
{
    const WindowsPathHelper WndPath{strPath};
    return WndPath.PathFileExists_();
}

void WindowsFileSystem::SetWorkingDirectory(const Char* strWorkingDir)
{
    WindowsPathHelper::SetWorkingDirectory(strWorkingDir);
}

WindowsFileSystem::SearchFilesResult WindowsFileSystem::Search(const Char* SearchPattern)
{
    SearchFilesResult SearchRes;

    WIN32_FIND_DATAA ffd;
    // Find the first file in the directory.
    auto hFind = FindFirstFileA(SearchPattern, &ffd);

    if (INVALID_HANDLE_VALUE == hFind)
    {
        return SearchRes;
    }

    // List all the files in the directory
    do
    {
        // Skip '.' and '..' that add no value
        if (IsDot(ffd.cFileName) || IsDblDot(ffd.cFileName))
            continue;

        SearchRes.emplace_back(FindFileData{ffd.cFileName, (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0});
    } while (FindNextFileA(hFind, &ffd) != 0);

    auto dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES)
    {
        //ErrorHandler(TEXT("FindFirstFile"));
    }

    FindClose(hFind);

    return SearchRes;
}

WindowsFileSystem::SearchFilesResult WindowsFileSystem::SearchRecursive(const Char* Dir, const Char* SearchPattern)
{
    return Diligent::SearchRecursive<WindowsFileSystem>(Dir, SearchPattern);
}

static DWORD FileDialogFlagsToOFNFlags(FILE_DIALOG_FLAGS FileDialogFlags)
{
    DWORD OFNFlags = 0;
    while (FileDialogFlags != FILE_DIALOG_FLAG_NONE)
    {
        auto Flag = FileDialogFlags & ~static_cast<FILE_DIALOG_FLAGS>(static_cast<Uint32>(FileDialogFlags) - 1);
        switch (Flag)
        {
            case FILE_DIALOG_FLAG_DONT_ADD_TO_RECENT:
                OFNFlags |= OFN_DONTADDTORECENT;
                break;

            case FILE_DIALOG_FLAG_FILE_MUST_EXIST:
                OFNFlags |= OFN_FILEMUSTEXIST;
                break;

            case FILE_DIALOG_FLAG_NO_CHANGE_DIR:
                OFNFlags |= OFN_NOCHANGEDIR;
                break;

            case FILE_DIALOG_FLAG_OVERWRITE_PROMPT:
                OFNFlags |= OFN_OVERWRITEPROMPT;
                break;

            default:
                UNEXPECTED("Unknown file dialog flag (", Flag, ")");
        }
        FileDialogFlags &= ~Flag;
    }
    return OFNFlags;
}

std::string WindowsFileSystem::FileDialog(const FileDialogAttribs& DialogAttribs)
{
    OPENFILENAMEA ofn = {};

    char buffer[1024] = {};
    ofn.lStructSize   = sizeof(ofn);
    ofn.lpstrFilter   = DialogAttribs.Filter;
    ofn.lpstrFile     = buffer;
    ofn.nMaxFile      = _countof(buffer);
    ofn.lpstrTitle    = DialogAttribs.Title;
    ofn.Flags         = FileDialogFlagsToOFNFlags(DialogAttribs.Flags);

    std::string FileName;
    if (DialogAttribs.Type == FILE_DIALOG_TYPE_OPEN ? GetOpenFileNameA(&ofn) : GetSaveFileNameA(&ofn))
    {
        FileName = buffer;
    }
    return FileName;
}

std::string WindowsFileSystem::OpenFolderDialog(const char* Title)
{
#if !(defined(__MINGW64__) || defined(__MINGW32__))
    if (Title == nullptr)
        Title = "Select Folder";

    CComPtr<IFileOpenDialog> pDialog;
    // Create the FileOpenDialog object.
    auto hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pDialog));
    if (FAILED(hr))
    {
        UNEXPECTED("Failed to create File Open Dialog");
        return "";
    }

    DWORD dwOptions = 0;
    if (SUCCEEDED(pDialog->GetOptions(&dwOptions)))
    {
        pDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_NOCHANGEDIR);
    }
    pDialog->SetTitle(WidenString(Title).c_str());

    // Show the dialog box.
    hr = pDialog->Show(NULL);
    if (FAILED(hr))
    {
        // The user pressed Cancel
        return "";
    }

    // Get the file name from the dialog box.
    CComPtr<IShellItem> pItem;
    hr = pDialog->GetResult(&pItem);
    if (FAILED(hr))
        return "";

    PWSTR pszFilePath = nullptr;
    hr                = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    if (FAILED(hr))
        return "";

    std::string FolderPath{NarrowString(pszFilePath)};
    CoTaskMemFree(pszFilePath);

    return FolderPath;
#else
    LOG_WARNING_MESSAGE("Open folder dialog is not supported on MinGW");
    return "";
#endif
}

bool WindowsFileSystem::IsDirectory(const Char* strPath)
{
    const WindowsPathHelper WndPath{strPath};
    if (!WndPath.PathFileExists_())
    {
        LOG_WARNING_MESSAGE("Path '", strPath, "' does not exist. Use PathExists function to check if path exists.");
        return false;
    }

    return (WndPath.GetFileAttributes_() & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::string GetCurrentDirectoryImpl()
{
    return WindowsPathHelper::GetCurrentDirectory_();
}


std::string GetLocalAppDataDirectoryImpl(const char* AppName, bool Create)
{
    std::string AppDataDir;
    PWSTR       Path = nullptr;
    auto        hr   = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &Path);
    if (SUCCEEDED(hr))
    {
        VERIFY_EXPR(Path != nullptr);
        AppDataDir = NarrowString(Path);
        if (!WindowsFileSystem::IsSlash(AppDataDir.back()))
            AppDataDir.push_back(WindowsFileSystem::SlashSymbol);

        if (AppName != nullptr)
        {
            AppDataDir.append(AppName);
        }
        else
        {
            CHAR ExeFilePath[MAX_PATH];
            // If the GetModuleFileNameA succeeds, the return value is the length of the string that is
            // copied to the buffer, in characters, not including the terminating null character.
            if (GetModuleFileNameA(NULL, ExeFilePath, MAX_PATH) > 0)
            {
                std::string FileName;
                WindowsFileSystem::GetPathComponents(ExeFilePath, nullptr, &FileName);
                // Remove extension
                auto DotPos = FileName.find_last_of('.');
                if (DotPos != std::string::npos)
                    FileName.resize(DotPos);
                AppDataDir.append(FileName);
            }
        }
        if (Create && !WindowsFileSystem::PathExists(AppDataDir.c_str()))
            CreateDirectoryImpl(AppDataDir.c_str());
    }

    if (Path != nullptr)
    {
        CoTaskMemFree(Path);
    }

    return AppDataDir;
}

} // namespace Diligent
