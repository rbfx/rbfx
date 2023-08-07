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

#include <memory>
#include <vector>
#include <fstream>
#include <android/asset_manager.h>

#include "../../Basic/interface/BasicFileSystem.hpp"
#include "../../../Primitives/interface/DataBlob.h"

struct ANativeActivity;
struct AAssetManager;

namespace Diligent
{

class AndroidFile : public BasicFile
{
public:
    AndroidFile(const FileOpenAttribs& OpenAttribs);
    ~AndroidFile();

    bool Read(IDataBlob* pData);

    bool Read(void* Data, size_t BufferSize);

    bool Write(const void* Data, size_t BufferSize);

    size_t GetSize() { return m_Size; }

    size_t GetPos();

    bool SetPos(size_t Offset, FilePosOrigin Origin);

    static bool Open(const char* FileName, std::ifstream& IFS, AAsset*& AssetFile, size_t& Size);

private:
    std::ifstream m_IFS;
    AAsset*       m_AssetFile = nullptr;
    size_t        m_Size      = 0;
};


/// Android file system implementation.
struct AndroidFileSystem : public BasicFileSystem
{
public:
    /// Initializes the file system.

    /// \param [in] NativeActivity          - Pointer to the native activity object (ANativeActivity).
    /// \param [in] NativeActivityClassName - Native activity class name.
    /// \param [in] AssetManager            - Pointer to the asset manager (AAssetManager).
    ///
    /// \remarks The file system can be initialized to use either native activity or asset manager, or both.
    ///          When NativeActivity is not null, the file system will try to use it first when opening files.
    ///          It will then resort to using the asset manager. When NativeActivity is not null, but AssetManager
    ///          parameter is null, the file system will use the asset manager from the activity.
    ///          If NativeActivity is null, the file system will only use the asset manager.
    static void Init(struct ANativeActivity* NativeActivity,
                     const char*             NativeActivityClassName,
                     struct AAssetManager*   AssetManager);


    static AndroidFile* OpenFile(const FileOpenAttribs& OpenAttribs);

    static bool FileExists(const Char* strFilePath);
    static bool PathExists(const Char* strPath);

    static bool CreateDirectory(const Char* strPath);
    static void ClearDirectory(const Char* strPath);
    static void DeleteFile(const Char* strPath);

    static std::vector<std::unique_ptr<FindFileData>> Search(const Char* SearchPattern);

    static std::string GetLocalAppDataDirectory(const char* AppName = nullptr, bool Create = true);
};

} // namespace Diligent
