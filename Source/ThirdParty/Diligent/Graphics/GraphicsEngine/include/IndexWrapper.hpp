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

#include "BasicTypes.h"
#include "DebugUtilities.hpp"
#include "Cast.hpp"

namespace Diligent
{

template <typename IndexType, typename UniqueTag>
struct IndexWrapper
{
public:
    IndexWrapper() noexcept {}

    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
    explicit IndexWrapper(T Value) noexcept :
        m_Value{StaticCast<IndexType>(Value)}
    {
    }

    template <typename OtherType, typename OtherTag>
    explicit IndexWrapper(const IndexWrapper<OtherType, OtherTag>& OtherIdx) noexcept :
        IndexWrapper{static_cast<Uint32>(OtherIdx)}
    {
    }

    explicit constexpr IndexWrapper(IndexType Value) :
        m_Value{Value}
    {}

    constexpr IndexWrapper(const IndexWrapper&) = default;
    constexpr IndexWrapper(IndexWrapper&&)      = default;

    constexpr operator Uint32() const
    {
        return m_Value;
    }

    template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
    IndexWrapper& operator=(const T& Value)
    {
        m_Value = static_cast<IndexType>(Value);
        VERIFY(static_cast<T>(m_Value) == Value, "Not enough bits to store value ", Value);
        return *this;
    }

    constexpr bool operator==(const IndexWrapper& Other) const
    {
        return m_Value == Other.m_Value;
    }

    struct Hasher
    {
        size_t operator()(const IndexWrapper& Idx) const noexcept
        {
            return size_t{Idx.m_Value};
        }
    };

private:
    IndexType m_Value = 0;
};

using HardwareQueueIndex = IndexWrapper<Uint8, struct _HardwareQueueIndexTag>;
using SoftwareQueueIndex = IndexWrapper<Uint8, struct _SoftwareQueueIndexTag>;
using DeviceContextIndex = IndexWrapper<Uint8, struct _DeviceContextIndexTag>;

} // namespace Diligent
