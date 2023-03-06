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

#include <unordered_map>

#include "RefCntAutoPtr.hpp"
#include "DataBlobImpl.hpp"
#include "ObjectBase.hpp"
#include "Serializer.hpp"
#include "BytecodeCache.h"
#include "XXH128Hasher.hpp"
#include "DefaultRawMemoryAllocator.hpp"

namespace Diligent
{

/// Implementation of IBytecodeCache
class BytecodeCacheImpl final : public ObjectBase<IBytecodeCache>
{
public:
    using TBase = ObjectBase<IBytecodeCache>;

    struct BytecodeCacheHeader
    {
        static constexpr Uint32 HeaderMagic   = 0x7ADECACE;
        static constexpr Uint32 HeaderVersion = 1;

        Uint32 Magic   = HeaderMagic;
        Uint32 Version = HeaderVersion;

        Uint64 ElementCount = 0;

        template <typename SerType>
        void Serialize(SerType& Stream)
        {
            Stream(Magic, Version, ElementCount);
        }
    };

    struct BytecodeCacheElementHeader
    {
        XXH128Hash Hash     = {};
        size_t     DataSize = 0;

        template <typename SerType>
        void Serialize(SerType& Stream)
        {
            Stream(Hash.LowPart, Hash.HighPart, DataSize);
        }
    };

public:
    BytecodeCacheImpl(IReferenceCounters*            pRefCounters,
                      const BytecodeCacheCreateInfo& CreateInfo) :
        TBase{pRefCounters},
        m_DeviceType{CreateInfo.DeviceType}
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_BytecodeCache, TBase);

    virtual bool DILIGENT_CALL_TYPE Load(IDataBlob* pDataBlob) override final
    {
        if (pDataBlob == nullptr)
        {
            DEV_ERROR("Data blob must not be null");
            return false;
        }

        Serializer<SerializerMode::Read> Stream{SerializedData{pDataBlob->GetDataPtr(), pDataBlob->GetSize()}};

        BytecodeCacheHeader Header;
        Header.Serialize(Stream);
        if (Header.Magic != BytecodeCacheHeader::HeaderMagic)
        {
            LOG_ERROR_MESSAGE("Incorrect bytecode header magic number");
            return false;
        }

        if (Header.Version != BytecodeCacheHeader::HeaderVersion)
        {
            LOG_ERROR_MESSAGE("Incorrect bytecode header version (", Header.Version, "). ", Uint32{BytecodeCacheHeader::HeaderVersion}, " is expected.");
            return false;
        }

        for (Uint64 ItemID = 0; ItemID < Header.ElementCount; ItemID++)
        {
            BytecodeCacheElementHeader ElementHeader;
            ElementHeader.Serialize(Stream);

            auto pBytecode = DataBlobImpl::Create(ElementHeader.DataSize);
            Stream.CopyBytes(pBytecode->GetDataPtr(), ElementHeader.DataSize);
            m_HashMap.emplace(ElementHeader.Hash, pBytecode);
        }

        return true;
    }

    virtual void DILIGENT_CALL_TYPE GetBytecode(const ShaderCreateInfo& ShaderCI, IDataBlob** ppByteCode) override final
    {
        DEV_CHECK_ERR(ppByteCode != nullptr, "ppByteCode must not be null.");
        DEV_CHECK_ERR(*ppByteCode == nullptr, "*ppByteCode is not null. Make sure you are not overwriting reference to an existing object as this may result in memory leaks.");
        const auto Hash = ComputeHash(ShaderCI);
        const auto Iter = m_HashMap.find(Hash);
        if (Iter != m_HashMap.end())
        {
            auto pObject = Iter->second;
            *ppByteCode  = pObject.Detach();
        }
    }

    virtual void DILIGENT_CALL_TYPE AddBytecode(const ShaderCreateInfo& ShaderCI, IDataBlob* pByteCode) override final
    {
        DEV_CHECK_ERR(pByteCode != nullptr, "pByteCode must not be null.");
        const auto Hash = ComputeHash(ShaderCI);
        const auto Iter = m_HashMap.emplace(Hash, pByteCode);
        if (!Iter.second)
            Iter.first->second = pByteCode;
    }

    virtual void DILIGENT_CALL_TYPE RemoveBytecode(const ShaderCreateInfo& ShaderCI) override final
    {
        const auto Hash = ComputeHash(ShaderCI);
        m_HashMap.erase(Hash);
    }

    virtual void DILIGENT_CALL_TYPE Store(IDataBlob** ppDataBlob) override final
    {
        DEV_CHECK_ERR(ppDataBlob != nullptr, "ppDataBlob must not be null.");
        DEV_CHECK_ERR(*ppDataBlob == nullptr, "*ppDataBlob is not null. Make sure you are not overwriting reference to an existing object as this may result in memory leaks.");

        auto WriteData = [&](auto& Stream) //
        {
            BytecodeCacheHeader Header{};
            Header.ElementCount = m_HashMap.size();
            Header.Serialize(Stream);

            for (auto const& Pair : m_HashMap)
            {
                const auto& pBytecode = Pair.second;

                BytecodeCacheElementHeader ElementHeader;
                ElementHeader.Hash     = Pair.first;
                ElementHeader.DataSize = pBytecode->GetSize();
                ElementHeader.Serialize(Stream);

                Stream.CopyBytes(pBytecode->GetConstDataPtr(), ElementHeader.DataSize);
            }
        };

        Serializer<SerializerMode::Measure> MeasureStream{};
        WriteData(MeasureStream);

        const auto Memory = MeasureStream.AllocateData(DefaultRawMemoryAllocator::GetAllocator());

        Serializer<SerializerMode::Write> WriteStream{Memory};
        WriteData(WriteStream);
        VERIFY_EXPR(WriteStream.IsEnded());

        *ppDataBlob = DataBlobImpl::Create(Memory.Size(), Memory.Ptr()).Detach();
    }

    virtual void DILIGENT_CALL_TYPE Clear() override final
    {
        m_HashMap.clear();
    }

private:
    XXH128Hash ComputeHash(const ShaderCreateInfo& ShaderCI) const
    {
        XXH128State Hasher;
        Hasher.Update(ShaderCI, m_DeviceType);
        return Hasher.Digest();
    }

private:
    RENDER_DEVICE_TYPE m_DeviceType;

    std::unordered_map<XXH128Hash, RefCntAutoPtr<IDataBlob>> m_HashMap;
};

void CreateBytecodeCache(const BytecodeCacheCreateInfo& CreateInfo,
                         IBytecodeCache**               ppCache)
{
    try
    {
        RefCntAutoPtr<IBytecodeCache> pCache{MakeNewRCObj<BytecodeCacheImpl>()(CreateInfo)};
        if (pCache)
            pCache->QueryInterface(IID_BytecodeCache, reinterpret_cast<IObject**>(ppCache));
    }
    catch (...)
    {
        LOG_ERROR("Failed to create the bytecode cache");
    }
}

} // namespace Diligent

extern "C"
{
    void CreateBytecodeCache(const Diligent::BytecodeCacheCreateInfo& CreateInfo,
                             Diligent::IBytecodeCache**               ppCache)
    {
        Diligent::CreateBytecodeCache(CreateInfo, ppCache);
    }
}
