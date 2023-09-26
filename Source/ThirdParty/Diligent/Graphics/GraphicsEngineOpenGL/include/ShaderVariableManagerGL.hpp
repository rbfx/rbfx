/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

// ShaderVariableManagerGL class manages static resources of a pipeline resource signature, and
// all types of resources for an SRB.

//
//        .-==========================-.              _______________________________________________________________________________________________________________
//        ||                          ||             |           |           |       |            |            |       |            |         |           |          |
//      __|| ShaderVariableManagerGL  ||------------>| UBInfo[0] | UBInfo[1] |  ...  | TexInfo[0] | TexInfo[1] |  ...  | ImgInfo[0] |   ...   |  SSBO[0]  |   ...    |
//     |  ||                          ||             |___________|___________|_______|____________|____________|_______|____________|_________|___________|__________|
//     |  '-==========================-'                          /                         \                              |
//     |                 |                                 m_ResIndex                   m_ResIndex                    m_ResIndex
//     |            m_pSignature                               /                              \                            |
//     |    _____________V___________________         ________V________________________________V___________________________V__________________________________________
//     |   |                                 |       |          |          |       |        |        |       |        |        |       |          |          |       |
//     |   | PipelineResourceSignatureGLImpl |------>|   UB[0]  |   UB[1]  |  ...  | Tex[0] | Tex[1] |  ...  | Img[0] | Img[1] |  ...  | SSBOs[0] | SSBOs[1] |  ...  |
//     |   |_________________________________|       |__________|__________|_______|________|________|_______|________|________|_______|__________|__________|_______|
//     |                                                  |           |                |         |                |        |                |           |
// m_ResourceCache                                     Binding     Binding          Binding    Binding          Binding  Binding         Binding      Binding
//     |                                                  |           |                |         |                |        |                |           |
//     |    _______________________                   ____V___________V________________V_________V________________V________V________________V___________V_____________
//     |   |                       |                 |                           |                           |                           |                           |
//     '-->| ShaderResourceCacheGL |--------     --->|       Uniform Buffers     |          Textures         |          Images           |      Storage Buffers      |
//         |_______________________|                 |___________________________|___________________________|___________________________|___________________________|
//

#include <array>

#include "EngineGLImplTraits.hpp"
#include "Object.h"
#include "PipelineResourceAttribsGL.hpp"
#include "ShaderResourceVariableBase.hpp"
#include "ShaderResourceCacheGL.hpp"

namespace Diligent
{

class PipelineResourceSignatureGLImpl;

// sizeof(ShaderVariableManagerGL) == 40 (x64, msvc, Release)
class ShaderVariableManagerGL : ShaderVariableManagerBase<EngineGLImplTraits, void>
{
public:
    using TBase = ShaderVariableManagerBase<EngineGLImplTraits, void>;
    ShaderVariableManagerGL(IObject& Owner, ShaderResourceCacheGL& ResourceCache) noexcept :
        TBase{Owner, ResourceCache}
    {}

    void Destroy(IMemoryAllocator& Allocator);

    // No copies, only moves are allowed
    // clang-format off
    ShaderVariableManagerGL             (const ShaderVariableManagerGL&)  = delete;
    ShaderVariableManagerGL& operator = (const ShaderVariableManagerGL&)  = delete;
    ShaderVariableManagerGL             (      ShaderVariableManagerGL&&) = default;
    ShaderVariableManagerGL& operator = (      ShaderVariableManagerGL&&) = delete;
    // clang-format on

    void Initialize(const PipelineResourceSignatureGLImpl& Signature,
                    IMemoryAllocator&                      Allocator,
                    const SHADER_RESOURCE_VARIABLE_TYPE*   AllowedVarTypes,
                    Uint32                                 NumAllowedTypes,
                    SHADER_TYPE                            ShaderType);

    static size_t GetRequiredMemorySize(const PipelineResourceSignatureGLImpl& Signature,
                                        const SHADER_RESOURCE_VARIABLE_TYPE*   AllowedVarTypes,
                                        Uint32                                 NumAllowedTypes,
                                        SHADER_TYPE                            ShaderType);

    using ResourceAttribs = PipelineResourceAttribsGL;

    // These two methods can't be implemented in the header because they depend on PipelineResourceSignatureGLImpl
    const PipelineResourceDesc& GetResourceDesc(Uint32 Index) const;
    const ResourceAttribs&      GetResourceAttribs(Uint32 Index) const;

    template <typename ThisImplType>
    struct GLVariableBase : public ShaderVariableBase<ThisImplType, ShaderVariableManagerGL>
    {
    public:
        GLVariableBase(ShaderVariableManagerGL& ParentLayout, Uint32 ResIndex) :
            ShaderVariableBase<ThisImplType, ShaderVariableManagerGL>{ParentLayout, ResIndex}
        {}

        const ResourceAttribs& GetAttribs() const { return this->m_ParentManager.GetResourceAttribs(this->m_ResIndex); }

        void SetDynamicOffset(Uint32 ArrayIndex, Uint32 Offset)
        {
            UNSUPPORTED("Dynamic offset may only be set for uniform and storage buffers");
        }
    };


    struct UniformBuffBindInfo final : GLVariableBase<UniformBuffBindInfo>
    {
        UniformBuffBindInfo(ShaderVariableManagerGL& ParentLayout, Uint32 ResIndex) :
            GLVariableBase<UniformBuffBindInfo>{ParentLayout, ResIndex}
        {}

        void BindResource(const BindResourceInfo& BindInfo);

        virtual IDeviceObject* DILIGENT_CALL_TYPE Get(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < GetDesc().ArraySize);
            const auto& UB = m_ParentManager.m_ResourceCache.GetConstUB(GetAttribs().CacheOffset + ArrayIndex);
            return UB.pBuffer;
        }

        void SetDynamicOffset(Uint32 ArrayIndex, Uint32 Offset);
    };


    struct TextureBindInfo final : GLVariableBase<TextureBindInfo>
    {
        TextureBindInfo(ShaderVariableManagerGL& ParentLayout, Uint32 ResIndex) :
            GLVariableBase<TextureBindInfo>{ParentLayout, ResIndex}
        {}

        void BindResource(const BindResourceInfo& BindInfo);

        virtual IDeviceObject* DILIGENT_CALL_TYPE Get(Uint32 ArrayIndex) const override final
        {
            const auto& Desc = GetDesc();
            VERIFY_EXPR(ArrayIndex < Desc.ArraySize);
            const auto& Tex = m_ParentManager.m_ResourceCache.GetConstTexture(GetAttribs().CacheOffset + ArrayIndex);
            return Tex.pView;
        }
    };


    struct ImageBindInfo final : GLVariableBase<ImageBindInfo>
    {
        ImageBindInfo(ShaderVariableManagerGL& ParentLayout, Uint32 ResIndex) :
            GLVariableBase<ImageBindInfo>{ParentLayout, ResIndex}
        {}

        void BindResource(const BindResourceInfo& BindInfo);

        virtual IDeviceObject* DILIGENT_CALL_TYPE Get(Uint32 ArrayIndex) const override final
        {
            const auto& Desc = GetDesc();
            VERIFY_EXPR(ArrayIndex < Desc.ArraySize);
            const auto& Img = m_ParentManager.m_ResourceCache.GetConstImage(GetAttribs().CacheOffset + ArrayIndex);
            return Img.pView;
        }
    };


    struct StorageBufferBindInfo final : GLVariableBase<StorageBufferBindInfo>
    {
        StorageBufferBindInfo(ShaderVariableManagerGL& ParentLayout, Uint32 ResIndex) :
            GLVariableBase<StorageBufferBindInfo>{ParentLayout, ResIndex}
        {}

        void BindResource(const BindResourceInfo& BindInfo);

        virtual IDeviceObject* DILIGENT_CALL_TYPE Get(Uint32 ArrayIndex) const override final
        {
            VERIFY_EXPR(ArrayIndex < GetDesc().ArraySize);
            const auto& SSBO = m_ParentManager.m_ResourceCache.GetConstSSBO(GetAttribs().CacheOffset + ArrayIndex);
            return SSBO.pBufferView;
        }

        void SetDynamicOffset(Uint32 ArrayIndex, Uint32 Offset);
    };

    void BindResources(IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags);

    void CheckResources(IResourceMapping*                    pResourceMapping,
                        BIND_SHADER_RESOURCES_FLAGS          Flags,
                        SHADER_RESOURCE_VARIABLE_TYPE_FLAGS& StaleVarTypes) const;

    IShaderResourceVariable* GetVariable(const Char* Name) const;
    IShaderResourceVariable* GetVariable(Uint32 Index) const;

    IObject& GetOwner() { return m_Owner; }

    Uint32 GetVariableCount() const
    {
        return GetNumUBs() + GetNumTextures() + GetNumImages() + GetNumStorageBuffers();
    }

    // clang-format off
    Uint32 GetNumUBs()            const { return (m_TextureOffset       - m_UBOffset)            / sizeof(UniformBuffBindInfo);    }
    Uint32 GetNumTextures()       const { return (m_ImageOffset         - m_TextureOffset)       / sizeof(TextureBindInfo);        }
    Uint32 GetNumImages()         const { return (m_StorageBufferOffset - m_ImageOffset)         / sizeof(ImageBindInfo) ;         }
    Uint32 GetNumStorageBuffers() const { return (m_VariableEndOffset   - m_StorageBufferOffset) / sizeof(StorageBufferBindInfo);  }
    // clang-format on

    template <typename ResourceType> Uint32 GetNumResources() const;

    template <typename ResourceType>
    const ResourceType& GetConstResource(Uint32 ResIndex) const
    {
        VERIFY(ResIndex < GetNumResources<ResourceType>(), "Resource index (", ResIndex, ") exceeds max allowed value (", GetNumResources<ResourceType>(), ")");
        auto Offset = GetResourceOffset<ResourceType>();
        return reinterpret_cast<const ResourceType*>(reinterpret_cast<const Uint8*>(m_pVariables) + Offset)[ResIndex];
    }

    Uint32 GetVariableIndex(const IShaderResourceVariable& Var) const;

private:
    struct ResourceCounters
    {
        Uint32 NumUBs           = 0;
        Uint32 NumTextures      = 0;
        Uint32 NumImages        = 0;
        Uint32 NumStorageBlocks = 0;
    };
    static ResourceCounters CountResources(const PipelineResourceSignatureGLImpl& Signature,
                                           const SHADER_RESOURCE_VARIABLE_TYPE*   AllowedVarTypes,
                                           Uint32                                 NumAllowedTypes,
                                           SHADER_TYPE                            ShaderType);

    // Offsets in bytes
    using OffsetType = Uint16;

    template <typename ResourceType> OffsetType GetResourceOffset() const;

    template <typename ResourceType>
    ResourceType& GetResource(Uint32 ResIndex) const
    {
        VERIFY(ResIndex < GetNumResources<ResourceType>(), "Resource index (", ResIndex, ") exceeds max allowed value (", GetNumResources<ResourceType>() - 1, ")");
        auto Offset = GetResourceOffset<ResourceType>();
        return reinterpret_cast<ResourceType*>(reinterpret_cast<Uint8*>(m_pVariables) + Offset)[ResIndex];
    }

    template <typename ResourceType>
    IShaderResourceVariable* GetResourceByName(const Char* Name) const;

    template <typename THandleUB,
              typename THandleTexture,
              typename THandleImage,
              typename THandleStorageBuffer>
    void HandleResources(THandleUB            HandleUB,
                         THandleTexture       HandleTexture,
                         THandleImage         HandleImage,
                         THandleStorageBuffer HandleStorageBuffer)
    {
        for (Uint32 ub = 0; ub < GetNumResources<UniformBuffBindInfo>(); ++ub)
            HandleUB(GetResource<UniformBuffBindInfo>(ub));

        for (Uint32 s = 0; s < GetNumResources<TextureBindInfo>(); ++s)
            HandleTexture(GetResource<TextureBindInfo>(s));

        for (Uint32 i = 0; i < GetNumResources<ImageBindInfo>(); ++i)
            HandleImage(GetResource<ImageBindInfo>(i));

        for (Uint32 s = 0; s < GetNumResources<StorageBufferBindInfo>(); ++s)
            HandleStorageBuffer(GetResource<StorageBufferBindInfo>(s));
    }

    template <typename THandleUB,
              typename THandleTexture,
              typename THandleImage,
              typename THandleStorageBuffer>
    void HandleConstResources(THandleUB            HandleUB,
                              THandleTexture       HandleTexture,
                              THandleImage         HandleImage,
                              THandleStorageBuffer HandleStorageBuffer) const
    {
        for (Uint32 ub = 0; ub < GetNumResources<UniformBuffBindInfo>(); ++ub)
            if (!HandleUB(GetConstResource<UniformBuffBindInfo>(ub)))
                return;

        for (Uint32 s = 0; s < GetNumResources<TextureBindInfo>(); ++s)
            if (!HandleTexture(GetConstResource<TextureBindInfo>(s)))
                return;

        for (Uint32 i = 0; i < GetNumResources<ImageBindInfo>(); ++i)
            if (!HandleImage(GetConstResource<ImageBindInfo>(i)))
                return;

        for (Uint32 s = 0; s < GetNumResources<StorageBufferBindInfo>(); ++s)
            if (!HandleStorageBuffer(GetConstResource<StorageBufferBindInfo>(s)))
                return;
    }

    friend class ShaderVariableIndexLocator;
    friend class ShaderVariableLocator;

private:
    static constexpr OffsetType m_UBOffset            = 0;
    OffsetType                  m_TextureOffset       = 0;
    OffsetType                  m_ImageOffset         = 0;
    OffsetType                  m_StorageBufferOffset = 0;
    OffsetType                  m_VariableEndOffset   = 0;
};


template <>
inline Uint32 ShaderVariableManagerGL::GetNumResources<ShaderVariableManagerGL::UniformBuffBindInfo>() const
{
    return GetNumUBs();
}

template <>
inline Uint32 ShaderVariableManagerGL::GetNumResources<ShaderVariableManagerGL::TextureBindInfo>() const
{
    return GetNumTextures();
}

template <>
inline Uint32 ShaderVariableManagerGL::GetNumResources<ShaderVariableManagerGL::ImageBindInfo>() const
{
    return GetNumImages();
}

template <>
inline Uint32 ShaderVariableManagerGL::GetNumResources<ShaderVariableManagerGL::StorageBufferBindInfo>() const
{
    return GetNumStorageBuffers();
}



template <>
inline ShaderVariableManagerGL::OffsetType ShaderVariableManagerGL::
    GetResourceOffset<ShaderVariableManagerGL::UniformBuffBindInfo>() const
{
    return m_UBOffset;
}

template <>
inline ShaderVariableManagerGL::OffsetType ShaderVariableManagerGL::
    GetResourceOffset<ShaderVariableManagerGL::TextureBindInfo>() const
{
    return m_TextureOffset;
}

template <>
inline ShaderVariableManagerGL::OffsetType ShaderVariableManagerGL::
    GetResourceOffset<ShaderVariableManagerGL::ImageBindInfo>() const
{
    return m_ImageOffset;
}

template <>
inline ShaderVariableManagerGL::OffsetType ShaderVariableManagerGL::
    GetResourceOffset<ShaderVariableManagerGL::StorageBufferBindInfo>() const
{
    return m_StorageBufferOffset;
}

} // namespace Diligent
