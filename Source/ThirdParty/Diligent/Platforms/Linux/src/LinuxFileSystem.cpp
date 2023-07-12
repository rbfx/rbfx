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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <ftw.h>
#include <glob.h>
#include <mutex>
#include <pwd.h>
#include <errno.h>

#include "LinuxFileSystem.hpp"
#include "Errors.hpp"
#include "DebugUtilities.hpp"

namespace Diligent
{

LinuxFile* LinuxFileSystem::OpenFile(const FileOpenAttribs& OpenAttribs)
{
    LinuxFile* pFile = nullptr;
    try
    {
        pFile = new LinuxFile{OpenAttribs};
    }
    catch (const std::runtime_error& err)
    {
    }
    return pFile;
}

bool LinuxFileSystem::FileExists(const Char* strFilePath)
{
    std::string path{strFilePath};
    CorrectSlashes(path);

    struct stat StatBuff;
    if (stat(path.c_str(), &StatBuff) != 0)
        return false;

    return !S_ISDIR(StatBuff.st_mode);
}

bool LinuxFileSystem::PathExists(const Char* strPath)
{
    std::string path{strPath};
    CorrectSlashes(path);

    struct stat StatBuff;
    return (stat(path.c_str(), &StatBuff) == 0);
}

bool LinuxFileSystem::CreateDirectory(const Char* strPath)
{
    if (strPath == nullptr || strPath[0] == '\0')
    {
        UNEXPECTED("Path must not be null or empty");
        return false;
    }

    std::string path = strPath;
    CorrectSlashes(path);

    size_t position = 0;
    while (position != std::string::npos)
    {
        position     = path.find(SlashSymbol, position + 1);
        auto subPath = path.substr(0, position);
        if (!PathExists(subPath.c_str()))
        {
            if (mkdir(subPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) != 0)
            {
                // If multiple threads are trying to create the same directory, it is possible
                // that the directory has been created by another thread, which is OK.
                if (errno != EEXIST)
                    return false;
            }
        }
    }
    return true;
}

static constexpr int MaxOpenNTFWDescriptors = 32;

void LinuxFileSystem::ClearDirectory(const Char* strPath, bool Recursive)
{
    std::string path{strPath};
    LinuxFileSystem::CorrectSlashes(path);

    auto Callaback = Recursive ?
        [](const char* Path, const struct stat* pStat, int Type, FTW* pFTWB) {
            if (pFTWB->level >= 1)
                return remove(Path);
            else
                return 0;
        } :
        [](const char* Path, const struct stat* pStat, int Type, FTW* pFTWB) {
            if (pFTWB->level == 1 && !S_ISDIR(pStat->st_mode))
                return remove(Path);
            else
                return 0;
        };

    nftw(path.c_str(), Callaback, MaxOpenNTFWDescriptors, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
}

void LinuxFileSystem::DeleteFile(const Char* strPath)
{
    remove(strPath);
}

bool LinuxFileSystem::DeleteDirectory(const Char* strPath)
{
    std::string path{strPath};
    LinuxFileSystem::CorrectSlashes(path);

    auto Callaback =
        [](const char* Path, const struct stat* pStat, int Type, FTW* pFTWB) {
            return remove(Path);
        };

    const auto res = nftw(path.c_str(), Callaback, MaxOpenNTFWDescriptors, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
    return res == 0;
}

bool LinuxFileSystem::IsDirectory(const Char* strPath)
{
    std::string path{strPath};
    CorrectSlashes(path);

    struct stat StatBuff;
    if (stat(path.c_str(), &StatBuff) != 0)
        return false;

    return S_ISDIR(StatBuff.st_mode);
}

struct LinuxFindFileData : public FindFileData
{
    virtual const Char* Name() const override { return m_Name.c_str(); }

    virtual bool IsDirectory() const override { return m_IsDirectory; }

    const std::string m_Name;
    const bool        m_IsDirectory;

    LinuxFindFileData(std::string _Name, bool _IsDirectory) :
        m_Name{std::move(_Name)},
        m_IsDirectory{_IsDirectory}
    {}
};

std::vector<std::unique_ptr<FindFileData>> LinuxFileSystem::Search(const Char* SearchPattern)
{
    std::vector<std::unique_ptr<FindFileData>> SearchRes;

    glob_t glob_result = {};
    if (glob(SearchPattern, GLOB_TILDE, NULL, &glob_result) == 0)
    {
        for (unsigned int i = 0; i < glob_result.gl_pathc; ++i)
        {
            const auto* path = glob_result.gl_pathv[i];

            struct stat StatBuff = {};
            stat(path, &StatBuff);

            std::string FileName;
            GetPathComponents(path, nullptr, &FileName);
            SearchRes.emplace_back(std::make_unique<LinuxFindFileData>(std::move(FileName), S_ISDIR(StatBuff.st_mode)));
        }
    }
    globfree(&glob_result);

    return SearchRes;
}

// popen/pclose are not thread-safe
static std::mutex g_popen_mtx{};

FILE* LinuxFileSystem::popen(const char* command, const char* type)
{
    std::lock_guard<std::mutex> Lock{g_popen_mtx};
    return ::popen(command, type);
}

int LinuxFileSystem::pclose(FILE* stream)
{
    std::lock_guard<std::mutex> Lock{g_popen_mtx};
    return ::pclose(stream);
}

std::string LinuxFileSystem::GetCurrentDirectory()
{
    std::string CurrDir;
    if (auto* cwd = getcwd(NULL, 0))
    {
        CurrDir = cwd;
        free(cwd);
    }
    return CurrDir;
}

std::string LinuxFileSystem::GetLocalAppDataDirectory(const char* AppName, bool Create)
{
    const auto* pwuid = getpwuid(getuid());
    std::string AppDataDir{pwuid->pw_dir};
    if (!IsSlash(AppDataDir.back()))
        AppDataDir += SlashSymbol;
#if PLATFORM_MACOS
    AppDataDir += "Library/Caches";
#else
    AppDataDir += ".cache";
#endif

    if (AppName == nullptr)
    {
#ifdef _GNU_SOURCE
        AppName = program_invocation_short_name;
#elif defined(__APPLE__)
        AppName = getprogname();
#endif
    }

    if (AppName != nullptr)
    {
        AppDataDir += SlashSymbol;
        AppDataDir += AppName;
        if (Create && !PathExists(AppDataDir.c_str()))
            CreateDirectory(AppDataDir.c_str());
    }
    return AppDataDir;
}

} // namespace Diligent
