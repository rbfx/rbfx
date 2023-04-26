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

#include <string>

#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace Diligent
{

/// Helper class to manage scoped debug group.
class ScopedDebugGroup
{
public:
    ScopedDebugGroup() noexcept {}

    ScopedDebugGroup(IDeviceContext* pContext,
                     const Char*     Name,
                     const float*    pColor = nullptr) noexcept :
        m_pContext{pContext}
    {
        VERIFY_EXPR(pContext != nullptr && Name != nullptr);
        pContext->BeginDebugGroup(Name, pColor);
    }

    ScopedDebugGroup(IDeviceContext*    pContext,
                     const std::string& Name,
                     const float*       pColor = nullptr) noexcept :
        ScopedDebugGroup{pContext, Name.c_str(), pColor}
    {}

    ~ScopedDebugGroup()
    {
        if (m_pContext != nullptr)
        {
            m_pContext->EndDebugGroup();
        }
    }

    // clang-format off
    ScopedDebugGroup           (const ScopedDebugGroup&) = delete;
    ScopedDebugGroup& operator=(const ScopedDebugGroup&) = delete;
    // clang-format on

    ScopedDebugGroup(ScopedDebugGroup&& rhs) noexcept :
        m_pContext{rhs.m_pContext}
    {
        rhs.m_pContext = nullptr;
    }

    ScopedDebugGroup& operator=(ScopedDebugGroup&& rhs) noexcept
    {
        m_pContext     = rhs.m_pContext;
        rhs.m_pContext = nullptr;
        return *this;
    }

private:
    IDeviceContext* m_pContext = nullptr;
};

} // namespace Diligent
