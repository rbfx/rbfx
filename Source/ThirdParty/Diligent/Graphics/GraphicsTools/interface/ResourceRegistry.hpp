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

#include <vector>

#include "../../../Common/interface/RefCntAutoPtr.hpp"
#include "../../GraphicsEngine/interface/Buffer.h"
#include "../../GraphicsEngine/interface/Texture.h"
#include "GraphicsUtilities.h"

namespace Diligent
{

/// Helper class that facilitates resource management.
class ResourceRegistry
{
public:
    using ResourceIdType = Uint32;

    ResourceRegistry()  = default;
    ~ResourceRegistry() = default;

    explicit ResourceRegistry(size_t ResourceCount) :
        m_Resources(ResourceCount)
    {}

    void SetSize(size_t ResourceCount)
    {
        m_Resources.resize(ResourceCount);
    }

    size_t GetSize() const
    {
        return m_Resources.size();
    }

    void Insert(ResourceIdType Id, IDeviceObject* pObject)
    {
        DEV_CHECK_ERR(Id < m_Resources.size(), "Resource index is out of range");
        m_Resources[Id] = pObject;
    }

    template <typename T>
    struct ResourceAccessor
    {
        explicit operator bool() const
        {
            return Object != nullptr;
        }

        ITexture* AsTexture() const
        {
            DEV_CHECK_ERR(Object != nullptr, "Resource is null");
            DEV_CHECK_ERR(RefCntAutoPtr<ITexture>(Object, IID_Texture), "Resource is not a texture");
            return StaticCast<ITexture*>(Object);
        }

        IBuffer* AsBuffer() const
        {
            DEV_CHECK_ERR(Object != nullptr, "Resource is null");
            DEV_CHECK_ERR(RefCntAutoPtr<IBuffer>(Object, IID_Buffer), "Resource is not a buffer");
            return StaticCast<IBuffer*>(Object);
        }

        operator ITexture*() const
        {
            return AsTexture();
        }

        operator IBuffer*() const
        {
            return AsBuffer();
        }

        operator IDeviceObject*() const
        {
            return Object;
        }

        IDeviceObject* operator->() const
        {
            return Object;
        }

        ITextureView* GetTextureSRV() const
        {
            return GetTextureDefaultSRV(Object);
        }

        ITextureView* GetTextureRTV() const
        {
            return GetTextureDefaultRTV(Object);
        }

        ITextureView* GetTextureDSV() const
        {
            return GetTextureDefaultDSV(Object);
        }

        ITextureView* GetTextureUAV() const
        {
            return GetTextureDefaultUAV(Object);
        }

        IBufferView* GetBufferSRV() const
        {
            return GetBufferDefaultSRV(Object);
        }

        IBufferView* GetBufferUAV() const
        {
            return GetBufferDefaultUAV(Object);
        }

        template <typename Y = T, typename = typename std::enable_if<!std::is_const<Y>::value>::type>
        void Release()
        {
            Object.Release();
        }

    private:
        ResourceAccessor(T& _Object) :
            Object{_Object}
        {}
        friend ResourceRegistry;
        T& Object;
    };

    ResourceAccessor<const RefCntAutoPtr<IDeviceObject>> operator[](ResourceIdType Id) const
    {
        DEV_CHECK_ERR(Id < m_Resources.size(), "Resource index is out of range");
        return {m_Resources[Id]};
    }

    ResourceAccessor<RefCntAutoPtr<IDeviceObject>> operator[](ResourceIdType Id)
    {
        DEV_CHECK_ERR(Id < m_Resources.size(), "Resource index is out of range");
        return {m_Resources[Id]};
    }

    void Clear()
    {
        for (auto& pRes : m_Resources)
            pRes.Release();
    }

private:
    std::vector<RefCntAutoPtr<IDeviceObject>> m_Resources;
};

} // namespace Diligent
