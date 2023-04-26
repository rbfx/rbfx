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

#include "../../../Primitives/interface/BasicTypes.h"

namespace Diligent
{

enum class ThreadPriority
{
    Unknown,
    Lowest,
    BelowNormal,
    Normal,
    AboveNormal,
    Highest
};

struct BasicPlatformMisc
{
    template <typename Type>
    static Uint32 GetMSB(Type Val)
    {
        if (Val == 0) return sizeof(Type) * 8;

        Uint32 MSB = sizeof(Type) * 8 - 1;
        while (!(Val & (Type{1} << MSB)))
            --MSB;

        return MSB;
    }

    template <typename Type>
    static Uint32 GetLSB(Type Val)
    {
        if (Val == 0) return sizeof(Type) * 8;

        Uint32 LSB = 0;
        while (!(Val & (Type{1} << LSB)))
            ++LSB;

        return LSB;
    }

    template <typename Type>
    static Uint32 CountOneBits(Type Val)
    {
        Uint32 bits = 0;
        while (Val != 0)
        {
            Val &= (Val - 1);
            ++bits;
        }
        return bits;
    }

    template <typename Type>
    static typename std::enable_if<sizeof(Type) == 2, Type>::type SwapBytes(Type Val)
    {
        SwapBytes16(reinterpret_cast<Uint16&>(Val));
        return Val;
    }

    template <typename Type>
    static typename std::enable_if<sizeof(Type) == 4, Type>::type SwapBytes(Type Val)
    {
        SwapBytes32(reinterpret_cast<Uint32&>(Val));
        return Val;
    }

    template <typename Type>
    static typename std::enable_if<sizeof(Type) == 8, Type>::type SwapBytes(Type Val)
    {
        SwapBytes64(reinterpret_cast<Uint64&>(Val));
        return Val;
    }

    static ThreadPriority GetCurrentThreadPriority();

    /// Sets the current thread priority and on success returns the previous priority.
    /// On failure, returns ThreadPriority::Unknown.
    static ThreadPriority SetCurrentThreadPriority(ThreadPriority Priority);

private:
    static void SwapBytes16(Uint16& Val)
    {
        Val = (Val << 8u) | (Val >> 8u);
    }

    static void SwapBytes32(Uint32& Val)
    {
        Val = (Val << 24u) | ((Val & 0xFF00u) << 8u) | ((Val & 0xFF0000u) >> 8u) | (Val >> 24u);
    }

    static void SwapBytes64(Uint64& Val)
    {
        Val = //             -7-6-5-4-3-2-1-0
            ((Val & Uint64{0x00000000000000FF}) << Uint64{7 * 8}) |
            ((Val & Uint64{0x000000000000FF00}) << Uint64{5 * 8}) |
            ((Val & Uint64{0x0000000000FF0000}) << Uint64{3 * 8}) |
            ((Val & Uint64{0x00000000FF000000}) << Uint64{1 * 8}) |
            ((Val & Uint64{0x000000FF00000000}) >> Uint64{1 * 8}) |
            ((Val & Uint64{0x0000FF0000000000}) >> Uint64{3 * 8}) |
            ((Val & Uint64{0x00FF000000000000}) >> Uint64{5 * 8}) |
            ((Val & Uint64{0xFF00000000000000}) >> Uint64{7 * 8});
    }
};

} // namespace Diligent
