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

#include "xxhash.h"

#include "XXH128Hasher.hpp"
#include "DebugUtilities.hpp"
#include "Cast.hpp"

namespace Diligent
{

XXH128State::XXH128State() :
    m_State{XXH3_createState()}
{
    VERIFY_EXPR(m_State != nullptr);
    XXH3_128bits_reset(m_State);
}

XXH128State::~XXH128State()
{
    XXH3_freeState(m_State);
}

XXH128State::XXH128State(XXH128State&& RHS) noexcept :
    m_State{RHS.m_State}
{
    RHS.m_State = nullptr;
}

XXH128State& XXH128State::operator=(XXH128State&& RHS) noexcept
{
    this->m_State = RHS.m_State;
    RHS.m_State   = nullptr;

    return *this;
}

void XXH128State::Update(const void* pData, uint64_t Size)
{
    VERIFY_EXPR(pData != nullptr);
    VERIFY_EXPR(Size != 0);
    XXH3_128bits_update(m_State, pData, StaticCast<size_t>(Size));
}

XXH128Hash XXH128State::Digest()
{
    XXH128_hash_t Hash = XXH3_128bits_digest(m_State);
    return {Hash.low64, Hash.high64};
}

} // namespace Diligent
