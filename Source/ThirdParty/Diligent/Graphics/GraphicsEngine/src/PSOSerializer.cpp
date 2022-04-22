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

#include "PSOSerializer.hpp"

namespace Diligent
{

template <SerializerMode Mode>
void SerializeImmutableSampler(
    Serializer<Mode>&                                                    Ser,
    typename Serializer<Mode>::template ConstQual<ImmutableSamplerDesc>& SampDesc)
{
    Ser(SampDesc.SamplerOrTextureName,
        SampDesc.ShaderStages,
        SampDesc.Desc.Name,
        SampDesc.Desc.MinFilter,
        SampDesc.Desc.MagFilter,
        SampDesc.Desc.MipFilter,
        SampDesc.Desc.AddressU,
        SampDesc.Desc.AddressV,
        SampDesc.Desc.AddressW,
        SampDesc.Desc.Flags,
        SampDesc.Desc.MipLODBias,
        SampDesc.Desc.MaxAnisotropy,
        SampDesc.Desc.ComparisonFunc,
        SampDesc.Desc.BorderColor,
        SampDesc.Desc.MinLOD,
        SampDesc.Desc.MaxLOD);

    ASSERT_SIZEOF64(ImmutableSamplerDesc, 72, "Did you add a new member to ImmutableSamplerDesc? Please add serialization here.");
}

template <SerializerMode Mode>
void PRSSerializer<Mode>::SerializeDesc(
    Serializer<Mode>&                         Ser,
    ConstQual<PipelineResourceSignatureDesc>& Desc,
    DynamicLinearAllocator*                   Allocator)
{
    // Serialize PipelineResourceSignatureDesc
    Ser(Desc.BindingIndex,
        Desc.UseCombinedTextureSamplers,
        Desc.CombinedSamplerSuffix);
    // skip Name
    // skip SRBAllocationGranularity

    Ser.SerializeArray(Allocator, Desc.Resources, Desc.NumResources,
                       [](Serializer<Mode>&                Ser,
                          ConstQual<PipelineResourceDesc>& ResDesc) //
                       {
                           Ser(ResDesc.Name,
                               ResDesc.ShaderStages,
                               ResDesc.ArraySize,
                               ResDesc.ResourceType,
                               ResDesc.VarType,
                               ResDesc.Flags);
                       });

    Ser.SerializeArray(Allocator, Desc.ImmutableSamplers, Desc.NumImmutableSamplers, SerializeImmutableSampler<Mode>);

    ASSERT_SIZEOF64(PipelineResourceSignatureDesc, 56, "Did you add a new member to PipelineResourceSignatureDesc? Please add serialization here.");
    ASSERT_SIZEOF64(PipelineResourceDesc, 24, "Did you add a new member to PipelineResourceDesc? Please add serialization here.");
}

template <SerializerMode Mode>
void PRSSerializer<Mode>::SerializeInternalData(Serializer<Mode>&                                 Ser,
                                                ConstQual<PipelineResourceSignatureInternalData>& InternalData,
                                                DynamicLinearAllocator*                           Allocator)
{
    Ser(InternalData.ShaderStages,
        InternalData.StaticResShaderStages,
        InternalData.PipelineType,
        InternalData.StaticResStageIndex);
    ASSERT_SIZEOF(PipelineResourceSignatureInternalData, 16, "Did you add a new member to PipelineResourceSignatureInternalData? Please add serialization here.");
}

template <SerializerMode Mode>
void PSOSerializer<Mode>::SerializeCreateInfo(
    Serializer<Mode>&                   Ser,
    ConstQual<PipelineStateCreateInfo>& CreateInfo,
    ConstQual<TPRSNames>&               PRSNames,
    DynamicLinearAllocator*             Allocator)
{
    // Serialize PipelineStateCreateInfo
    //   Serialize PipelineStateDesc
    Ser(CreateInfo.PSODesc.PipelineType);
    Ser(CreateInfo.ResourceSignaturesCount,
        CreateInfo.Flags);
    // skip SRBAllocationGranularity
    // skip ImmediateContextMask
    // skip pPSOCache

    auto& ResourceLayout = CreateInfo.PSODesc.ResourceLayout;
    Ser(ResourceLayout.DefaultVariableType, ResourceLayout.DefaultVariableMergeStages);
    Ser.SerializeArray(Allocator, ResourceLayout.Variables, ResourceLayout.NumVariables,
                       [](Serializer<Mode>&                      Ser,
                          ConstQual<ShaderResourceVariableDesc>& VarDesc) //
                       {
                           Ser(VarDesc.Name,
                               VarDesc.ShaderStages,
                               VarDesc.Type,
                               VarDesc.Flags);
                       });

    Ser.SerializeArray(Allocator, ResourceLayout.ImmutableSamplers, ResourceLayout.NumImmutableSamplers, SerializeImmutableSampler<Mode>);

    // Instead of ppResourceSignatures
    for (Uint32 i = 0; i < std::max(CreateInfo.ResourceSignaturesCount, 1u); ++i)
    {
        Ser(PRSNames[i]);
    }

    ASSERT_SIZEOF64(ShaderResourceVariableDesc, 24, "Did you add a new member to ShaderResourceVariableDesc? Please add serialization here.");
    ASSERT_SIZEOF64(PipelineStateCreateInfo, 96, "Did you add a new member to PipelineStateCreateInfo? Please add serialization here.");
}

template <SerializerMode Mode>
void PSOSerializer<Mode>::SerializeCreateInfo(
    Serializer<Mode>&                           Ser,
    ConstQual<GraphicsPipelineStateCreateInfo>& CreateInfo,
    ConstQual<TPRSNames>&                       PRSNames,
    DynamicLinearAllocator*                     Allocator,
    ConstQual<const char*>&                     RenderPassName)
{
    SerializeCreateInfo(Ser, static_cast<ConstQual<PipelineStateCreateInfo>&>(CreateInfo), PRSNames, Allocator);

    // Serialize GraphicsPipelineDesc
    Ser(CreateInfo.GraphicsPipeline.BlendDesc,
        CreateInfo.GraphicsPipeline.SampleMask,
        CreateInfo.GraphicsPipeline.RasterizerDesc,
        CreateInfo.GraphicsPipeline.DepthStencilDesc);

    // Serialize InputLayoutDesc
    {
        auto& InputLayout = CreateInfo.GraphicsPipeline.InputLayout;
        Ser.SerializeArray(Allocator, InputLayout.LayoutElements, InputLayout.NumElements,
                           [](Serializer<Mode>&         Ser,
                              ConstQual<LayoutElement>& Elem) //
                           {
                               Ser(Elem.HLSLSemantic,
                                   Elem.InputIndex,
                                   Elem.BufferSlot,
                                   Elem.NumComponents,
                                   Elem.ValueType,
                                   Elem.IsNormalized,
                                   Elem.RelativeOffset,
                                   Elem.Stride,
                                   Elem.Frequency,
                                   Elem.InstanceDataStepRate);
                           });
    }

    Ser(CreateInfo.GraphicsPipeline.PrimitiveTopology,
        CreateInfo.GraphicsPipeline.NumViewports,
        CreateInfo.GraphicsPipeline.NumRenderTargets,
        CreateInfo.GraphicsPipeline.SubpassIndex,
        CreateInfo.GraphicsPipeline.ShadingRateFlags,
        CreateInfo.GraphicsPipeline.RTVFormats,
        CreateInfo.GraphicsPipeline.DSVFormat,
        CreateInfo.GraphicsPipeline.SmplDesc,
        RenderPassName); // for CreateInfo.GraphicsPipeline.pRenderPass

    // Skip NodeMask

    ASSERT_SIZEOF64(GraphicsPipelineStateCreateInfo, 344, "Did you add a new member to GraphicsPipelineStateCreateInfo? Please add serialization here.");
    ASSERT_SIZEOF64(LayoutElement, 40, "Did you add a new member to LayoutElement? Please add serialization here.");
}

template <SerializerMode Mode>
void PSOSerializer<Mode>::SerializeCreateInfo(
    Serializer<Mode>&                          Ser,
    ConstQual<ComputePipelineStateCreateInfo>& CreateInfo,
    ConstQual<TPRSNames>&                      PRSNames,
    DynamicLinearAllocator*                    Allocator)
{
    SerializeCreateInfo(Ser, static_cast<ConstQual<PipelineStateCreateInfo>&>(CreateInfo), PRSNames, Allocator);

    ASSERT_SIZEOF64(ComputePipelineStateCreateInfo, 104, "Did you add a new member to ComputePipelineStateCreateInfo? Please add serialization here.");
}

template <SerializerMode Mode>
void PSOSerializer<Mode>::SerializeCreateInfo(
    Serializer<Mode>&                       Ser,
    ConstQual<TilePipelineStateCreateInfo>& CreateInfo,
    ConstQual<TPRSNames>&                   PRSNames,
    DynamicLinearAllocator*                 Allocator)
{
    SerializeCreateInfo(Ser, static_cast<ConstQual<PipelineStateCreateInfo>&>(CreateInfo), PRSNames, Allocator);

    // Serialize TilePipelineDesc
    Ser(CreateInfo.TilePipeline.NumRenderTargets,
        CreateInfo.TilePipeline.SampleCount,
        CreateInfo.TilePipeline.RTVFormats);

    ASSERT_SIZEOF64(TilePipelineStateCreateInfo, 128, "Did you add a new member to TilePipelineStateCreateInfo? Please add serialization here.");
}

template <SerializerMode Mode>
void PSOSerializer<Mode>::SerializeCreateInfo(
    Serializer<Mode>&                                         Ser,
    ConstQual<RayTracingPipelineStateCreateInfo>&             CreateInfo,
    ConstQual<TPRSNames>&                                     PRSNames,
    DynamicLinearAllocator*                                   Allocator,
    const std::function<void(Uint32&, ConstQual<IShader*>&)>& ShaderToIndex)
{
    const bool IsReading = (Allocator != nullptr);
    const bool IsWriting = !IsReading;

    SerializeCreateInfo(Ser, static_cast<ConstQual<PipelineStateCreateInfo>&>(CreateInfo), PRSNames, Allocator);

    // Serialize RayTracingPipelineDesc
    Ser(CreateInfo.RayTracingPipeline.ShaderRecordSize,
        CreateInfo.RayTracingPipeline.MaxRecursionDepth);

    // Serialize RayTracingPipelineStateCreateInfo
    Ser(CreateInfo.pShaderRecordName,
        CreateInfo.MaxAttributeSize,
        CreateInfo.MaxPayloadSize);

    // Serialize RayTracingGeneralShaderGroup
    Ser.SerializeArray(Allocator, CreateInfo.pGeneralShaders, CreateInfo.GeneralShaderCount,
                       [&](Serializer<Mode>&                        Ser,
                           ConstQual<RayTracingGeneralShaderGroup>& Group) //
                       {
                           Uint32 ShaderIndex = ~0u;
                           if (IsWriting)
                           {
                               ShaderToIndex(ShaderIndex, Group.pShader);
                           }
                           Ser(Group.Name, ShaderIndex);
                           VERIFY_EXPR(ShaderIndex != ~0u);
                           if (IsReading)
                           {
                               ShaderToIndex(ShaderIndex, Group.pShader);
                           }
                       });

    // Serialize RayTracingTriangleHitShaderGroup
    Ser.SerializeArray(Allocator, CreateInfo.pTriangleHitShaders, CreateInfo.TriangleHitShaderCount,
                       [&](Serializer<Mode>&                            Ser,
                           ConstQual<RayTracingTriangleHitShaderGroup>& Group) //
                       {
                           Uint32 ClosestHitShaderIndex = ~0u;
                           Uint32 AnyHitShaderIndex     = ~0u;
                           if (IsWriting)
                           {
                               ShaderToIndex(ClosestHitShaderIndex, Group.pClosestHitShader);
                               ShaderToIndex(AnyHitShaderIndex, Group.pAnyHitShader);
                           }
                           Ser(Group.Name, ClosestHitShaderIndex, AnyHitShaderIndex);
                           VERIFY_EXPR(ClosestHitShaderIndex != ~0u);
                           if (IsReading)
                           {
                               ShaderToIndex(ClosestHitShaderIndex, Group.pClosestHitShader);
                               ShaderToIndex(AnyHitShaderIndex, Group.pAnyHitShader);
                           }
                       });

    // Serialize RayTracingProceduralHitShaderGroup
    Ser.SerializeArray(Allocator, CreateInfo.pProceduralHitShaders, CreateInfo.ProceduralHitShaderCount,
                       [&](Serializer<Mode>&                              Ser,
                           ConstQual<RayTracingProceduralHitShaderGroup>& Group) //
                       {
                           Uint32 IntersectionShaderIndex = ~0u;
                           Uint32 ClosestHitShaderIndex   = ~0u;
                           Uint32 AnyHitShaderIndex       = ~0u;
                           if (IsWriting)
                           {
                               ShaderToIndex(IntersectionShaderIndex, Group.pIntersectionShader);
                               ShaderToIndex(ClosestHitShaderIndex, Group.pClosestHitShader);
                               ShaderToIndex(AnyHitShaderIndex, Group.pAnyHitShader);
                           }
                           Ser(Group.Name, IntersectionShaderIndex, ClosestHitShaderIndex, AnyHitShaderIndex);
                           VERIFY_EXPR(IntersectionShaderIndex != ~0u);
                           if (IsReading)
                           {
                               ShaderToIndex(IntersectionShaderIndex, Group.pIntersectionShader);
                               ShaderToIndex(ClosestHitShaderIndex, Group.pClosestHitShader);
                               ShaderToIndex(AnyHitShaderIndex, Group.pAnyHitShader);
                           }
                       });

    ASSERT_SIZEOF64(RayTracingPipelineStateCreateInfo, 168, "Did you add a new member to RayTracingPipelineStateCreateInfo? Please add serialization here.");
    ASSERT_SIZEOF64(RayTracingGeneralShaderGroup, 16, "Did you add a new member to RayTracingGeneralShaderGroup? Please add serialization here.");
    ASSERT_SIZEOF64(RayTracingTriangleHitShaderGroup, 24, "Did you add a new member to RayTracingTriangleHitShaderGroup? Please add serialization here.");
    ASSERT_SIZEOF64(RayTracingProceduralHitShaderGroup, 32, "Did you add a new member to RayTracingProceduralHitShaderGroup? Please add serialization here.");
}

template <SerializerMode Mode>
void RPSerializer<Mode>::SerializeDesc(
    Serializer<Mode>&          Ser,
    ConstQual<RenderPassDesc>& RPDesc,
    DynamicLinearAllocator*    Allocator)
{
    Ser.SerializeArray(Allocator, RPDesc.pAttachments, RPDesc.AttachmentCount,
                       [](Serializer<Mode>&                    Ser,
                          ConstQual<RenderPassAttachmentDesc>& Attachment) //
                       {
                           Ser(Attachment.Format,
                               Attachment.SampleCount,
                               Attachment.LoadOp,
                               Attachment.StoreOp,
                               Attachment.StencilLoadOp,
                               Attachment.StencilStoreOp,
                               Attachment.InitialState,
                               Attachment.FinalState);
                       });

    Ser.SerializeArray(Allocator, RPDesc.pSubpasses, RPDesc.SubpassCount,
                       [&Allocator](Serializer<Mode>&       Ser,
                                    ConstQual<SubpassDesc>& Subpass) //
                       {
                           auto SerializeAttachmentRef = [](Serializer<Mode>&               Ser,
                                                            ConstQual<AttachmentReference>& AttachRef) //
                           {
                               Ser(AttachRef.AttachmentIndex,
                                   AttachRef.State);
                           };
                           Ser.SerializeArray(Allocator, Subpass.pInputAttachments, Subpass.InputAttachmentCount, SerializeAttachmentRef);
                           Ser.SerializeArray(Allocator, Subpass.pRenderTargetAttachments, Subpass.RenderTargetAttachmentCount, SerializeAttachmentRef);

                           // Note: in Read mode, ResolveAttachCount, DepthStencilAttachCount, and ShadingRateAttachCount will be overwritten
                           Uint32 ResolveAttachCount = Subpass.pResolveAttachments != nullptr ? Subpass.RenderTargetAttachmentCount : 0;
                           Ser.SerializeArray(Allocator, Subpass.pResolveAttachments, ResolveAttachCount, SerializeAttachmentRef);

                           Uint32 DepthStencilAttachCount = Subpass.pDepthStencilAttachment != nullptr ? 1 : 0;
                           Ser.SerializeArray(Allocator, Subpass.pDepthStencilAttachment, DepthStencilAttachCount, SerializeAttachmentRef);

                           Ser.SerializeArray(Allocator, Subpass.pPreserveAttachments, Subpass.PreserveAttachmentCount,
                                              [](Serializer<Mode>&  Ser,
                                                 ConstQual<Uint32>& Attach) //
                                              {
                                                  Ser(Attach);
                                              });

                           Uint32 ShadingRateAttachCount = Subpass.pShadingRateAttachment != nullptr ? 1 : 0;
                           Ser.SerializeArray(Allocator, Subpass.pShadingRateAttachment, ShadingRateAttachCount,
                                              [](Serializer<Mode>&                 Ser,
                                                 ConstQual<ShadingRateAttachment>& SRAttachment) //
                                              {
                                                  Ser(SRAttachment.Attachment.AttachmentIndex,
                                                      SRAttachment.Attachment.State,
                                                      SRAttachment.TileSize);
                                              });
                       });

    Ser.SerializeArray(Allocator, RPDesc.pDependencies, RPDesc.DependencyCount,
                       [](Serializer<Mode>&                 Ser,
                          ConstQual<SubpassDependencyDesc>& Dep) //
                       {
                           Ser(Dep.SrcSubpass,
                               Dep.DstSubpass,
                               Dep.SrcStageMask,
                               Dep.DstStageMask,
                               Dep.SrcAccessMask,
                               Dep.DstAccessMask);
                       });

    ASSERT_SIZEOF64(RenderPassDesc, 56, "Did you add a new member to RenderPassDesc? Please add serialization here.");
    ASSERT_SIZEOF64(SubpassDesc, 72, "Did you add a new member to SubpassDesc? Please add serialization here.");
    ASSERT_SIZEOF(RenderPassAttachmentDesc, 16, "Did you add a new member to RenderPassAttachmentDesc? Please add serialization here.");
    ASSERT_SIZEOF(SubpassDependencyDesc, 24, "Did you add a new member to SubpassDependencyDesc? Please add serialization here.");
    ASSERT_SIZEOF(ShadingRateAttachment, 16, "Did you add a new member to ShadingRateAttachment? Please add serialization here.");
    ASSERT_SIZEOF(AttachmentReference, 8, "Did you add a new member to AttachmentReference? Please add serialization here.");
}

template <SerializerMode Mode>
void PSOSerializer<Mode>::SerializeShaderIndices(
    Serializer<Mode>&            Ser,
    ConstQual<ShaderIndexArray>& Shaders,
    DynamicLinearAllocator*      Allocator)
{
    Ser.SerializeArrayRaw(Allocator, Shaders.pIndices, Shaders.Count);
}

template <SerializerMode Mode>
void PSOSerializer<Mode>::SerializeAuxData(Serializer<Mode>&                Ser,
                                           ConstQual<SerializedPSOAuxData>& AuxData,
                                           DynamicLinearAllocator*          Allocator)
{
    Ser(AuxData.NoShaderReflection);
    ASSERT_SIZEOF(SerializedPSOAuxData, 1, "Did you add a new member to SerializedPSOAuxData? Please add serialization here.");
}

template <SerializerMode Mode>
void ShaderSerializer<Mode>::SerializeBytecodeOrSource(Serializer<Mode>&            Ser,
                                                       ConstQual<ShaderCreateInfo>& CI)
{
    VERIFY((CI.Source != nullptr) ^ (CI.ByteCode != nullptr), "Only one of Source or Bytecode must not be null");
    const Uint8 UseBytecode = CI.ByteCode != nullptr ? 1 : 0;
    Ser(UseBytecode);
    if (UseBytecode)
        Ser.SerializeBytes(CI.ByteCode, CI.ByteCodeSize);
    else
        Ser.SerializeBytes(CI.Source, CI.SourceLength);
}

template <>
void ShaderSerializer<SerializerMode::Read>::SerializeBytecodeOrSource(Serializer<SerializerMode::Read>& Ser,
                                                                       ConstQual<ShaderCreateInfo>&      CI)
{
    Uint8 UseBytecode = 0;
    Ser(UseBytecode);
    if (UseBytecode)
    {
        Ser.SerializeBytes(CI.ByteCode, CI.ByteCodeSize);
    }
    else
    {
        const void* pSource = nullptr;
        Ser.SerializeBytes(pSource, CI.SourceLength);
        CI.Source = reinterpret_cast<const char*>(pSource);
    }
}

template <SerializerMode Mode>
void ShaderSerializer<Mode>::SerializeCI(Serializer<Mode>&            Ser,
                                         ConstQual<ShaderCreateInfo>& CI)
{
    Ser(CI.Desc.Name,
        CI.Desc.ShaderType,
        CI.EntryPoint,
        CI.SourceLanguage,
        CI.ShaderCompiler,
        CI.UseCombinedTextureSamplers,
        CI.CombinedSamplerSuffix);

    SerializeBytecodeOrSource(Ser, CI);
}

template struct PSOSerializer<SerializerMode::Read>;
template struct PSOSerializer<SerializerMode::Write>;
template struct PSOSerializer<SerializerMode::Measure>;

template struct PRSSerializer<SerializerMode::Read>;
template struct PRSSerializer<SerializerMode::Write>;
template struct PRSSerializer<SerializerMode::Measure>;

template struct RPSerializer<SerializerMode::Read>;
template struct RPSerializer<SerializerMode::Write>;
template struct RPSerializer<SerializerMode::Measure>;

template struct ShaderSerializer<SerializerMode::Read>;
template struct ShaderSerializer<SerializerMode::Write>;
template struct ShaderSerializer<SerializerMode::Measure>;

} // namespace Diligent
