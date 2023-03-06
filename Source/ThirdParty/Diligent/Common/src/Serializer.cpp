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

#include "Serializer.hpp"

#include "HashUtils.hpp"

namespace Diligent
{

SerializedData::SerializedData(void* pData, size_t Size) noexcept :
    m_Ptr{pData},
    m_Size{Size}
{}

SerializedData::SerializedData(size_t Size, IMemoryAllocator& Allocator) noexcept :
    m_pAllocator{Size > 0 ? &Allocator : nullptr},
    m_Ptr{Size > 0 ? Allocator.Allocate(Size, "Serialized data memory", __FILE__, __LINE__) : nullptr},
    m_Size{Size}
{
    // We need to zero out memory as due to element alignment, there may be gaps
    // in the data that will be filled with garbage, which will result in
    // operator==() and GetHash() returning invalid values.
    std::memset(m_Ptr, 0, m_Size);
}

SerializedData::~SerializedData()
{
    Free();
}

void SerializedData::Free()
{
    if (m_pAllocator != nullptr)
    {
        m_pAllocator->Free(m_Ptr);
    }

    m_pAllocator = nullptr;
    m_Ptr        = nullptr;
    m_Size       = 0;
    m_Hash.store(0);
}

SerializedData& SerializedData::operator=(SerializedData&& Rhs) noexcept
{
    Free();

    ASSERT_SIZEOF64(*this, 32, "Please handle new members here");

    m_pAllocator = Rhs.m_pAllocator;
    m_Ptr        = Rhs.m_Ptr;
    m_Size       = Rhs.m_Size;
    m_Hash.store(Rhs.m_Hash.load());

    Rhs.m_pAllocator = nullptr;
    Rhs.m_Ptr        = nullptr;
    Rhs.m_Size       = 0;
    Rhs.m_Hash.store(0);
    return *this;
}

size_t SerializedData::GetHash() const
{
    if (m_Ptr == nullptr || m_Size == 0)
        return 0;

    auto Hash = m_Hash.load();
    if (Hash != 0)
        return Hash;

    Hash = ComputeHash(m_Size, ComputeHashRaw(m_Ptr, m_Size));
    m_Hash.store(Hash);

    return Hash;
}

bool SerializedData::operator==(const SerializedData& Rhs) const noexcept
{
    if (m_Size != Rhs.m_Size)
        return false;

    return std::memcmp(m_Ptr, Rhs.m_Ptr, m_Size) == 0;
}

SerializedData SerializedData::MakeCopy(IMemoryAllocator& Allocator) const
{
    SerializedData Copy{m_Size, Allocator};
    if (m_Ptr != nullptr)
        std::memcpy(Copy.m_Ptr, m_Ptr, m_Size);

    return Copy;
}

} // namespace Diligent
