/*
 *  Copyright 2024 Diligent Graphics LLC
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
/// Vertex pool C++ struct wrappers.

#include "VertexPool.h"

#include <string>
#include <vector>

namespace Diligent
{

class VertexPoolDescWrapper
{
public:
    VertexPoolDescWrapper(VertexPoolDesc& Desc) :
        m_Desc{Desc}
    {
        SyncDesc();
    }

    VertexPoolDescWrapper& SetName(const char* Name)
    {
        m_Name      = Name != nullptr ? Name : "";
        m_Desc.Name = Name != nullptr ? m_Name.c_str() : nullptr;
        return *this;
    }

    VertexPoolDescWrapper& AddElement(const VertexPoolElementDesc& Element)
    {
        m_Elements.push_back(Element);
        return SyncElements();
    }

    VertexPoolDescWrapper& ClearElements()
    {
        m_Elements.clear();
        return SyncElements();
    }

    VertexPoolDescWrapper& SyncDesc()
    {
        SetName(m_Desc.Name);
        m_Elements.assign(m_Desc.pElements, m_Desc.pElements + m_Desc.NumElements);
        return SyncElements();
    }

    VertexPoolDescWrapper& SetVertexCount(Uint32 VertexCount)
    {
        m_Desc.VertexCount = VertexCount;
        return *this;
    }

    const VertexPoolDesc& Get() const
    {
        return m_Desc;
    }

    operator const VertexPoolDesc&() const
    {
        return m_Desc;
    }

private:
    VertexPoolDescWrapper& SyncElements()
    {
        m_Desc.NumElements = static_cast<Uint32>(m_Elements.size());
        m_Desc.pElements   = m_Elements.data();
        return *this;
    }

private:
    VertexPoolDesc& m_Desc;

    std::string                        m_Name;
    std::vector<VertexPoolElementDesc> m_Elements;
};

struct _VertexPoolDesc
{
public:
    _VertexPoolDesc() noexcept
    {}

    _VertexPoolDesc(const VertexPoolDesc& Desc) noexcept :
        m_PrivateDesc{Desc}
    {}

protected:
    VertexPoolDesc m_PrivateDesc;
};

// We need VertexPoolDesc to be constructed before the VertexPoolDescWrapper.
// At the same time, we don't want to expose it directly to the user.
// If we simply use VertexPoolDesc as private base class, then operator const VertexPoolDesc&
// will be inaccessible. So we use a wrapper class.
struct VertexPoolDescX : private _VertexPoolDesc, public VertexPoolDescWrapper
{
    VertexPoolDescX() :
        VertexPoolDescWrapper{m_PrivateDesc}
    {}

    VertexPoolDescX(const VertexPoolDesc& Desc) :
        _VertexPoolDesc{Desc},
        VertexPoolDescWrapper{m_PrivateDesc}
    {}

    operator const VertexPoolDesc&() const
    {
        return Get();
    }

    VertexPoolDescWrapper& operator=(const VertexPoolDesc& Desc)
    {
        m_PrivateDesc = Desc;
        SyncDesc();
        return *this;
    }
};

struct _VertexPoolCreateInfo
{
public:
    _VertexPoolCreateInfo() noexcept
    {}

    _VertexPoolCreateInfo(const VertexPoolCreateInfo& CI) noexcept :
        m_PrivateCI{CI}
    {}

protected:
    VertexPoolCreateInfo m_PrivateCI;
};

struct VertexPoolCreateInfoX : private _VertexPoolCreateInfo, public VertexPoolDescWrapper
{
    VertexPoolCreateInfoX() :
        VertexPoolDescWrapper{m_PrivateCI.Desc}
    {}

    VertexPoolCreateInfoX(const VertexPoolCreateInfo& CI) :
        _VertexPoolCreateInfo{CI},
        VertexPoolDescWrapper{m_PrivateCI.Desc}
    {}

    VertexPoolCreateInfoX& SetExtraVertexCount(Uint32 _ExtraVertexCount)
    {
        m_PrivateCI.ExtraVertexCount = _ExtraVertexCount;
        return *this;
    }

    VertexPoolCreateInfoX& SetMaxVertexCount(Uint32 _MaxVertexCount)
    {
        m_PrivateCI.MaxVertexCount = _MaxVertexCount;
        return *this;
    }

    VertexPoolCreateInfoX& SetDisableDebugValidation(bool _DisableDebugValidation)
    {
        m_PrivateCI.DisableDebugValidation = _DisableDebugValidation;
        return *this;
    }

    operator const VertexPoolCreateInfo&() const
    {
        return m_PrivateCI;
    }

    VertexPoolDescWrapper& operator=(const VertexPoolCreateInfo& CI)
    {
        m_PrivateCI = CI;
        SyncDesc();
        return *this;
    }
};

} // namespace Diligent
