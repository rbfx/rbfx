/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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
/// Implementation of the Diligent::DeviceObjectBase template class

#include <cstring>
#include <cstdio>
#include "RefCntAutoPtr.hpp"
#include "ObjectBase.hpp"
#include "UniqueIdentifier.hpp"
#include "EngineMemory.h"

namespace Diligent
{

/// Template class implementing base functionality of the device object
template <class BaseInterface, typename RenderDeviceImplType, typename ObjectDescType>
class DeviceObjectBase : public ObjectBase<BaseInterface>
{
public:
    typedef ObjectBase<BaseInterface> TBase;

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this device object
    /// \param pDevice           - Pointer to the render device.
    /// \param ObjDesc           - Object description.
    /// \param bIsDeviceInternal - Flag indicating if the object is an internal device object
    ///							   and must not keep a strong reference to the device.
    DeviceObjectBase(IReferenceCounters*   pRefCounters,
                     RenderDeviceImplType* pDevice,
                     const ObjectDescType& ObjDesc,
                     bool                  bIsDeviceInternal = false) :
        // clang-format off
        TBase              {pRefCounters},
        m_pDevice          {pDevice          },
        m_Desc             {ObjDesc          },
        m_UniqueID         {pDevice != nullptr ? pDevice->GenerateUniqueId() : 0},
        m_bIsDeviceInternal{bIsDeviceInternal}
    //clang-format on
    {
        // Do not keep strong reference to the device if the object is an internal device object
        if (!m_bIsDeviceInternal)
        {
            // Device can be null if object is used for serialization
            VERIFY_EXPR(m_pDevice != nullptr);
            m_pDevice->AddRef();
        }

        if (ObjDesc.Name != nullptr)
        {
            auto  size     = strlen(ObjDesc.Name) + 1;
            auto* NameCopy = ALLOCATE(GetStringAllocator(), "Object name copy", char, size);
            memcpy(NameCopy, ObjDesc.Name, size);
            m_Desc.Name = NameCopy;
        }
        else
        {
            size_t size       = 16 + 2 + 1; // 0x12345678
            auto*  AddressStr = ALLOCATE(GetStringAllocator(), "Object address string", char, size);
            snprintf(AddressStr, size, "0x%llX", static_cast<unsigned long long>(reinterpret_cast<size_t>(this)));
            m_Desc.Name = AddressStr;
        }

        //                        !!!WARNING!!!
        // We cannot add resource to the hash table from here, because the object
        // has not been completely created yet and the reference counters object
        // is not initialized!
        //m_pDevice->AddResourceToHash( this ); - ERROR!
    }

    // clang-format off
    DeviceObjectBase             (const DeviceObjectBase& ) = delete;
    DeviceObjectBase             (      DeviceObjectBase&&) = delete;
    DeviceObjectBase& operator = (const DeviceObjectBase& ) = delete;
    DeviceObjectBase& operator = (      DeviceObjectBase&&) = delete;
    // clang-format on

    virtual ~DeviceObjectBase()
    {
        FREE(GetStringAllocator(), const_cast<Char*>(m_Desc.Name));

        if (!m_bIsDeviceInternal)
        {
            m_pDevice->Release();
        }
    }

    inline virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() override final
    {
        // Render device owns allocators for all types of device objects,
        // so it must be destroyed after all device objects are released.
        // Consider the following scenario: an object A owns the last strong
        // reference to the device:
        //
        // 1. A::~A() completes
        // 2. A::~DeviceObjectBase() completes
        // 3. A::m_pDevice is released
        //       Render device is destroyed, all allocators are invalid
        // 4. RefCountersImpl::ObjectWrapperBase::DestroyObject() calls
        //    m_pAllocator->Free(m_pObject) - crash!

        RefCntAutoPtr<RenderDeviceImplType> pDevice;
        return TBase::Release(
            [&]() //
            {
                // We must keep the device alive while the object is being destroyed
                // Note that internal device objects do not keep strong reference to the device
                if (!m_bIsDeviceInternal)
                {
                    pDevice = m_pDevice;
                }
            });
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_DeviceObject, TBase)

    virtual const ObjectDescType& DILIGENT_CALL_TYPE GetDesc() const override
    {
        return m_Desc;
    }

    /// Returns unique identifier

    /// \remarks
    ///     This unique ID is used to unambiguously identify device object for
    ///     tracking purposes within the device.
    ///     Neither GL handle nor pointer could be safely used for this purpose
    ///     as both GL reuses released handles and we pool device objects and reuse
    ///     released ones.
    ///
    /// \note
    ///      Objects created from different devices may have the same unique ID.
    virtual Int32 DILIGENT_CALL_TYPE GetUniqueID() const override final
    {
        VERIFY(m_UniqueID != 0, "Unique ID is not initialized. This indicates that this device object has been created without a device");
        return m_UniqueID;
    }

    /// Implementation of IDeviceObject::SetUserData.
    virtual void DILIGENT_CALL_TYPE SetUserData(IObject* pUserData) override final
    {
        m_pUserData = pUserData;
    }

    /// Implementation of IDeviceObject::GetUserData.
    virtual IObject* DILIGENT_CALL_TYPE GetUserData() const override final
    {
        return m_pUserData;
    }

    static bool IsSameObject(const DeviceObjectBase* pObj1, const DeviceObjectBase* pObj2)
    {
        UniqueIdentifier Id1 = (pObj1 != nullptr) ? pObj1->GetUniqueID() : 0;
        UniqueIdentifier Id2 = (pObj2 != nullptr) ? pObj2->GetUniqueID() : 0;
        return Id1 == Id2;
    }

    bool HasDevice() const { return m_pDevice != nullptr; }

    RenderDeviceImplType* GetDevice() const
    {
        VERIFY_EXPR(m_pDevice);
        return m_pDevice;
    }

protected:
    /// Pointer to the device
    RenderDeviceImplType* const m_pDevice;

    /// Object description
    ObjectDescType m_Desc;

    // WARNING: using static counter for unique ID is not safe
    //          as there may be different counter instances in different
    //          executable units (e.g. in different DLLs).
    const UniqueIdentifier m_UniqueID;
    const bool             m_bIsDeviceInternal;

    RefCntAutoPtr<IObject> m_pUserData;
};

} // namespace Diligent
