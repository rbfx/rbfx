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

#include "../../../../Graphics/GraphicsEngine/include/PSOSerializer.hpp"
#include "../../../../Graphics/GraphicsEngine/include/EngineMemory.h"

#include <algorithm>
#include <array>
#include <cstring>

#include "gtest/gtest.h"

#include "PipelineState.h"

using namespace Diligent;

namespace
{

struct RandomValue
{
    explicit RandomValue(Uint64 MaxValueLimit = 2048) :
        m_MaxValueLimit{MaxValueLimit}
    {
    }

    template <typename T>
    std::enable_if_t<std::is_floating_point<T>::value> operator()(T& Value, T minValue, T maxValue, T Step = T(1))
    {
        VERIFY_EXPR(minValue < maxValue);
        VERIFY_EXPR(Step != T(0));

        const auto Count = static_cast<Uint64>((maxValue - minValue) / Step);
        VERIFY_EXPR(Count < (1ull << 16));

        m_MaxValue = std::max(m_MaxValue, Count);
        Value      = minValue + static_cast<T>(m_CurValue % Count) * Step;
    }

    template <typename T>
    std::enable_if_t<!std::is_floating_point<T>::value> operator()(T& Value, T minValue, Uint64 maxValue, Uint64 Step = 1)
    {
        VERIFY_EXPR(static_cast<T>(maxValue) > minValue);
        VERIFY_EXPR(Step != 0);

        const auto Count = static_cast<Uint64>(maxValue - minValue) / static_cast<Uint64>(Step);
        VERIFY_EXPR(Count < (1ull << 16));

        m_MaxValue = std::min(std::max(m_MaxValue, Count), m_MaxValueLimit);
        Value      = static_cast<T>(static_cast<Uint64>(minValue) + (m_CurValue % Count) * static_cast<Uint64>(Step));
    }

    bool IsComplete()
    {
        return ++m_CurValue > m_MaxValue;
    }

private:
    const Uint64 m_MaxValueLimit;

    Uint64 m_MaxValue = 0;
    Uint64 m_CurValue = 0;
};


TEST(PSOSerializerTest, SerializePRSDesc)
{
    RandomValue RndValue;
    do
    {
        PipelineResourceDesc Resources[] = //
            {
                {SHADER_TYPE_UNKNOWN, "Resource1", 1, SHADER_RESOURCE_TYPE_UNKNOWN},
                {SHADER_TYPE_UNKNOWN, "Resource2", 1, SHADER_RESOURCE_TYPE_UNKNOWN},
                {SHADER_TYPE_UNKNOWN, "Resource3", 1, SHADER_RESOURCE_TYPE_UNKNOWN},
                {SHADER_TYPE_UNKNOWN, "Resource4", 1, SHADER_RESOURCE_TYPE_UNKNOWN},
                {SHADER_TYPE_UNKNOWN, "Resource5", 1, SHADER_RESOURCE_TYPE_UNKNOWN} //
            };

        RndValue(Resources[0].ShaderStages, SHADER_TYPE_VERTEX, (SHADER_TYPE_LAST << 1) - 1);
        RndValue(Resources[1].ArraySize, 0u, 100u);
        RndValue(Resources[2].ResourceType, static_cast<SHADER_RESOURCE_TYPE>(1), SHADER_RESOURCE_TYPE_LAST);
        RndValue(Resources[3].VarType, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC);
        RndValue(Resources[4].Flags, PIPELINE_RESOURCE_FLAG_NONE, (PIPELINE_RESOURCE_FLAG_LAST << 1) - 1);

        ImmutableSamplerDesc ImmutableSamplers[] = //
            {
                {SHADER_TYPE_UNKNOWN, "Sampler1", SamplerDesc{}},
                {SHADER_TYPE_UNKNOWN, "Sampler2", SamplerDesc{}} //
            };

        RndValue(ImmutableSamplers[0].ShaderStages, SHADER_TYPE_VERTEX, (SHADER_TYPE_LAST << 1) - 1);
        RndValue(ImmutableSamplers[0].Desc.MinFilter, FILTER_TYPE_UNKNOWN, FILTER_TYPE_NUM_FILTERS);
        RndValue(ImmutableSamplers[1].Desc.MagFilter, FILTER_TYPE_UNKNOWN, FILTER_TYPE_NUM_FILTERS);
        RndValue(ImmutableSamplers[0].Desc.MipFilter, FILTER_TYPE_UNKNOWN, FILTER_TYPE_NUM_FILTERS);
        RndValue(ImmutableSamplers[1].Desc.AddressU, TEXTURE_ADDRESS_UNKNOWN, TEXTURE_ADDRESS_NUM_MODES);
        RndValue(ImmutableSamplers[0].Desc.AddressV, TEXTURE_ADDRESS_UNKNOWN, TEXTURE_ADDRESS_NUM_MODES);
        RndValue(ImmutableSamplers[1].Desc.AddressW, TEXTURE_ADDRESS_UNKNOWN, TEXTURE_ADDRESS_NUM_MODES);
        RndValue(ImmutableSamplers[1].Desc.Flags, SAMPLER_FLAG_NONE, SAMPLER_FLAG_SUBSAMPLED_COARSE_RECONSTRUCTION);
        RndValue(ImmutableSamplers[1].Desc.MipLODBias, -2.0f, 2.0f);
        RndValue(ImmutableSamplers[0].Desc.MaxAnisotropy, 0u, 16u);
        RndValue(ImmutableSamplers[0].Desc.ComparisonFunc, COMPARISON_FUNC_UNKNOWN, COMPARISON_FUNC_NUM_FUNCTIONS);
        RndValue(ImmutableSamplers[0].Desc.BorderColor[0], 0.f, 1.f, 0.1f);
        RndValue(ImmutableSamplers[0].Desc.BorderColor[1], 0.f, 1.f, 0.12f);
        RndValue(ImmutableSamplers[0].Desc.BorderColor[2], 0.f, 1.f, 0.17f);
        RndValue(ImmutableSamplers[0].Desc.BorderColor[3], 0.f, 1.f, 0.08f);
        RndValue(ImmutableSamplers[1].Desc.MinLOD, 0.f, 10.f);
        RndValue(ImmutableSamplers[1].Desc.MaxLOD, 0.f, 10.f);

        PipelineResourceSignatureDesc SrcPRSDesc;
        SrcPRSDesc.Resources            = Resources;
        SrcPRSDesc.NumResources         = _countof(Resources);
        SrcPRSDesc.ImmutableSamplers    = ImmutableSamplers;
        SrcPRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);

        RndValue(SrcPRSDesc.BindingIndex, Uint8{0}, Uint8{DILIGENT_MAX_RESOURCE_SIGNATURES - 1});

        PipelineResourceSignatureInternalData SrcInternalData;

        RndValue(SrcInternalData.ShaderStages, SHADER_TYPE_VERTEX, (SHADER_TYPE_LAST << 1) - 1);
        RndValue(SrcInternalData.StaticResShaderStages, SHADER_TYPE_VERTEX, (SHADER_TYPE_LAST << 1) - 1);
        RndValue(SrcInternalData.PipelineType, PIPELINE_TYPE_GRAPHICS, PIPELINE_TYPE_LAST);

        for (size_t i = 0; i < SrcInternalData.StaticResStageIndex.size(); ++i)
            RndValue(SrcInternalData.StaticResStageIndex[i], static_cast<Int8>(std::numeric_limits<Int8>::min() + i), static_cast<Int8>(std::numeric_limits<Int8>::max() - i));


        DynamicLinearAllocator Allocator{GetRawAllocator()};
        SerializedData         Data;
        {
            Serializer<SerializerMode::Measure> MSer;
            PRSSerializer<SerializerMode::Measure>::SerializeDesc(MSer, SrcPRSDesc, nullptr);
            PRSSerializer<SerializerMode::Measure>::SerializeInternalData(MSer, SrcInternalData, nullptr);

            Data = MSer.AllocateData(GetRawAllocator());
        }

        {
            Serializer<SerializerMode::Write> WSer{Data};
            PRSSerializer<SerializerMode::Write>::SerializeDesc(WSer, SrcPRSDesc, nullptr);
            PRSSerializer<SerializerMode::Write>::SerializeInternalData(WSer, SrcInternalData, nullptr);

            EXPECT_EQ(Data.Size(), WSer.GetSize());
        }

        PipelineResourceSignatureDesc         DstPRSDesc;
        PipelineResourceSignatureInternalData DstInternalData;
        {
            Serializer<SerializerMode::Read> RSer{Data};
            PRSSerializer<SerializerMode::Read>::SerializeDesc(RSer, DstPRSDesc, &Allocator);
            PRSSerializer<SerializerMode::Read>::SerializeInternalData(RSer, DstInternalData, nullptr);

            EXPECT_TRUE(RSer.IsEnded());
        }

        EXPECT_EQ(SrcPRSDesc, DstPRSDesc);
        EXPECT_EQ(SrcInternalData, DstInternalData);

    } while (!RndValue.IsComplete());
}

using TPRSNames = DeviceObjectArchiveBase::TPRSNames;

template <typename PSOCIType, typename HelperType>
static void TestSerializePSOCreateInfo(HelperType&& Helper)
{
    const String PRSNames[] = {"PRS-1", "Signature-2", "ResSign-3", "PRS-4", "Signature-5", "ResSign-6"};

    RandomValue RndValue;
    do
    {
        TPRSNames SrcPRSNames = {};
        PSOCIType SrcPSO;

        RndValue(SrcPSO.PSODesc.PipelineType, PIPELINE_TYPE_GRAPHICS, PIPELINE_TYPE_LAST);
        RndValue(SrcPSO.Flags, PSO_CREATE_FLAG_NONE, (PSO_CREATE_FLAG_LAST << 1) - 1);
        RndValue(SrcPSO.ResourceSignaturesCount, 1u, std::min<Uint32>(MAX_RESOURCE_SIGNATURES, _countof(PRSNames)) - 1);

        for (Uint32 i = 0; i < SrcPSO.ResourceSignaturesCount; ++i)
            SrcPRSNames[i] = PRSNames[i].c_str();

        Helper.Init(SrcPSO, RndValue);

        Serializer<SerializerMode::Measure> MSer;
        Helper.Measure(MSer, SrcPSO, SrcPRSNames);

        DynamicLinearAllocator Allocator{GetRawAllocator()};
        SerializedData         Data{MSer.GetSize(), GetRawAllocator()};

        Serializer<SerializerMode::Write> WSer{Data};
        Helper.Write(WSer, SrcPSO, SrcPRSNames);
        ASSERT_EQ(Data.Size(), WSer.GetSize());

        TPRSNames DstPRSNames = {};
        PSOCIType DstPSO;

        Serializer<SerializerMode::Read> RSer{Data};
        Helper.Read(RSer, DstPSO, DstPRSNames, &Allocator);

        EXPECT_TRUE(RSer.IsEnded());
        EXPECT_EQ(SrcPSO, DstPSO);

        for (size_t i = 0; i < DstPRSNames.size(); ++i)
        {
            if (i < SrcPSO.ResourceSignaturesCount)
            {
                EXPECT_EQ(PRSNames[i], DstPRSNames[i]);
            }
            else
            {
                EXPECT_EQ(DstPRSNames[i], nullptr);
            }
        }

    } while (!RndValue.IsComplete());
}


TEST(PSOSerializerTest, SerializePSOCreateInfo)
{
    struct Helper
    {
        void Init(PipelineStateCreateInfo&, RandomValue&)
        {}

        void Measure(Serializer<SerializerMode::Measure>& Ser, const PipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            PSOSerializer<SerializerMode::Measure>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr);
        }

        void Write(Serializer<SerializerMode::Write>& Ser, const PipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            PSOSerializer<SerializerMode::Write>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr);
        }

        void Read(Serializer<SerializerMode::Read>& Ser, PipelineStateCreateInfo& CI, TPRSNames& PRSNames, DynamicLinearAllocator* Allocator)
        {
            PSOSerializer<SerializerMode::Read>::SerializeCreateInfo(Ser, CI, PRSNames, Allocator);
        }
    };
    TestSerializePSOCreateInfo<PipelineStateCreateInfo>(Helper{});
}


TEST(PSOSerializerTest, SerializeGraphicsPSOCreateInfo)
{
    struct Helper
    {
        const String               SrcRenderPassName;
        std::vector<LayoutElement> LayoutElements;

        void Init(GraphicsPipelineStateCreateInfo& CI, RandomValue& RndValue)
        {
            RndValue(CI.GraphicsPipeline.BlendDesc.AlphaToCoverageEnable, false, true);
            RndValue(CI.GraphicsPipeline.BlendDesc.IndependentBlendEnable, false, true);
            RndValue(CI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable, false, true);
            RndValue(CI.GraphicsPipeline.BlendDesc.RenderTargets[0].LogicOperationEnable, false, true);
            RndValue(CI.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlend, BLEND_FACTOR_UNDEFINED, BLEND_FACTOR_NUM_FACTORS);
            RndValue(CI.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlend, BLEND_FACTOR_UNDEFINED, BLEND_FACTOR_NUM_FACTORS);
            RndValue(CI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOp, BLEND_OPERATION_UNDEFINED, BLEND_OPERATION_NUM_OPERATIONS);
            RndValue(CI.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlendAlpha, BLEND_FACTOR_UNDEFINED, BLEND_FACTOR_NUM_FACTORS);
            RndValue(CI.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlendAlpha, BLEND_FACTOR_UNDEFINED, BLEND_FACTOR_NUM_FACTORS);
            RndValue(CI.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOpAlpha, BLEND_OPERATION_UNDEFINED, BLEND_OPERATION_NUM_OPERATIONS);
            RndValue(CI.GraphicsPipeline.BlendDesc.RenderTargets[0].LogicOp, LOGIC_OP_CLEAR, LOGIC_OP_NUM_OPERATIONS);
            RndValue(CI.GraphicsPipeline.BlendDesc.RenderTargets[0].RenderTargetWriteMask, COLOR_MASK_NONE, COLOR_MASK_ALL);
            RndValue(CI.GraphicsPipeline.SampleMask, 0u, 0xFFFFFFFFu, 0xFFFFFu);
            RndValue(CI.GraphicsPipeline.RasterizerDesc.FillMode, FILL_MODE_UNDEFINED, FILL_MODE_NUM_MODES);
            RndValue(CI.GraphicsPipeline.RasterizerDesc.CullMode, CULL_MODE_UNDEFINED, CULL_MODE_NUM_MODES);
            RndValue(CI.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise, false, true);
            RndValue(CI.GraphicsPipeline.RasterizerDesc.DepthClipEnable, false, true);
            RndValue(CI.GraphicsPipeline.RasterizerDesc.ScissorEnable, false, true);
            RndValue(CI.GraphicsPipeline.RasterizerDesc.AntialiasedLineEnable, false, true);
            RndValue(CI.GraphicsPipeline.RasterizerDesc.DepthBias, -10, 10);
            RndValue(CI.GraphicsPipeline.RasterizerDesc.DepthBiasClamp, -10.f, 10.f);
            RndValue(CI.GraphicsPipeline.RasterizerDesc.SlopeScaledDepthBias, -10.f, 10.f);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.DepthEnable, false, true);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable, false, true);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.DepthFunc, COMPARISON_FUNC_UNKNOWN, COMPARISON_FUNC_NUM_FUNCTIONS);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.StencilEnable, false, true);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.StencilReadMask, Uint8{0}, Uint8{0xFF});
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.StencilWriteMask, Uint8{0}, Uint8{0xFF});
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFailOp, STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilDepthFailOp, STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilPassOp, STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFunc, COMPARISON_FUNC_UNKNOWN, COMPARISON_FUNC_NUM_FUNCTIONS);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFailOp, STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilDepthFailOp, STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilPassOp, STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS);
            RndValue(CI.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFunc, COMPARISON_FUNC_UNKNOWN, COMPARISON_FUNC_NUM_FUNCTIONS);
            RndValue(CI.GraphicsPipeline.InputLayout.NumElements, 0u, 4u);

            if (CI.GraphicsPipeline.InputLayout.NumElements > 0)
            {
                LayoutElements.resize(CI.GraphicsPipeline.InputLayout.NumElements);
                CI.GraphicsPipeline.InputLayout.LayoutElements = LayoutElements.data();

                for (size_t i = 0; i < LayoutElements.size(); ++i)
                {
                    auto& Elem = LayoutElements[i];
                    RndValue(Elem.InputIndex, Uint32(i), 16u);
                    RndValue(Elem.BufferSlot, Uint32(i / 2), 4u);
                    RndValue(Elem.NumComponents, 0u, 4u);
                    RndValue(Elem.ValueType, VT_UNDEFINED, VT_NUM_TYPES);
                    RndValue(Elem.IsNormalized, false, true);
                    RndValue(Elem.RelativeOffset, 0u, (1u << 12), 128u);
                    if (i == 2)
                        Elem.RelativeOffset = LAYOUT_ELEMENT_AUTO_OFFSET;
                    RndValue(Elem.Stride, 0u, (1u << 10), 128u);
                    if (i == 1)
                        Elem.Stride = LAYOUT_ELEMENT_AUTO_STRIDE;
                    RndValue(Elem.Frequency, INPUT_ELEMENT_FREQUENCY_UNDEFINED, INPUT_ELEMENT_FREQUENCY_NUM_FREQUENCIES);
                    RndValue(Elem.InstanceDataStepRate, 1u, 128u);
                }
            }

            RndValue(CI.GraphicsPipeline.PrimitiveTopology, PRIMITIVE_TOPOLOGY_UNDEFINED, PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES);
            RndValue(CI.GraphicsPipeline.NumViewports, Uint8{1}, Uint8{8});
            RndValue(CI.GraphicsPipeline.SubpassIndex, Uint8{1}, Uint8{8});
            RndValue(CI.GraphicsPipeline.ShadingRateFlags, PIPELINE_SHADING_RATE_FLAG_NONE, (PIPELINE_SHADING_RATE_FLAG_LAST << 1) - 1);
            RndValue(CI.GraphicsPipeline.NumRenderTargets, Uint8{1}, Uint8{8});
            for (Uint32 i = 0; i < CI.GraphicsPipeline.NumRenderTargets; ++i)
            {
                RndValue(CI.GraphicsPipeline.RTVFormats[i], TEX_FORMAT_UNKNOWN, TEX_FORMAT_NUM_FORMATS, i + 1);
            }
            RndValue(CI.GraphicsPipeline.SmplDesc.Count, Uint8{0}, Uint8{64});
            RndValue(CI.GraphicsPipeline.SmplDesc.Quality, Uint8{0}, Uint8{8});
        }

        void Measure(Serializer<SerializerMode::Measure>& Ser, const GraphicsPipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            const char* RPName = SrcRenderPassName.c_str();
            PSOSerializer<SerializerMode::Measure>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr, RPName);
        }

        void Write(Serializer<SerializerMode::Write>& Ser, const GraphicsPipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            const char* RPName = SrcRenderPassName.c_str();
            PSOSerializer<SerializerMode::Write>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr, RPName);
        }

        void Read(Serializer<SerializerMode::Read>& Ser, GraphicsPipelineStateCreateInfo& CI, TPRSNames& PRSNames, DynamicLinearAllocator* Allocator)
        {
            const char* RPName = nullptr;
            PSOSerializer<SerializerMode::Read>::SerializeCreateInfo(Ser, CI, PRSNames, Allocator, RPName);
            EXPECT_EQ(SrcRenderPassName, RPName);
        }
    };
    TestSerializePSOCreateInfo<GraphicsPipelineStateCreateInfo>(Helper{});
}


TEST(PSOSerializerTest, SerializeComputePSOCreateInfo)
{
    struct Helper
    {
        void Init(ComputePipelineStateCreateInfo&, RandomValue&)
        {}

        void Measure(Serializer<SerializerMode::Measure>& Ser, const ComputePipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            PSOSerializer<SerializerMode::Measure>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr);
        }

        void Write(Serializer<SerializerMode::Write>& Ser, const ComputePipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            PSOSerializer<SerializerMode::Write>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr);
        }

        void Read(Serializer<SerializerMode::Read>& Ser, ComputePipelineStateCreateInfo& CI, TPRSNames& PRSNames, DynamicLinearAllocator* Allocator)
        {
            PSOSerializer<SerializerMode::Read>::SerializeCreateInfo(Ser, CI, PRSNames, Allocator);
        }
    };
    TestSerializePSOCreateInfo<ComputePipelineStateCreateInfo>(Helper{});
}


TEST(PSOSerializerTest, SerializeTilePSOCreateInfo)
{
    struct Helper
    {
        void Init(TilePipelineStateCreateInfo& CI, RandomValue& RndValue)
        {
            RndValue(CI.TilePipeline.SampleCount, Uint8{1}, Uint8{64});
            RndValue(CI.TilePipeline.NumRenderTargets, Uint8{1}, Uint8{8});
            for (Uint32 i = 0; i < CI.TilePipeline.NumRenderTargets; ++i)
            {
                RndValue(CI.TilePipeline.RTVFormats[i], TEX_FORMAT_UNKNOWN, TEX_FORMAT_NUM_FORMATS, i + 1);
            }
        }

        void Measure(Serializer<SerializerMode::Measure>& Ser, const TilePipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            PSOSerializer<SerializerMode::Measure>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr);
        }

        void Write(Serializer<SerializerMode::Write>& Ser, const TilePipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            PSOSerializer<SerializerMode::Write>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr);
        }

        void Read(Serializer<SerializerMode::Read>& Ser, TilePipelineStateCreateInfo& CI, TPRSNames& PRSNames, DynamicLinearAllocator* Allocator)
        {
            PSOSerializer<SerializerMode::Read>::SerializeCreateInfo(Ser, CI, PRSNames, Allocator);
        }
    };
    TestSerializePSOCreateInfo<TilePipelineStateCreateInfo>(Helper{});
}


TEST(PSOSerializerTest, SerializeRayTracingPSOCreateInfo)
{
    struct Helper
    {
        const String                                    ShaderRecordName{"pShaderRecord"};
        std::vector<RayTracingGeneralShaderGroup>       GeneralShaders;
        std::vector<RayTracingTriangleHitShaderGroup>   TriangleHitShaders;
        std::vector<RayTracingProceduralHitShaderGroup> ProceduralHitShaders;
        DynamicLinearAllocator                          StringAlloc;

        Helper() :
            StringAlloc{GetRawAllocator()}
        {}

        const char* GenGroupName(const String& Prefix, Uint32 Index)
        {
            String Name{Prefix};
            Name += " - ";
            Name += std::to_string(Index * 1000);

            return StringAlloc.CopyString(Name);
        }

        void Init(RayTracingPipelineStateCreateInfo& CI, RandomValue& RndValue)
        {
            RndValue(CI.RayTracingPipeline.MaxRecursionDepth, Uint8{0}, Uint8{16});
            RndValue(CI.RayTracingPipeline.ShaderRecordSize, Uint16{0}, Uint16{128});

            RndValue(CI.MaxAttributeSize, 0u, 128u);
            RndValue(CI.MaxPayloadSize, 0u, 128u);

            bool UseShaderRecord;
            RndValue(UseShaderRecord, false, true);
            CI.pShaderRecordName = UseShaderRecord ? ShaderRecordName.c_str() : nullptr;

            RndValue(CI.GeneralShaderCount, 1u, 4u);
            RndValue(CI.TriangleHitShaderCount, 0u, 16u);
            RndValue(CI.ProceduralHitShaderCount, 0u, 8u);

            GeneralShaders.resize(CI.GeneralShaderCount);
            TriangleHitShaders.resize(CI.TriangleHitShaderCount);
            ProceduralHitShaders.resize(CI.ProceduralHitShaderCount);

            Uint32 ShaderIndex = 0x10000;

            for (Uint32 i = 0; i < CI.GeneralShaderCount; ++i)
            {
                auto& Group   = GeneralShaders[i];
                Group.Name    = GenGroupName("General", i);
                Group.pShader = BitCast<IShader*>(++ShaderIndex);
            }
            for (Uint32 i = 0; i < CI.TriangleHitShaderCount; ++i)
            {
                auto& Group             = TriangleHitShaders[i];
                Group.Name              = GenGroupName("TriangleHit", i);
                Group.pClosestHitShader = BitCast<IShader*>(++ShaderIndex);
                Group.pAnyHitShader     = BitCast<IShader*>(++ShaderIndex);
            }
            for (Uint32 i = 0; i < CI.ProceduralHitShaderCount; ++i)
            {
                auto& Group               = ProceduralHitShaders[i];
                Group.Name                = GenGroupName("ProceduralHit", i);
                Group.pIntersectionShader = BitCast<IShader*>(++ShaderIndex);
                Group.pClosestHitShader   = BitCast<IShader*>(++ShaderIndex);
                Group.pAnyHitShader       = BitCast<IShader*>(++ShaderIndex);
            }

            CI.pGeneralShaders       = CI.GeneralShaderCount ? GeneralShaders.data() : nullptr;
            CI.pTriangleHitShaders   = CI.TriangleHitShaderCount ? TriangleHitShaders.data() : nullptr;
            CI.pProceduralHitShaders = CI.ProceduralHitShaderCount ? ProceduralHitShaders.data() : nullptr;
        }

        void Measure(Serializer<SerializerMode::Measure>& Ser, const RayTracingPipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            PSOSerializer<SerializerMode::Measure>::SerializeCreateInfo(
                Ser, CI, PRSNames, nullptr,
                [](Uint32& outIndex, IShader* const& inShader) { outIndex = static_cast<Uint32>(BitCast<size_t>(inShader)); });
        }

        void Write(Serializer<SerializerMode::Write>& Ser, const RayTracingPipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            PSOSerializer<SerializerMode::Write>::SerializeCreateInfo(
                Ser, CI, PRSNames, nullptr,
                [](Uint32& outIndex, IShader* const& inShader) { outIndex = static_cast<Uint32>(BitCast<size_t>(inShader)); });
        }

        void Read(Serializer<SerializerMode::Read>& Ser, RayTracingPipelineStateCreateInfo& CI, TPRSNames& PRSNames, DynamicLinearAllocator* Allocator)
        {
            PSOSerializer<SerializerMode::Read>::SerializeCreateInfo(
                Ser, CI, PRSNames, Allocator,
                [](Uint32& inIndex, IShader*& outShader) { outShader = BitCast<IShader*>(inIndex); });
        }
    };
    TestSerializePSOCreateInfo<RayTracingPipelineStateCreateInfo>(Helper{});
}


TEST(PSOSerializerTest, SerializeRenderPassDesc)
{
    RandomValue RndValue;
    do
    {
        RenderPassDesc SrcRP;

        RenderPassAttachmentDesc Attachments[8]  = {};
        SubpassDesc              Subpasses[3]    = {};
        SubpassDependencyDesc    Dependencies[4] = {};
        DynamicLinearAllocator   TmpAlloc{GetRawAllocator()};

        RndValue(SrcRP.AttachmentCount, 1u, _countof(Attachments));
        RndValue(SrcRP.SubpassCount, 1u, _countof(Subpasses));
        RndValue(SrcRP.DependencyCount, 0u, _countof(Dependencies));

        const auto GenAttachmentIndex = [&SrcRP, &RndValue](Uint32 Offset = 0) {
            Uint32 Idx;
            RndValue(Idx, 0u, SrcRP.AttachmentCount);
            Idx = (Idx + Offset) % SrcRP.AttachmentCount;
            if (Idx == SrcRP.AttachmentCount)
                Idx = ATTACHMENT_UNUSED;
            return Idx;
        };
        const auto GenState = [&RndValue](Uint32 Step) {
            RESOURCE_STATE State;
            RndValue(State, RESOURCE_STATE_UNKNOWN, (RESOURCE_STATE_MAX_BIT << 1) - 1, Step);
            return State;
        };

        for (Uint32 i = 0; i < SrcRP.AttachmentCount; ++i)
        {
            RndValue(Attachments[i].Format, TEX_FORMAT_UNKNOWN, TEX_FORMAT_NUM_FORMATS, i + 1);
            RndValue(Attachments[i].SampleCount, Uint8{1}, Uint8{32});
            RndValue(Attachments[i].LoadOp, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_LOAD_OP_DISCARD);
            RndValue(Attachments[i].StoreOp, ATTACHMENT_STORE_OP_STORE, ATTACHMENT_STORE_OP_DISCARD);
            RndValue(Attachments[i].StencilLoadOp, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_LOAD_OP_DISCARD);
            RndValue(Attachments[i].StencilStoreOp, ATTACHMENT_STORE_OP_STORE, ATTACHMENT_STORE_OP_DISCARD);
            Attachments[i].InitialState = GenState(0xEF + i);
            Attachments[i].FinalState   = GenState(0x47 + i);
        }
        for (Uint32 i = 0; i < SrcRP.SubpassCount; ++i)
        {
            RndValue(Subpasses[i].InputAttachmentCount, 0u, 2u);
            RndValue(Subpasses[i].RenderTargetAttachmentCount, 0u, SrcRP.AttachmentCount);
            RndValue(Subpasses[i].PreserveAttachmentCount, 0u, SrcRP.AttachmentCount);

            bool HasDepthStencil;
            bool HasShadingRate;
            bool HasResolveAttachments;
            RndValue(HasDepthStencil, false, true);
            RndValue(HasShadingRate, false, true);
            RndValue(HasResolveAttachments, false, true);

            auto* pInputAttachments        = TmpAlloc.ConstructArray<AttachmentReference>(Subpasses[i].InputAttachmentCount);
            auto* pRenderTargetAttachments = TmpAlloc.ConstructArray<AttachmentReference>(Subpasses[i].RenderTargetAttachmentCount);
            auto* pResolveAttachments      = TmpAlloc.ConstructArray<AttachmentReference>(HasResolveAttachments ? Subpasses[i].RenderTargetAttachmentCount : 0);
            auto* pPreserveAttachments     = TmpAlloc.ConstructArray<Uint32>(Subpasses[i].PreserveAttachmentCount);

            for (Uint32 j = 0; j < Subpasses[i].InputAttachmentCount; ++j)
            {
                pInputAttachments[i].AttachmentIndex = GenAttachmentIndex(0);
                pInputAttachments[i].State           = GenState(0x55 + j + i * 10);
            }
            for (Uint32 j = 0; j < Subpasses[i].RenderTargetAttachmentCount; ++j)
            {
                pRenderTargetAttachments[i].AttachmentIndex = GenAttachmentIndex(1);
                pRenderTargetAttachments[i].State           = GenState(0x49 + j + i * 16);
            }
            if (pResolveAttachments)
            {
                for (Uint32 j = 0; j < Subpasses[i].RenderTargetAttachmentCount; ++j)
                {
                    pResolveAttachments[i].AttachmentIndex = GenAttachmentIndex(2);
                    pResolveAttachments[i].State           = GenState(0x38 + j + i * 9);
                }
            }
            for (Uint32 j = 0; j < Subpasses[i].PreserveAttachmentCount; ++j)
            {
                pPreserveAttachments[j] = GenAttachmentIndex(3);
            }

            Subpasses[i].pInputAttachments        = pInputAttachments;
            Subpasses[i].pRenderTargetAttachments = pRenderTargetAttachments;
            Subpasses[i].pResolveAttachments      = pResolveAttachments;
            Subpasses[i].pPreserveAttachments     = pPreserveAttachments;

            if (HasDepthStencil)
            {
                auto* pDepthStencilAttachment            = TmpAlloc.Construct<AttachmentReference>();
                Subpasses[i].pDepthStencilAttachment     = pDepthStencilAttachment;
                pDepthStencilAttachment->AttachmentIndex = GenAttachmentIndex(4);
                pDepthStencilAttachment->State           = GenState(0x82 + i);
            }
            if (HasShadingRate)
            {
                auto* pShadingRateAttachment        = TmpAlloc.Construct<ShadingRateAttachment>();
                Subpasses[i].pShadingRateAttachment = pShadingRateAttachment;
                RndValue(pShadingRateAttachment->TileSize[0], 0u, 32u, 2);
                RndValue(pShadingRateAttachment->TileSize[1], 0u, 64u, 4);
                pShadingRateAttachment->Attachment.AttachmentIndex = GenAttachmentIndex(5);
                pShadingRateAttachment->Attachment.State           = GenState(0x63 + i);
            }
        }
        for (Uint32 i = 0; i < SrcRP.DependencyCount; ++i)
        {
            RndValue(Dependencies[i].SrcSubpass, 0u, 10u);
            RndValue(Dependencies[i].DstSubpass, 1u, 8u);
            RndValue(Dependencies[i].SrcStageMask, PIPELINE_STAGE_FLAG_UNDEFINED, PIPELINE_STAGE_FLAG_DEFAULT, 0x17877 + i);
            RndValue(Dependencies[i].DstStageMask, PIPELINE_STAGE_FLAG_UNDEFINED, PIPELINE_STAGE_FLAG_DEFAULT, 0x17888 + i);
            RndValue(Dependencies[i].SrcAccessMask, ACCESS_FLAG_NONE, ACCESS_FLAG_DEFAULT, 0x17866 + i);
            RndValue(Dependencies[i].DstAccessMask, ACCESS_FLAG_NONE, ACCESS_FLAG_DEFAULT, 0x17899 + i);
        }

        SrcRP.pAttachments  = Attachments;
        SrcRP.pSubpasses    = Subpasses;
        SrcRP.pDependencies = SrcRP.DependencyCount ? Dependencies : nullptr;

        Serializer<SerializerMode::Measure> MSer;
        RPSerializer<SerializerMode::Measure>::SerializeDesc(MSer, SrcRP, nullptr);

        DynamicLinearAllocator Allocator{GetRawAllocator()};
        SerializedData         Data{MSer.GetSize(), GetRawAllocator()};

        Serializer<SerializerMode::Write> WSer{Data};
        RPSerializer<SerializerMode::Write>::SerializeDesc(WSer, SrcRP, nullptr);

        EXPECT_EQ(Data.Size(), WSer.GetSize());

        RenderPassDesc DstRP;

        Serializer<SerializerMode::Read> RSer{Data};
        RPSerializer<SerializerMode::Read>::SerializeDesc(RSer, DstRP, &Allocator);

        EXPECT_TRUE(RSer.IsEnded());
        EXPECT_EQ(SrcRP, DstRP);

    } while (!RndValue.IsComplete());
}

void SerializeShaderCreateInfo(bool UseBytecode)
{
    ShaderCreateInfo RefCI;
    RefCI.Desc.Name                  = "Serialized Shader";
    RefCI.Desc.ShaderType            = SHADER_TYPE_COMPUTE;
    RefCI.EntryPoint                 = "Entry_Point";
    RefCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
    RefCI.ShaderCompiler             = SHADER_COMPILER_GLSLANG;
    RefCI.UseCombinedTextureSamplers = true;
    RefCI.CombinedSamplerSuffix      = "suff";

    constexpr size_t RefBytecodeSize              = 7;
    const Uint8      RefBytecode[RefBytecodeSize] = {42, 13, 179, 211, 97, 65, 71};

    constexpr char   RefSource[]  = "Test shader source";
    constexpr size_t RefSourceLen = sizeof(RefSource);

    if (UseBytecode)
    {
        RefCI.ByteCode     = RefBytecode;
        RefCI.ByteCodeSize = RefBytecodeSize;
    }
    else
    {
        RefCI.Source       = RefSource;
        RefCI.SourceLength = RefSourceLen;
    }

    SerializedData Data;
    {
        Serializer<SerializerMode::Measure> Ser;
        ShaderSerializer<SerializerMode::Measure>::SerializeCI(Ser, RefCI);
        Data = Ser.AllocateData(GetRawAllocator());
    }

    {
        Serializer<SerializerMode::Write> Ser{Data};
        ShaderSerializer<SerializerMode::Write>::SerializeCI(Ser, RefCI);
    }

    ShaderCreateInfo CI;
    {
        Serializer<SerializerMode::Read> Ser{Data};
        ShaderSerializer<SerializerMode::Read>::SerializeCI(Ser, CI);
    }

    // clang-format off
    EXPECT_STREQ(CI.Desc.Name,                  RefCI.Desc.Name);
    EXPECT_EQ   (CI.Desc.ShaderType,            RefCI.Desc.ShaderType);
    EXPECT_STREQ(CI.EntryPoint,                 RefCI.EntryPoint);
    EXPECT_EQ   (CI.SourceLanguage,             RefCI.SourceLanguage);
    EXPECT_EQ   (CI.ShaderCompiler,             RefCI.ShaderCompiler);
    EXPECT_EQ   (CI.UseCombinedTextureSamplers, RefCI.UseCombinedTextureSamplers);
    EXPECT_STREQ(CI.CombinedSamplerSuffix,      RefCI.CombinedSamplerSuffix);
    // clang-format on

    if (UseBytecode)
    {
        EXPECT_EQ(CI.ByteCodeSize, RefBytecodeSize);
        EXPECT_EQ(std::memcmp(CI.ByteCode, RefBytecode, RefBytecodeSize), 0);
    }
    else
    {
        EXPECT_EQ(CI.SourceLength, RefSourceLen);
        EXPECT_STREQ(CI.Source, RefSource);
    }
}

TEST(PSOSerializerTest, SerializeShaderCI_Bytecode)
{
    SerializeShaderCreateInfo(true);
}

TEST(PSOSerializerTest, SerializeShaderCI_Source)
{
    SerializeShaderCreateInfo(false);
}

} // namespace
