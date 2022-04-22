/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include "DeviceObjectArchiveBase.hpp"

namespace Diligent
{

class ArchiveRepacker
{
public:
    using DeviceType = DeviceObjectArchiveBase::DeviceType;

    explicit ArchiveRepacker(IArchive* pArchive);

    void RemoveDeviceData(DeviceType Dev) noexcept(false);
    void AppendDeviceData(const ArchiveRepacker& Src, DeviceType Dev) noexcept(false);
    void Serialize(IFileStream* pStream) noexcept(false);
    bool Validate() const;
    void Print() const;

private:
    using ArchiveHeader     = DeviceObjectArchiveBase::ArchiveHeader;
    using BlockOffsetType   = DeviceObjectArchiveBase::BlockOffsetType;
    using ChunkHeader       = DeviceObjectArchiveBase::ChunkHeader;
    using ChunkType         = DeviceObjectArchiveBase::ChunkType;
    using FileOffsetAndSize = DeviceObjectArchiveBase::FileOffsetAndSize;
    using BaseDataHeader    = DeviceObjectArchiveBase::BaseDataHeader;
    using RPDataHeader      = DeviceObjectArchiveBase::RPDataHeader;
    using ShadersDataHeader = DeviceObjectArchiveBase::ShadersDataHeader;

    using NameOffsetMap = std::unordered_map<HashMapStringKey, FileOffsetAndSize, HashMapStringKey::Hasher>;

    static constexpr auto HeaderMagicNumber = DeviceObjectArchiveBase::HeaderMagicNumber;
    static constexpr auto HeaderVersion     = DeviceObjectArchiveBase::HeaderVersion;
    static constexpr auto InvalidOffset     = DeviceObjectArchiveBase::BaseDataHeader::InvalidOffset;

    void ReadNamedResources(const ChunkHeader& Chunk, NameOffsetMap& NameAndOffset) noexcept(false);

    struct ArchiveBlock
    {
        RefCntAutoPtr<IArchive> pArchive;
        std::vector<Uint8>      Memory; // can be used for patching
        Uint32                  Offset = InvalidOffset;
        Uint32                  Size   = 0;

        ArchiveBlock() noexcept {}
        ArchiveBlock(const ArchiveBlock&) = default;

        ArchiveBlock(IArchive* _pArchive, Uint32 _Offset, Uint32 _Size) noexcept :
            pArchive{_pArchive}, Offset{_Offset}, Size{_Size} {}

        ArchiveBlock& operator=(ArchiveBlock&& Rhs) = default;
        ArchiveBlock& operator=(const ArchiveBlock& Rhs) = default;

        bool IsValid() const { return pArchive != nullptr && Offset != InvalidOffset && Size != 0; }

        bool LoadToMemory();
        bool Read(Uint64 Offset, Uint64 Size, void* pData) const;
        bool Write(Uint64 Offset, Uint64 Size, const void* pData);
    };
    using DeviceSpecificBlocks = std::array<ArchiveBlock, static_cast<size_t>(BlockOffsetType::Count)>;

    ArchiveBlock         m_CommonData;
    DeviceSpecificBlocks m_DeviceSpecific;

    std::vector<ChunkHeader> m_Chunks;

    NameOffsetMap m_PRSMap;
    NameOffsetMap m_GraphicsPSOMap;
    NameOffsetMap m_ComputePSOMap;
    NameOffsetMap m_TilePSOMap;
    NameOffsetMap m_RayTracingPSOMap;
    NameOffsetMap m_RenderPassMap;
};

} // namespace Diligent
