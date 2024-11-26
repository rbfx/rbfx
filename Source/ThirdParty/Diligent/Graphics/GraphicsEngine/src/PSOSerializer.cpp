/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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
bool SerializeImmutableSampler(
    Serializer<Mode>&                                                    Ser,
    typename Serializer<Mode>::template ConstQual<ImmutableSamplerDesc>& SampDesc)
{
    return Ser(SampDesc.SamplerOrTextureName,
               SampDesc.ShaderStages,
               SampDesc.Desc.Name,
               SampDesc.Desc.MinFilter,
               SampDesc.Desc.MagFilter,
               SampDesc.Desc.MipFilter,
               SampDesc.Desc.AddressU,
               SampDesc.Desc.AddressV,
               SampDesc.Desc.AddressW,
               SampDesc.Desc.Flags,
               SampDesc.Desc.UnnormalizedCoords,
               SampDesc.Desc.MipLODBias,
               SampDesc.Desc.MaxAnisotropy,
               SampDesc.Desc.ComparisonFunc,
               SampDesc.Desc.BorderColor,
               SampDesc.Desc.MinLOD,
               SampDesc.Desc.MaxLOD);

    ASSERT_SIZEOF64(ImmutableSamplerDesc, 72, "Did you add a new member to ImmutableSamplerDesc? Please add serialization here.");
}

template <SerializerMode Mode>
bool PRSSerializer<Mode>::SerializeDesc(
    Serializer<Mode>&                         Ser,
    ConstQual<PipelineResourceSignatureDesc>& Desc,
    DynamicLinearAllocator*                   Allocator)
{
    // Serialize PipelineResourceSignatureDesc
    if (!Ser(Desc.BindingIndex,
             Desc.UseCombinedTextureSamplers,
             Desc.CombinedSamplerSuffix))
        return false;
    // skip Name
    // skip SRBAllocationGranularity

    if (!Ser.SerializeArray(Allocator, Desc.Resources, Desc.NumResources,
                            [](Serializer<Mode>&                Ser,
                               ConstQual<PipelineResourceDesc>& ResDesc) //
                            {
                                return Ser(ResDesc.Name,
                                           ResDesc.ShaderStages,
                                           ResDesc.ArraySize,
                                           ResDesc.ResourceType,
                                           ResDesc.VarType,
                                           ResDesc.Flags,
                                           ResDesc.WebGPUAttribs.BindingType,
                                           ResDesc.WebGPUAttribs.TextureViewDim,
                                           ResDesc.WebGPUAttribs.UAVTextureFormat);
                            }))
        return false;

    return Ser.SerializeArray(Allocator, Desc.ImmutableSamplers, Desc.NumImmutableSamplers, SerializeImmutableSampler<Mode>);

    ASSERT_SIZEOF64(PipelineResourceSignatureDesc, 56, "Did you add a new member to PipelineResourceSignatureDesc? Please add serialization here.");
    ASSERT_SIZEOF64(PipelineResourceDesc, 24, "Did you add a new member to PipelineResourceDesc? Please add serialization here.");
}

template <SerializerMode Mode>
bool PSOSerializer<Mode>::SerializeCreateInfo(
    Serializer<Mode>&                   Ser,
    ConstQual<PipelineStateCreateInfo>& CreateInfo,
    ConstQual<TPRSNames>&               PRSNames,
    DynamicLinearAllocator*             Allocator)
{
    // Serialize PipelineStateCreateInfo
    // Serialize PipelineStateDesc
    if (!Ser(CreateInfo.PSODesc.PipelineType))
        return false;

    if (!Ser(CreateInfo.ResourceSignaturesCount,
             CreateInfo.Flags))
        return false;
    // skip SRBAllocationGranularity
    // skip ImmediateContextMask
    // skip pPSOCache

    auto& ResourceLayout = CreateInfo.PSODesc.ResourceLayout;
    if (!Ser(ResourceLayout.DefaultVariableType, ResourceLayout.DefaultVariableMergeStages))
        return false;

    if (!Ser.SerializeArray(Allocator, ResourceLayout.Variables, ResourceLayout.NumVariables,
                            [](Serializer<Mode>&                      Ser,
                               ConstQual<ShaderResourceVariableDesc>& VarDesc) //
                            {
                                return Ser(VarDesc.Name,
                                           VarDesc.ShaderStages,
                                           VarDesc.Type,
                                           VarDesc.Flags);
                            }))
        return false;

    if (!Ser.SerializeArray(Allocator, ResourceLayout.ImmutableSamplers, ResourceLayout.NumImmutableSamplers, SerializeImmutableSampler<Mode>))
        return false;

    // Instead of ppResourceSignatures
    for (Uint32 i = 0; i < std::max(CreateInfo.ResourceSignaturesCount, 1u); ++i)
    {
        if (!Ser(PRSNames[i]))
            return false;
    }

    ASSERT_SIZEOF64(ShaderResourceVariableDesc, 16, "Did you add a new member to ShaderResourceVariableDesc? Please add serialization here.");
    ASSERT_SIZEOF64(PipelineStateCreateInfo, 96, "Did you add a new member to PipelineStateCreateInfo? Please add serialization here.");

    return true;
}

template <SerializerMode Mode>
bool PSOSerializer<Mode>::SerializeCreateInfo(
    Serializer<Mode>&                           Ser,
    ConstQual<GraphicsPipelineStateCreateInfo>& CreateInfo,
    ConstQual<TPRSNames>&                       PRSNames,
    DynamicLinearAllocator*                     Allocator,
    ConstQual<const char*>&                     RenderPassName)
{
    if (!SerializeCreateInfo(Ser, static_cast<ConstQual<PipelineStateCreateInfo>&>(CreateInfo), PRSNames, Allocator))
        return false;

    // Serialize GraphicsPipelineDesc
    if (!Ser(CreateInfo.GraphicsPipeline.BlendDesc,
             CreateInfo.GraphicsPipeline.SampleMask,
             CreateInfo.GraphicsPipeline.RasterizerDesc,
             CreateInfo.GraphicsPipeline.DepthStencilDesc))
        return false;

    // Serialize InputLayoutDesc
    {
        auto& InputLayout = CreateInfo.GraphicsPipeline.InputLayout;
        if (!Ser.SerializeArray(Allocator, InputLayout.LayoutElements, InputLayout.NumElements,
                                [](Serializer<Mode>&         Ser,
                                   ConstQual<LayoutElement>& Elem) //
                                {
                                    return Ser(Elem.HLSLSemantic,
                                               Elem.InputIndex,
                                               Elem.BufferSlot,
                                               Elem.NumComponents,
                                               Elem.ValueType,
                                               Elem.IsNormalized,
                                               Elem.RelativeOffset,
                                               Elem.Stride,
                                               Elem.Frequency,
                                               Elem.InstanceDataStepRate);
                                }))
            return false;
    }

    return Ser(CreateInfo.GraphicsPipeline.PrimitiveTopology,
               CreateInfo.GraphicsPipeline.NumViewports,
               CreateInfo.GraphicsPipeline.NumRenderTargets,
               CreateInfo.GraphicsPipeline.SubpassIndex,
               CreateInfo.GraphicsPipeline.ShadingRateFlags,
               CreateInfo.GraphicsPipeline.RTVFormats,
               CreateInfo.GraphicsPipeline.DSVFormat,
               CreateInfo.GraphicsPipeline.ReadOnlyDSV,
               CreateInfo.GraphicsPipeline.SmplDesc,
               RenderPassName); // for CreateInfo.GraphicsPipeline.pRenderPass

    // Skip NodeMask

    ASSERT_SIZEOF64(GraphicsPipelineStateCreateInfo, 344, "Did you add a new member to GraphicsPipelineStateCreateInfo? Please add serialization here.");
    ASSERT_SIZEOF64(LayoutElement, 40, "Did you add a new member to LayoutElement? Please add serialization here.");
}

template <SerializerMode Mode>
bool PSOSerializer<Mode>::SerializeCreateInfo(
    Serializer<Mode>&                          Ser,
    ConstQual<ComputePipelineStateCreateInfo>& CreateInfo,
    ConstQual<TPRSNames>&                      PRSNames,
    DynamicLinearAllocator*                    Allocator)
{
    return SerializeCreateInfo(Ser, static_cast<ConstQual<PipelineStateCreateInfo>&>(CreateInfo), PRSNames, Allocator);

    ASSERT_SIZEOF64(ComputePipelineStateCreateInfo, 104, "Did you add a new member to ComputePipelineStateCreateInfo? Please add serialization here.");
}

template <SerializerMode Mode>
bool PSOSerializer<Mode>::SerializeCreateInfo(
    Serializer<Mode>&                       Ser,
    ConstQual<TilePipelineStateCreateInfo>& CreateInfo,
    ConstQual<TPRSNames>&                   PRSNames,
    DynamicLinearAllocator*                 Allocator)
{
    if (!SerializeCreateInfo(Ser, static_cast<ConstQual<PipelineStateCreateInfo>&>(CreateInfo), PRSNames, Allocator))
        return false;

    // Serialize TilePipelineDesc
    return Ser(CreateInfo.TilePipeline.NumRenderTargets,
               CreateInfo.TilePipeline.SampleCount,
               CreateInfo.TilePipeline.RTVFormats);

    ASSERT_SIZEOF64(TilePipelineStateCreateInfo, 128, "Did you add a new member to TilePipelineStateCreateInfo? Please add serialization here.");
}

template <SerializerMode Mode>
bool PSOSerializer<Mode>::SerializeCreateInfo(
    Serializer<Mode>&                                         Ser,
    ConstQual<RayTracingPipelineStateCreateInfo>&             CreateInfo,
    ConstQual<TPRSNames>&                                     PRSNames,
    DynamicLinearAllocator*                                   Allocator,
    const std::function<void(Uint32&, ConstQual<IShader*>&)>& ShaderToIndex)
{
    const bool IsReading = (Allocator != nullptr);
    const bool IsWriting = !IsReading;

    if (!SerializeCreateInfo(Ser, static_cast<ConstQual<PipelineStateCreateInfo>&>(CreateInfo), PRSNames, Allocator))
        return false;

    // Serialize RayTracingPipelineDesc
    if (!Ser(CreateInfo.RayTracingPipeline.ShaderRecordSize,
             CreateInfo.RayTracingPipeline.MaxRecursionDepth))
        return false;

    // Serialize RayTracingPipelineStateCreateInfo
    if (!Ser(CreateInfo.pShaderRecordName,
             CreateInfo.MaxAttributeSize,
             CreateInfo.MaxPayloadSize))
        return false;

    // Serialize RayTracingGeneralShaderGroup
    auto res =
        Ser.SerializeArray(Allocator, CreateInfo.pGeneralShaders, CreateInfo.GeneralShaderCount,
                           [&](Serializer<Mode>&                        Ser,
                               ConstQual<RayTracingGeneralShaderGroup>& Group) //
                           {
                               Uint32 ShaderIndex = ~0u;
                               if (IsWriting)
                               {
                                   ShaderToIndex(ShaderIndex, Group.pShader);
                               }

                               if (!Ser(Group.Name, ShaderIndex))
                                   return false;

                               VERIFY_EXPR(ShaderIndex != ~0u);
                               if (IsReading)
                               {
                                   ShaderToIndex(ShaderIndex, Group.pShader);
                               }

                               return true;
                           });
    if (!res) return false;

    // Serialize RayTracingTriangleHitShaderGroup
    res =
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

                               if (!Ser(Group.Name, ClosestHitShaderIndex, AnyHitShaderIndex))
                                   return false;

                               VERIFY_EXPR(ClosestHitShaderIndex != ~0u);
                               if (IsReading)
                               {
                                   ShaderToIndex(ClosestHitShaderIndex, Group.pClosestHitShader);
                                   ShaderToIndex(AnyHitShaderIndex, Group.pAnyHitShader);
                               }

                               return true;
                           });
    if (!res) return false;

    // Serialize RayTracingProceduralHitShaderGroup
    res =
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

                               if (!Ser(Group.Name, IntersectionShaderIndex, ClosestHitShaderIndex, AnyHitShaderIndex))
                                   return false;

                               VERIFY_EXPR(IntersectionShaderIndex != ~0u);
                               if (IsReading)
                               {
                                   ShaderToIndex(IntersectionShaderIndex, Group.pIntersectionShader);
                                   ShaderToIndex(ClosestHitShaderIndex, Group.pClosestHitShader);
                                   ShaderToIndex(AnyHitShaderIndex, Group.pAnyHitShader);
                               }

                               return true;
                           });
    return res;

    ASSERT_SIZEOF64(RayTracingPipelineStateCreateInfo, 168, "Did you add a new member to RayTracingPipelineStateCreateInfo? Please add serialization here.");
    ASSERT_SIZEOF64(RayTracingGeneralShaderGroup, 16, "Did you add a new member to RayTracingGeneralShaderGroup? Please add serialization here.");
    ASSERT_SIZEOF64(RayTracingTriangleHitShaderGroup, 24, "Did you add a new member to RayTracingTriangleHitShaderGroup? Please add serialization here.");
    ASSERT_SIZEOF64(RayTracingProceduralHitShaderGroup, 32, "Did you add a new member to RayTracingProceduralHitShaderGroup? Please add serialization here.");
}

template <SerializerMode Mode>
bool RPSerializer<Mode>::SerializeDesc(
    Serializer<Mode>&          Ser,
    ConstQual<RenderPassDesc>& RPDesc,
    DynamicLinearAllocator*    Allocator)
{
    auto res =
        Ser.SerializeArray(Allocator, RPDesc.pAttachments, RPDesc.AttachmentCount,
                           [](Serializer<Mode>&                    Ser,
                              ConstQual<RenderPassAttachmentDesc>& Attachment) //
                           {
                               return Ser(Attachment.Format,
                                          Attachment.SampleCount,
                                          Attachment.LoadOp,
                                          Attachment.StoreOp,
                                          Attachment.StencilLoadOp,
                                          Attachment.StencilStoreOp,
                                          Attachment.InitialState,
                                          Attachment.FinalState);
                           });
    if (!res) return false;

    res =
        Ser.SerializeArray(Allocator, RPDesc.pSubpasses, RPDesc.SubpassCount,
                           [&Allocator](Serializer<Mode>&       Ser,
                                        ConstQual<SubpassDesc>& Subpass) //
                           {
                               auto SerializeAttachmentRef = [](Serializer<Mode>&               Ser,
                                                                ConstQual<AttachmentReference>& AttachRef) //
                               {
                                   return Ser(AttachRef.AttachmentIndex,
                                              AttachRef.State);
                               };

                               if (!Ser.SerializeArray(Allocator, Subpass.pInputAttachments, Subpass.InputAttachmentCount, SerializeAttachmentRef))
                                   return false;
                               if (!Ser.SerializeArray(Allocator, Subpass.pRenderTargetAttachments, Subpass.RenderTargetAttachmentCount, SerializeAttachmentRef))
                                   return false;

                               // Note: in Read mode, ResolveAttachCount, DepthStencilAttachCount, and ShadingRateAttachCount will be overwritten
                               Uint32 ResolveAttachCount = Subpass.pResolveAttachments != nullptr ? Subpass.RenderTargetAttachmentCount : 0;
                               if (!Ser.SerializeArray(Allocator, Subpass.pResolveAttachments, ResolveAttachCount, SerializeAttachmentRef))
                                   return false;

                               Uint32 DepthStencilAttachCount = Subpass.pDepthStencilAttachment != nullptr ? 1 : 0;
                               if (!Ser.SerializeArray(Allocator, Subpass.pDepthStencilAttachment, DepthStencilAttachCount, SerializeAttachmentRef))
                                   return false;


                               if (!Ser.SerializeArray(Allocator, Subpass.pPreserveAttachments, Subpass.PreserveAttachmentCount,
                                                       [](Serializer<Mode>&  Ser,
                                                          ConstQual<Uint32>& Attach) //
                                                       {
                                                           return Ser(Attach);
                                                       }))
                                   return false;

                               Uint32 ShadingRateAttachCount = Subpass.pShadingRateAttachment != nullptr ? 1 : 0;
                               return Ser.SerializeArray(Allocator, Subpass.pShadingRateAttachment, ShadingRateAttachCount,
                                                         [](Serializer<Mode>&                 Ser,
                                                            ConstQual<ShadingRateAttachment>& SRAttachment) //
                                                         {
                                                             return Ser(SRAttachment.Attachment.AttachmentIndex,
                                                                        SRAttachment.Attachment.State,
                                                                        SRAttachment.TileSize);
                                                         });
                           });
    if (!res) return false;

    res =
        Ser.SerializeArray(Allocator, RPDesc.pDependencies, RPDesc.DependencyCount,
                           [](Serializer<Mode>&                 Ser,
                              ConstQual<SubpassDependencyDesc>& Dep) //
                           {
                               return Ser(Dep.SrcSubpass,
                                          Dep.DstSubpass,
                                          Dep.SrcStageMask,
                                          Dep.DstStageMask,
                                          Dep.SrcAccessMask,
                                          Dep.DstAccessMask);
                           });
    return res;

    ASSERT_SIZEOF64(RenderPassDesc, 56, "Did you add a new member to RenderPassDesc? Please add serialization here.");
    ASSERT_SIZEOF64(SubpassDesc, 72, "Did you add a new member to SubpassDesc? Please add serialization here.");
    ASSERT_SIZEOF(RenderPassAttachmentDesc, 16, "Did you add a new member to RenderPassAttachmentDesc? Please add serialization here.");
    ASSERT_SIZEOF(SubpassDependencyDesc, 24, "Did you add a new member to SubpassDependencyDesc? Please add serialization here.");
    ASSERT_SIZEOF(ShadingRateAttachment, 16, "Did you add a new member to ShadingRateAttachment? Please add serialization here.");
    ASSERT_SIZEOF(AttachmentReference, 8, "Did you add a new member to AttachmentReference? Please add serialization here.");
}

template <SerializerMode Mode>
bool PSOSerializer<Mode>::SerializeShaderIndices(
    Serializer<Mode>&            Ser,
    ConstQual<ShaderIndexArray>& Shaders,
    DynamicLinearAllocator*      Allocator)
{
    return Ser.SerializeArrayRaw(Allocator, Shaders.pIndices, Shaders.Count);
}

template <SerializerMode Mode>
bool PSOSerializer<Mode>::SerializeAuxData(Serializer<Mode>&                Ser,
                                           ConstQual<SerializedPSOAuxData>& AuxData,
                                           DynamicLinearAllocator*          Allocator)
{
    return Ser(AuxData.NoShaderReflection);
    ASSERT_SIZEOF(SerializedPSOAuxData, 1, "Did you add a new member to SerializedPSOAuxData? Please add serialization here.");
}

template <SerializerMode Mode>
bool ShaderSerializer<Mode>::SerializeBytecodeOrSource(Serializer<Mode>&            Ser,
                                                       ConstQual<ShaderCreateInfo>& CI)
{
    VERIFY(CI.Source == nullptr || CI.ByteCode == nullptr, "Only one of Source or Bytecode can be non-null");
    const Uint8 UseBytecode = CI.ByteCode != nullptr ? 1 : 0;

    if (!Ser(UseBytecode))
        return false;

    if (UseBytecode)
        return Ser.SerializeBytes(CI.ByteCode, CI.ByteCodeSize);
    else
        return Ser.SerializeBytes(CI.Source, CI.SourceLength);
}

template <>
bool ShaderSerializer<SerializerMode::Read>::SerializeBytecodeOrSource(Serializer<SerializerMode::Read>& Ser,
                                                                       ConstQual<ShaderCreateInfo>&      CI)
{
    Uint8 UseBytecode = 0;
    Ser(UseBytecode);
    if (UseBytecode)
    {
        return Ser.SerializeBytes(CI.ByteCode, CI.ByteCodeSize);
    }
    else
    {
        const void* pSource = nullptr;
        if (!Ser.SerializeBytes(pSource, CI.SourceLength))
            return false;

        CI.Source = reinterpret_cast<const char*>(pSource);
        return true;
    }
}

template <SerializerMode Mode>
bool ShaderSerializer<Mode>::SerializeCI(Serializer<Mode>&            Ser,
                                         ConstQual<ShaderCreateInfo>& CI)
{
    if (!Ser(CI.Desc.Name,
             CI.Desc.ShaderType,
             CI.Desc.UseCombinedTextureSamplers,
             CI.Desc.CombinedSamplerSuffix,
             CI.EntryPoint,
             CI.SourceLanguage,
             CI.ShaderCompiler,
             CI.HLSLVersion,
             CI.GLSLVersion,
             CI.GLESSLVersion,
             CI.MSLVersion,
             CI.CompileFlags,
             CI.LoadConstantBufferReflection,
             CI.GLSLExtensions,
             CI.WebGPUEmulatedArrayIndexSuffix))
        return false;

    return SerializeBytecodeOrSource(Ser, CI);

    ASSERT_SIZEOF64(ShaderCreateInfo, 152, "Did you add a new member to ShaderCreateInfo? Please add serialization here.");
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
