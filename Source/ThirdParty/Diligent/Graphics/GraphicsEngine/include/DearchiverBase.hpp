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
/// Implementation of the Diligent::DearchiverBase class

#include "Dearchiver.h"
#include "ObjectBase.hpp"
#include "EngineMemory.h"

namespace Diligent
{

/// Class implementing base functionality of the dearchiver
class DearchiverBase : public ObjectBase<IDearchiver>
{
public:
    using TObjectBase = ObjectBase<IDearchiver>;

    explicit DearchiverBase(IReferenceCounters* pRefCounters) noexcept :
        TObjectBase{pRefCounters}
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Dearchiver, TObjectBase)

protected:
    template <typename DeviceObjectArchiveImplType>
    void CreateDeviceObjectArchiveImpl(IArchive*              pSource,
                                       IDeviceObjectArchive** ppArchive) const
    {
        DEV_CHECK_ERR(ppArchive != nullptr, "ppArchive must not be null");
        if (!ppArchive)
            return;

        *ppArchive = nullptr;
        try
        {
            auto& RawMemAllocator = GetRawAllocator();
            auto* pArchiveImpl    = NEW_RC_OBJ(RawMemAllocator, "Device object archive instance", DeviceObjectArchiveImplType)(pSource);
            pArchiveImpl->QueryInterface(IID_DeviceObjectArchive, reinterpret_cast<IObject**>(ppArchive));
        }
        catch (...)
        {
            LOG_ERROR("Failed to create the device object archive");
        }
    }

    template <typename DeviceObjectArchiveImplType>
    void UnpackPipelineStateImpl(const PipelineStateUnpackInfo& DeArchiveInfo, IPipelineState** ppPSO) const
    {
        if (!VerifyPipelineStateUnpackInfo(DeArchiveInfo, ppPSO))
            return;

        *ppPSO = nullptr;

        auto* pArchiveImpl = ClassPtrCast<DeviceObjectArchiveImplType>(DeArchiveInfo.pArchive);
        switch (DeArchiveInfo.PipelineType)
        {
            case PIPELINE_TYPE_GRAPHICS:
            case PIPELINE_TYPE_MESH:
                pArchiveImpl->UnpackGraphicsPSO(DeArchiveInfo, ppPSO);
                break;

            case PIPELINE_TYPE_COMPUTE:
                pArchiveImpl->UnpackComputePSO(DeArchiveInfo, ppPSO);
                break;

            case PIPELINE_TYPE_RAY_TRACING:
                pArchiveImpl->UnpackRayTracingPSO(DeArchiveInfo, ppPSO);
                break;

            case PIPELINE_TYPE_TILE:
                pArchiveImpl->UnpackTilePSO(DeArchiveInfo, ppPSO);
                break;

            case PIPELINE_TYPE_INVALID:
            default:
                LOG_ERROR_MESSAGE("Unsupported pipeline type");
                return;
        }
    }

    template <typename DeviceObjectArchiveImplType>
    void UnpackResourceSignatureImpl(const ResourceSignatureUnpackInfo& DeArchiveInfo,
                                     IPipelineResourceSignature**       ppSignature) const
    {
        if (!VerifyResourceSignatureUnpackInfo(DeArchiveInfo, ppSignature))
            return;

        *ppSignature = nullptr;

        auto* pArchiveImpl = ClassPtrCast<DeviceObjectArchiveImplType>(DeArchiveInfo.pArchive);
        auto  pSignature   = pArchiveImpl->UnpackResourceSignature(DeArchiveInfo, false /*IsImplicit*/);
        *ppSignature       = pSignature.Detach();
    }

    template <typename DeviceObjectArchiveImplType>
    void UnpackRenderPassImpl(const RenderPassUnpackInfo& DeArchiveInfo, IRenderPass** ppRP) const
    {
        if (!VerifyRenderPassUnpackInfo(DeArchiveInfo, ppRP))
            return;

        *ppRP = nullptr;

        auto* pArchiveImpl = ClassPtrCast<DeviceObjectArchiveImplType>(DeArchiveInfo.pArchive);
        pArchiveImpl->UnpackRenderPass(DeArchiveInfo, ppRP);
    }


    bool VerifyPipelineStateUnpackInfo(const PipelineStateUnpackInfo& DeArchiveInfo, IPipelineState** ppPSO) const;
    bool VerifyResourceSignatureUnpackInfo(const ResourceSignatureUnpackInfo& DeArchiveInfo, IPipelineResourceSignature** ppSignature) const;
    bool VerifyRenderPassUnpackInfo(const RenderPassUnpackInfo& DeArchiveInfo, IRenderPass** ppRP) const;
};

} // namespace Diligent
