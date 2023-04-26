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

/// \file
/// C++ struct wrappers for render state cache.

#include "RenderStateCache.h"
#include "../../GraphicsEngine/interface/GraphicsTypesX.hpp"

namespace Diligent
{

/// C++ wrapper over IRenderDevice and IRenderStateCache.
template <bool ThrowOnError = true>
class RenderDeviceWithCache : public RenderDeviceX<ThrowOnError>
{
public:
    using TBase = RenderDeviceX<ThrowOnError>;

    explicit RenderDeviceWithCache(IRenderDevice*     pDevice,
                                   IRenderStateCache* pCache = nullptr) noexcept :
        TBase{pDevice},
        m_pCache{pCache}
    {
    }

    RefCntAutoPtr<IShader> CreateShader(const ShaderCreateInfo& ShaderCI) noexcept(!ThrowOnError)
    {
        return m_pCache ?
            UnpackCachedObject<IShader>("shader", ShaderCI.Desc.Name, &IRenderStateCache::CreateShader, ShaderCI) :
            TBase::CreateShader(ShaderCI);
    }

    RefCntAutoPtr<IPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return m_pCache ?
            UnpackCachedObject<IPipelineState>("graphics pipeline", CreateInfo.PSODesc.Name, &IRenderStateCache::CreateGraphicsPipelineState, CreateInfo) :
            TBase::CreateGraphicsPipelineState(CreateInfo);
    }

    RefCntAutoPtr<IPipelineState> CreateComputePipelineState(const ComputePipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return m_pCache ?
            UnpackCachedObject<IPipelineState>("compute pipeline", CreateInfo.PSODesc.Name, &IRenderStateCache::CreateComputePipelineState, CreateInfo) :
            TBase::CreateComputePipelineState(CreateInfo);
    }

    RefCntAutoPtr<IPipelineState> CreateRayTracingPipelineState(const RayTracingPipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return m_pCache ?
            UnpackCachedObject<IPipelineState>("ray-tracing pipeline", CreateInfo.PSODesc.Name, &IRenderStateCache::CreateRayTracingPipelineState, CreateInfo) :
            TBase::CreateRayTracingPipelineState(CreateInfo);
    }

    RefCntAutoPtr<IPipelineState> CreateTilePipelineState(const TilePipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return m_pCache ?
            UnpackCachedObject<IPipelineState>("tile pipeline", CreateInfo.PSODesc.Name, &IRenderStateCache::CreateTilePipelineState, CreateInfo) :
            TBase::CreateTilePipelineState(CreateInfo);
    }

    RefCntAutoPtr<IPipelineState> CreatePipelineState(const GraphicsPipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateGraphicsPipelineState(CreateInfo);
    }
    RefCntAutoPtr<IPipelineState> CreatePipelineState(const ComputePipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateComputePipelineState(CreateInfo);
    }
    RefCntAutoPtr<IPipelineState> CreatePipelineState(const RayTracingPipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateRayTracingPipelineState(CreateInfo);
    }
    RefCntAutoPtr<IPipelineState> CreatePipelineState(const TilePipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateTilePipelineState(CreateInfo);
    }

    IRenderStateCache* GetCache() const
    {
        return m_pCache.RawPtr<IRenderStateCache>();
    }

private:
    template <typename ObjectType, typename CreateMethodType, typename... CreateArgsType>
    RefCntAutoPtr<ObjectType> UnpackCachedObject(const char* ObjectTypeName, const char* ObjectName, CreateMethodType Create, CreateArgsType&&... Args) noexcept(!ThrowOnError)
    {
        RefCntAutoPtr<ObjectType> pObject;
        (m_pCache->*Create)(std::forward<CreateArgsType>(Args)..., &pObject);
        if (ThrowOnError)
        {
            if (!pObject)
                LOG_ERROR_AND_THROW("Failed to create ", ObjectTypeName, " '", (ObjectName != nullptr ? ObjectName : "<unnamed>"), "'.");
        }
        return pObject;
    }

private:
    RefCntAutoPtr<IRenderStateCache> m_pCache;
};

} // namespace Diligent
