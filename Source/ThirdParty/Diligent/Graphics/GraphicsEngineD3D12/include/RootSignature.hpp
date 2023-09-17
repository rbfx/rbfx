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
/// Declaration of Diligent::RootSignatureD3D12 class

// Root signature object combines multiple pipeline resource signatures into a single
// d3d12 root signature. The signatures "stack" on top of each other. Their "local"
// root indices and register spaces are biased by the root indices and spaces of
// previous signatures.
//
//   __________________________________________________________
//  |                                                          |
//  |  Pipeline Resource Signature 2 (NRootIndices2, NSpaces2) |  BaseRootIndex2 = BaseRootIndex1 + NRootIndices1
//  |__________________________________________________________|  BaseSpace2     = BaseSpace1 + NSpaces1
//  |                                                          |
//  |  Pipeline Resource Signature 1 (NRootIndices1, NSpaces1) |  BaseRootIndex1 = BaseRootIndex0 + NRootIndices0
//  |__________________________________________________________|  BaseSpace1     = BaseSpace0 + NSpaces0
//  |                                                          |
//  |  Pipeline Resource Signature 0 (NRootIndices0, NSpaces0) |  BaseRootIndex0 = 0
//  |__________________________________________________________|  BaseSpace0     = 0
//

#include <mutex>
#include <unordered_map>
#include <memory>

#include "PrivateConstants.h"
#include "ShaderResources.hpp"
#include "ObjectBase.hpp"
#include "ResourceBindingMap.hpp"

namespace Diligent
{

class RenderDeviceD3D12Impl;
class RootSignatureCacheD3D12;
class PipelineResourceSignatureD3D12Impl;

/// Implementation of the Diligent::RootSignature class
class RootSignatureD3D12 final : public ObjectBase<IObject>
{
public:
    RootSignatureD3D12(IReferenceCounters*                                     pRefCounters,
                       RenderDeviceD3D12Impl*                                  pDeviceD3D12Impl,
                       const RefCntAutoPtr<PipelineResourceSignatureD3D12Impl> ppSignatures[],
                       Uint32                                                  SignatureCount,
                       size_t                                                  Hash);
    ~RootSignatureD3D12();

    size_t GetHash() const { return m_Hash; }

    Uint32 GetSignatureCount() const { return m_SignatureCount; }

    PipelineResourceSignatureD3D12Impl* GetResourceSignature(Uint32 index) const
    {
        VERIFY_EXPR(index < m_SignatureCount);
        return m_ResourceSignatures[index].pSignature;
    }

    ID3D12RootSignature* GetD3D12RootSignature() const
    {
        VERIFY_EXPR(m_pd3d12RootSignature);
        return m_pd3d12RootSignature;
    }

    Uint32 GetBaseRootIndex(Uint32 BindingIndex) const
    {
        VERIFY_EXPR(BindingIndex < m_SignatureCount);
        return m_ResourceSignatures[BindingIndex].BaseRootIndex;
    }

    Uint32 GetBaseRegisterSpace(Uint32 BindingIndex) const
    {
        VERIFY_EXPR(BindingIndex <= m_SignatureCount);
        return m_ResourceSignatures[BindingIndex].BaseRegisterSpace;
    }

    /// Returns the total number of register spaces used by all resource signatures
    Uint32 GetTotalSpaces() const
    {
        return m_TotalSpacesUsed;
    }

    bool IsCompatibleWith(const RefCntAutoPtr<PipelineResourceSignatureD3D12Impl> ppSignatures[], Uint32 SignatureCount) const noexcept;

private:
    // The number of pipeline resource signatures used to initialize this root signature.
    const Uint32 m_SignatureCount;

    // The total number of register spaces used by this root signature.
    Uint32 m_TotalSpacesUsed = 0;

    // Root signature hash.
    const size_t m_Hash;

    CComPtr<ID3D12RootSignature> m_pd3d12RootSignature;

    struct ResourceSignatureInfo
    {
        RefCntAutoPtr<PipelineResourceSignatureD3D12Impl> pSignature;

        Uint32 BaseRootIndex     = 0;
        Uint32 BaseRegisterSpace = 0;
    };
    std::unique_ptr<ResourceSignatureInfo[]> m_ResourceSignatures;

    RootSignatureCacheD3D12* m_pCache = nullptr;
};



class LocalRootSignatureD3D12
{
public:
    LocalRootSignatureD3D12(const char* pCBName, Uint32 ShaderRecordSize);

    bool IsShaderRecord(const D3DShaderResourceAttribs& CB) const;

    bool Create(ID3D12Device* pDevice, Uint32 RegisterSpace);

    ID3D12RootSignature* GetD3D12RootSignature() const { return m_pd3d12RootSignature; }
    bool                 IsDefined() const { return m_ShaderRecordSize > 0 && !m_Name.empty(); }
    const char*          GetName() const { return m_Name.c_str(); }
    Uint32               GetShaderRegister() const { return 0; }

    Uint32 GetRegisterSpace() const
    {
        VERIFY_EXPR(m_RegisterSpace != ~0U);
        return m_RegisterSpace;
    }

private:
    const std::string            m_Name;
    const Uint32                 m_ShaderRecordSize = 0;
    Uint32                       m_RegisterSpace    = ~0u;
    CComPtr<ID3D12RootSignature> m_pd3d12RootSignature;
};


/// Root signature cache that deduplicates RootSignatureD3D12 objects.
class RootSignatureCacheD3D12
{
public:
    RootSignatureCacheD3D12(RenderDeviceD3D12Impl& DeviceD3D12Impl);

    // clang-format off
    RootSignatureCacheD3D12             (const RootSignatureCacheD3D12&) = delete;
    RootSignatureCacheD3D12             (RootSignatureCacheD3D12&&)      = delete;
    RootSignatureCacheD3D12& operator = (const RootSignatureCacheD3D12&) = delete;
    RootSignatureCacheD3D12& operator = (RootSignatureCacheD3D12&&)      = delete;
    // clang-format on

    ~RootSignatureCacheD3D12();

    RefCntAutoPtr<RootSignatureD3D12> GetRootSig(const RefCntAutoPtr<PipelineResourceSignatureD3D12Impl>* ppSignatures, Uint32 SignatureCount);

    void OnDestroyRootSig(RootSignatureD3D12* pRootSig);

private:
    RenderDeviceD3D12Impl& m_DeviceD3D12Impl;

    std::mutex                                                         m_RootSigCacheMtx;
    std::unordered_multimap<size_t, RefCntWeakPtr<RootSignatureD3D12>> m_RootSigCache;
};

} // namespace Diligent
