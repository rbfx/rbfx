/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include "../../Linux/interface/LinuxFileSystem.hpp"
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

    size_t GetSize();

    size_t GetPos();

    bool SetPos(size_t Offset, FilePosOrigin Origin);

    static bool Open(const FileOpenAttribs& OpenAttribs, std::fstream& FS, AAsset*& AssetFile);

private:
    std::fstream m_FS;
    AAsset*      m_AssetFile = nullptr;
};


/// Android file system implementation.
struct AndroidFileSystem : public LinuxFileSystem
{
public:
    /// Initializes the file system.

    /// \param [in] AssetManager     - A pointer to the asset manager (AAssetManager).
    /// \param [in] ExternalFilesDir - External files directory.
    /// \param [in] OutputFilesDir   - Output files directory.
    ///
    /// \remarks The file system can be initialized to use either the external assets path or asset manager, or both.
    ///          When ExternalFilesDir is not null, the file system will try to use it first when opening files.
    ///          It will then resort to using the asset manager.
    ///          If ExternalFilesDir is null, the file system will only use the asset manager.
    // clang-format off
    static void Init(struct AAssetManager* AssetManager,
                     const char*           ExternalFilesDir DEFAULT_INITIALIZER(nullptr),
                     const char*           OutputFilesDir   DEFAULT_INITIALIZER(nullptr));
    // clang-format on

    static AndroidFile* OpenFile(const FileOpenAttribs& OpenAttribs);

    static bool FileExists(const Char* strFilePath);

    static std::string GetLocalAppDataDirectory(const char* AppName = nullptr, bool Create = true);
};

} // namespace Diligent
