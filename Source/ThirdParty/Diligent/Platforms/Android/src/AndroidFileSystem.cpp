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

#include "AndroidFileSystem.hpp"
#include "Errors.hpp"
#include "DebugUtilities.hpp"

namespace Diligent
{

namespace
{

std::ios_base::openmode FileAccessModeToIosOpenMode(EFileAccessMode Mode)
{
    switch (Mode)
    {
        case EFileAccessMode::Read:
            return std::ios::binary | std::ios::in;

        case EFileAccessMode::Overwrite:
            return std::ios::binary | std::ios::out | std::ios::trunc;

        case EFileAccessMode::Append:
            return std::ios::binary | std::ios::out | std::ios::app;

        case EFileAccessMode::ReadUpdate:
            return std::ios::binary | std::ios::in | std::ios::out;

        case EFileAccessMode::OverwriteUpdate:
            return std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc;

        case EFileAccessMode::AppendUpdate:
            return std::ios::binary | std::ios::in | std::ios::out | std::ios::app;

        default:
            UNEXPECTED("Unknown file access mode");
            return std::ios::binary;
    }
}

struct AndroidFileSystemHelper
{
    static AndroidFileSystemHelper& GetInstance()
    {
        static AndroidFileSystemHelper Instance;
        return Instance;
    }

    void Init(AAssetManager* AssetManager, const char* ExternalFilesDir, const char* OutputFilesDir)
    {
        m_AssetManager     = AssetManager;
        m_ExternalFilesDir = ExternalFilesDir != nullptr ? ExternalFilesDir : "";
        m_OutputFilesDir   = OutputFilesDir != nullptr ? OutputFilesDir : "";
    }

    bool OpenFile(const FileOpenAttribs& OpenAttribs, std::fstream& FS, AAsset*& AssetFile)
    {
        const char* fileName = OpenAttribs.strFilePath;
        if (fileName == nullptr || fileName[0] == '\0')
        {
            return false;
        }

        const auto IsAbsolutePath = AndroidFileSystem::IsPathAbsolute(fileName);
        if (!IsAbsolutePath && m_ExternalFilesDir.empty() && m_AssetManager == nullptr)
        {
            LOG_ERROR_MESSAGE("File system has not been initialized. Call AndroidFileSystem::Init().");
            return false;
        }

        // First, try reading from the external directory
        if (IsAbsolutePath)
        {
            FS.open(fileName, FileAccessModeToIosOpenMode(OpenAttribs.AccessMode));
        }
        else if (!m_ExternalFilesDir.empty())
        {
            auto ExternalFilesPath = m_ExternalFilesDir;
            if (ExternalFilesPath.back() != '/')
                ExternalFilesPath.append("/");
            ExternalFilesPath.append(fileName);
            FS.open(ExternalFilesPath.c_str(), FileAccessModeToIosOpenMode(OpenAttribs.AccessMode));
        }

        if (FS && FS.is_open())
        {
            return true;
        }
        else if (!IsAbsolutePath && m_AssetManager != nullptr)
        {
            if (OpenAttribs.AccessMode != EFileAccessMode::Read)
            {
                LOG_ERROR_MESSAGE("Asset files can only be open for reading");
                return false;
            }

            // Fallback to assetManager
            AssetFile = AAssetManager_open(m_AssetManager, fileName, AASSET_MODE_BUFFER);
            if (!AssetFile)
            {
                return false;
            }
            uint8_t* data = (uint8_t*)AAsset_getBuffer(AssetFile);
            if (data == nullptr)
            {
                AAsset_close(AssetFile);

                LOG_ERROR_MESSAGE("Failed to open: ", fileName);
                return false;
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    const std::string& GetExternalFilesDir() const
    {
        return m_ExternalFilesDir;
    }

    const std::string& GetOutputFilesDir() const
    {
        return m_OutputFilesDir;
    }

private:
    AndroidFileSystemHelper() {}

private:
    AAssetManager* m_AssetManager = nullptr;
    std::string    m_ExternalFilesDir;
    std::string    m_OutputFilesDir;
};

} // namespace

bool AndroidFile::Open(const FileOpenAttribs& OpenAttribs, std::fstream& IFS, AAsset*& AssetFile)
{
    return AndroidFileSystemHelper::GetInstance().OpenFile(OpenAttribs, IFS, AssetFile);
}

AndroidFile::AndroidFile(const FileOpenAttribs& OpenAttribs) :
    BasicFile{OpenAttribs}
{
    if (!Open(m_OpenAttribs, m_FS, m_AssetFile))
    {
        LOG_ERROR_AND_THROW("Failed to open file ", m_OpenAttribs.strFilePath);
    }
}

AndroidFile::~AndroidFile()
{
    if (m_FS && m_FS.is_open())
        m_FS.close();

    if (m_AssetFile != nullptr)
        AAsset_close(m_AssetFile);
}

bool AndroidFile::Read(IDataBlob* pData)
{
    pData->Resize(GetSize());
    return Read(pData->GetDataPtr(), pData->GetSize());
}

bool AndroidFile::Read(void* Data, size_t BufferSize)
{
    if (m_FS && m_FS.is_open())
    {
        m_FS.read((char*)Data, BufferSize);
        return true;
    }
    else if (m_AssetFile != nullptr)
    {
        const uint8_t* src_data = (uint8_t*)AAsset_getBuffer(m_AssetFile);
        off_t          FileSize = AAsset_getLength(m_AssetFile);

        VERIFY(BufferSize == static_cast<size_t>(FileSize), "Only whole asset file reads are currently supported");
        if (FileSize > static_cast<off_t>(BufferSize))
        {
            LOG_WARNING_MESSAGE("Requested buffer size (", BufferSize, ") exceeds file size (", FileSize, ")");
            BufferSize = FileSize;
        }
        memcpy(Data, src_data, BufferSize);
        return true;
    }
    else
    {
        return false;
    }
}

bool AndroidFile::Write(const void* Data, size_t BufferSize)
{
    if (m_FS && m_FS.is_open())
    {
        m_FS.write((char*)Data, BufferSize);
        return true;
    }

    return false;
}

size_t AndroidFile::GetSize()
{
    if (m_FS && m_FS.is_open())
    {
        auto Pos = m_FS.tellg();
        m_FS.seekg(0, std::ios_base::end);
        auto FileSize = m_FS.tellg();
        m_FS.seekg(Pos, std::ios_base::beg);
        return FileSize;
    }
    else if (m_AssetFile != nullptr)
    {
        return AAsset_getLength(m_AssetFile);
    }
    else
    {
        return 0;
    }
}

size_t AndroidFile::GetPos()
{
    if (m_FS && m_FS.is_open())
    {
        return m_FS.tellg();
    }

    return 0;
}

bool AndroidFile::SetPos(size_t Offset, FilePosOrigin Origin)
{
    if (m_FS && m_FS.is_open())
    {
        m_FS.seekg(Offset, std::ios_base::beg);
    }
    return false;
}


void AndroidFileSystem::Init(struct AAssetManager* AssetManager,
                             const char*           ExternalFilesDir,
                             const char*           OutputFilesDir)
{
    AndroidFileSystemHelper::GetInstance().Init(AssetManager, ExternalFilesDir, OutputFilesDir);
}

AndroidFile* AndroidFileSystem::OpenFile(const FileOpenAttribs& OpenAttribs)
{
    AndroidFile* pFile = nullptr;
    try
    {
        pFile = new AndroidFile(OpenAttribs);
    }
    catch (const std::runtime_error& err)
    {
    }

    return pFile;
}

bool AndroidFileSystem::FileExists(const Char* strFilePath)
{
    std::fstream    FS;
    AAsset*         AssetFile = nullptr;
    FileOpenAttribs OpenAttribs;
    OpenAttribs.strFilePath = strFilePath;
    BasicFile   DummyFile{OpenAttribs};
    const auto& Path   = DummyFile.GetPath(); // This is necessary to correct slashes
    bool        Exists = AndroidFile::Open(Path.c_str(), FS, AssetFile);

    if (FS && FS.is_open())
        FS.close();
    if (AssetFile != nullptr)
        AAsset_close(AssetFile);

    return Exists;
}

std::string AndroidFileSystem::GetLocalAppDataDirectory(const char* AppName /*= nullptr*/, bool Create /*= true*/)
{
    const std::string& OutputFilesDir = AndroidFileSystemHelper::GetInstance().GetOutputFilesDir();
    if (OutputFilesDir.empty())
    {
        LOG_ERROR_MESSAGE("Output files directory has not been initialized. Call AndroidFileSystem::Init().");
    }
    return OutputFilesDir;
}

} // namespace Diligent
