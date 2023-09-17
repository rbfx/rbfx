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

#include "../../Basic/interface/BasicPlatformMisc.hpp"
#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"

#include <intrin.h>

namespace Diligent
{

struct WindowsMisc : public BasicPlatformMisc
{
    inline static Uint32 GetMSB(Uint32 Val)
    {
        if (Val == 0) return 32;

        unsigned long MSB = 32;
        _BitScanReverse(&MSB, Val);
        VERIFY_EXPR(MSB == BasicPlatformMisc::GetMSB(Val));

        return MSB;
    }

    inline static Uint32 GetMSB(Uint64 Val)
    {
        if (Val == 0) return 64;

        unsigned long MSB = 64;
#if _WIN64
        _BitScanReverse64(&MSB, Val);
#else
        Uint32 high = static_cast<Uint32>((Val >> 32) & 0xFFFFFFFF);
        if (high != 0)
        {
            MSB = 32 + GetMSB(high);
        }
        else
        {
            Uint32 low = static_cast<Uint32>(Val & 0xFFFFFFFF);
            VERIFY_EXPR(low != 0);
            MSB = GetMSB(low);
        }
#endif
        VERIFY_EXPR(MSB == BasicPlatformMisc::GetMSB(Val));

        return MSB;
    }

    inline static Uint32 GetLSB(Uint32 Val)
    {
        if (Val == 0) return 32;

        unsigned long LSB = 32;
        _BitScanForward(&LSB, Val);
        VERIFY_EXPR(LSB == BasicPlatformMisc::GetLSB(Val));

        return LSB;
    }

    inline static Uint32 GetLSB(Uint64 Val)
    {
        if (Val == 0) return 64;

        unsigned long LSB = 64;
#if _WIN64
        _BitScanForward64(&LSB, Val);
#else
        Uint32 low = static_cast<Uint32>(Val & 0xFFFFFFFF);
        if (low != 0)
        {
            LSB = GetLSB(low);
        }
        else
        {
            Uint32 high = static_cast<Uint32>((Val >> 32) & 0xFFFFFFFF);
            VERIFY_EXPR(high != 0);
            LSB = 32 + GetLSB(high);
        }
#endif

        VERIFY_EXPR(LSB == BasicPlatformMisc::GetLSB(Val));
        return LSB;
    }

    inline static Uint32 CountOneBits(Uint32 Val)
    {
#if defined _M_ARM || defined _M_ARM64
        // MSVC _CountOneBits intrinsics undefined for ARM64
        // Cast bits to 8x8 datatype and use VCNT on result
        const uint8x8_t Vsum = vcnt_u8(vcreate_u8(static_cast<uint64_t>(Val)));
        // Pairwise sums: 8x8 -> 16x4 -> 32x2
        auto Bits = static_cast<Uint32>(vget_lane_u32(vpaddl_u16(vpaddl_u8(Vsum)), 0));
#else
        auto Bits = __popcnt(Val);
#endif
        VERIFY_EXPR(Bits == BasicPlatformMisc::CountOneBits(Val));
        return Bits;
    }

    inline static Uint32 CountOneBits(Uint64 Val)
    {
#if defined _M_ARM || defined _M_ARM64
        // Cast bits to 8x8 datatype and use VCNT on result
        const uint8x8_t Vsum = vcnt_u8(vcreate_u8(Val));
        // Pairwise sums: 8x8 -> 16x4 -> 32x2 -> 64x1
        auto Bits = static_cast<Uint32>(vget_lane_u64(vpaddl_u32(vpaddl_u16(vpaddl_u8(Vsum))), 0));
#elif _WIN64
        auto Bits = __popcnt64(Val);
#else
        auto Bits =
            CountOneBits(static_cast<Uint32>((Val >> 0) & 0xFFFFFFFF)) +
            CountOneBits(static_cast<Uint32>((Val >> 32) & 0xFFFFFFFF));
#endif
        VERIFY_EXPR(Bits == BasicPlatformMisc::CountOneBits(Val));
        return static_cast<Uint32>(Bits);
    }

    template <typename Type>
    static typename std::enable_if<sizeof(Type) == 2, Type>::type SwapBytes(Type Val)
    {
        auto SwappedBytes = _byteswap_ushort(reinterpret_cast<unsigned short&>(Val));
        return reinterpret_cast<const Type&>(SwappedBytes);
    }

    template <typename Type>
    static typename std::enable_if<sizeof(Type) == 4, Type>::type SwapBytes(Type Val)
    {
        auto SwappedBytes = _byteswap_ulong(reinterpret_cast<unsigned long&>(Val));
        return reinterpret_cast<const Type&>(SwappedBytes);
    }

    template <typename Type>
    static typename std::enable_if<sizeof(Type) == 8, Type>::type SwapBytes(Type Val)
    {
        auto SwappedBytes = _byteswap_uint64(reinterpret_cast<unsigned __int64&>(Val));
        return reinterpret_cast<const Type&>(SwappedBytes);
    }

    /// Sets the current thread affinity mask and on success returns the previous mask.
    /// On failure, returns 0.
    static Uint64 SetCurrentThreadAffinity(Uint64 Mask);

    static ThreadPriority GetCurrentThreadPriority();

    /// Sets the current thread priority and on success returns the previous priority.
    /// On failure, returns ThreadPriority::Unknown.
    static ThreadPriority SetCurrentThreadPriority(ThreadPriority Priority);
};

} // namespace Diligent
