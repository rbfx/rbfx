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

#include <cstring>

#include "../../../Primitives/interface/BasicTypes.h"

struct XXH3_state_s;

namespace Diligent
{

struct XXH128Hash
{
    Uint64 LowPart  = {};
    Uint64 HighPart = {};

    constexpr bool operator==(const XXH128Hash& RHS) const
    {
        return LowPart == RHS.LowPart && HighPart == RHS.HighPart;
    }
};

struct XXH128State final
{
    XXH128State();

    ~XXH128State();

    XXH128State(const XXH128State& RHS) = delete;

    XXH128State& operator=(const XXH128State& RHS) = delete;

    XXH128State(XXH128State&& RHS) noexcept;

    XXH128State& operator=(XXH128State&& RHS) noexcept;

    void Update(const void* pData, Uint64 Size);

    template <typename Type>
    void Update(const Type& Val)
    {
        Update(&Val, sizeof(Val));
    }

    void Update(const char* pData, size_t Len = 0)
    {
        if (pData == nullptr)
            return;

        if (Len == 0)
            Len = std::strlen(pData);

        Update(static_cast<const void*>(pData), Len);
    }

    XXH128Hash Digest();

private:
    XXH3_state_s* m_State = nullptr;
};

} // namespace Diligent
