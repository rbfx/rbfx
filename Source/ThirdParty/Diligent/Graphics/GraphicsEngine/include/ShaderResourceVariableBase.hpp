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
/// Implementation of the Diligent::ShaderBase template class

#include <vector>

#include "ShaderResourceVariable.h"
#include "PipelineState.h"
#include "StringTools.hpp"
#include "GraphicsAccessories.hpp"
#include "ShaderResourceCacheCommon.hpp"
#include "RefCntAutoPtr.hpp"
#include "EngineMemory.h"

namespace Diligent
{

template <typename TNameCompare>
SHADER_RESOURCE_VARIABLE_TYPE GetShaderVariableType(SHADER_TYPE                       ShaderStage,
                                                    SHADER_RESOURCE_VARIABLE_TYPE     DefaultVariableType,
                                                    const ShaderResourceVariableDesc* Variables,
                                                    Uint32                            NumVars,
                                                    TNameCompare                      NameCompare)
{
    for (Uint32 v = 0; v < NumVars; ++v)
    {
        const auto& CurrVarDesc = Variables[v];
        if (((CurrVarDesc.ShaderStages & ShaderStage) != 0) && NameCompare(CurrVarDesc.Name))
        {
            return CurrVarDesc.Type;
        }
    }
    return DefaultVariableType;
}

inline SHADER_RESOURCE_VARIABLE_TYPE GetShaderVariableType(SHADER_TYPE                       ShaderStage,
                                                           const Char*                       Name,
                                                           SHADER_RESOURCE_VARIABLE_TYPE     DefaultVariableType,
                                                           const ShaderResourceVariableDesc* Variables,
                                                           Uint32                            NumVars)
{
    return GetShaderVariableType(ShaderStage, DefaultVariableType, Variables, NumVars,
                                 [&](const char* VarName) //
                                 {
                                     return strcmp(VarName, Name) == 0;
                                 });
}

inline SHADER_RESOURCE_VARIABLE_TYPE GetShaderVariableType(SHADER_TYPE                       ShaderStage,
                                                           const Char*                       Name,
                                                           const PipelineResourceLayoutDesc& LayoutDesc)
{
    return GetShaderVariableType(ShaderStage, Name, LayoutDesc.DefaultVariableType, LayoutDesc.Variables, LayoutDesc.NumVariables);
}

inline SHADER_RESOURCE_VARIABLE_TYPE GetShaderVariableType(SHADER_TYPE                       ShaderStage,
                                                           const String&                     Name,
                                                           SHADER_RESOURCE_VARIABLE_TYPE     DefaultVariableType,
                                                           const ShaderResourceVariableDesc* Variables,
                                                           Uint32                            NumVars)
{
    return GetShaderVariableType(ShaderStage, DefaultVariableType, Variables, NumVars,
                                 [&](const char* VarName) //
                                 {
                                     return Name.compare(VarName) == 0;
                                 });
}

inline SHADER_RESOURCE_VARIABLE_TYPE GetShaderVariableType(SHADER_TYPE                       ShaderStage,
                                                           const String&                     Name,
                                                           const PipelineResourceLayoutDesc& LayoutDesc)
{
    return GetShaderVariableType(ShaderStage, Name, LayoutDesc.DefaultVariableType, LayoutDesc.Variables, LayoutDesc.NumVariables);
}

inline bool IsAllowedType(SHADER_RESOURCE_VARIABLE_TYPE VarType, Uint32 AllowedTypeBits) noexcept
{
    return ((1 << VarType) & AllowedTypeBits) != 0;
}

inline Uint32 GetAllowedTypeBit(SHADER_RESOURCE_VARIABLE_TYPE VarType)
{
    return 1 << static_cast<Uint32>(VarType);
}

inline Uint32 GetAllowedTypeBits(const SHADER_RESOURCE_VARIABLE_TYPE* AllowedVarTypes, Uint32 NumAllowedTypes) noexcept
{
    if (AllowedVarTypes == nullptr)
        return 0xFFFFFFFF;

    Uint32 AllowedTypeBits = 0;
    for (Uint32 i = 0; i < NumAllowedTypes; ++i)
        AllowedTypeBits |= GetAllowedTypeBit(AllowedVarTypes[i]);
    return AllowedTypeBits;
}

struct BindResourceInfo
{
    IDeviceObject* const pObject;

    const SET_SHADER_RESOURCE_FLAGS Flags;

    const Uint32 ArrayIndex;
    const Uint64 BufferBaseOffset;
    const Uint64 BufferRangeSize;

    BindResourceInfo(Uint32                    _ArrayIndex,
                     IDeviceObject*            _pObject,
                     SET_SHADER_RESOURCE_FLAGS _Flags,
                     Uint64                    _BufferBaseOffset = 0,
                     Uint64                    _BufferRangeSize  = 0) noexcept :
        // clang-format off
        pObject         {_pObject},
        Flags           {_Flags},
        ArrayIndex      {_ArrayIndex},
        BufferBaseOffset{_BufferBaseOffset},
        BufferRangeSize {_BufferRangeSize}
    // clang-format on
    {}

    explicit BindResourceInfo(IDeviceObject* _pObject, SET_SHADER_RESOURCE_FLAGS _Flags) noexcept :
        BindResourceInfo{0, _pObject, _Flags}
    {}
};

#ifdef DILIGENT_DEBUG
#    define RESOURCE_VALIDATION_FAILURE UNEXPECTED
#else
#    define RESOURCE_VALIDATION_FAILURE LOG_ERROR_MESSAGE
#endif

template <typename ResourceImplType>
bool VerifyResourceBinding(const char*                 ExpectedResourceTypeName,
                           const PipelineResourceDesc& ResDesc,
                           const BindResourceInfo&     BindInfo,
                           const ResourceImplType*     pResourceImpl, // Expected resource implementation
                           const IDeviceObject*        pCachedObject, // Object already set in the cache
                           const char*                 SignatureName)
{
    if (BindInfo.pObject != nullptr && pResourceImpl == nullptr)
    {
        std::stringstream ss;
        ss << "Failed to bind object '" << BindInfo.pObject->GetDesc().Name << "' to variable '" << GetShaderResourcePrintName(ResDesc, BindInfo.ArrayIndex) << '\'';
        if (SignatureName != nullptr)
        {
            ss << " defined by signature '" << SignatureName << '\'';
        }
        ss << ". Invalid resource type: " << ExpectedResourceTypeName << " is expected.";
        RESOURCE_VALIDATION_FAILURE(ss.str());

        return false;
    }

    if (ResDesc.VarType != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC &&
        (BindInfo.Flags & SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE) == 0 &&
        pCachedObject != nullptr && pCachedObject != pResourceImpl)
    {
        auto VarTypeStr = GetShaderVariableTypeLiteralName(ResDesc.VarType);

        std::stringstream ss;
        ss << "Non-null " << ExpectedResourceTypeName << " '" << pCachedObject->GetDesc().Name << "' is already bound to " << VarTypeStr
           << " shader variable '" << GetShaderResourcePrintName(ResDesc, BindInfo.ArrayIndex) << '\'';
        if (SignatureName != nullptr)
        {
            ss << " defined by signature '" << SignatureName << '\'';
        }
        ss << ". Overwriting the binding with ";
        if (pResourceImpl != nullptr)
        {
            ss << "another resource ('" << pResourceImpl->GetDesc().Name << "')";
        }
        else
        {
            ss << "null";
        }
        ss << " is disallowed by default.";

        if (ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_STATIC)
        {
            ss << " If this is intended, use the SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE flag. Otherwise,"
               << " label the variable as mutable and use another shader resource binding instance, or label the variable as dynamic.";
        }
        else if (ResDesc.VarType == SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        {
            ss << " If this is intended and you ensured proper synchronization, use the SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE flag. Otherwise,"
               << " use another shader resource binding instance or label the variable as dynamic.";
        }

        RESOURCE_VALIDATION_FAILURE(ss.str());

        return false;
    }

    return true;
}

template <typename BufferImplType>
bool VerifyConstantBufferBinding(const PipelineResourceDesc& ResDesc,
                                 const BindResourceInfo&     BindInfo,
                                 const BufferImplType*       pBufferImpl,
                                 const IDeviceObject*        pCachedBuffer,
                                 Uint64                      CachedBaseOffset,
                                 Uint64                      CachedRangeSize,
                                 const char*                 SignatureName)
{
    bool BindingOK = VerifyResourceBinding("buffer", ResDesc, BindInfo, pBufferImpl, pCachedBuffer, SignatureName);

    if (pBufferImpl != nullptr)
    {
        const auto& BuffDesc = pBufferImpl->GetDesc();
        if ((BuffDesc.BindFlags & BIND_UNIFORM_BUFFER) == 0)
        {
            std::stringstream ss;
            ss << "Error binding buffer '" << BuffDesc.Name << "' to variable '" << GetShaderResourcePrintName(ResDesc, BindInfo.ArrayIndex) << '\'';
            if (SignatureName != nullptr)
            {
                ss << " defined by signature '" << SignatureName << '\'';
            }
            ss << ". The buffer was not created with BIND_UNIFORM_BUFFER flag.";
            RESOURCE_VALIDATION_FAILURE(ss.str());

            BindingOK = false;
        }

        if (BuffDesc.Usage == USAGE_DYNAMIC && (ResDesc.Flags & PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS) != 0)
        {
            std::stringstream ss;
            ss << "Error binding USAGE_DYNAMIC buffer '" << BuffDesc.Name << "' to variable '" << GetShaderResourcePrintName(ResDesc, BindInfo.ArrayIndex) << '\'';
            if (SignatureName != nullptr)
            {
                ss << " defined by signature '" << SignatureName << '\'';
            }
            ss << ". The variable was initialized with PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS flag.";
            RESOURCE_VALIDATION_FAILURE(ss.str());

            BindingOK = false;
        }

        bool RangeIsOutOfBounds = false;
        if (BindInfo.BufferBaseOffset + BindInfo.BufferRangeSize > BuffDesc.Size)
        {
            RESOURCE_VALIDATION_FAILURE("Buffer range [", BindInfo.BufferBaseOffset, ", ", BindInfo.BufferBaseOffset + BindInfo.BufferRangeSize,
                                        ") specified for buffer '", BuffDesc.Name, "' of size ", BuffDesc.Size, " is out of the buffer bounds.");
            BindingOK          = false;
            RangeIsOutOfBounds = true;
        }

        const auto OffsetAlignment = pBufferImpl->GetDevice()->GetAdapterInfo().Buffer.ConstantBufferOffsetAlignment;
        VERIFY_EXPR(OffsetAlignment != 0);
        if ((BindInfo.BufferBaseOffset % OffsetAlignment) != 0)
        {
            RESOURCE_VALIDATION_FAILURE("Buffer base offset (", BindInfo.BufferBaseOffset, ") is not a multiple of required constant buffer offset alignment (",
                                        OffsetAlignment, ").");
            BindingOK = false;
        }

        if (!RangeIsOutOfBounds && ResDesc.VarType != SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC && pCachedBuffer != nullptr)
        {
            if (CachedRangeSize == 0)
                CachedRangeSize = BuffDesc.Size - CachedBaseOffset;
            auto NewBufferRangeSize = BindInfo.BufferRangeSize;
            if (NewBufferRangeSize == 0)
                NewBufferRangeSize = BuffDesc.Size - BindInfo.BufferBaseOffset;

            if (CachedBaseOffset != BindInfo.BufferBaseOffset ||
                CachedRangeSize != NewBufferRangeSize)
            {
                std::stringstream ss;
                ss << "Error binding buffer '" << BuffDesc.Name << "' to variable '" << GetShaderResourcePrintName(ResDesc, BindInfo.ArrayIndex) << '\'';
                if (SignatureName != nullptr)
                {
                    ss << " defined by signature '" << SignatureName << '\'';
                }
                ss << ". The new range [" << BindInfo.BufferBaseOffset << ", " << BindInfo.BufferBaseOffset + NewBufferRangeSize
                   << ") does not match current range [" << CachedBaseOffset << ", " << CachedBaseOffset + CachedRangeSize
                   << "). This is treated as binding a new resource even if the buffer itself stays the same. "
                      "Use another SRB or label the variable as dynamic, or use SetBufferOffset() method if you only need to change the offset.";
                RESOURCE_VALIDATION_FAILURE(ss.str());
            }
        }
    }
    else
    {
        if (BindInfo.BufferBaseOffset != 0 || BindInfo.BufferRangeSize != 0)
            RESOURCE_VALIDATION_FAILURE("Non-empty buffer range is specified for a null buffer.");
    }

    return BindingOK;
}

template <typename TViewTypeEnum>
const char* GetResourceTypeName();

template <>
inline const char* GetResourceTypeName<TEXTURE_VIEW_TYPE>()
{
    return "texture view";
}

template <>
inline const char* GetResourceTypeName<BUFFER_VIEW_TYPE>()
{
    return "buffer view";
}

inline RESOURCE_DIMENSION GetResourceViewDimension(const ITextureView* pTexView)
{
    VERIFY_EXPR(pTexView != nullptr);
    return pTexView->GetDesc().TextureDim;
}

inline RESOURCE_DIMENSION GetResourceViewDimension(const IBufferView* /*pBuffView*/)
{
    return RESOURCE_DIM_BUFFER;
}

inline Uint32 GetResourceSampleCount(const ITextureView* pTexView)
{
    VERIFY_EXPR(pTexView != nullptr);
    return const_cast<ITextureView*>(pTexView)->GetTexture()->GetDesc().SampleCount;
}

inline Uint32 GetResourceSampleCount(const IBufferView* /*pBuffView*/)
{
    return 0;
}



template <typename TextureViewImplType>
bool ValidateResourceViewDimension(const char*                ResName,
                                   Uint32                     ArraySize,
                                   Uint32                     ArrayInd,
                                   const TextureViewImplType* pViewImpl,
                                   RESOURCE_DIMENSION         ExpectedResourceDim,
                                   bool                       IsMultisample)
{
    bool BindingsOK = true;

    if (ExpectedResourceDim != RESOURCE_DIM_UNDEFINED)
    {
        const auto ResourceDim = GetResourceViewDimension(pViewImpl);
        if (ResourceDim != ExpectedResourceDim)
        {
            RESOURCE_VALIDATION_FAILURE("The dimension of resource view '", pViewImpl->GetDesc().Name,
                                        "' bound to variable '", GetShaderResourcePrintName(ResName, ArraySize, ArrayInd), "' is ", GetResourceDimString(ResourceDim),
                                        ", but resource dimension expected by the shader is ", GetResourceDimString(ExpectedResourceDim), ".");
            BindingsOK = false;
        }

        if (ResourceDim == RESOURCE_DIM_TEX_2D || ResourceDim == RESOURCE_DIM_TEX_2D_ARRAY)
        {
            auto SampleCount = GetResourceSampleCount(pViewImpl);
            if (IsMultisample && SampleCount == 1)
            {
                RESOURCE_VALIDATION_FAILURE("Texture view '", pViewImpl->GetDesc().Name, "' bound to variable '",
                                            GetShaderResourcePrintName(ResName, ArraySize, ArrayInd), "' is invalid: multisample texture is expected.");
                BindingsOK = false;
            }
            else if (!IsMultisample && SampleCount > 1)
            {
                RESOURCE_VALIDATION_FAILURE("Texture view '", pViewImpl->GetDesc().Name, "' bound to variable '",
                                            GetShaderResourcePrintName(ResName, ArraySize, ArrayInd), "' is invalid: single-sample texture is expected.");
                BindingsOK = false;
            }
        }
    }

    return BindingsOK;
}


template <typename ResourceViewImplType,
          typename ViewTypeEnumType>
bool VerifyResourceViewBinding(const PipelineResourceDesc&             ResDesc,
                               const BindResourceInfo&                 BindInfo,
                               const ResourceViewImplType*             pViewImpl,
                               std::initializer_list<ViewTypeEnumType> ExpectedViewTypes,
                               RESOURCE_DIMENSION                      ExpectedResourceDimension,
                               bool                                    IsMultisample,
                               const IDeviceObject*                    pCachedView,
                               const char*                             SignatureName)
{
    const char* ExpectedResourceType = GetResourceTypeName<ViewTypeEnumType>();

    bool BindingOK = VerifyResourceBinding(ExpectedResourceType, ResDesc, BindInfo, pViewImpl, pCachedView, SignatureName);

    if (pViewImpl)
    {
        auto ViewType           = pViewImpl->GetDesc().ViewType;
        bool IsExpectedViewType = false;
        for (auto ExpectedViewType : ExpectedViewTypes)
        {
            if (ExpectedViewType == ViewType)
                IsExpectedViewType = true;
        }

        if (!IsExpectedViewType)
        {
            std::stringstream ss;
            ss << "Error binding " << ExpectedResourceType << " '" << pViewImpl->GetDesc().Name << "' to variable '"
               << GetShaderResourcePrintName(ResDesc, BindInfo.ArrayIndex) << '\'';
            if (SignatureName != nullptr)
            {
                ss << " defined by signature '" << SignatureName << '\'';
            }
            ss << ". Incorrect view type: ";
            bool IsFirstViewType = true;
            for (auto ExpectedViewType : ExpectedViewTypes)
            {
                if (!IsFirstViewType)
                {
                    ss << " or ";
                }
                ss << GetViewTypeLiteralName(ExpectedViewType);
                IsFirstViewType = false;
            }
            ss << " is expected, " << GetViewTypeLiteralName(ViewType) << " is provided.";
            RESOURCE_VALIDATION_FAILURE(ss.str());

            BindingOK = false;
        }

        if (!ValidateResourceViewDimension(ResDesc.Name, ResDesc.ArraySize, BindInfo.ArrayIndex, pViewImpl, ExpectedResourceDimension, IsMultisample))
        {
            BindingOK = false;
        }
    }

    if (BindInfo.BufferBaseOffset != 0 || BindInfo.BufferRangeSize != 0)
    {
        RESOURCE_VALIDATION_FAILURE("Buffer range may only be directly specified for constant buffers. "
                                    "To specify a range for a structured buffer, create a buffer view.");
    }

    return BindingOK;
}

template <typename BufferViewImplType>
bool ValidateBufferMode(const PipelineResourceDesc& ResDesc,
                        Uint32                      ArrayIndex,
                        const BufferViewImplType*   pBufferView)
{
    bool BindingOK = true;
    if (pBufferView != nullptr)
    {
        const auto& BuffDesc = pBufferView->GetBuffer()->GetDesc();
        if (ResDesc.Flags & PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER)
        {
            if (BuffDesc.Mode != BUFFER_MODE_FORMATTED)
            {
                RESOURCE_VALIDATION_FAILURE("Error binding buffer view '", pBufferView->GetDesc().Name, "' of buffer '", BuffDesc.Name, "' to shader variable '",
                                            GetShaderResourcePrintName(ResDesc, ArrayIndex), "': formatted buffer view is expected.");

                BindingOK = false;
            }
        }
        else
        {
            if (BuffDesc.Mode != BUFFER_MODE_STRUCTURED && BuffDesc.Mode != BUFFER_MODE_RAW)
            {
                RESOURCE_VALIDATION_FAILURE("Error binding buffer view '", pBufferView->GetDesc().Name, "' of buffer '", BuffDesc.Name, "' to shader variable '",
                                            GetShaderResourcePrintName(ResDesc, ArrayIndex), "': structured or raw buffer view is expected.");

                BindingOK = false;
            }
        }
    }

    return BindingOK;
}


template <typename SamplerImplType>
bool VerifySamplerBinding(const PipelineResourceDesc& ResDesc,
                          const BindResourceInfo&     BindInfo,
                          const SamplerImplType*      pSamplerImpl,
                          const IDeviceObject*        pCachedSampler,
                          const char*                 SignatureName)
{
    if (BindInfo.BufferBaseOffset != 0 || BindInfo.BufferRangeSize != 0)
    {
        RESOURCE_VALIDATION_FAILURE("Buffer range can't be specified for samplers.");
    }
    if (pSamplerImpl != nullptr && (pSamplerImpl->GetDesc().Flags & SAMPLER_FLAG_SUBSAMPLED) != 0)
    {
        RESOURCE_VALIDATION_FAILURE("Subsampled sampler must be added as an immutable sampler to the PSO or resource signature");
    }
    return VerifyResourceBinding("sampler", ResDesc, BindInfo, pSamplerImpl, pCachedSampler, SignatureName);
}

template <typename TLASImplType>
bool VerifyTLASResourceBinding(const PipelineResourceDesc& ResDesc,
                               const BindResourceInfo&     BindInfo,
                               const TLASImplType*         pTLASImpl,
                               const IDeviceObject*        pCachedAS,
                               const char*                 SignatureName)
{
    if (BindInfo.BufferBaseOffset != 0 || BindInfo.BufferRangeSize != 0)
    {
        RESOURCE_VALIDATION_FAILURE("Buffer range can't be specified for TLAS.");
    }
    return VerifyResourceBinding("TLAS", ResDesc, BindInfo, pTLASImpl, pCachedAS, SignatureName);
}

template <typename BufferImplType, typename BufferViewImplType>
bool VerifyDynamicBufferOffset(const PipelineResourceDesc& ResDesc,
                               const IDeviceObject*        pObject,
                               Uint64                      BufferBaseOffset,
                               Uint64                      BufferRangeSize,
                               Uint64                      BufferDynamicOffset)
{
    bool BindingOK = true;

    if ((ResDesc.Flags & PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS) != 0)
    {
        RESOURCE_VALIDATION_FAILURE("Error setting dynamic buffer offset for variable '", ResDesc.Name, "': the variable was created with PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS flag.");
        BindingOK = false;
    }

    const BufferImplType* pBuffer = nullptr;
    switch (ResDesc.ResourceType)
    {
        case SHADER_RESOURCE_TYPE_CONSTANT_BUFFER:
            pBuffer = ClassPtrCast<const BufferImplType>(pObject);
            break;

        case SHADER_RESOURCE_TYPE_BUFFER_SRV:
        case SHADER_RESOURCE_TYPE_BUFFER_UAV:
            if (const auto* pBuffView = ClassPtrCast<const BufferViewImplType>(pObject))
            {
                pBuffer = pObject != nullptr ? pBuffView->template GetBuffer<const BufferImplType>() : nullptr;

                const auto& ViewDesc = pBuffView->GetDesc();
                VERIFY_EXPR(BufferBaseOffset == 0 || BufferBaseOffset == ViewDesc.ByteOffset);
                VERIFY_EXPR(BufferRangeSize == 0 || BufferRangeSize == ViewDesc.ByteWidth);
                BufferBaseOffset = ViewDesc.ByteOffset;
                BufferRangeSize  = ViewDesc.ByteWidth;
            }
            break;

        default:
            RESOURCE_VALIDATION_FAILURE("Error setting dynamic buffer offset for variable '", ResDesc.Name, "': the offset may only be set for constant and structured buffers.");
    }

    if (pBuffer != nullptr)
    {
        const auto& BuffDesc = pBuffer->GetDesc();
        if (BufferBaseOffset + BufferRangeSize + BufferDynamicOffset > BuffDesc.Size)
        {
            RESOURCE_VALIDATION_FAILURE("Dynamic offset ", BufferDynamicOffset, " specified for variable '", ResDesc.Name,
                                        "' defines buffer range [", BufferBaseOffset + BufferDynamicOffset, ", ",
                                        BufferBaseOffset + BufferRangeSize + BufferDynamicOffset,
                                        ") that is past the bounds of buffer '", BuffDesc.Name, "' of size ", BuffDesc.Size, ".");
            BindingOK = false;
        }

        const auto& BufferProps = pBuffer->GetDevice()->GetAdapterInfo().Buffer;

        const auto OffsetAlignment = (ResDesc.ResourceType == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER) ?
            BufferProps.ConstantBufferOffsetAlignment :
            BufferProps.StructuredBufferOffsetAlignment;
        VERIFY_EXPR(OffsetAlignment != 0);

        if ((BufferDynamicOffset % OffsetAlignment) != 0)
        {
            RESOURCE_VALIDATION_FAILURE("Dynamic offset (", BufferDynamicOffset, ") specified for variable '", ResDesc.Name,
                                        "' is not a multiple of required offset alignment (", OffsetAlignment, ").");
            BindingOK = false;
        }
    }

    return BindingOK;
}


#undef RESOURCE_VALIDATION_FAILURE

template <typename ShaderVectorType>
std::string GetShaderGroupName(const ShaderVectorType& Shaders)
{
    std::string Name;
    if (Shaders.size() == 1)
    {
        Name = Shaders[0]->GetDesc().Name;
    }
    else
    {
        Name = "{";
        for (size_t s = 0; s < Shaders.size(); ++s)
        {
            if (s > 0)
                Name += ", ";
            Name += Shaders[s]->GetDesc().Name;
        }
        Name += "}";
    }
    return Name;
}

/// Base implementation of a shader variable
template <typename ThisImplType,
          typename VarManagerType,
          typename ResourceVariableBaseInterface = IShaderResourceVariable>
struct ShaderVariableBase : public ResourceVariableBaseInterface
{
    ShaderVariableBase(VarManagerType& ParentManager, Uint32 ResIndex) :
        m_ParentManager{ParentManager},
        m_ResIndex{ResIndex}
    {
    }

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override
    {
        if (ppInterface == nullptr)
            return;

        *ppInterface = nullptr;
        if (IID == IID_ShaderResourceVariable || IID == IID_Unknown)
        {
            *ppInterface = this;
            (*ppInterface)->AddRef();
        }
    }

    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE AddRef() override final
    {
        return m_ParentManager.GetOwner().AddRef();
    }

    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() override final
    {
        return m_ParentManager.GetOwner().Release();
    }

    virtual IReferenceCounters* DILIGENT_CALL_TYPE GetReferenceCounters() const override final
    {
        return m_ParentManager.GetOwner().GetReferenceCounters();
    }

    virtual void DILIGENT_CALL_TYPE Set(IDeviceObject* pObject, SET_SHADER_RESOURCE_FLAGS Flags) override final
    {
        static_cast<ThisImplType*>(this)->BindResource(BindResourceInfo{pObject, Flags});
    }

    virtual void DILIGENT_CALL_TYPE SetArray(IDeviceObject* const*     ppObjects,
                                             Uint32                    FirstElement,
                                             Uint32                    NumElements,
                                             SET_SHADER_RESOURCE_FLAGS Flags) override final
    {
        const auto& Desc = GetDesc();

        DEV_CHECK_ERR(FirstElement + NumElements <= Desc.ArraySize,
                      "SetArray arguments are invalid for '", Desc.Name, "' variable: specified element range (", FirstElement, " .. ",
                      FirstElement + NumElements - 1, ") is out of array bounds 0 .. ", Desc.ArraySize - 1);

        for (Uint32 elem = 0; elem < NumElements; ++elem)
            static_cast<ThisImplType*>(this)->BindResource(BindResourceInfo{FirstElement + elem, ppObjects[elem], Flags});
    }

    virtual void DILIGENT_CALL_TYPE SetBufferRange(IDeviceObject*            pObject,
                                                   Uint64                    Offset,
                                                   Uint64                    Size,
                                                   Uint32                    ArrayIndex,
                                                   SET_SHADER_RESOURCE_FLAGS Flags) override
    {
        DEV_CHECK_ERR(GetDesc().ResourceType == SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, "SetBufferRange() is only allowed for constant buffers.");
        static_cast<ThisImplType*>(this)->BindResource(BindResourceInfo{ArrayIndex, pObject, Flags, Offset, Size});
    }

    virtual void DILIGENT_CALL_TYPE SetBufferOffset(Uint32 Offset,
                                                    Uint32 ArrayIndex) override final
    {
#ifdef DILIGENT_DEVELOPMENT
        {
            const auto& Desc = GetDesc();
            DEV_CHECK_ERR((Desc.Flags & PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS) == 0,
                          "SetBufferOffset() is not only allowed for variables created with PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS flag.");
            DEV_CHECK_ERR(Desc.VarType != SHADER_RESOURCE_VARIABLE_TYPE_STATIC,
                          "SetBufferOffset() is not allowed for static variables.");
        }
#endif

        static_cast<ThisImplType*>(this)->SetDynamicOffset(ArrayIndex, Offset);
    }


    virtual SHADER_RESOURCE_VARIABLE_TYPE DILIGENT_CALL_TYPE GetType() const override final
    {
        return GetDesc().VarType;
    }

    virtual void DILIGENT_CALL_TYPE GetResourceDesc(ShaderResourceDesc& ResourceDesc) const override final
    {
        const auto& Desc       = GetDesc();
        ResourceDesc.Name      = Desc.Name;
        ResourceDesc.Type      = Desc.ResourceType;
        ResourceDesc.ArraySize = Desc.ArraySize;
    }

    virtual Uint32 DILIGENT_CALL_TYPE GetIndex() const override final
    {
        return m_ParentManager.GetVariableIndex(*static_cast<const ThisImplType*>(this));
    }

    void BindResources(IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags)
    {
        auto* const pThis   = static_cast<ThisImplType*>(this);
        const auto& ResDesc = pThis->GetDesc();

        if ((Flags & (1u << ResDesc.VarType)) == 0)
            return;

        for (Uint32 ArrInd = 0; ArrInd < ResDesc.ArraySize; ++ArrInd)
        {
            if ((Flags & BIND_SHADER_RESOURCES_KEEP_EXISTING) != 0 && pThis->Get(ArrInd) != nullptr)
                continue;

            if (auto* pObj = pResourceMapping->GetResource(ResDesc.Name, ArrInd))
            {
                const auto SetResFlags = (Flags & BIND_SHADER_RESOURCES_ALLOW_OVERWRITE) != 0 ?
                    SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE :
                    SET_SHADER_RESOURCE_FLAG_NONE;
                pThis->BindResource(BindResourceInfo{ArrInd, pObj, SetResFlags});
            }
            else
            {
                if ((Flags & BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED) && pThis->Get(ArrInd) == nullptr)
                {
                    LOG_ERROR_MESSAGE("Unable to bind resource to shader variable '",
                                      GetShaderResourcePrintName(ResDesc, ArrInd),
                                      "': resource is not found in the resource mapping. "
                                      "Do not use BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED flag to suppress the message if this is not an issue.");
                }
            }
        }
    }

    void CheckResources(IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags, SHADER_RESOURCE_VARIABLE_TYPE_FLAGS& StaleVarTypes) const
    {
        auto* const pThis   = static_cast<const ThisImplType*>(this);
        const auto& ResDesc = pThis->GetDesc();

        const auto VarTypeFlag = static_cast<SHADER_RESOURCE_VARIABLE_TYPE_FLAGS>(1u << ResDesc.VarType);
        if ((Flags & VarTypeFlag) == 0)
            return; // This variable type is not being processed

        if ((StaleVarTypes & VarTypeFlag) != 0)
            return; // This variable type is already stale

        for (Uint32 ArrInd = 0; ArrInd < ResDesc.ArraySize; ++ArrInd)
        {
            const auto* const pBoundObj = pThis->Get(ArrInd);
            if ((pBoundObj != nullptr) && (Flags & BIND_SHADER_RESOURCES_KEEP_EXISTING) != 0)
                continue;

            if ((pBoundObj == nullptr) && (Flags & BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED) != 0)
            {
                StaleVarTypes |= VarTypeFlag;
                return;
            }

            if (pResourceMapping != nullptr)
            {
                if (auto* pObj = pResourceMapping->GetResource(ResDesc.Name, ArrInd))
                {
                    if (pObj != pBoundObj)
                    {
                        StaleVarTypes |= VarTypeFlag;
                        return;
                    }
                }
            }
        }
    }

    const PipelineResourceDesc& GetDesc() const { return m_ParentManager.GetResourceDesc(m_ResIndex); }

protected:
    // Variable manager that owns this variable
    VarManagerType& m_ParentManager;

    // Resource index in pipeline resource signature m_Desc.Resources[]
    const Uint32 m_ResIndex;
};

template <class EngineImplTraits, typename VariableType>
class ShaderVariableManagerBase
{
protected:
    using ShaderResourceCacheType       = typename EngineImplTraits::ShaderResourceCacheImplType;
    using PipelineResourceSignatureType = typename EngineImplTraits::PipelineResourceSignatureImplType;
    using ThisImplType                  = typename EngineImplTraits::ShaderVariableManagerImplType;

    ShaderVariableManagerBase(IObject&                 Owner,
                              ShaderResourceCacheType& ResourceCache) noexcept :
        m_Owner{Owner},
        m_ResourceCache{ResourceCache}
    {}

    ~ShaderVariableManagerBase()
    {
        VERIFY(m_pVariables == nullptr, "Destroy() has not been called. The shader variable memory will leak.");
    }

    void Initialize(const PipelineResourceSignatureType& Signature, IMemoryAllocator& Allocator, size_t Size)
    {
        VERIFY_EXPR(m_pSignature == nullptr);
        m_pSignature = &Signature;

        if (Size > 0)
        {
            auto* pRawMem = ALLOCATE_RAW(Allocator, "Memory buffer for shader variables", Size);
            m_pVariables  = reinterpret_cast<VariableType*>(pRawMem);
        }

#ifdef DILIGENT_DEBUG
        m_pDbgAllocator = &Allocator;
#endif
    }

    void Destroy(IMemoryAllocator& Allocator)
    {
        if (m_pVariables != nullptr)
        {
            VERIFY(m_pDbgAllocator == &Allocator, "The allocator is not the same as the one that was used to allocate memory");
            Allocator.Free(m_pVariables);
            m_pVariables = nullptr;
        }
#ifdef DILIGENT_DEBUG
        m_pDbgAllocator = nullptr;
#endif
    }

    void BindResources(IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags)
    {
        DEV_CHECK_ERR(pResourceMapping != nullptr, "Failed to bind resources: resource mapping is null");

        if ((Flags & BIND_SHADER_RESOURCES_UPDATE_ALL) == 0)
            Flags |= BIND_SHADER_RESOURCES_UPDATE_ALL;

        for (Uint32 v = 0; v < static_cast<ThisImplType*>(this)->m_NumVariables; ++v)
        {
            m_pVariables[v].BindResources(pResourceMapping, Flags);
        }
    }

    void CheckResources(IResourceMapping*                    pResourceMapping,
                        BIND_SHADER_RESOURCES_FLAGS          Flags,
                        SHADER_RESOURCE_VARIABLE_TYPE_FLAGS& StaleVarTypes) const
    {
        if ((Flags & BIND_SHADER_RESOURCES_UPDATE_ALL) == 0)
            Flags |= BIND_SHADER_RESOURCES_UPDATE_ALL;

        const auto* pThis        = static_cast<const ThisImplType*>(this);
        const auto  AllowedTypes = m_ResourceCache.GetContentType() == ResourceCacheContentType::SRB ?
            SHADER_RESOURCE_VARIABLE_TYPE_FLAG_MUT_DYN :
            SHADER_RESOURCE_VARIABLE_TYPE_FLAG_STATIC;
        for (Uint32 v = 0; (v < pThis->m_NumVariables) && (StaleVarTypes & AllowedTypes) != AllowedTypes; ++v)
        {
            m_pVariables[v].CheckResources(pResourceMapping, Flags, StaleVarTypes);
        }
    }


protected:
    IObject& m_Owner;

    // Variable manager is owned by either Pipeline Resource Signature (in which case m_ResourceCache references
    // static resource cache owned by the same signature object), or by SRB object (in which case
    // m_ResourceCache references the cache in the SRB). Thus the cache and the signature
    // (which the variables reference) are guaranteed to be alive while the manager is alive.
    ShaderResourceCacheType& m_ResourceCache;

    PipelineResourceSignatureType const* m_pSignature = nullptr;

    // Memory is allocated through the allocator provided by the pipeline resource signature. If allocation
    // granularity > 1, fixed block memory allocator is used. This ensures that all resources from different
    // shader resource bindings reside in continuous memory. If allocation granularity == 1, raw allocator is used.
    VariableType* m_pVariables = nullptr;

private:
#ifdef DILIGENT_DEBUG
    // Memory allocator that was used to allocate memory for m_pVariables (for debug purposes only).
    IMemoryAllocator* m_pDbgAllocator = nullptr;
#endif
};

} // namespace Diligent
