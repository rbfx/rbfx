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

#include "../../Basic/interface/BasicFileSystem.hpp"
#include "../../../Primitives/interface/DataBlob.h"

// Do not include windows headers here as they will mess up CreateDirectory()
// and DeleteFile() functions!
//#define NOMINMAX
//#include <wrl.h>

namespace Diligent
{

class WindowsStoreFile : public BasicFile
{
public:
    WindowsStoreFile(const FileOpenAttribs& OpenAttribs);
    ~WindowsStoreFile();

    void Read(IDataBlob* pData);

    bool Read(void* Data, size_t BufferSize);

    void Write(IDataBlob* pData);
    bool Write(const void* Data, size_t BufferSize);

    size_t GetSize();

    size_t GetPos();

    bool SetPos(size_t Offset, FilePosOrigin Origin);

private:
    // We have to do such tricks, because we cannot #include <wrl.h>
    // to avoid name clashes.
    std::unique_ptr<class FileHandleWrapper> m_FileHandle;
};

struct WindowsStoreFileSystem : public BasicFileSystem
{
public:
    static WindowsStoreFile* OpenFile(const FileOpenAttribs& OpenAttribs);

    static bool FileExists(const Char* strFilePath);
    static bool PathExists(const Char* strPath);

    static bool CreateDirectory(const Char* strPath);
    static void ClearDirectory(const Char* strPath);
    static void DeleteFile(const Char* strPath);

    static std::vector<std::unique_ptr<FindFileData>> Search(const Char* SearchPattern);

    static std::string GetLocalAppDataDirectory(const char* AppName = nullptr, bool Create = true);
};

} // namespace Diligent
