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
/// Implementation of the Diligent::EngineFactoryBase template class

#include "Object.h"
#include "EngineFactory.h"
#include "DefaultShaderSourceStreamFactory.h"
#include "Dearchiver.h"
#include "Atomics.hpp"
#include "DummyReferenceCounters.hpp"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

const APIInfo& GetAPIInfo();

/// Validates engine create info EngineCI and throws an exception in case of an error.
void VerifyEngineCreateInfo(const EngineCreateInfo& EngineCI, const GraphicsAdapterInfo& AdapterInfo) noexcept(false);

/// Template class implementing base functionality of the engine factory

/// \tparam BaseInterface - Base interface that this class will inherit
///                         (Diligent::IEngineFactoryD3D11, Diligent::IEngineFactoryD3D12,
///                          Diligent::IEngineFactoryVk or Diligent::IEngineFactoryOpenGL).
template <class BaseInterface>
class EngineFactoryBase : public BaseInterface
{
public:
    EngineFactoryBase(const INTERFACE_ID& FactoryIID, IDearchiver* pDearchiver) noexcept :
        // clang-format off
        m_FactoryIID  {FactoryIID},
        m_RefCounters {*this     },
        m_pDearchiver {pDearchiver}
    // clang-format on
    {
    }

    virtual ~EngineFactoryBase() = default;

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final
    {
        if (ppInterface == nullptr)
            return;

        *ppInterface = nullptr;
        if (IID == IID_Unknown || IID == m_FactoryIID || IID == IID_EngineFactory)
        {
            *ppInterface = this;
            (*ppInterface)->AddRef();
        }
    }

    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE AddRef() override final
    {
        return m_RefCounters.AddStrongRef();
    }

    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() override final
    {
        return m_RefCounters.ReleaseStrongRef();
    }

    virtual IReferenceCounters* DILIGENT_CALL_TYPE GetReferenceCounters() const override final
    {
        return const_cast<IReferenceCounters*>(static_cast<const IReferenceCounters*>(&m_RefCounters));
    }

    virtual const APIInfo& DILIGENT_CALL_TYPE GetAPIInfo() const override final
    {
        return Diligent::GetAPIInfo();
    }

    virtual void DILIGENT_CALL_TYPE CreateDefaultShaderSourceStreamFactory(const Char*                       SearchDirectories,
                                                                           IShaderSourceInputStreamFactory** ppShaderSourceFactory) const override final
    {
        Diligent::CreateDefaultShaderSourceStreamFactory(SearchDirectories, ppShaderSourceFactory);
    }

    virtual IDearchiver* DILIGENT_CALL_TYPE GetDearchiver() const override final
    {
        return m_pDearchiver.RawPtr<IDearchiver>();
    }

    virtual void DILIGENT_CALL_TYPE SetMessageCallback(DebugMessageCallbackType MessageCallback) const override final
    {
        SetDebugMessageCallback(MessageCallback);
    }

private:
    const INTERFACE_ID                        m_FactoryIID;
    DummyReferenceCounters<EngineFactoryBase> m_RefCounters;

    RefCntAutoPtr<IDearchiver> m_pDearchiver;
};

} // namespace Diligent
