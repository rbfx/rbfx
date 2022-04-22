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

#include "D3D12/TestingEnvironmentD3D12.hpp"
#include "D3D12/TestingSwapChainD3D12.hpp"
#include "../include/DXGITypeConversions.hpp"
#include "../include/d3dx12_win.h"

#include "Align.hpp"
#include "BasicMath.hpp"

#include "InlineShaders/RayTracingTestHLSL.h"
#include "RayTracingTestConstants.hpp"

namespace Diligent
{

namespace Testing
{

namespace
{

struct RTContext
{
    struct AccelStruct
    {
        CComPtr<ID3D12Resource> pAS;
        UINT64                  BuildScratchSize  = 0;
        UINT64                  UpdateScratchSize = 0;
    };

    CComPtr<ID3D12Device5>               pDevice;
    CComPtr<ID3D12GraphicsCommandList4>  pCmdList;
    CComPtr<ID3D12StateObject>           pRayTracingSO;
    CComPtr<ID3D12StateObjectProperties> pStateObjectProperties;
    CComPtr<ID3D12RootSignature>         pGlobalRootSignature;
    CComPtr<ID3D12RootSignature>         pLocalRootSignature;
    AccelStruct                          BLAS;
    AccelStruct                          TLAS;
    CComPtr<ID3D12Resource>              pScratchBuffer;
    CComPtr<ID3D12Resource>              pVertexBuffer;
    CComPtr<ID3D12Resource>              pIndexBuffer;
    CComPtr<ID3D12Resource>              pInstanceBuffer;
    CComPtr<ID3D12Resource>              pSBTBuffer;
    CComPtr<ID3D12Resource>              pUploadBuffer;
    void*                                MappedPtr       = nullptr;
    size_t                               MappedOffset    = 0;
    size_t                               MaxMappedOffset = 0;
    ID3D12Resource*                      pRenderTarget   = nullptr;
    CComPtr<ID3D12DescriptorHeap>        pDescHeap;
    Uint32                               DescHeapCount  = 0;
    Uint32                               DescHandleSize = 0;

    RTContext()
    {}

    ~RTContext()
    {
        if (pUploadBuffer && MappedPtr)
            pUploadBuffer->Unmap(0, nullptr);
    }

    void ClearRenderTarget(TestingSwapChainD3D12* pTestingSwapChainD3D12)
    {
        pTestingSwapChainD3D12->TransitionRenderTarget(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto RTVDescriptorHandle = pTestingSwapChainD3D12->GetRTVDescriptorHandle();

        pCmdList->OMSetRenderTargets(1, &RTVDescriptorHandle, FALSE, nullptr);

        float ClearColor[] = {0, 0, 0, 0};
        pCmdList->ClearRenderTargetView(RTVDescriptorHandle, ClearColor, 0, nullptr);

        pCmdList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
    }

    static constexpr UINT DescriptorHeapSize = 16;
};

struct RTSubobjectsHelper
{
    std::vector<D3D12_STATE_SUBOBJECT>   Subobjects;
    std::vector<D3D12_EXPORT_DESC>       ExportDescs;
    std::vector<D3D12_DXIL_LIBRARY_DESC> LibDescs;
    std::vector<D3D12_HIT_GROUP_DESC>    HitGroups;
    std::vector<CComPtr<ID3DBlob>>       ShadersByteCode;

    void SetShaderCount(Uint32 NumShaders, Uint32 NumHitGroups)
    {
        ShadersByteCode.resize(NumShaders);
        ExportDescs.resize(NumShaders);
        LibDescs.resize(NumShaders);
        HitGroups.resize(NumHitGroups);
    }

    void SetDxilLibrary(Uint32 Index, const String& Source, const wchar_t* ExportName)
    {
        auto* pEnv = TestingEnvironmentD3D12::GetInstance();
        auto  hr   = pEnv->CompileDXILShader(Source, L"main", nullptr, 0, L"lib_6_3", &ShadersByteCode[Index]);
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to compile ray tracing shader";

        D3D12_EXPORT_DESC&       RGExportDesc = ExportDescs[Index];
        D3D12_DXIL_LIBRARY_DESC& RGLibDesc    = LibDescs[Index];
        RGExportDesc.Flags                    = D3D12_EXPORT_FLAG_NONE;
        RGExportDesc.ExportToRename           = L"main"; // shader entry name
        RGExportDesc.Name                     = ExportName;
        RGLibDesc.DXILLibrary.BytecodeLength  = ShadersByteCode[Index]->GetBufferSize();
        RGLibDesc.DXILLibrary.pShaderBytecode = ShadersByteCode[Index]->GetBufferPointer();
        RGLibDesc.NumExports                  = 1;
        RGLibDesc.pExports                    = &RGExportDesc;
        Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &RGLibDesc});
    }

    void SetTriangleHitGroup(Uint32 Index, const wchar_t* GroupName, const wchar_t* ClosestHitShaderImport, const wchar_t* AnyHitShaderImport = nullptr)
    {
        D3D12_HIT_GROUP_DESC& HitGroupDesc    = HitGroups[Index];
        HitGroupDesc.HitGroupExport           = GroupName;
        HitGroupDesc.Type                     = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        HitGroupDesc.ClosestHitShaderImport   = ClosestHitShaderImport;
        HitGroupDesc.AnyHitShaderImport       = AnyHitShaderImport;
        HitGroupDesc.IntersectionShaderImport = nullptr;
        Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &HitGroupDesc});
    }

    void SetProceduralHitGroup(Uint32 Index, const wchar_t* GroupName, const wchar_t* IntersectionShaderImport, const wchar_t* ClosestHitShaderImport, const wchar_t* AnyHitShaderImport = nullptr)
    {
        D3D12_HIT_GROUP_DESC& HitGroupDesc    = HitGroups[Index];
        HitGroupDesc.HitGroupExport           = GroupName;
        HitGroupDesc.Type                     = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
        HitGroupDesc.ClosestHitShaderImport   = ClosestHitShaderImport;
        HitGroupDesc.AnyHitShaderImport       = AnyHitShaderImport;
        HitGroupDesc.IntersectionShaderImport = IntersectionShaderImport;
        Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &HitGroupDesc});
    }
};

template <typename PSOCtorType, typename RootSigCtorType>
void InitializeRTContext(RTContext& Ctx, ISwapChain* pSwapChain, Uint32 ShaderRecordSize, PSOCtorType&& PSOCtor, RootSigCtorType&& RootSigCtor)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pTestingSwapChainD3D12 = ClassPtrCast<TestingSwapChainD3D12>(pSwapChain);

    auto hr = pEnv->GetD3D12Device()->QueryInterface(IID_PPV_ARGS(&Ctx.pDevice));
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to get ID3D12Device2";

    Ctx.pRenderTarget = pTestingSwapChainD3D12->GetD3D12RenderTarget();

    hr = pEnv->CreateGraphicsCommandList()->QueryInterface(IID_PPV_ARGS(&Ctx.pCmdList));
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to get ID3D12GraphicsCommandList4";

    // create descriptor heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC Desc = {};

        Desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        Desc.NumDescriptors = Ctx.DescriptorHeapSize;
        Desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        Desc.NodeMask       = 0;

        hr = Ctx.pDevice->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Ctx.pDescHeap));
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create descriptor heap";

        Ctx.DescHeapCount  = 0;
        Ctx.DescHandleSize = Ctx.pDevice->GetDescriptorHandleIncrementSize(Desc.Type);

        D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};

        UAVDesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

        D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle = Ctx.pDescHeap->GetCPUDescriptorHandleForHeapStart();
        ASSERT_LT(Ctx.DescHeapCount, Ctx.DescriptorHeapSize);
        ASSERT_TRUE(Ctx.DescHeapCount == 0);
        UAVHandle.ptr += Ctx.DescHandleSize * Ctx.DescHeapCount++;
        Ctx.pDevice->CreateUnorderedAccessView(pTestingSwapChainD3D12->GetD3D12RenderTarget(), nullptr, &UAVDesc, UAVHandle);
    }

    // create global root signature
    {
        D3D12_ROOT_SIGNATURE_DESC           RootSignatureDesc = {};
        D3D12_ROOT_PARAMETER                Param             = {};
        D3D12_DESCRIPTOR_RANGE              Range             = {};
        std::vector<D3D12_DESCRIPTOR_RANGE> DescriptorRanges;

        RootSigCtor(DescriptorRanges);

        Range.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        Range.NumDescriptors                    = 1;
        Range.OffsetInDescriptorsFromTableStart = 0;
        DescriptorRanges.push_back(Range); // g_ColorBuffer

        Range.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        Range.NumDescriptors                    = 1;
        Range.OffsetInDescriptorsFromTableStart = 1;
        DescriptorRanges.push_back(Range); // g_TLAS

        Param.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        Param.ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
        Param.DescriptorTable.NumDescriptorRanges = static_cast<Uint32>(DescriptorRanges.size());
        Param.DescriptorTable.pDescriptorRanges   = DescriptorRanges.data();

        RootSignatureDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE;
        RootSignatureDesc.NumParameters = 1;
        RootSignatureDesc.pParameters   = &Param;

        CComPtr<ID3DBlob> signature;
        hr = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
        ASSERT_HRESULT_SUCCEEDED(hr);

        hr = Ctx.pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&Ctx.pGlobalRootSignature));
        ASSERT_HRESULT_SUCCEEDED(hr);
    }

    // create local root signature
    if (ShaderRecordSize > 0)
    {
        D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
        D3D12_ROOT_PARAMETER      Param             = {};

        Param.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        Param.ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;
        Param.Constants.Num32BitValues = ShaderRecordSize / 4;
        Param.Constants.RegisterSpace  = 0;
        Param.Constants.ShaderRegister = 0;

        RootSignatureDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
        RootSignatureDesc.NumParameters = 1;
        RootSignatureDesc.pParameters   = &Param;

        CComPtr<ID3DBlob> signature;
        hr = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
        ASSERT_HRESULT_SUCCEEDED(hr);

        hr = Ctx.pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&Ctx.pLocalRootSignature));
        ASSERT_HRESULT_SUCCEEDED(hr);
    }

    // create ray tracing state object
    {
        RTSubobjectsHelper Helper;
        PSOCtor(Helper);

        D3D12_RAYTRACING_PIPELINE_CONFIG PipelineConfig;
        PipelineConfig.MaxTraceRecursionDepth = 1;
        Helper.Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &PipelineConfig});

        D3D12_RAYTRACING_SHADER_CONFIG ShaderConfig;
        ShaderConfig.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;
        ShaderConfig.MaxPayloadSizeInBytes   = 4 * sizeof(float);
        Helper.Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &ShaderConfig});

        D3D12_GLOBAL_ROOT_SIGNATURE GlobalRoot;
        GlobalRoot.pGlobalRootSignature = Ctx.pGlobalRootSignature;
        Helper.Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &GlobalRoot});

        D3D12_LOCAL_ROOT_SIGNATURE LocalRoot;
        LocalRoot.pLocalRootSignature = Ctx.pLocalRootSignature;
        if (Ctx.pLocalRootSignature)
            Helper.Subobjects.push_back({D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, &LocalRoot});

        D3D12_STATE_OBJECT_DESC RTPipelineDesc;
        RTPipelineDesc.Type          = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        RTPipelineDesc.NumSubobjects = static_cast<UINT>(Helper.Subobjects.size());
        RTPipelineDesc.pSubobjects   = Helper.Subobjects.data();

        hr = Ctx.pDevice->CreateStateObject(&RTPipelineDesc, IID_PPV_ARGS(&Ctx.pRayTracingSO));
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create state object";

        hr = Ctx.pRayTracingSO->QueryInterface(IID_PPV_ARGS(&Ctx.pStateObjectProperties));
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to get state object properties";
    }
}

template <typename PSOCtorType>
void InitializeRTContext(RTContext& Ctx, ISwapChain* pSwapChain, Uint32 ShaderRecordSize, PSOCtorType&& PSOCtor)
{
    InitializeRTContext(Ctx, pSwapChain, ShaderRecordSize, PSOCtor, [](std::vector<D3D12_DESCRIPTOR_RANGE>&) {});
}

void CreateBLAS(RTContext& Ctx, D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& BottomLevelInputs)
{
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO BottomLevelPrebuildInfo = {};

    BottomLevelInputs.Type        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    BottomLevelInputs.Flags       = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
    BottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    Ctx.pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&BottomLevelInputs, &BottomLevelPrebuildInfo);
    ASSERT_GT(BottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, UINT64{0});

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask     = 1;
    HeapProps.VisibleNodeMask      = 1;

    D3D12_RESOURCE_DESC ASDesc = {};
    ASDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    ASDesc.Alignment           = 0;
    ASDesc.Width               = BottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;
    ASDesc.Height              = 1;
    ASDesc.DepthOrArraySize    = 1;
    ASDesc.MipLevels           = 1;
    ASDesc.Format              = DXGI_FORMAT_UNKNOWN;
    ASDesc.SampleDesc.Count    = 1;
    ASDesc.SampleDesc.Quality  = 0;
    ASDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ASDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    auto hr = Ctx.pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                   &ASDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr,
                                                   IID_PPV_ARGS(&Ctx.BLAS.pAS));
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create acceleration structure";

    Ctx.BLAS.BuildScratchSize  = BottomLevelPrebuildInfo.ScratchDataSizeInBytes;
    Ctx.BLAS.UpdateScratchSize = BottomLevelPrebuildInfo.UpdateScratchDataSizeInBytes;
}

void CreateTLAS(RTContext& Ctx, D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& TopLevelInputs)
{
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO TopLevelPrebuildInfo = {};

    TopLevelInputs.Type        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    TopLevelInputs.Flags       = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
    TopLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    Ctx.pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&TopLevelInputs, &TopLevelPrebuildInfo);
    ASSERT_GT(TopLevelPrebuildInfo.ResultDataMaxSizeInBytes, UINT64{0});

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask     = 1;
    HeapProps.VisibleNodeMask      = 1;

    D3D12_RESOURCE_DESC ASDesc = {};
    ASDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    ASDesc.Alignment           = 0;
    ASDesc.Width               = TopLevelPrebuildInfo.ResultDataMaxSizeInBytes;
    ASDesc.Height              = 1;
    ASDesc.DepthOrArraySize    = 1;
    ASDesc.MipLevels           = 1;
    ASDesc.Format              = DXGI_FORMAT_UNKNOWN;
    ASDesc.SampleDesc.Count    = 1;
    ASDesc.SampleDesc.Quality  = 0;
    ASDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ASDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    auto hr = Ctx.pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                   &ASDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr,
                                                   IID_PPV_ARGS(&Ctx.TLAS.pAS));
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create acceleration structure";

    Ctx.TLAS.BuildScratchSize  = TopLevelPrebuildInfo.ScratchDataSizeInBytes;
    Ctx.TLAS.UpdateScratchSize = TopLevelPrebuildInfo.UpdateScratchDataSizeInBytes;

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc          = {};
    SRVDesc.ViewDimension                            = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    SRVDesc.Shader4ComponentMapping                  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Format                                   = DXGI_FORMAT_UNKNOWN;
    SRVDesc.RaytracingAccelerationStructure.Location = Ctx.TLAS.pAS->GetGPUVirtualAddress();

    D3D12_CPU_DESCRIPTOR_HANDLE DescHandle = Ctx.pDescHeap->GetCPUDescriptorHandleForHeapStart();
    ASSERT_LT(Ctx.DescHeapCount, Ctx.DescriptorHeapSize);
    ASSERT_TRUE(Ctx.DescHeapCount == 1);
    DescHandle.ptr += Ctx.DescHandleSize * Ctx.DescHeapCount++;

    Ctx.pDevice->CreateShaderResourceView(nullptr, &SRVDesc, DescHandle);
}

void CreateRTBuffers(RTContext& Ctx, Uint32 VBSize, Uint32 IBSize, Uint32 InstanceCount, Uint32 NumMissShaders, Uint32 NumHitShaders, Uint32 ShaderRecordSize = 0, UINT64 UploadSize = 0)
{
    D3D12_RESOURCE_DESC BuffDesc = {};
    BuffDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    BuffDesc.Alignment           = 0;
    BuffDesc.Height              = 1;
    BuffDesc.DepthOrArraySize    = 1;
    BuffDesc.MipLevels           = 1;
    BuffDesc.Format              = DXGI_FORMAT_UNKNOWN;
    BuffDesc.SampleDesc.Count    = 1;
    BuffDesc.SampleDesc.Quality  = 0;
    BuffDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    BuffDesc.Flags               = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask     = 1;
    HeapProps.VisibleNodeMask      = 1;

    BuffDesc.Width = std::max(Ctx.BLAS.BuildScratchSize, Ctx.BLAS.UpdateScratchSize);
    BuffDesc.Width = std::max(BuffDesc.Width, Ctx.TLAS.BuildScratchSize);
    BuffDesc.Width = std::max(BuffDesc.Width, Ctx.TLAS.UpdateScratchSize);

    auto hr = Ctx.pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                   &BuffDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
                                                   IID_PPV_ARGS(&Ctx.pScratchBuffer));
    ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create buffer";

    if (VBSize > 0)
    {
        BuffDesc.Width = VBSize;

        hr = Ctx.pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                  &BuffDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                  IID_PPV_ARGS(&Ctx.pVertexBuffer));
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create buffer";
        UploadSize += BuffDesc.Width;
    }

    if (IBSize > 0)
    {
        BuffDesc.Width = IBSize;

        hr = Ctx.pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                  &BuffDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                  IID_PPV_ARGS(&Ctx.pIndexBuffer));
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create buffer";
        UploadSize += BuffDesc.Width;
    }

    if (InstanceCount > 0)
    {
        BuffDesc.Width = InstanceCount * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

        hr = Ctx.pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                  &BuffDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                  IID_PPV_ARGS(&Ctx.pInstanceBuffer));
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create buffer";
        UploadSize += BuffDesc.Width;
    }

    // SBT
    {
        const UINT64 RecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + ShaderRecordSize;
        const UINT64 align      = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

        BuffDesc.Width = AlignUp(RecordSize, align);
        BuffDesc.Width = AlignUp(BuffDesc.Width + NumMissShaders * RecordSize, align);
        BuffDesc.Width = AlignUp(BuffDesc.Width + NumHitShaders * RecordSize, align);

        hr = Ctx.pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                  &BuffDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                  IID_PPV_ARGS(&Ctx.pSBTBuffer));
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create buffer";
        UploadSize += BuffDesc.Width;
    }

    BuffDesc.Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    if (UploadSize > 0)
    {
        BuffDesc.Width = UploadSize;
        hr             = Ctx.pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                  &BuffDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                  IID_PPV_ARGS(&Ctx.pUploadBuffer));
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create buffer";

        hr = Ctx.pUploadBuffer->Map(0, nullptr, &Ctx.MappedPtr);
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to map buffer";

        Ctx.MappedOffset    = 0;
        Ctx.MaxMappedOffset = static_cast<size_t>(UploadSize);
    }
}

void UpdateBuffer(RTContext& Ctx, ID3D12Resource* pBuffer, UINT64 Offset, const void* pData, UINT64 DataSize)
{
    VERIFY_EXPR(pBuffer != nullptr);
    VERIFY_EXPR(pData != nullptr);
    VERIFY_EXPR(Ctx.MappedOffset + DataSize <= Ctx.MaxMappedOffset);

    Ctx.pCmdList->CopyBufferRegion(pBuffer, Offset, Ctx.pUploadBuffer, Ctx.MappedOffset, DataSize);

    std::memcpy(reinterpret_cast<char*>(Ctx.MappedPtr) + Ctx.MappedOffset, pData, static_cast<size_t>(DataSize));
    Ctx.MappedOffset += static_cast<size_t>(DataSize);
}

void ASPrebuildBarriers(const RTContext& Ctx)
{
    std::vector<D3D12_RESOURCE_BARRIER> Barriers;
    D3D12_RESOURCE_BARRIER              Barrier;

    Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    Barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    if (Ctx.pVertexBuffer)
    {
        Barrier.Transition.pResource = Ctx.pVertexBuffer;
        Barriers.push_back(Barrier);
    }
    if (Ctx.pIndexBuffer)
    {
        Barrier.Transition.pResource = Ctx.pIndexBuffer;
        Barriers.push_back(Barrier);
    }
    if (Ctx.pInstanceBuffer)
    {
        Barrier.Transition.pResource = Ctx.pInstanceBuffer;
        Barriers.push_back(Barrier);
    }
    Ctx.pCmdList->ResourceBarrier(static_cast<UINT>(Barriers.size()), Barriers.data());
}

void SBTBufferBarrier(const RTContext& Ctx)
{
    D3D12_RESOURCE_BARRIER Barrier;
    Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    Barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    Barrier.Transition.pResource   = Ctx.pSBTBuffer;
    Ctx.pCmdList->ResourceBarrier(1, &Barrier);
}

void UAVBarrier(const RTContext& Ctx, ID3D12Resource* pResource)
{
    D3D12_RESOURCE_BARRIER Barrier;
    Barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    Barrier.Flags         = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.UAV.pResource = pResource;

    Ctx.pCmdList->ResourceBarrier(1, &Barrier);
}

} // namespace


void RayTracingTriangleClosestHitReferenceD3D12(ISwapChain* pSwapChain)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pTestingSwapChainD3D12 = ClassPtrCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    RTContext Ctx = {};
    InitializeRTContext(Ctx, pSwapChain, 0,
                        [](RTSubobjectsHelper& SubObj) {
                            SubObj.SetShaderCount(3, 1);
                            SubObj.SetDxilLibrary(0, HLSL::RayTracingTest1_RG, L"Main");
                            SubObj.SetDxilLibrary(1, HLSL::RayTracingTest1_RM, L"Miss");
                            SubObj.SetDxilLibrary(2, HLSL::RayTracingTest1_RCH, L"ClosestHitShader");
                            SubObj.SetTriangleHitGroup(0, L"HitGroup", L"ClosestHitShader", nullptr);
                        });

    // Create acceleration structures
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    BLASDesc          = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& BottomLevelInputs = BLASDesc.Inputs;
        D3D12_RAYTRACING_GEOMETRY_DESC                        Geometry          = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    TLASDesc          = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& TopLevelInputs    = TLASDesc.Inputs;
        D3D12_RAYTRACING_INSTANCE_DESC                        Instance          = {};

        const auto& Vertices = TestingConstants::TriangleClosestHit::Vertices;

        Geometry.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        Geometry.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        Geometry.Triangles.VertexBuffer.StartAddress  = 0;
        Geometry.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertices[0]);
        Geometry.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
        Geometry.Triangles.VertexCount                = _countof(Vertices);
        Geometry.Triangles.IndexCount                 = 0;
        Geometry.Triangles.IndexFormat                = DXGI_FORMAT_UNKNOWN;
        Geometry.Triangles.IndexBuffer                = 0;
        Geometry.Triangles.Transform3x4               = 0;

        BottomLevelInputs.pGeometryDescs = &Geometry;
        BottomLevelInputs.NumDescs       = 1;

        TopLevelInputs.NumDescs = 1;

        CreateBLAS(Ctx, BottomLevelInputs);
        CreateTLAS(Ctx, TopLevelInputs);
        CreateRTBuffers(Ctx, sizeof(Vertices), 0, 1, 1, 1);

        Instance.InstanceID                          = 0;
        Instance.InstanceContributionToHitGroupIndex = 0;
        Instance.InstanceMask                        = 0xFF;
        Instance.Flags                               = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        Instance.AccelerationStructure               = Ctx.BLAS.pAS->GetGPUVirtualAddress();
        Instance.Transform[0][0]                     = 1.0f;
        Instance.Transform[1][1]                     = 1.0f;
        Instance.Transform[2][2]                     = 1.0f;

        UpdateBuffer(Ctx, Ctx.pVertexBuffer, 0, Vertices, sizeof(Vertices));
        UpdateBuffer(Ctx, Ctx.pInstanceBuffer, 0, &Instance, sizeof(Instance));
        ASPrebuildBarriers(Ctx);

        Geometry.Triangles.VertexBuffer.StartAddress = Ctx.pVertexBuffer->GetGPUVirtualAddress();

        BLASDesc.DestAccelerationStructureData    = Ctx.BLAS.pAS->GetGPUVirtualAddress();
        BLASDesc.ScratchAccelerationStructureData = Ctx.pScratchBuffer->GetGPUVirtualAddress();
        BLASDesc.SourceAccelerationStructureData  = 0;

        ASSERT_TRUE(Geometry.Triangles.VertexBuffer.StartAddress != 0);
        ASSERT_TRUE(BLASDesc.DestAccelerationStructureData != 0);
        ASSERT_TRUE(BLASDesc.ScratchAccelerationStructureData != 0);

        Ctx.pCmdList->BuildRaytracingAccelerationStructure(&BLASDesc, 0, nullptr);

        UAVBarrier(Ctx, Ctx.pScratchBuffer);

        TopLevelInputs.InstanceDescs = Ctx.pInstanceBuffer->GetGPUVirtualAddress();

        TLASDesc.DestAccelerationStructureData    = Ctx.TLAS.pAS->GetGPUVirtualAddress();
        TLASDesc.ScratchAccelerationStructureData = Ctx.pScratchBuffer->GetGPUVirtualAddress();
        TLASDesc.SourceAccelerationStructureData  = 0;

        ASSERT_TRUE(TLASDesc.DestAccelerationStructureData != 0);
        ASSERT_TRUE(TLASDesc.ScratchAccelerationStructureData != 0);

        Ctx.pCmdList->BuildRaytracingAccelerationStructure(&TLASDesc, 0, nullptr);
    }

    Ctx.ClearRenderTarget(pTestingSwapChainD3D12);

    // Trace rays
    {
        pTestingSwapChainD3D12->TransitionRenderTarget(Ctx.pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        ID3D12DescriptorHeap* DescHeaps[] = {Ctx.pDescHeap};

        Ctx.pCmdList->SetPipelineState1(Ctx.pRayTracingSO);
        Ctx.pCmdList->SetComputeRootSignature(Ctx.pGlobalRootSignature);

        Ctx.pCmdList->SetDescriptorHeaps(_countof(DescHeaps), &DescHeaps[0]);
        Ctx.pCmdList->SetComputeRootDescriptorTable(0, DescHeaps[0]->GetGPUDescriptorHandleForHeapStart());

        D3D12_DISPATCH_RAYS_DESC Desc = {};

        Desc.Width  = SCDesc.Width;
        Desc.Height = SCDesc.Height;
        Desc.Depth  = 1;

        const UINT64 handleSize     = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        const UINT64 align          = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        const UINT64 RayGenOffset   = 0;
        const UINT64 RayMissOffset  = AlignUp(RayGenOffset + handleSize, align);
        const UINT64 HitGroupOffset = AlignUp(RayMissOffset + handleSize, align);

        Desc.RayGenerationShaderRecord.StartAddress = Ctx.pSBTBuffer->GetGPUVirtualAddress() + RayGenOffset;
        Desc.RayGenerationShaderRecord.SizeInBytes  = handleSize;
        Desc.MissShaderTable.StartAddress           = Ctx.pSBTBuffer->GetGPUVirtualAddress() + RayMissOffset;
        Desc.MissShaderTable.SizeInBytes            = handleSize;
        Desc.MissShaderTable.StrideInBytes          = handleSize;
        Desc.HitGroupTable.StartAddress             = Ctx.pSBTBuffer->GetGPUVirtualAddress() + HitGroupOffset;
        Desc.HitGroupTable.SizeInBytes              = handleSize;
        Desc.HitGroupTable.StrideInBytes            = handleSize;

        UpdateBuffer(Ctx, Ctx.pSBTBuffer, RayGenOffset, Ctx.pStateObjectProperties->GetShaderIdentifier(L"Main"), handleSize);
        UpdateBuffer(Ctx, Ctx.pSBTBuffer, RayMissOffset, Ctx.pStateObjectProperties->GetShaderIdentifier(L"Miss"), handleSize);
        UpdateBuffer(Ctx, Ctx.pSBTBuffer, HitGroupOffset, Ctx.pStateObjectProperties->GetShaderIdentifier(L"HitGroup"), handleSize);
        SBTBufferBarrier(Ctx);

        Ctx.pCmdList->DispatchRays(&Desc);
    }

    Ctx.pCmdList->Close();
    pEnv->ExecuteCommandList(Ctx.pCmdList);
}

void RayTracingTriangleAnyHitReferenceD3D12(ISwapChain* pSwapChain)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pTestingSwapChainD3D12 = ClassPtrCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    RTContext Ctx = {};
    InitializeRTContext(Ctx, pSwapChain, 0,
                        [](RTSubobjectsHelper& SubObj) {
                            SubObj.SetShaderCount(4, 1);
                            SubObj.SetDxilLibrary(0, HLSL::RayTracingTest2_RG, L"Main");
                            SubObj.SetDxilLibrary(1, HLSL::RayTracingTest2_RM, L"Miss");
                            SubObj.SetDxilLibrary(2, HLSL::RayTracingTest2_RCH, L"ClosestHitShader");
                            SubObj.SetDxilLibrary(3, HLSL::RayTracingTest2_RAH, L"AnyHitShader");
                            SubObj.SetTriangleHitGroup(0, L"HitGroup", L"ClosestHitShader", L"AnyHitShader");
                        });

    // Create acceleration structures
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    BLASDesc          = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& BottomLevelInputs = BLASDesc.Inputs;
        D3D12_RAYTRACING_GEOMETRY_DESC                        Geometry          = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    TLASDesc          = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& TopLevelInputs    = TLASDesc.Inputs;
        D3D12_RAYTRACING_INSTANCE_DESC                        Instance          = {};

        const auto& Vertices = TestingConstants::TriangleAnyHit::Vertices;

        Geometry.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        Geometry.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
        Geometry.Triangles.VertexBuffer.StartAddress  = 0;
        Geometry.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertices[0]);
        Geometry.Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
        Geometry.Triangles.VertexCount                = _countof(Vertices);
        Geometry.Triangles.IndexCount                 = 0;
        Geometry.Triangles.IndexFormat                = DXGI_FORMAT_UNKNOWN;
        Geometry.Triangles.IndexBuffer                = 0;
        Geometry.Triangles.Transform3x4               = 0;

        BottomLevelInputs.pGeometryDescs = &Geometry;
        BottomLevelInputs.NumDescs       = 1;

        TopLevelInputs.NumDescs = 1;

        CreateBLAS(Ctx, BottomLevelInputs);
        CreateTLAS(Ctx, TopLevelInputs);
        CreateRTBuffers(Ctx, sizeof(Vertices), 0, 1, 1, 1);

        Instance.InstanceID                          = 0;
        Instance.InstanceContributionToHitGroupIndex = 0;
        Instance.InstanceMask                        = 0xFF;
        Instance.Flags                               = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        Instance.AccelerationStructure               = Ctx.BLAS.pAS->GetGPUVirtualAddress();
        Instance.Transform[0][0]                     = 1.0f;
        Instance.Transform[1][1]                     = 1.0f;
        Instance.Transform[2][2]                     = 1.0f;

        UpdateBuffer(Ctx, Ctx.pVertexBuffer, 0, Vertices, sizeof(Vertices));
        UpdateBuffer(Ctx, Ctx.pInstanceBuffer, 0, &Instance, sizeof(Instance));
        ASPrebuildBarriers(Ctx);

        Geometry.Triangles.VertexBuffer.StartAddress = Ctx.pVertexBuffer->GetGPUVirtualAddress();

        BLASDesc.DestAccelerationStructureData    = Ctx.BLAS.pAS->GetGPUVirtualAddress();
        BLASDesc.ScratchAccelerationStructureData = Ctx.pScratchBuffer->GetGPUVirtualAddress();
        BLASDesc.SourceAccelerationStructureData  = 0;

        ASSERT_TRUE(Geometry.Triangles.VertexBuffer.StartAddress != 0);
        ASSERT_TRUE(BLASDesc.DestAccelerationStructureData != 0);
        ASSERT_TRUE(BLASDesc.ScratchAccelerationStructureData != 0);

        Ctx.pCmdList->BuildRaytracingAccelerationStructure(&BLASDesc, 0, nullptr);

        UAVBarrier(Ctx, Ctx.pScratchBuffer);

        TopLevelInputs.InstanceDescs = Ctx.pInstanceBuffer->GetGPUVirtualAddress();

        TLASDesc.DestAccelerationStructureData    = Ctx.TLAS.pAS->GetGPUVirtualAddress();
        TLASDesc.ScratchAccelerationStructureData = Ctx.pScratchBuffer->GetGPUVirtualAddress();
        TLASDesc.SourceAccelerationStructureData  = 0;

        ASSERT_TRUE(TLASDesc.DestAccelerationStructureData != 0);
        ASSERT_TRUE(TLASDesc.ScratchAccelerationStructureData != 0);

        Ctx.pCmdList->BuildRaytracingAccelerationStructure(&TLASDesc, 0, nullptr);
    }

    Ctx.ClearRenderTarget(pTestingSwapChainD3D12);

    // Trace rays
    {
        pTestingSwapChainD3D12->TransitionRenderTarget(Ctx.pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        ID3D12DescriptorHeap* DescHeaps[] = {Ctx.pDescHeap};

        Ctx.pCmdList->SetPipelineState1(Ctx.pRayTracingSO);
        Ctx.pCmdList->SetComputeRootSignature(Ctx.pGlobalRootSignature);

        Ctx.pCmdList->SetDescriptorHeaps(_countof(DescHeaps), &DescHeaps[0]);
        Ctx.pCmdList->SetComputeRootDescriptorTable(0, DescHeaps[0]->GetGPUDescriptorHandleForHeapStart());

        D3D12_DISPATCH_RAYS_DESC Desc = {};

        Desc.Width  = SCDesc.Width;
        Desc.Height = SCDesc.Height;
        Desc.Depth  = 1;

        const UINT64 handleSize     = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        const UINT64 align          = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        const UINT64 RayGenOffset   = 0;
        const UINT64 RayMissOffset  = AlignUp(RayGenOffset + handleSize, align);
        const UINT64 HitGroupOffset = AlignUp(RayMissOffset + handleSize, align);

        Desc.RayGenerationShaderRecord.StartAddress = Ctx.pSBTBuffer->GetGPUVirtualAddress() + RayGenOffset;
        Desc.RayGenerationShaderRecord.SizeInBytes  = handleSize;
        Desc.MissShaderTable.StartAddress           = Ctx.pSBTBuffer->GetGPUVirtualAddress() + RayMissOffset;
        Desc.MissShaderTable.SizeInBytes            = handleSize;
        Desc.MissShaderTable.StrideInBytes          = handleSize;
        Desc.HitGroupTable.StartAddress             = Ctx.pSBTBuffer->GetGPUVirtualAddress() + HitGroupOffset;
        Desc.HitGroupTable.SizeInBytes              = handleSize;
        Desc.HitGroupTable.StrideInBytes            = handleSize;

        UpdateBuffer(Ctx, Ctx.pSBTBuffer, RayGenOffset, Ctx.pStateObjectProperties->GetShaderIdentifier(L"Main"), handleSize);
        UpdateBuffer(Ctx, Ctx.pSBTBuffer, RayMissOffset, Ctx.pStateObjectProperties->GetShaderIdentifier(L"Miss"), handleSize);
        UpdateBuffer(Ctx, Ctx.pSBTBuffer, HitGroupOffset, Ctx.pStateObjectProperties->GetShaderIdentifier(L"HitGroup"), handleSize);
        SBTBufferBarrier(Ctx);

        Ctx.pCmdList->DispatchRays(&Desc);
    }

    Ctx.pCmdList->Close();
    pEnv->ExecuteCommandList(Ctx.pCmdList);
}


void RayTracingProceduralIntersectionReferenceD3D12(ISwapChain* pSwapChain)
{
    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pTestingSwapChainD3D12 = ClassPtrCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    RTContext Ctx = {};
    InitializeRTContext(Ctx, pSwapChain, 0,
                        [](RTSubobjectsHelper& SubObj) {
                            SubObj.SetShaderCount(4, 1);
                            SubObj.SetDxilLibrary(0, HLSL::RayTracingTest3_RG, L"Main");
                            SubObj.SetDxilLibrary(1, HLSL::RayTracingTest3_RM, L"Miss");
                            SubObj.SetDxilLibrary(2, HLSL::RayTracingTest3_RCH, L"ClosestHitShader");
                            SubObj.SetDxilLibrary(3, HLSL::RayTracingTest3_RI, L"IntersectionShader");
                            SubObj.SetProceduralHitGroup(0, L"HitGroup", L"IntersectionShader", L"ClosestHitShader");
                        });

    // Create acceleration structures
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    BLASDesc          = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& BottomLevelInputs = BLASDesc.Inputs;
        D3D12_RAYTRACING_GEOMETRY_DESC                        Geometry          = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    TLASDesc          = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& TopLevelInputs    = TLASDesc.Inputs;
        D3D12_RAYTRACING_INSTANCE_DESC                        Instance          = {};

        const auto& Boxes = TestingConstants::ProceduralIntersection::Boxes;

        Geometry.Type                      = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
        Geometry.Flags                     = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        Geometry.AABBs.AABBs.StartAddress  = 0;
        Geometry.AABBs.AABBs.StrideInBytes = 0;
        Geometry.AABBs.AABBCount           = _countof(Boxes) / 2;

        BottomLevelInputs.pGeometryDescs = &Geometry;
        BottomLevelInputs.NumDescs       = 1;

        TopLevelInputs.NumDescs = 1;

        CreateBLAS(Ctx, BottomLevelInputs);
        CreateTLAS(Ctx, TopLevelInputs);
        CreateRTBuffers(Ctx, sizeof(Boxes), 0, 1, 1, 1);

        Instance.InstanceID                          = 0;
        Instance.InstanceContributionToHitGroupIndex = 0;
        Instance.InstanceMask                        = 0xFF;
        Instance.Flags                               = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        Instance.AccelerationStructure               = Ctx.BLAS.pAS->GetGPUVirtualAddress();
        Instance.Transform[0][0]                     = 1.0f;
        Instance.Transform[1][1]                     = 1.0f;
        Instance.Transform[2][2]                     = 1.0f;

        UpdateBuffer(Ctx, Ctx.pVertexBuffer, 0, Boxes, sizeof(Boxes));
        UpdateBuffer(Ctx, Ctx.pInstanceBuffer, 0, &Instance, sizeof(Instance));
        ASPrebuildBarriers(Ctx);

        Geometry.AABBs.AABBs.StartAddress = Ctx.pVertexBuffer->GetGPUVirtualAddress();

        BLASDesc.DestAccelerationStructureData    = Ctx.BLAS.pAS->GetGPUVirtualAddress();
        BLASDesc.ScratchAccelerationStructureData = Ctx.pScratchBuffer->GetGPUVirtualAddress();
        BLASDesc.SourceAccelerationStructureData  = 0;

        ASSERT_TRUE(Geometry.AABBs.AABBs.StartAddress != 0);
        ASSERT_TRUE(BLASDesc.DestAccelerationStructureData != 0);
        ASSERT_TRUE(BLASDesc.ScratchAccelerationStructureData != 0);

        Ctx.pCmdList->BuildRaytracingAccelerationStructure(&BLASDesc, 0, nullptr);

        UAVBarrier(Ctx, Ctx.pScratchBuffer);

        TopLevelInputs.InstanceDescs = Ctx.pInstanceBuffer->GetGPUVirtualAddress();

        TLASDesc.DestAccelerationStructureData    = Ctx.TLAS.pAS->GetGPUVirtualAddress();
        TLASDesc.ScratchAccelerationStructureData = Ctx.pScratchBuffer->GetGPUVirtualAddress();
        TLASDesc.SourceAccelerationStructureData  = 0;

        ASSERT_TRUE(TLASDesc.DestAccelerationStructureData != 0);
        ASSERT_TRUE(TLASDesc.ScratchAccelerationStructureData != 0);

        Ctx.pCmdList->BuildRaytracingAccelerationStructure(&TLASDesc, 0, nullptr);
    }

    Ctx.ClearRenderTarget(pTestingSwapChainD3D12);

    // Trace rays
    {
        pTestingSwapChainD3D12->TransitionRenderTarget(Ctx.pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        ID3D12DescriptorHeap* DescHeaps[] = {Ctx.pDescHeap};

        Ctx.pCmdList->SetPipelineState1(Ctx.pRayTracingSO);
        Ctx.pCmdList->SetComputeRootSignature(Ctx.pGlobalRootSignature);

        Ctx.pCmdList->SetDescriptorHeaps(_countof(DescHeaps), &DescHeaps[0]);
        Ctx.pCmdList->SetComputeRootDescriptorTable(0, DescHeaps[0]->GetGPUDescriptorHandleForHeapStart());

        D3D12_DISPATCH_RAYS_DESC Desc = {};

        Desc.Width  = SCDesc.Width;
        Desc.Height = SCDesc.Height;
        Desc.Depth  = 1;

        const UINT64 handleSize     = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        const UINT64 align          = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        const UINT64 RayGenOffset   = 0;
        const UINT64 RayMissOffset  = AlignUp(RayGenOffset + handleSize, align);
        const UINT64 HitGroupOffset = AlignUp(RayMissOffset + handleSize, align);

        Desc.RayGenerationShaderRecord.StartAddress = Ctx.pSBTBuffer->GetGPUVirtualAddress() + RayGenOffset;
        Desc.RayGenerationShaderRecord.SizeInBytes  = handleSize;
        Desc.MissShaderTable.StartAddress           = Ctx.pSBTBuffer->GetGPUVirtualAddress() + RayMissOffset;
        Desc.MissShaderTable.SizeInBytes            = handleSize;
        Desc.MissShaderTable.StrideInBytes          = handleSize;
        Desc.HitGroupTable.StartAddress             = Ctx.pSBTBuffer->GetGPUVirtualAddress() + HitGroupOffset;
        Desc.HitGroupTable.SizeInBytes              = handleSize;
        Desc.HitGroupTable.StrideInBytes            = handleSize;

        UpdateBuffer(Ctx, Ctx.pSBTBuffer, RayGenOffset, Ctx.pStateObjectProperties->GetShaderIdentifier(L"Main"), handleSize);
        UpdateBuffer(Ctx, Ctx.pSBTBuffer, RayMissOffset, Ctx.pStateObjectProperties->GetShaderIdentifier(L"Miss"), handleSize);
        UpdateBuffer(Ctx, Ctx.pSBTBuffer, HitGroupOffset, Ctx.pStateObjectProperties->GetShaderIdentifier(L"HitGroup"), handleSize);
        SBTBufferBarrier(Ctx);

        Ctx.pCmdList->DispatchRays(&Desc);
    }

    Ctx.pCmdList->Close();
    pEnv->ExecuteCommandList(Ctx.pCmdList);
}


void RayTracingMultiGeometryReferenceD3D12(ISwapChain* pSwapChain)
{
    static constexpr Uint32 InstanceCount = TestingConstants::MultiGeometry::InstanceCount;
    static constexpr Uint32 GeometryCount = 3;
    static constexpr Uint32 HitGroupCount = InstanceCount * GeometryCount;

    auto* pEnv                   = TestingEnvironmentD3D12::GetInstance();
    auto* pTestingSwapChainD3D12 = ClassPtrCast<TestingSwapChainD3D12>(pSwapChain);

    const auto& SCDesc = pSwapChain->GetDesc();

    RTContext Ctx = {};
    InitializeRTContext(
        Ctx, pSwapChain,
        TestingConstants::MultiGeometry::ShaderRecordSize,
        [](RTSubobjectsHelper& SubObj) {
            SubObj.SetShaderCount(4, 2);
            SubObj.SetDxilLibrary(0, HLSL::RayTracingTest4_RG, L"Main");
            SubObj.SetDxilLibrary(1, HLSL::RayTracingTest4_RM, L"Miss");
            SubObj.SetDxilLibrary(2, HLSL::RayTracingTest4_RCH1, L"ClosestHitShader1");
            SubObj.SetDxilLibrary(3, HLSL::RayTracingTest4_RCH2, L"ClosestHitShader2");
            SubObj.SetTriangleHitGroup(0, L"HitGroup1", L"ClosestHitShader1");
            SubObj.SetTriangleHitGroup(1, L"HitGroup2", L"ClosestHitShader2");
        },
        [](std::vector<D3D12_DESCRIPTOR_RANGE>& DescriptorRanges) {
            D3D12_DESCRIPTOR_RANGE Range = {};
            Range.RangeType              = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            Range.NumDescriptors         = 1;

            Range.BaseShaderRegister                = 1;
            Range.OffsetInDescriptorsFromTableStart = 2;
            DescriptorRanges.push_back(Range); // g_Vertices

            Range.BaseShaderRegister                = 4;
            Range.OffsetInDescriptorsFromTableStart = 3;
            DescriptorRanges.push_back(Range); // g_Primitives

            Range.BaseShaderRegister                = 2;
            Range.NumDescriptors                    = 2;
            Range.OffsetInDescriptorsFromTableStart = 4;
            DescriptorRanges.push_back(Range); // g_PerInstance[2]
        });

    const auto& PrimitiveOffsets = TestingConstants::MultiGeometry::PrimitiveOffsets;
    const auto& Primitives       = TestingConstants::MultiGeometry::Primitives;
    const auto& Vertices         = TestingConstants::MultiGeometry::Vertices;

    // create acceleration structurea
    {
        const auto& Indices = TestingConstants::MultiGeometry::Indices;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    BLASDesc          = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& BottomLevelInputs = BLASDesc.Inputs;
        D3D12_RAYTRACING_GEOMETRY_DESC                        Geometries[3]     = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    TLASDesc          = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& TopLevelInputs    = TLASDesc.Inputs;
        D3D12_RAYTRACING_INSTANCE_DESC                        Instances[2]      = {};

        static_assert(GeometryCount == _countof(Geometries), "size mismatch");
        static_assert(InstanceCount == _countof(Instances), "size mismatch");

        Geometries[0].Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        Geometries[0].Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        Geometries[0].Triangles.VertexBuffer.StartAddress  = 0;
        Geometries[0].Triangles.VertexBuffer.StrideInBytes = sizeof(Vertices[0]);
        Geometries[0].Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
        Geometries[0].Triangles.VertexCount                = _countof(Vertices);
        Geometries[0].Triangles.IndexCount                 = PrimitiveOffsets[1] * 3;
        Geometries[0].Triangles.IndexFormat                = DXGI_FORMAT_R32_UINT;
        Geometries[0].Triangles.IndexBuffer                = 0;
        Geometries[0].Triangles.Transform3x4               = 0;

        Geometries[1].Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        Geometries[1].Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        Geometries[1].Triangles.VertexBuffer.StartAddress  = 0;
        Geometries[1].Triangles.VertexBuffer.StrideInBytes = sizeof(Vertices[0]);
        Geometries[1].Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
        Geometries[1].Triangles.VertexCount                = _countof(Vertices);
        Geometries[1].Triangles.IndexCount                 = (PrimitiveOffsets[2] - PrimitiveOffsets[1]) * 3;
        Geometries[1].Triangles.IndexFormat                = DXGI_FORMAT_R32_UINT;
        Geometries[1].Triangles.IndexBuffer                = 0;
        Geometries[1].Triangles.Transform3x4               = 0;

        Geometries[2].Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        Geometries[2].Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        Geometries[2].Triangles.VertexBuffer.StartAddress  = 0;
        Geometries[2].Triangles.VertexBuffer.StrideInBytes = sizeof(Vertices[0]);
        Geometries[2].Triangles.VertexFormat               = DXGI_FORMAT_R32G32B32_FLOAT;
        Geometries[2].Triangles.VertexCount                = _countof(Vertices);
        Geometries[2].Triangles.IndexCount                 = (_countof(Primitives) - PrimitiveOffsets[2]) * 3;
        Geometries[2].Triangles.IndexFormat                = DXGI_FORMAT_R32_UINT;
        Geometries[2].Triangles.IndexBuffer                = 0;
        Geometries[2].Triangles.Transform3x4               = 0;

        BottomLevelInputs.pGeometryDescs = Geometries;
        BottomLevelInputs.NumDescs       = _countof(Geometries);

        TopLevelInputs.NumDescs = _countof(Instances);

        CreateBLAS(Ctx, BottomLevelInputs);
        CreateTLAS(Ctx, TopLevelInputs);
        CreateRTBuffers(Ctx, sizeof(Vertices), sizeof(Indices), InstanceCount, 1, HitGroupCount,
                        TestingConstants::MultiGeometry::ShaderRecordSize,
                        sizeof(PrimitiveOffsets) + sizeof(Primitives));

        Instances[0].InstanceID                          = 0;
        Instances[0].InstanceContributionToHitGroupIndex = 0;
        Instances[0].InstanceMask                        = 0xFF;
        Instances[0].Flags                               = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        Instances[0].AccelerationStructure               = Ctx.BLAS.pAS->GetGPUVirtualAddress();
        Instances[0].Transform[0][0]                     = 1.0f;
        Instances[0].Transform[1][1]                     = 1.0f;
        Instances[0].Transform[2][2]                     = 1.0f;

        Instances[1].InstanceID                          = 0;
        Instances[1].InstanceContributionToHitGroupIndex = HitGroupCount / 2;
        Instances[1].InstanceMask                        = 0xFF;
        Instances[1].Flags                               = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        Instances[1].AccelerationStructure               = Ctx.BLAS.pAS->GetGPUVirtualAddress();
        Instances[1].Transform[0][0]                     = 1.0f;
        Instances[1].Transform[1][1]                     = 1.0f;
        Instances[1].Transform[2][2]                     = 1.0f;
        Instances[1].Transform[0][3]                     = 0.1f;
        Instances[1].Transform[1][3]                     = 0.5f;
        Instances[1].Transform[2][3]                     = 0.0f;

        UpdateBuffer(Ctx, Ctx.pVertexBuffer, 0, Vertices, sizeof(Vertices));
        UpdateBuffer(Ctx, Ctx.pIndexBuffer, 0, Indices, sizeof(Indices));
        UpdateBuffer(Ctx, Ctx.pInstanceBuffer, 0, Instances, sizeof(Instances));
        ASPrebuildBarriers(Ctx);

        Geometries[0].Triangles.VertexBuffer.StartAddress = Ctx.pVertexBuffer->GetGPUVirtualAddress();
        Geometries[1].Triangles.VertexBuffer.StartAddress = Ctx.pVertexBuffer->GetGPUVirtualAddress();
        Geometries[2].Triangles.VertexBuffer.StartAddress = Ctx.pVertexBuffer->GetGPUVirtualAddress();

        Geometries[0].Triangles.IndexBuffer = Ctx.pIndexBuffer->GetGPUVirtualAddress() + PrimitiveOffsets[0] * sizeof(uint) * 3;
        Geometries[1].Triangles.IndexBuffer = Ctx.pIndexBuffer->GetGPUVirtualAddress() + PrimitiveOffsets[1] * sizeof(uint) * 3;
        Geometries[2].Triangles.IndexBuffer = Ctx.pIndexBuffer->GetGPUVirtualAddress() + PrimitiveOffsets[2] * sizeof(uint) * 3;

        BLASDesc.DestAccelerationStructureData    = Ctx.BLAS.pAS->GetGPUVirtualAddress();
        BLASDesc.ScratchAccelerationStructureData = Ctx.pScratchBuffer->GetGPUVirtualAddress();
        BLASDesc.SourceAccelerationStructureData  = 0;

        ASSERT_TRUE(BLASDesc.DestAccelerationStructureData != 0);
        ASSERT_TRUE(BLASDesc.ScratchAccelerationStructureData != 0);

        Ctx.pCmdList->BuildRaytracingAccelerationStructure(&BLASDesc, 0, nullptr);

        UAVBarrier(Ctx, Ctx.pScratchBuffer);

        TopLevelInputs.InstanceDescs = Ctx.pInstanceBuffer->GetGPUVirtualAddress();

        TLASDesc.DestAccelerationStructureData    = Ctx.TLAS.pAS->GetGPUVirtualAddress();
        TLASDesc.ScratchAccelerationStructureData = Ctx.pScratchBuffer->GetGPUVirtualAddress();
        TLASDesc.SourceAccelerationStructureData  = 0;

        ASSERT_TRUE(TLASDesc.DestAccelerationStructureData != 0);
        ASSERT_TRUE(TLASDesc.ScratchAccelerationStructureData != 0);

        Ctx.pCmdList->BuildRaytracingAccelerationStructure(&TLASDesc, 0, nullptr);
    }

    // update descriptors
    CComPtr<ID3D12Resource> pPerInstanceBuffer;
    CComPtr<ID3D12Resource> pPrimitiveBuffer;
    {
        D3D12_RESOURCE_DESC BuffDesc = {};
        BuffDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
        BuffDesc.Alignment           = 0;
        BuffDesc.Width               = sizeof(PrimitiveOffsets);
        BuffDesc.Height              = 1;
        BuffDesc.DepthOrArraySize    = 1;
        BuffDesc.MipLevels           = 1;
        BuffDesc.Format              = DXGI_FORMAT_UNKNOWN;
        BuffDesc.SampleDesc.Count    = 1;
        BuffDesc.SampleDesc.Quality  = 0;
        BuffDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        BuffDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES HeapProps;
        HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProps.CreationNodeMask     = 1;
        HeapProps.VisibleNodeMask      = 1;

        auto hr = Ctx.pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                       &BuffDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                       IID_PPV_ARGS(&pPerInstanceBuffer));
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create per instance buffer";

        BuffDesc.Width = sizeof(Primitives);

        hr = Ctx.pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
                                                  &BuffDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                  IID_PPV_ARGS(&pPrimitiveBuffer));
        ASSERT_HRESULT_SUCCEEDED(hr) << "Failed to create per instance buffer";

        UpdateBuffer(Ctx, pPrimitiveBuffer, 0, Primitives, sizeof(Primitives));
        UpdateBuffer(Ctx, pPerInstanceBuffer, 0, PrimitiveOffsets, sizeof(PrimitiveOffsets));

        // buffer barrier
        {
            D3D12_RESOURCE_BARRIER Barrier = {};
            Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            Barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            Barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            Barrier.Transition.pResource   = pPerInstanceBuffer;
            Ctx.pCmdList->ResourceBarrier(1, &Barrier);

            Barrier.Transition.pResource = pPrimitiveBuffer;
            Ctx.pCmdList->ResourceBarrier(1, &Barrier);
        }

        D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
        D3D12_CPU_DESCRIPTOR_HANDLE     SRVHandle;

        SRVDesc.Format                     = DXGI_FORMAT_UNKNOWN;
        SRVDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        SRVDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        SRVDesc.Buffer.NumElements         = _countof(Vertices);
        SRVDesc.Buffer.StructureByteStride = sizeof(Vertices[0]);

        ASSERT_LT(Ctx.DescHeapCount, Ctx.DescriptorHeapSize);
        ASSERT_TRUE(Ctx.DescHeapCount == 2);
        SRVHandle = Ctx.pDescHeap->GetCPUDescriptorHandleForHeapStart();
        SRVHandle.ptr += Ctx.DescHandleSize * Ctx.DescHeapCount++;
        Ctx.pDevice->CreateShaderResourceView(Ctx.pVertexBuffer, &SRVDesc, SRVHandle); // g_Vertices

        SRVDesc.Buffer.NumElements         = _countof(Primitives);
        SRVDesc.Buffer.StructureByteStride = sizeof(Primitives[0]);
        ASSERT_LT(Ctx.DescHeapCount, Ctx.DescriptorHeapSize);
        ASSERT_TRUE(Ctx.DescHeapCount == 3);
        SRVHandle = Ctx.pDescHeap->GetCPUDescriptorHandleForHeapStart();
        SRVHandle.ptr += Ctx.DescHandleSize * Ctx.DescHeapCount++;
        Ctx.pDevice->CreateShaderResourceView(pPrimitiveBuffer, &SRVDesc, SRVHandle); // g_Primitives

        SRVDesc.Buffer.NumElements         = _countof(PrimitiveOffsets);
        SRVDesc.Buffer.StructureByteStride = sizeof(PrimitiveOffsets[0]);
        ASSERT_LT(Ctx.DescHeapCount, Ctx.DescriptorHeapSize);
        ASSERT_TRUE(Ctx.DescHeapCount == 4);
        SRVHandle = Ctx.pDescHeap->GetCPUDescriptorHandleForHeapStart();
        SRVHandle.ptr += Ctx.DescHandleSize * Ctx.DescHeapCount++;
        Ctx.pDevice->CreateShaderResourceView(pPerInstanceBuffer, &SRVDesc, SRVHandle); // g_PerInstance[0]

        ASSERT_TRUE(Ctx.DescHeapCount == 5);
        SRVHandle = Ctx.pDescHeap->GetCPUDescriptorHandleForHeapStart();
        SRVHandle.ptr += Ctx.DescHandleSize * Ctx.DescHeapCount++;
        Ctx.pDevice->CreateShaderResourceView(pPerInstanceBuffer, &SRVDesc, SRVHandle); // g_PerInstance[1]
    }

    Ctx.ClearRenderTarget(pTestingSwapChainD3D12);

    // trace rays
    {
        pTestingSwapChainD3D12->TransitionRenderTarget(Ctx.pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        ID3D12DescriptorHeap* DescHeaps[] = {Ctx.pDescHeap};

        Ctx.pCmdList->SetPipelineState1(Ctx.pRayTracingSO);
        Ctx.pCmdList->SetComputeRootSignature(Ctx.pGlobalRootSignature);

        Ctx.pCmdList->SetDescriptorHeaps(_countof(DescHeaps), &DescHeaps[0]);
        Ctx.pCmdList->SetComputeRootDescriptorTable(0, DescHeaps[0]->GetGPUDescriptorHandleForHeapStart());

        D3D12_DISPATCH_RAYS_DESC Desc = {};

        Desc.Width  = SCDesc.Width;
        Desc.Height = SCDesc.Height;
        Desc.Depth  = 1;

        const UINT64 handleSize       = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        const UINT64 align            = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        const UINT64 ShaderRecordSize = handleSize + TestingConstants::MultiGeometry::ShaderRecordSize;
        const UINT64 RayGenOffset     = 0;
        const UINT64 RayMissOffset    = AlignUp(RayGenOffset + handleSize, align);
        const UINT64 HitGroupOffset   = AlignUp(RayMissOffset + handleSize, align);
        const auto&  Weights          = TestingConstants::MultiGeometry::Weights;

        Desc.RayGenerationShaderRecord.StartAddress = Ctx.pSBTBuffer->GetGPUVirtualAddress() + RayGenOffset;
        Desc.RayGenerationShaderRecord.SizeInBytes  = ShaderRecordSize;
        Desc.MissShaderTable.StartAddress           = Ctx.pSBTBuffer->GetGPUVirtualAddress() + RayMissOffset;
        Desc.MissShaderTable.SizeInBytes            = ShaderRecordSize;
        Desc.MissShaderTable.StrideInBytes          = ShaderRecordSize;
        Desc.HitGroupTable.StartAddress             = Ctx.pSBTBuffer->GetGPUVirtualAddress() + HitGroupOffset;
        Desc.HitGroupTable.SizeInBytes              = ShaderRecordSize * HitGroupCount;
        Desc.HitGroupTable.StrideInBytes            = ShaderRecordSize;

        UpdateBuffer(Ctx, Ctx.pSBTBuffer, RayGenOffset, Ctx.pStateObjectProperties->GetShaderIdentifier(L"Main"), handleSize);
        UpdateBuffer(Ctx, Ctx.pSBTBuffer, RayMissOffset, Ctx.pStateObjectProperties->GetShaderIdentifier(L"Miss"), handleSize);

        const auto SetHitGroup = [&](Uint32 Index, const wchar_t* GroupName, const void* ShaderRecord) {
            VERIFY_EXPR(Index < HitGroupCount);
            UINT64 Offset = HitGroupOffset + Index * ShaderRecordSize;
            UpdateBuffer(Ctx, Ctx.pSBTBuffer, Offset, Ctx.pStateObjectProperties->GetShaderIdentifier(GroupName), handleSize);
            UpdateBuffer(Ctx, Ctx.pSBTBuffer, Offset + handleSize, ShaderRecord, sizeof(Weights[0]));
        };
        // instance 1
        SetHitGroup(0, L"HitGroup1", &Weights[0]); // geometry 1
        SetHitGroup(1, L"HitGroup1", &Weights[1]); // geometry 2
        SetHitGroup(2, L"HitGroup1", &Weights[2]); // geometry 3
        // instance 2
        SetHitGroup(3, L"HitGroup2", &Weights[3]); // geometry 1
        SetHitGroup(4, L"HitGroup2", &Weights[4]); // geometry 2
        SetHitGroup(5, L"HitGroup2", &Weights[5]); // geometry 3

        SBTBufferBarrier(Ctx);

        Ctx.pCmdList->DispatchRays(&Desc);
    }

    Ctx.pCmdList->Close();
    pEnv->ExecuteCommandList(Ctx.pCmdList);
}

} // namespace Testing

} // namespace Diligent
