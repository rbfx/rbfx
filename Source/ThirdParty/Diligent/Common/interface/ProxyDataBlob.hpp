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
/// Implementation for the IDataBlob interface

#include "../../Primitives/interface/BasicTypes.h"
#include "../../Primitives/interface/DataBlob.h"
#include "ObjectBase.hpp"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

/// Implementation of the proxy data blob that does not own the data
class ProxyDataBlob : public ObjectBase<IDataBlob>
{
public:
    typedef ObjectBase<IDataBlob> TBase;

    ProxyDataBlob(IReferenceCounters* pRefCounters, void* pData, size_t Size) :
        TBase{pRefCounters},
        m_pData{pData},
        m_pConstData{pData},
        m_Size{Size}
    {}

    ProxyDataBlob(IReferenceCounters* pRefCounters, const void* pData, size_t Size) :
        TBase{pRefCounters},
        m_pData{nullptr},
        m_pConstData{pData},
        m_Size{Size}
    {}

    template <typename... ArgsType>
    static RefCntAutoPtr<ProxyDataBlob> Create(ArgsType&&... Args)
    {
        return RefCntAutoPtr<ProxyDataBlob>{MakeNewRCObj<ProxyDataBlob>()(std::forward<ArgsType>(Args)...)};
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_DataBlob, TBase)

    /// Sets the size of the internal data buffer
    virtual void DILIGENT_CALL_TYPE Resize(size_t NewSize) override
    {
        UNEXPECTED("Resize is not supported by proxy data blob.");
    }

    /// Returns the size of the internal data buffer
    virtual size_t DILIGENT_CALL_TYPE GetSize() const override
    {
        return m_Size;
    }

    /// Returns the pointer to the internal data buffer
    virtual void* DILIGENT_CALL_TYPE GetDataPtr() override
    {
        return m_pData;
    }

    /// Returns the pointer to the internal data buffer
    virtual const void* DILIGENT_CALL_TYPE GetConstDataPtr() const override
    {
        return m_pConstData;
    }

private:
    void* const       m_pData;
    const void* const m_pConstData;
    const size_t      m_Size;
};

} // namespace Diligent
