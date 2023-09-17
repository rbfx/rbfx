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

// ShaderResourcesGL class allocates single continuous chunk of memory to store all program resources, as follows:
//
//
//       m_UniformBuffers        m_Textures                 m_Images                   m_StorageBlocks
//        |                       |                          |                          |                         |                  |
//        |  UB[0]  ... UB[Nu-1]  |  Tex[0]  ...  Tex[Ns-1]  |  Img[0]  ...  Img[Ni-1]  |  SB[0]  ...  SB[Nsb-1]  |  Resource Names  |
//
//  Nu  - number of uniform buffers
//  Ns  - number of samplers
//  Ni  - number of images
//  Nsb - number of storage blocks

#include <vector>

#include "Object.h"
#include "StringPool.hpp"
#include "HashUtils.hpp"
#include "ShaderResourceVariableBase.hpp"
#include "STDAllocator.hpp"

namespace Diligent
{

class ShaderResourcesGL
{
public:
    ShaderResourcesGL() {}
    ~ShaderResourcesGL();
    // clang-format off
    ShaderResourcesGL             (ShaderResourcesGL&& Program)noexcept;

    ShaderResourcesGL             (const ShaderResourcesGL&)  = delete;
    ShaderResourcesGL& operator = (const ShaderResourcesGL&)  = delete;
    ShaderResourcesGL& operator = (      ShaderResourcesGL&&) = delete;
    // clang-format on

    struct LoadUniformsAttribs
    {
        const SHADER_TYPE                     ShaderStages;
        const PIPELINE_RESOURCE_FLAGS         SamplerResourceFlag;
        const GLObjectWrappers::GLProgramObj& GLProgram;
        class GLContextState&                 State;
        const bool                            LoadUniformBufferReflection = false;
        const SHADER_SOURCE_LANGUAGE          SourceLang                  = SHADER_SOURCE_LANGUAGE_DEFAULT;
    };
    /// Loads program uniforms and assigns bindings
    void LoadUniforms(const LoadUniformsAttribs& Attribs);

    struct GLResourceAttribs
    {
        // clang-format off
/*  0 */    const Char*                             Name;
/*  8 */    const SHADER_TYPE                       ShaderStages;
/* 12 */    const SHADER_RESOURCE_TYPE              ResourceType;
/* 13 */    const PIPELINE_RESOURCE_FLAGS           ResourceFlags;
/* 14 */
/* 16 */          Uint32                            ArraySize;
/* 20 */
/* 24 */    // End of data
        // clang-format on

        GLResourceAttribs(const Char*             _Name,
                          SHADER_TYPE             _ShaderStages,
                          SHADER_RESOURCE_TYPE    _ResourceType,
                          PIPELINE_RESOURCE_FLAGS _ResourceFlags,
                          Uint32                  _ArraySize) noexcept :
            // clang-format off
            Name         {_Name         },
            ShaderStages {_ShaderStages },
            ResourceType {_ResourceType },
            ResourceFlags{_ResourceFlags},
            ArraySize    {_ArraySize    }
        // clang-format on
        {
            VERIFY(_ShaderStages != SHADER_TYPE_UNKNOWN, "At least one shader stage must be specified");
            VERIFY(_ResourceType != SHADER_RESOURCE_TYPE_UNKNOWN, "Unknown shader resource type");
            VERIFY(_ArraySize >= 1, "Array size must be greater than 1");
            VERIFY((ResourceFlags & PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER) == 0 || ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV,
                   "PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER is only allowed for texture SRVs");
            VERIFY((ResourceFlags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER) == 0 || ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV || ResourceType == SHADER_RESOURCE_TYPE_BUFFER_UAV,
                   "PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER is only allowed for buffer SRVs and UAVs");
        }

        GLResourceAttribs(const GLResourceAttribs& Attribs,
                          StringPool&              NamesPool) noexcept :
            // clang-format off
            GLResourceAttribs
            {
                NamesPool.CopyString(Attribs.Name),
                Attribs.ShaderStages,
                Attribs.ResourceType,
                Attribs.ResourceFlags,
                Attribs.ArraySize
            }
        // clang-format on
        {
        }

        ShaderResourceDesc GetResourceDesc() const
        {
            ShaderResourceDesc ResourceDesc;
            ResourceDesc.Name      = Name;
            ResourceDesc.ArraySize = ArraySize;
            ResourceDesc.Type      = ResourceType;
            return ResourceDesc;
        }
    };

    struct UniformBufferInfo final : GLResourceAttribs
    {
        // clang-format off
        UniformBufferInfo            (const UniformBufferInfo&)  = delete;
        UniformBufferInfo& operator= (const UniformBufferInfo&)  = delete;
        UniformBufferInfo            (      UniformBufferInfo&&) = default;
        UniformBufferInfo& operator= (      UniformBufferInfo&&) = delete;

        UniformBufferInfo(const Char*           _Name,
                          SHADER_TYPE           _ShaderStages,
                          SHADER_RESOURCE_TYPE  _ResourceType,
                          Uint32                _ArraySize,
                          GLuint                _UBIndex)noexcept :
            GLResourceAttribs{_Name, _ShaderStages, _ResourceType, PIPELINE_RESOURCE_FLAG_NONE, _ArraySize},
            UBIndex          {_UBIndex}
        {}

        UniformBufferInfo(const UniformBufferInfo& UB,
                          StringPool&              NamesPool)noexcept :
            GLResourceAttribs{UB, NamesPool},
            UBIndex          {UB.UBIndex   }
        {}
        // clang-format on

        const GLuint UBIndex;
    };
    static_assert((sizeof(UniformBufferInfo) % sizeof(void*)) == 0, "sizeof(UniformBufferInfo) must be multiple of sizeof(void*)");


    struct TextureInfo final : GLResourceAttribs
    {
        // clang-format off
        TextureInfo            (const TextureInfo&)  = delete;
        TextureInfo& operator= (const TextureInfo&)  = delete;
        TextureInfo            (      TextureInfo&&) = default;
        TextureInfo& operator= (      TextureInfo&&) = delete;

        TextureInfo(const Char*             _Name,
                    SHADER_TYPE             _ShaderStages,
                    SHADER_RESOURCE_TYPE    _ResourceType,
                    PIPELINE_RESOURCE_FLAGS _ResourceFlags,
                    Uint32                  _ArraySize,
                    GLenum                  _TextureType,
                    RESOURCE_DIMENSION      _ResourceDim,
                    bool                    _IsMultisample) noexcept :
            GLResourceAttribs{_Name, _ShaderStages, _ResourceType, _ResourceFlags, _ArraySize},
            TextureType      {_TextureType},
            ResourceDim      {_ResourceDim},
            IsMultisample    {_IsMultisample}
        {}

        TextureInfo(const TextureInfo& Tex,
                    StringPool&        NamesPool) noexcept :
            GLResourceAttribs{Tex, NamesPool},
            TextureType      {Tex.TextureType},
            ResourceDim      {Tex.ResourceDim},
            IsMultisample    {Tex.IsMultisample}
        {}
        // clang-format on

        const GLenum             TextureType;
        const RESOURCE_DIMENSION ResourceDim;
        const bool               IsMultisample;
    };
    static_assert((sizeof(TextureInfo) % sizeof(void*)) == 0, "sizeof(TextureInfo) must be multiple of sizeof(void*)");


    struct ImageInfo final : GLResourceAttribs
    {
        // clang-format off
        ImageInfo            (const ImageInfo&)  = delete;
        ImageInfo& operator= (const ImageInfo&)  = delete;
        ImageInfo            (      ImageInfo&&) = default;
        ImageInfo& operator= (      ImageInfo&&) = delete;

        ImageInfo(const Char*             _Name,
                  SHADER_TYPE             _ShaderStages,
                  SHADER_RESOURCE_TYPE    _ResourceType,
                  PIPELINE_RESOURCE_FLAGS _ResourceFlags,
                  Uint32                  _ArraySize,
                  GLenum                  _ImageType,
                  RESOURCE_DIMENSION      _ResourceDim,
                  bool                    _IsMultisample) noexcept :
            GLResourceAttribs{_Name,  _ShaderStages, _ResourceType, _ResourceFlags, _ArraySize},
            ImageType        {_ImageType},
            ResourceDim      {_ResourceDim},
            IsMultisample    {_IsMultisample}
        {}

        ImageInfo(const ImageInfo& Img,
                  StringPool&      NamesPool) noexcept :
            GLResourceAttribs{Img, NamesPool},
            ImageType        {Img.ImageType},
            ResourceDim      {Img.ResourceDim},
            IsMultisample    {Img.IsMultisample}
        {}
        // clang-format on

        const GLenum             ImageType;
        const RESOURCE_DIMENSION ResourceDim;
        const bool               IsMultisample;
    };
    static_assert((sizeof(ImageInfo) % sizeof(void*)) == 0, "sizeof(ImageInfo) must be multiple of sizeof(void*)");


    struct StorageBlockInfo final : GLResourceAttribs
    {
        // clang-format off
        StorageBlockInfo            (const StorageBlockInfo&)  = delete;
        StorageBlockInfo& operator= (const StorageBlockInfo&)  = delete;
        StorageBlockInfo            (      StorageBlockInfo&&) = default;
        StorageBlockInfo& operator= (      StorageBlockInfo&&) = delete;

        StorageBlockInfo(const Char*          _Name,
                         SHADER_TYPE          _ShaderStages,
                         SHADER_RESOURCE_TYPE _ResourceType,
                         Uint32               _ArraySize,
                         GLint                _SBIndex)noexcept :
            GLResourceAttribs{_Name, _ShaderStages, _ResourceType, PIPELINE_RESOURCE_FLAG_NONE, _ArraySize},
            SBIndex          {_SBIndex}
        {}

        StorageBlockInfo(const StorageBlockInfo& SB,
                         StringPool&             NamesPool)noexcept :
            GLResourceAttribs{SB, NamesPool},
            SBIndex          {SB.SBIndex}
        {}
        // clang-format on

        const GLint SBIndex;
    };
    static_assert((sizeof(StorageBlockInfo) % sizeof(void*)) == 0, "sizeof(StorageBlockInfo) must be multiple of sizeof(void*)");


    // clang-format off
    Uint32 GetNumUniformBuffers()const { return m_NumUniformBuffers; }
    Uint32 GetNumTextures()      const { return m_NumTextures;       }
    Uint32 GetNumImages()        const { return m_NumImages;         }
    Uint32 GetNumStorageBlocks() const { return m_NumStorageBlocks;  }
    // clang-format on

    UniformBufferInfo& GetUniformBuffer(Uint32 Index)
    {
        VERIFY(Index < m_NumUniformBuffers, "Uniform buffer index (", Index, ") is out of range");
        return m_UniformBuffers[Index];
    }

    TextureInfo& GetTexture(Uint32 Index)
    {
        VERIFY(Index < m_NumTextures, "Texture index (", Index, ") is out of range");
        return m_Textures[Index];
    }

    ImageInfo& GetImage(Uint32 Index)
    {
        VERIFY(Index < m_NumImages, "Image index (", Index, ") is out of range");
        return m_Images[Index];
    }

    StorageBlockInfo& GetStorageBlock(Uint32 Index)
    {
        VERIFY(Index < m_NumStorageBlocks, "Storage block index (", Index, ") is out of range");
        return m_StorageBlocks[Index];
    }


    const UniformBufferInfo& GetUniformBuffer(Uint32 Index) const
    {
        VERIFY(Index < m_NumUniformBuffers, "Uniform buffer index (", Index, ") is out of range");
        return m_UniformBuffers[Index];
    }

    const TextureInfo& GetTexture(Uint32 Index) const
    {
        VERIFY(Index < m_NumTextures, "Texture index (", Index, ") is out of range");
        return m_Textures[Index];
    }

    const ImageInfo& GetImage(Uint32 Index) const
    {
        VERIFY(Index < m_NumImages, "Image index (", Index, ") is out of range");
        return m_Images[Index];
    }

    const StorageBlockInfo& GetStorageBlock(Uint32 Index) const
    {
        VERIFY(Index < m_NumStorageBlocks, "Storage block index (", Index, ") is out of range");
        return m_StorageBlocks[Index];
    }

    Uint32 GetVariableCount() const
    {
        return m_NumUniformBuffers + m_NumTextures + m_NumImages + m_NumStorageBlocks;
    }

    ShaderResourceDesc GetResourceDesc(Uint32 Index) const;

    const ShaderCodeBufferDesc* GetUniformBufferDesc(Uint32 Index) const
    {
        if (Index >= GetNumUniformBuffers())
        {
            UNEXPECTED("Uniform buffer index (", Index, ") is out of range.");
            return nullptr;
        }

        if (!m_UBReflectionBuffer)
        {
            UNEXPECTED("Uniform buffer reflection information is not loaded. Please set the LoadConstantBufferReflection flag when creating the shader.");
            return nullptr;
        }

        return reinterpret_cast<const ShaderCodeBufferDesc*>(m_UBReflectionBuffer.get()) + Index;
    }


    SHADER_TYPE GetShaderStages() const { return m_ShaderStages; }

    template <typename THandleUB,
              typename THandleTexture,
              typename THandleImg,
              typename THandleSB>
    void ProcessConstResources(THandleUB                            HandleUB,
                               THandleTexture                       HandleTexture,
                               THandleImg                           HandleImg,
                               THandleSB                            HandleSB,
                               const PipelineResourceLayoutDesc*    pResourceLayout = nullptr,
                               const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes = nullptr,
                               Uint32                               NumAllowedTypes = 0) const
    {
        const Uint32 AllowedTypeBits = GetAllowedTypeBits(AllowedVarTypes, NumAllowedTypes);

        auto CheckResourceType = [&](const char* Name) //
        {
            if (pResourceLayout == nullptr)
                return true;
            else
            {
                auto VarType = GetShaderVariableType(m_ShaderStages, Name, *pResourceLayout);
                return IsAllowedType(VarType, AllowedTypeBits);
            }
        };

        for (Uint32 ub = 0; ub < m_NumUniformBuffers; ++ub)
        {
            const auto& UB = GetUniformBuffer(ub);
            if (CheckResourceType(UB.Name))
                HandleUB(UB);
        }

        for (Uint32 s = 0; s < m_NumTextures; ++s)
        {
            const auto& Sam = GetTexture(s);
            if (CheckResourceType(Sam.Name))
                HandleTexture(Sam);
        }

        for (Uint32 img = 0; img < m_NumImages; ++img)
        {
            const auto& Img = GetImage(img);
            if (CheckResourceType(Img.Name))
                HandleImg(Img);
        }

        for (Uint32 sb = 0; sb < m_NumStorageBlocks; ++sb)
        {
            const auto& SB = GetStorageBlock(sb);
            if (CheckResourceType(SB.Name))
                HandleSB(SB);
        }
    }

    template <typename THandleUB,
              typename THandleTexture,
              typename THandleImg,
              typename THandleSB>
    void ProcessResources(THandleUB      HandleUB,
                          THandleTexture HandleTexture,
                          THandleImg     HandleImg,
                          THandleSB      HandleSB)
    {
        for (Uint32 ub = 0; ub < m_NumUniformBuffers; ++ub)
            HandleUB(GetUniformBuffer(ub));

        for (Uint32 s = 0; s < m_NumTextures; ++s)
            HandleTexture(GetTexture(s));

        for (Uint32 img = 0; img < m_NumImages; ++img)
            HandleImg(GetImage(img));

        for (Uint32 sb = 0; sb < m_NumStorageBlocks; ++sb)
            HandleSB(GetStorageBlock(sb));
    }

private:
    void AllocateResources(std::vector<UniformBufferInfo>& UniformBlocks,
                           std::vector<TextureInfo>&       Textures,
                           std::vector<ImageInfo>&         Images,
                           std::vector<StorageBlockInfo>&  StorageBlocks);

    // clang-format off
    // There could be more than one stage if using non-separable programs
    SHADER_TYPE         m_ShaderStages = SHADER_TYPE_UNKNOWN;

    // Memory layout:
    //
    //  |  Uniform buffers  |   Textures  |   Images   |   Storage Blocks   |    String Pool Data   |
    //

    UniformBufferInfo*  m_UniformBuffers = nullptr;
    TextureInfo*        m_Textures       = nullptr;
    ImageInfo*          m_Images         = nullptr;
    StorageBlockInfo*   m_StorageBlocks  = nullptr;

    Uint32              m_NumUniformBuffers = 0;
    Uint32              m_NumTextures       = 0;
    Uint32              m_NumImages         = 0;
    Uint32              m_NumStorageBlocks  = 0;
    // clang-format on

    std::unique_ptr<void, STDDeleterRawMem<void>> m_UBReflectionBuffer;

    // When adding new member DO NOT FORGET TO UPDATE ShaderResourcesGL( ShaderResourcesGL&& ProgramResources )!!!
};

} // namespace Diligent
