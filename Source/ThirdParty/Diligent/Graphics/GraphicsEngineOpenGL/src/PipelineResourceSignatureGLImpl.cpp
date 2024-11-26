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

#include "pch.h"
#include "PipelineResourceSignatureGLImpl.hpp"

#include <algorithm>
#include <limits>
#include <functional>

#include "RenderDeviceGLImpl.hpp"

namespace Diligent
{

const char* GetBindingRangeName(BINDING_RANGE Range)
{
    static_assert(BINDING_RANGE_COUNT == 4, "Please update the switch below to handle the new shader resource range");
    switch (Range)
    {
        // clang-format off
        case BINDING_RANGE_UNIFORM_BUFFER: return "Uniform buffer";
        case BINDING_RANGE_TEXTURE:        return "Texture";
        case BINDING_RANGE_IMAGE:          return "Image";
        case BINDING_RANGE_STORAGE_BUFFER: return "Storage buffer";
        // clang-format on
        default:
            return "Unknown";
    }
}

BINDING_RANGE PipelineResourceToBindingRange(const PipelineResourceDesc& Desc)
{
    static_assert(SHADER_RESOURCE_TYPE_LAST == 8, "Please update the switch below to handle the new shader resource type");
    switch (Desc.ResourceType)
    {
        // clang-format off
        case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:  return BINDING_RANGE_UNIFORM_BUFFER;
        case SHADER_RESOURCE_TYPE_TEXTURE_SRV:      return BINDING_RANGE_TEXTURE;
        case SHADER_RESOURCE_TYPE_BUFFER_SRV:       return (Desc.Flags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER) ? BINDING_RANGE_TEXTURE : BINDING_RANGE_STORAGE_BUFFER;
        case SHADER_RESOURCE_TYPE_TEXTURE_UAV:      return BINDING_RANGE_IMAGE;
        case SHADER_RESOURCE_TYPE_BUFFER_UAV:       return (Desc.Flags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER) ? BINDING_RANGE_IMAGE : BINDING_RANGE_STORAGE_BUFFER;
        case SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT: return BINDING_RANGE_TEXTURE;
        // clang-format on
        case SHADER_RESOURCE_TYPE_SAMPLER:
        case SHADER_RESOURCE_TYPE_ACCEL_STRUCT:
        default:
            UNEXPECTED("Unsupported resource type");
            return BINDING_RANGE_UNKNOWN;
    }
}


PipelineResourceSignatureGLImpl::PipelineResourceSignatureGLImpl(IReferenceCounters*                  pRefCounters,
                                                                 RenderDeviceGLImpl*                  pDeviceGL,
                                                                 const PipelineResourceSignatureDesc& Desc,
                                                                 SHADER_TYPE                          ShaderStages,
                                                                 bool                                 bIsDeviceInternal) :
    TPipelineResourceSignatureBase{pRefCounters, pDeviceGL, Desc, ShaderStages, bIsDeviceInternal}
{
    try
    {
        Initialize(
            GetRawAllocator(), Desc, /*CreateImmutableSamplers = */ true,
            [this]() //
            {
                CreateLayout(/*IsSerialized*/ false);
            },
            [this]() //
            {
                return ShaderResourceCacheGL::GetRequiredMemorySize(m_BindingCount);
            });
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

void PipelineResourceSignatureGLImpl::CreateLayout(const bool IsSerialized)
{
    TBindings StaticResCounter = {};

    for (Uint32 i = 0; i < m_Desc.NumResources; ++i)
    {
        const auto& ResDesc = m_Desc.Resources[i];
        VERIFY(i == 0 || ResDesc.VarType >= m_Desc.Resources[i - 1].VarType, "Resources must be sorted by variable type");

        auto* const pAttribs = m_pResourceAttribs + i;
        if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER)
        {
            const auto ImtblSamplerIdx = FindImmutableSampler(ResDesc.ShaderStages, ResDesc.Name);
            // Create sampler resource without cache space
            if (!IsSerialized)
            {
                new (pAttribs) ResourceAttribs //
                    {
                        ResourceAttribs::InvalidCacheOffset,
                        ImtblSamplerIdx == InvalidImmutableSamplerIndex ? ResourceAttribs::InvalidSamplerInd : ImtblSamplerIdx,
                        ImtblSamplerIdx != InvalidImmutableSamplerIndex //
                    };
            }
            else
            {
                DEV_CHECK_ERR(pAttribs->CacheOffset == ResourceAttribs::InvalidCacheOffset, "Deserialized cache offset is invalid.");
                DEV_CHECK_ERR(pAttribs->SamplerInd == (ImtblSamplerIdx == InvalidImmutableSamplerIndex ? ResourceAttribs::InvalidSamplerInd : ImtblSamplerIdx),
                              "Deserialized sampler index is invalid.");
                DEV_CHECK_ERR(pAttribs->IsImmutableSamplerAssigned() == (ImtblSamplerIdx != InvalidImmutableSamplerIndex),
                              "Deserialized immutable sampler flag is invalid.");
            }
        }
        else
        {
            const auto Range = PipelineResourceToBindingRange(ResDesc);
            VERIFY_EXPR(Range != BINDING_RANGE_UNKNOWN);

            auto ImtblSamplerIdx = InvalidImmutableSamplerIndex;
            auto SamplerIdx      = ResourceAttribs::InvalidSamplerInd;
            if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV)
            {
                // Do not use combined sampler suffix - in OpenGL immutable samplers should be defined for textures directly
                ImtblSamplerIdx = Diligent::FindImmutableSampler(m_Desc.ImmutableSamplers, m_Desc.NumImmutableSamplers,
                                                                 ResDesc.ShaderStages, ResDesc.Name, nullptr);
                if (ImtblSamplerIdx != InvalidImmutableSamplerIndex)
                    SamplerIdx = ImtblSamplerIdx;
                else
                    SamplerIdx = FindAssignedSampler(ResDesc, ResourceAttribs::InvalidSamplerInd);
            }

            auto& CacheOffset = m_BindingCount[Range];
            if (!IsSerialized)
            {
                new (pAttribs) ResourceAttribs //
                    {
                        CacheOffset,
                        SamplerIdx,
                        ImtblSamplerIdx != InvalidImmutableSamplerIndex // _ImtblSamplerAssigned
                    };
            }
            else
            {
                DEV_CHECK_ERR(pAttribs->CacheOffset == CacheOffset, "Deserialized cache offset (", pAttribs->CacheOffset, ") is invalid: ", CacheOffset, " is expected.");
                DEV_CHECK_ERR(pAttribs->SamplerInd == SamplerIdx, "Deserialized sampler index (", pAttribs->SamplerInd, ") is invalid: ", SamplerIdx, " is expected.");
                DEV_CHECK_ERR(pAttribs->IsImmutableSamplerAssigned() == (ImtblSamplerIdx != InvalidImmutableSamplerIndex),
                              "Deserialized immutable sampler flag is invalid.");
            }

            if (Range == BINDING_RANGE_UNIFORM_BUFFER && (ResDesc.Flags & PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS) == 0)
            {
                DEV_CHECK_ERR(size_t{CacheOffset} + ResDesc.ArraySize < sizeof(m_DynamicUBOMask) * 8, "Dynamic UBO index exceeds maximum representable bit position in the mask");
                for (Uint64 elem = 0; elem < ResDesc.ArraySize; ++elem)
                    m_DynamicUBOMask |= Uint64{1} << (Uint64{CacheOffset} + elem);
            }
            else if (Range == BINDING_RANGE_STORAGE_BUFFER && (ResDesc.Flags & PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS) == 0)
            {
                DEV_CHECK_ERR(size_t{CacheOffset} + ResDesc.ArraySize < sizeof(m_DynamicSSBOMask) * 8, "Dynamic SSBO index exceeds maximum representable bit position in the mask");
                for (Uint64 elem = 0; elem < ResDesc.ArraySize; ++elem)
                    m_DynamicSSBOMask |= Uint64{1} << (Uint64{CacheOffset} + elem);
            }

            VERIFY(CacheOffset + ResDesc.ArraySize <= std::numeric_limits<TBindings::value_type>::max(), "Cache offset exceeds representable range");
            CacheOffset += static_cast<TBindings::value_type>(ResDesc.ArraySize);

            if (ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
            {
                // Since resources in the static cache are indexed by the same bindings, we need to
                // make sure that there is enough space in the cache.
                StaticResCounter[Range] = std::max(StaticResCounter[Range], CacheOffset);
            }
        }
    }

    if (m_pStaticResCache)
    {
        m_pStaticResCache->Initialize(StaticResCounter, GetRawAllocator(), 0x0, 0x0);
    }
}

PipelineResourceSignatureGLImpl::~PipelineResourceSignatureGLImpl()
{
    Destruct();
}

namespace
{

inline void ApplyUBBindigs(GLuint glProg, const char* UBName, Uint32 BaseBinding, Uint32 ArraySize)
{
    const auto UniformBlockIndex = glGetUniformBlockIndex(glProg, UBName);
    VERIFY(UniformBlockIndex != GL_INVALID_INDEX,
           "Failed to find uniform buffer '", UBName,
           "', which is unexpected as it is present in the list of program resources.");

    for (Uint32 ArrInd = 0; ArrInd < ArraySize; ++ArrInd)
    {
        glUniformBlockBinding(glProg, UniformBlockIndex + ArrInd, BaseBinding + ArrInd);
        DEV_CHECK_GL_ERROR("Failed to set binding point for uniform buffer '", UBName, '\'');
    }
}

inline void ApplyTextureBindings(GLuint glProg, const char* TexName, Uint32 BaseBinding, Uint32 ArraySize)
{
    const auto UniformLocation = glGetUniformLocation(glProg, TexName);
    VERIFY(UniformLocation >= 0, "Failed to find texture '", TexName,
           "', which is unexpected as it is present in the list of program resources.");

    for (Uint32 ArrInd = 0; ArrInd < ArraySize; ++ArrInd)
    {
        glUniform1i(UniformLocation + ArrInd, BaseBinding + ArrInd);
        DEV_CHECK_GL_ERROR("Failed to set binding point for sampler uniform '", TexName, '\'');
    }
}

#if GL_ARB_shader_image_load_store
inline void ApplyImageBindings(GLuint glProg, const char* ImgName, Uint32 BaseBinding, Uint32 ArraySize)
{
    const auto UniformLocation = glGetUniformLocation(glProg, ImgName);
    VERIFY(UniformLocation >= 0, "Failed to find image '", ImgName,
           "', which is unexpected as it is present in the list of program resources.");

    for (Uint32 ArrInd = 0; ArrInd < ArraySize; ++ArrInd)
    {
        // glUniform1i for image uniforms is not supported in at least GLES3.2.
        // glProgramUniform1i is not available in GLES3.0
        const Uint32 ImgBinding = BaseBinding + ArrInd;
        glUniform1i(UniformLocation + ArrInd, ImgBinding);
#    ifdef DILIGENT_DEVELOPMENT
        if (glGetError() != GL_NO_ERROR)
        {
            if (ArraySize > 1)
            {
                LOG_WARNING_MESSAGE("Failed to set binding for image uniform '", ImgName, "'[", ArrInd,
                                    "]. Expected binding: ", ImgBinding,
                                    ". Make sure that this binding is explicitly assigned in shader source code."
                                    " Note that if the source code is converted from HLSL and if images are only used"
                                    " by a single shader stage, then bindings automatically assigned by HLSL->GLSL"
                                    " converter will work fine.");
            }
            else
            {
                LOG_WARNING_MESSAGE("Failed to set binding for image uniform '", ImgName,
                                    "'. Expected binding: ", ImgBinding,
                                    ". Make sure that this binding is explicitly assigned in shader source code."
                                    " Note that if the source code is converted from HLSL and if images are only used"
                                    " by a single shader stage, then bindings automatically assigned by HLSL->GLSL"
                                    " converter will work fine.");
            }
        }
#    endif
    }
}
#endif

#if GL_ARB_shader_storage_buffer_object
inline void ApplySSBOBindings(GLuint glProg, const char* SBName, Uint32 BaseBinding, Uint32 ArraySize)
{
    const auto SBIndex = glGetProgramResourceIndex(glProg, GL_SHADER_STORAGE_BLOCK, SBName);
    VERIFY(SBIndex != GL_INVALID_INDEX, "Failed to find storage buffer '", SBName,
           "', which is unexpected as it is present in the list of program resources.");

    if (glShaderStorageBlockBinding)
    {
        for (Uint32 ArrInd = 0; ArrInd < ArraySize; ++ArrInd)
        {
            glShaderStorageBlockBinding(glProg, SBIndex + ArrInd, BaseBinding + ArrInd);
            DEV_CHECK_GL_ERROR("glShaderStorageBlockBinding() failed");
        }
    }
    else
    {
        const GLenum props[]                 = {GL_BUFFER_BINDING};
        GLint        params[_countof(props)] = {};
        glGetProgramResourceiv(glProg, GL_SHADER_STORAGE_BLOCK, SBIndex, _countof(props), props, _countof(params), nullptr, params);
        DEV_CHECK_GL_ERROR("glGetProgramResourceiv() failed");

        if (BaseBinding != static_cast<Uint32>(params[0]))
        {
            LOG_WARNING_MESSAGE("glShaderStorageBlockBinding is not available on this device and "
                                "the engine is unable to automatically assign shader storage block binding for '",
                                SBName, "' variable. Expected binding: ", BaseBinding, ", actual binding: ", params[0],
                                ". Make sure that this binding is explicitly assigned in shader source code."
                                " Note that if the source code is converted from HLSL and if storage blocks are only used"
                                " by a single shader stage, then bindings automatically assigned by HLSL->GLSL"
                                " converter will work fine.");
        }
    }
}
#endif

} // namespace

void PipelineResourceSignatureGLImpl::ApplyBindings(GLObjectWrappers::GLProgramObj& GLProgram,
                                                    const ShaderResourcesGL&        ProgResources,
                                                    GLContextState&                 State,
                                                    const TBindings&                BaseBindings) const
{
    VERIFY(GLProgram != 0, "Null GL program");
    State.SetProgram(GLProgram);

    auto AssignBinding = [&](const ShaderResourcesGL::GLResourceAttribs& Attribs, BINDING_RANGE Range) //
    {
        const auto ResIdx = FindResource(Attribs.ShaderStages, Attribs.Name);
        if (ResIdx == InvalidPipelineResourceIndex)
            return; // The resource is defined in another signature

        const auto& ResDesc = m_Desc.Resources[ResIdx];
        const auto& ResAttr = m_pResourceAttribs[ResIdx];

        VERIFY_EXPR(strcmp(ResDesc.Name, Attribs.Name) == 0);
        VERIFY_EXPR(ResDesc.ResourceType != SHADER_RESOURCE_TYPE_SAMPLER);
        VERIFY_EXPR(Attribs.ArraySize <= ResDesc.ArraySize);

        VERIFY_EXPR(Range == PipelineResourceToBindingRange(ResDesc));
        const Uint32 BindingIndex = BaseBindings[Range] + ResAttr.CacheOffset;

        static_assert(BINDING_RANGE_COUNT == 4, "Please update the switch below to handle the new shader resource range");
        switch (Range)
        {
            case BINDING_RANGE_UNIFORM_BUFFER:
                ApplyUBBindigs(GLProgram, ResDesc.Name, BindingIndex, Attribs.ArraySize);
                break;

            case BINDING_RANGE_TEXTURE:
                ApplyTextureBindings(GLProgram, ResDesc.Name, BindingIndex, Attribs.ArraySize);
                break;

#if GL_ARB_shader_image_load_store
            case BINDING_RANGE_IMAGE:
                ApplyImageBindings(GLProgram, ResDesc.Name, BindingIndex, Attribs.ArraySize);
                break;
#endif

#if GL_ARB_shader_storage_buffer_object
            case BINDING_RANGE_STORAGE_BUFFER:
                ApplySSBOBindings(GLProgram, ResDesc.Name, BindingIndex, Attribs.ArraySize);
                break;
#endif

            default:
                UNEXPECTED("Unsupported shader resource range type.");
        }
    };


    ProgResources.ProcessConstResources(
        std::bind(AssignBinding, std::placeholders::_1, BINDING_RANGE_UNIFORM_BUFFER),
        std::bind(AssignBinding, std::placeholders::_1, BINDING_RANGE_TEXTURE),
        std::bind(AssignBinding, std::placeholders::_1, BINDING_RANGE_IMAGE),
        std::bind(AssignBinding, std::placeholders::_1, BINDING_RANGE_STORAGE_BUFFER));

    State.SetProgram(GLObjectWrappers::GLProgramObj::Null());
}

void PipelineResourceSignatureGLImpl::CopyStaticResources(ShaderResourceCacheGL& DstResourceCache) const
{
    if (m_pStaticResCache == nullptr)
        return;

    // SrcResourceCache contains only static resources.
    // In case of SRB, DstResourceCache contains static, mutable and dynamic resources.
    // In case of Signature, DstResourceCache contains only static resources.
    const auto& SrcResourceCache = *m_pStaticResCache;

    VERIFY_EXPR(SrcResourceCache.GetContentType() == ResourceCacheContentType::Signature);
    const auto DstCacheType = DstResourceCache.GetContentType();

    const auto StaticResIdxRange = GetResourceIndexRange(SHADER_RESOURCE_VARIABLE_TYPE_STATIC);
    for (Uint32 r = StaticResIdxRange.first; r < StaticResIdxRange.second; ++r)
    {
        const auto& ResDesc = GetResourceDesc(r);
        const auto& ResAttr = GetResourceAttribs(r);
        VERIFY_EXPR(ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC);

        if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER)
            continue; // Skip separate samplers

        static_assert(BINDING_RANGE_COUNT == 4, "Please update the switch below to handle the new shader resource range");
        switch (PipelineResourceToBindingRange(ResDesc))
        {
            case BINDING_RANGE_UNIFORM_BUFFER:
                for (Uint32 ArrInd = 0; ArrInd < ResDesc.ArraySize; ++ArrInd)
                {
                    const auto& SrcCachedRes = SrcResourceCache.GetConstUB(ResAttr.CacheOffset + ArrInd);
                    if (!SrcCachedRes.pBuffer)
                    {
                        if (DstCacheType == ResourceCacheContentType::SRB)
                            LOG_ERROR_MESSAGE("No resource is assigned to static shader variable '", GetShaderResourcePrintName(ResDesc, ArrInd), "' in pipeline resource signature '", m_Desc.Name, "'.");
                        continue;
                    }

                    DstResourceCache.SetUniformBuffer(ResAttr.CacheOffset + ArrInd,
                                                      RefCntAutoPtr<BufferGLImpl>{SrcCachedRes.pBuffer},
                                                      SrcCachedRes.BaseOffset, SrcCachedRes.RangeSize);
                }
                break;
            case BINDING_RANGE_STORAGE_BUFFER:
                for (Uint32 ArrInd = 0; ArrInd < ResDesc.ArraySize; ++ArrInd)
                {
                    const auto& SrcCachedRes = SrcResourceCache.GetConstSSBO(ResAttr.CacheOffset + ArrInd);
                    if (!SrcCachedRes.pBufferView)
                    {
                        if (DstCacheType == ResourceCacheContentType::SRB)
                            LOG_ERROR_MESSAGE("No resource is assigned to static shader variable '", GetShaderResourcePrintName(ResDesc, ArrInd), "' in pipeline resource signature '", m_Desc.Name, "'.");
                        continue;
                    }

                    DstResourceCache.SetSSBO(ResAttr.CacheOffset + ArrInd, RefCntAutoPtr<BufferViewGLImpl>{SrcCachedRes.pBufferView});
                }
                break;
            case BINDING_RANGE_TEXTURE:
                for (Uint32 ArrInd = 0; ArrInd < ResDesc.ArraySize; ++ArrInd)
                {
                    const auto& SrcCachedRes = SrcResourceCache.GetConstTexture(ResAttr.CacheOffset + ArrInd);
                    if (!SrcCachedRes.pView)
                    {
                        if (DstCacheType == ResourceCacheContentType::SRB)
                            LOG_ERROR_MESSAGE("No resource is assigned to static shader variable '", GetShaderResourcePrintName(ResDesc, ArrInd), "' in pipeline resource signature '", m_Desc.Name, "'.");
                        continue;
                    }

                    if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV ||
                        ResDesc.ResourceType == SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT)
                    {
                        const auto HasImmutableSampler = GetImmutableSamplerIdx(ResAttr) != InvalidImmutableSamplerIndex;

                        RefCntAutoPtr<TextureViewGLImpl> pTexViewGl{SrcCachedRes.pView.RawPtr<TextureViewGLImpl>()};
                        DstResourceCache.SetTexture(ResAttr.CacheOffset + ArrInd, std::move(pTexViewGl), !HasImmutableSampler);
                        if (HasImmutableSampler && DstCacheType == ResourceCacheContentType::SRB)
                        {
                            VERIFY(DstResourceCache.GetConstTexture(ResAttr.CacheOffset + ArrInd).pSampler, "Immutable sampler is not initialized in the cache. This is a bug.");
                        }
                    }
                    else if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV)
                    {
                        RefCntAutoPtr<BufferViewGLImpl> pViewGl{SrcCachedRes.pView.RawPtr<BufferViewGLImpl>()};
                        DstResourceCache.SetTexelBuffer(ResAttr.CacheOffset + ArrInd, std::move(pViewGl));
                    }
                    else
                    {
                        UNEXPECTED("Unexpected resource type");
                    }
                }
                break;
            case BINDING_RANGE_IMAGE:
                for (Uint32 ArrInd = 0; ArrInd < ResDesc.ArraySize; ++ArrInd)
                {
                    const auto& SrcCachedRes = SrcResourceCache.GetConstImage(ResAttr.CacheOffset + ArrInd);
                    if (!SrcCachedRes.pView)
                    {
                        if (DstCacheType == ResourceCacheContentType::SRB)
                            LOG_ERROR_MESSAGE("No resource is assigned to static shader variable '", GetShaderResourcePrintName(ResDesc, ArrInd), "' in pipeline resource signature '", m_Desc.Name, "'.");
                        continue;
                    }

                    if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_UAV)
                    {
                        RefCntAutoPtr<TextureViewGLImpl> pTexViewGl{SrcCachedRes.pView.RawPtr<TextureViewGLImpl>()};
                        DstResourceCache.SetTexImage(ResAttr.CacheOffset + ArrInd, std::move(pTexViewGl));
                    }
                    else if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_UAV ||
                             ResDesc.ResourceType == SHADER_RESOURCE_TYPE_BUFFER_SRV)
                    {
                        RefCntAutoPtr<BufferViewGLImpl> pViewGl{SrcCachedRes.pView.RawPtr<BufferViewGLImpl>()};
                        DstResourceCache.SetBufImage(ResAttr.CacheOffset + ArrInd, std::move(pViewGl));
                    }
                    else
                    {
                        UNEXPECTED("Unexpected resource type");
                    }
                }
                break;
            default:
                UNEXPECTED("Unsupported shader resource range type.");
        }
    }

#ifdef DILIGENT_DEVELOPMENT
    if (DstCacheType == ResourceCacheContentType::SRB)
        DstResourceCache.SetStaticResourcesInitialized();
#endif
#ifdef DILIGENT_DEBUG
    DstResourceCache.DbgVerifyDynamicBufferMasks();
#endif
}

void PipelineResourceSignatureGLImpl::InitSRBResourceCache(ShaderResourceCacheGL& ResourceCache)
{
    ResourceCache.Initialize(m_BindingCount, m_SRBMemAllocator.GetResourceCacheDataAllocator(0), m_DynamicUBOMask, m_DynamicSSBOMask);

    // Initialize immutable samplers
    for (Uint32 r = 0; r < m_Desc.NumResources; ++r)
    {
        const auto& ResDesc = GetResourceDesc(r);
        const auto& ResAttr = GetResourceAttribs(r);

        if (ResDesc.ResourceType != SHADER_RESOURCE_TYPE_TEXTURE_SRV)
            continue;

        const auto ImtblSamplerIdx = GetImmutableSamplerIdx(ResAttr);
        if (ImtblSamplerIdx != InvalidImmutableSamplerIndex)
        {
            ISampler* pSampler = m_pImmutableSamplers[ImtblSamplerIdx];
            VERIFY(pSampler != nullptr, "Immutable sampler is not initialized - this is a bug");

            for (Uint32 ArrInd = 0; ArrInd < ResDesc.ArraySize; ++ArrInd)
                ResourceCache.SetSampler(ResAttr.CacheOffset + ArrInd, pSampler);
        }
    }
}

#ifdef DILIGENT_DEVELOPMENT
bool PipelineResourceSignatureGLImpl::DvpValidateCommittedResource(const ShaderResourcesGL::GLResourceAttribs& GLAttribs,
                                                                   RESOURCE_DIMENSION                          ResourceDim,
                                                                   bool                                        IsMultisample,
                                                                   Uint32                                      ResIndex,
                                                                   const ShaderResourceCacheGL&                ResourceCache,
                                                                   const char*                                 ShaderName,
                                                                   const char*                                 PSOName) const
{
    VERIFY_EXPR(ResIndex < m_Desc.NumResources);
    const auto& ResDesc = m_Desc.Resources[ResIndex];
    const auto& ResAttr = m_pResourceAttribs[ResIndex];
    VERIFY(strcmp(ResDesc.Name, GLAttribs.Name) == 0, "Inconsistent resource names");

    if (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_SAMPLER)
        return true; // Skip separate samplers

    VERIFY_EXPR(GLAttribs.ArraySize <= ResDesc.ArraySize);

    bool BindingsOK = true;

    static_assert(BINDING_RANGE_COUNT == 4, "Please update the switch below to handle the new shader resource range");
    switch (PipelineResourceToBindingRange(ResDesc))
    {
        case BINDING_RANGE_UNIFORM_BUFFER:
            for (Uint32 ArrInd = 0; ArrInd < GLAttribs.ArraySize; ++ArrInd)
            {
                if (!ResourceCache.IsUBBound(ResAttr.CacheOffset + ArrInd))
                {
                    LOG_ERROR_MESSAGE("No resource is bound to variable '", GetShaderResourcePrintName(GLAttribs, ArrInd),
                                      "' in shader '", ShaderName, "' of PSO '", PSOName, "'");
                    BindingsOK = false;
                }
            }
            break;

        case BINDING_RANGE_STORAGE_BUFFER:
            for (Uint32 ArrInd = 0; ArrInd < GLAttribs.ArraySize; ++ArrInd)
            {
                if (!ResourceCache.IsSSBOBound(ResAttr.CacheOffset + ArrInd))
                {
                    LOG_ERROR_MESSAGE("No resource is bound to variable '", GetShaderResourcePrintName(GLAttribs, ArrInd),
                                      "' in shader '", ShaderName, "' of PSO '", PSOName, "'");
                    BindingsOK = false;
                }
            }
            break;

        case BINDING_RANGE_TEXTURE:
            for (Uint32 ArrInd = 0; ArrInd < GLAttribs.ArraySize; ++ArrInd)
            {
                const bool IsTexView = (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV || ResDesc.ResourceType == SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT);
                if (!ResourceCache.IsTextureBound(ResAttr.CacheOffset + ArrInd, IsTexView))
                {
                    LOG_ERROR_MESSAGE("No resource is bound to variable '", GetShaderResourcePrintName(GLAttribs, ArrInd),
                                      "' in shader '", ShaderName, "' of PSO '", PSOName, "'");
                    BindingsOK = false;
                    continue;
                }

                const auto& Tex = ResourceCache.GetConstTexture(ResAttr.CacheOffset + ArrInd);
                if (Tex.pTexture)
                    ValidateResourceViewDimension(GLAttribs.Name, GLAttribs.ArraySize, ArrInd, Tex.pView.RawPtr<ITextureView>(), ResourceDim, IsMultisample);
                else
                    ValidateResourceViewDimension(GLAttribs.Name, GLAttribs.ArraySize, ArrInd, Tex.pView.RawPtr<IBufferView>(), ResourceDim, IsMultisample);

                const auto ImmutableSamplerIdx = GetImmutableSamplerIdx(ResAttr);
                if (ImmutableSamplerIdx != InvalidImmutableSamplerIndex)
                {
                    VERIFY(Tex.pSampler != nullptr, "Immutable sampler is not initialized in the cache - this is a bug");
                    VERIFY(Tex.pSampler == m_pImmutableSamplers[ImmutableSamplerIdx], "Immutable sampler initialized in the cache is not valid");
                }
            }
            break;

        case BINDING_RANGE_IMAGE:
            for (Uint32 ArrInd = 0; ArrInd < GLAttribs.ArraySize; ++ArrInd)
            {
                const bool IsTexView = (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_SRV || ResDesc.ResourceType == SHADER_RESOURCE_TYPE_TEXTURE_UAV);
                if (!ResourceCache.IsImageBound(ResAttr.CacheOffset + ArrInd, IsTexView))
                {
                    LOG_ERROR_MESSAGE("No resource is bound to variable '", GetShaderResourcePrintName(GLAttribs, ArrInd),
                                      "' in shader '", ShaderName, "' of PSO '", PSOName, "'");
                    BindingsOK = false;
                    continue;
                }

                const auto& Img = ResourceCache.GetConstImage(ResAttr.CacheOffset + ArrInd);
                if (Img.pTexture)
                    ValidateResourceViewDimension(GLAttribs.Name, GLAttribs.ArraySize, ArrInd, Img.pView.RawPtr<ITextureView>(), ResourceDim, IsMultisample);
                else
                    ValidateResourceViewDimension(GLAttribs.Name, GLAttribs.ArraySize, ArrInd, Img.pView.RawPtr<IBufferView>(), ResourceDim, IsMultisample);
            }
            break;

        default:
            UNEXPECTED("Unsupported shader resource range type.");
    }

    return BindingsOK;
}
#endif // DILIGENT_DEVELOPMENT


PipelineResourceSignatureGLImpl::PipelineResourceSignatureGLImpl(IReferenceCounters*                            pRefCounters,
                                                                 RenderDeviceGLImpl*                            pDeviceGL,
                                                                 const PipelineResourceSignatureDesc&           Desc,
                                                                 const PipelineResourceSignatureInternalDataGL& InternalData) :
    TPipelineResourceSignatureBase{pRefCounters, pDeviceGL, Desc, InternalData}
{
    try
    {
        Deserialize(
            GetRawAllocator(), Desc, InternalData, /*CreateImmutableSamplers = */ true,
            [this]() //
            {
                CreateLayout(/*IsSerialized*/ true);
            },
            [this]() //
            {
                return ShaderResourceCacheGL::GetRequiredMemorySize(m_BindingCount);
            });
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

} // namespace Diligent
