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
/// Alignment utilities

#include <cstdint>

#include "../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace Diligent
{

template <typename T>
bool IsPowerOfTwo(T val)
{
    return val > 0 && (val & (val - 1)) == 0;
}

template <typename T1, typename T2>
inline typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type AlignUp(T1 val, T2 alignment)
{
    static_assert(std::is_unsigned<T1>::value == std::is_unsigned<T2>::value, "both types must be signed or unsigned");
    static_assert(!std::is_pointer<T1>::value && !std::is_pointer<T2>::value, "types must not be pointers");
    VERIFY(IsPowerOfTwo(alignment), "Alignment (", alignment, ") must be a power of 2");

    using T = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type;
    return (static_cast<T>(val) + static_cast<T>(alignment - 1)) & ~static_cast<T>(alignment - 1);
}

template <typename PtrType, typename AlignType>
inline PtrType* AlignUp(PtrType* val, AlignType alignment)
{
    return reinterpret_cast<PtrType*>(AlignUp(reinterpret_cast<uintptr_t>(val), static_cast<uintptr_t>(alignment)));
}

template <typename T1, typename T2>
inline typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type AlignDown(T1 val, T2 alignment)
{
    static_assert(std::is_unsigned<T1>::value == std::is_unsigned<T2>::value, "both types must be signed or unsigned");
    static_assert(!std::is_pointer<T1>::value && !std::is_pointer<T2>::value, "types must not be pointers");
    VERIFY(IsPowerOfTwo(alignment), "Alignment (", alignment, ") must be a power of 2");

    using T = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type;
    return static_cast<T>(val) & ~static_cast<T>(alignment - 1);
}

template <typename PtrType, typename AlignType>
inline PtrType* AlignDown(PtrType* val, AlignType alignment)
{
    return reinterpret_cast<PtrType*>(AlignDown(reinterpret_cast<uintptr_t>(val), static_cast<uintptr_t>(alignment)));
}

template <typename T1, typename T2>
inline typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type AlignDownNonPw2(T1 val, T2 alignment)
{
    VERIFY_EXPR(alignment > 0);
    static_assert(std::is_unsigned<T1>::value == std::is_unsigned<T2>::value, "both types must be signed or unsigned");
    static_assert(!std::is_pointer<T1>::value && !std::is_pointer<T2>::value, "types must not be pointers");

    using T = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type;
    return static_cast<T>(val) - (static_cast<T>(val) % static_cast<T>(alignment));
}

template <typename T1, typename T2>
inline typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type AlignUpNonPw2(T1 val, T2 alignment)
{
    VERIFY_EXPR(alignment > 0);
    static_assert(std::is_unsigned<T1>::value == std::is_unsigned<T2>::value, "both types must be signed or unsigned");
    static_assert(!std::is_pointer<T1>::value && !std::is_pointer<T2>::value, "types must not be pointers");

    using T = typename std::conditional<sizeof(T1) >= sizeof(T2), T1, T2>::type;
    T tmp   = static_cast<T>(val) + static_cast<T>(alignment - 1);
    return tmp - (tmp % static_cast<T>(alignment));
}

} // namespace Diligent
