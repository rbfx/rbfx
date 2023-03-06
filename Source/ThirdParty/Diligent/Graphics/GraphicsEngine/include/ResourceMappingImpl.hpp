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

/// \file
/// Declaration of the Diligent::ResourceMappingImpl class

#include <unordered_map>

#include "ResourceMapping.h"
#include "ObjectBase.hpp"
#include "HashUtils.hpp"
#include "STDAllocator.hpp"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Implementation of the resource mapping
class ResourceMappingImpl : public ObjectBase<IResourceMapping>
{
public:
    typedef ObjectBase<IResourceMapping> TObjectBase;

    /// \param pRefCounters - reference counters object that controls the lifetime of this resource mapping
    /// \param RawMemAllocator - raw memory allocator that is used by the m_HashTable member
    ResourceMappingImpl(IReferenceCounters* pRefCounters, IMemoryAllocator& RawMemAllocator) :
        TObjectBase{pRefCounters},
        m_HashTable{STD_ALLOCATOR_RAW_MEM(HashTableElem, RawMemAllocator, "Allocator for unordered_map<ResMappingHashKey, RefCntAutoPtr<IDeviceObject>>")}
    {}

    ~ResourceMappingImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_ResourceMapping, TObjectBase)

    /// Implementation of IResourceMapping::AddResource()
    virtual void DILIGENT_CALL_TYPE AddResource(const Char*    Name,
                                                IDeviceObject* pObject,
                                                bool           bIsUnique) override final;

    /// Implementation of IResourceMapping::AddResourceArray()
    virtual void DILIGENT_CALL_TYPE AddResourceArray(const Char*           Name,
                                                     Uint32                StartIndex,
                                                     IDeviceObject* const* ppObjects,
                                                     Uint32                NumElements,
                                                     bool                  bIsUnique) override final;

    /// Implementation of IResourceMapping::RemoveResourceByName()
    virtual void DILIGENT_CALL_TYPE RemoveResourceByName(const Char* Name, Uint32 ArrayIndex) override final;

    /// Implementation of IResourceMapping::GetResource()
    virtual IDeviceObject* DILIGENT_CALL_TYPE GetResource(const Char* Name,
                                                          Uint32      ArrayIndex) override final;

    /// Returns number of resources in the resource mapping.
    virtual size_t DILIGENT_CALL_TYPE GetSize() override final;

private:
    struct ResMappingHashKey : public HashMapStringKey
    {
        using TBase = HashMapStringKey;

        ResMappingHashKey(const Char* Str, bool bMakeCopy, Uint32 ArrInd) noexcept :
            HashMapStringKey{Str, bMakeCopy},
            ArrayIndex{ArrInd}
        {
            Ownership_Hash = (ComputeHash(GetHash(), ArrInd) & HashMask) | (Ownership_Hash & StrOwnershipMask);
        }

        ResMappingHashKey(ResMappingHashKey&& rhs) noexcept :
            HashMapStringKey{std::move(rhs)},
            ArrayIndex{rhs.ArrayIndex}
        {}

        // clang-format off
        ResMappingHashKey             ( const ResMappingHashKey& ) = delete;
        ResMappingHashKey& operator = ( const ResMappingHashKey& ) = delete;
        ResMappingHashKey& operator = ( ResMappingHashKey&& )      = delete;
        // clang-format on

        bool operator==(const ResMappingHashKey& RHS) const noexcept
        {
            if (ArrayIndex != RHS.ArrayIndex)
            {
                // We must check array index first because TBase::operator==()
                // expects that if the hashes are different, the strings must be
                // different too. This will not be the case for different array
                // elements of the same variable.
                return false;
            }

            return static_cast<const TBase&>(*this) == static_cast<const TBase&>(RHS);
        }

        const Uint32 ArrayIndex;
    };

    Threading::SpinLock m_Lock;

    using HashTableElem = std::pair<const ResMappingHashKey, RefCntAutoPtr<IDeviceObject>>;
    std::unordered_map<ResMappingHashKey,
                       RefCntAutoPtr<IDeviceObject>,
                       ResMappingHashKey::Hasher,
                       std::equal_to<ResMappingHashKey>,
                       STDAllocatorRawMem<HashTableElem>>
        m_HashTable;
};

} // namespace Diligent
