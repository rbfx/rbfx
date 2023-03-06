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

struct ValueIterator
{
    explicit ValueIterator(Int64 CounterLimit = 2048) :
        m_CounterLimit{CounterLimit}
    {
    }

    // Iterates values in the range [RangeMin, RangeMax]
    template <typename T>
    std::enable_if_t<std::is_floating_point<T>::value, T> operator()(T RangeMin, T RangeMax, T Step = 1)
    {
        VERIFY_EXPR(RangeMin < RangeMax);
        VERIFY_EXPR(Step != 0);

        // Index of the last step that reaches RangeMax.
        const auto LastStep = static_cast<Int64>((RangeMax - RangeMin) / Step);
        VERIFY_EXPR(LastStep <= m_CounterLimit);

        m_MaxCounter = std::max(m_MaxCounter, LastStep);
        return std::min(RangeMin + static_cast<T>(m_Counter % (LastStep + 1)) * Step, RangeMax);
    }

    // Iterates values in the range [RangeMin, RangeMax]
    template <typename T>
    std::enable_if_t<!std::is_floating_point<T>::value, T> operator()(T _RangeMin, Int64 RangeMax, Int64 Step = 1)
    {
        const auto RangeMin = static_cast<Int64>(_RangeMin);
        const auto Range    = RangeMax - RangeMin;

        VERIFY_EXPR(Range > 0);
        VERIFY_EXPR(Step != 0);

        // Index of the last step that reaches RangeMax.
        const auto LastStep = std::min(Range / Step, m_CounterLimit);

        m_MaxCounter = std::max(m_MaxCounter, LastStep);
        return static_cast<T>(RangeMin + (Range * m_Counter / LastStep) % (Range + 1));
    }

    bool Bool() const
    {
        return (m_Counter & 0x01) != 0;
    }

    bool IsComplete()
    {
        return ++m_Counter > m_MaxCounter;
    }

private:
    const Int64 m_CounterLimit;

    // The last value of the counter that will be used.
    Int64 m_MaxCounter = 0;

    Int64 m_Counter = 0;
};


TEST(PSOSerializerTest, SerializePRSDesc)
{
    ValueIterator Val;
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

        Resources[0].ShaderStages = Val(SHADER_TYPE_VERTEX, (SHADER_TYPE_LAST << 1) - 1);
        Resources[1].ArraySize    = Val(0u, 100u);
        Resources[2].ResourceType = Val(static_cast<SHADER_RESOURCE_TYPE>(1), SHADER_RESOURCE_TYPE_LAST);
        Resources[3].VarType      = Val(SHADER_RESOURCE_VARIABLE_TYPE_STATIC, SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES - 1);
        Resources[4].Flags        = Val(PIPELINE_RESOURCE_FLAG_NONE, (PIPELINE_RESOURCE_FLAG_LAST << 1) - 1);

        ImmutableSamplerDesc ImmutableSamplers[] = //
            {
                {SHADER_TYPE_UNKNOWN, "Sampler1", SamplerDesc{}},
                {SHADER_TYPE_UNKNOWN, "Sampler2", SamplerDesc{}} //
            };

        ImmutableSamplers[0].ShaderStages            = Val(SHADER_TYPE_PIXEL, (SHADER_TYPE_LAST << 1) - 1);
        ImmutableSamplers[0].Desc.MinFilter          = Val(FILTER_TYPE_UNKNOWN, FILTER_TYPE_NUM_FILTERS - 1);
        ImmutableSamplers[1].Desc.MagFilter          = Val(FILTER_TYPE_UNKNOWN, FILTER_TYPE_NUM_FILTERS - 1);
        ImmutableSamplers[0].Desc.MipFilter          = Val(FILTER_TYPE_UNKNOWN, FILTER_TYPE_NUM_FILTERS - 1);
        ImmutableSamplers[1].Desc.AddressU           = Val(TEXTURE_ADDRESS_UNKNOWN, TEXTURE_ADDRESS_NUM_MODES - 1);
        ImmutableSamplers[0].Desc.AddressV           = Val(TEXTURE_ADDRESS_UNKNOWN, TEXTURE_ADDRESS_NUM_MODES - 1);
        ImmutableSamplers[1].Desc.AddressW           = Val(TEXTURE_ADDRESS_UNKNOWN, TEXTURE_ADDRESS_NUM_MODES - 1);
        ImmutableSamplers[1].Desc.Flags              = Val(SAMPLER_FLAG_NONE, SAMPLER_FLAG_LAST);
        ImmutableSamplers[1].Desc.UnnormalizedCoords = Val.Bool();
        ImmutableSamplers[1].Desc.MipLODBias         = Val(-2.0f, 2.0f);
        ImmutableSamplers[0].Desc.MaxAnisotropy      = Val(0u, 16u);
        ImmutableSamplers[0].Desc.ComparisonFunc     = Val(COMPARISON_FUNC_UNKNOWN, COMPARISON_FUNC_NUM_FUNCTIONS - 1);
        ImmutableSamplers[0].Desc.BorderColor[0]     = Val(0.f, 1.f, 0.1f);
        ImmutableSamplers[0].Desc.BorderColor[1]     = Val(0.f, 1.f, 0.12f);
        ImmutableSamplers[0].Desc.BorderColor[2]     = Val(0.f, 1.f, 0.17f);
        ImmutableSamplers[0].Desc.BorderColor[3]     = Val(0.f, 1.f, 0.08f);
        ImmutableSamplers[1].Desc.MinLOD             = Val(-10.f, 0.f);
        ImmutableSamplers[1].Desc.MaxLOD             = Val(0.f, 10.f);

        PipelineResourceSignatureDesc SrcPRSDesc;
        SrcPRSDesc.Resources            = Resources;
        SrcPRSDesc.NumResources         = _countof(Resources);
        SrcPRSDesc.ImmutableSamplers    = ImmutableSamplers;
        SrcPRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);

        SrcPRSDesc.BindingIndex = Val(Uint8{0}, Uint8{DILIGENT_MAX_RESOURCE_SIGNATURES});

        PipelineResourceSignatureInternalData SrcInternalData;

        SrcInternalData.ShaderStages          = Val(SHADER_TYPE_GEOMETRY, (SHADER_TYPE_LAST << 1) - 1);
        SrcInternalData.StaticResShaderStages = Val(SHADER_TYPE_HULL, (SHADER_TYPE_LAST << 1) - 1);
        SrcInternalData.PipelineType          = Val(PIPELINE_TYPE_GRAPHICS, PIPELINE_TYPE_LAST);

        for (size_t i = 0; i < SrcInternalData.StaticResStageIndex.size(); ++i)
            SrcInternalData.StaticResStageIndex[i] = Val(static_cast<Int8>(i), Int8{127});


        DynamicLinearAllocator Allocator{GetRawAllocator()};
        SerializedData         Data;
        {
            Serializer<SerializerMode::Measure> MSer;
            EXPECT_TRUE(PRSSerializer<SerializerMode::Measure>::SerializeDesc(MSer, SrcPRSDesc, nullptr));
            EXPECT_TRUE(PRSSerializer<SerializerMode::Measure>::SerializeInternalData(MSer, SrcInternalData, nullptr));

            Data = MSer.AllocateData(GetRawAllocator());
        }

        {
            Serializer<SerializerMode::Write> WSer{Data};
            EXPECT_TRUE(PRSSerializer<SerializerMode::Write>::SerializeDesc(WSer, SrcPRSDesc, nullptr));
            EXPECT_TRUE(PRSSerializer<SerializerMode::Write>::SerializeInternalData(WSer, SrcInternalData, nullptr));

            EXPECT_EQ(Data.Size(), WSer.GetSize());
        }

        PipelineResourceSignatureDesc         DstPRSDesc;
        PipelineResourceSignatureInternalData DstInternalData;
        {
            Serializer<SerializerMode::Read> RSer{Data};
            EXPECT_TRUE(PRSSerializer<SerializerMode::Read>::SerializeDesc(RSer, DstPRSDesc, &Allocator));
            EXPECT_TRUE(PRSSerializer<SerializerMode::Read>::SerializeInternalData(RSer, DstInternalData, nullptr));

            EXPECT_TRUE(RSer.IsEnded());
        }

        EXPECT_EQ(SrcPRSDesc, DstPRSDesc);
        EXPECT_EQ(SrcInternalData, DstInternalData);

    } while (!Val.IsComplete());
}

using TPRSNames = DeviceObjectArchive::TPRSNames;

template <typename PSOCIType, typename HelperType>
static void TestSerializePSOCreateInfo(HelperType&& Helper)
{
    const String PRSNames[] = {"PRS-1", "Signature-2", "ResSign-3", "PRS-4", "Signature-5", "ResSign-6"};

    ValueIterator Val;
    do
    {
        TPRSNames SrcPRSNames = {};
        PSOCIType SrcPSO;

        SrcPSO.PSODesc.PipelineType    = Val(PIPELINE_TYPE_GRAPHICS, PIPELINE_TYPE_LAST);
        SrcPSO.Flags                   = Val(PSO_CREATE_FLAG_NONE, (PSO_CREATE_FLAG_LAST << 1) - 1);
        SrcPSO.ResourceSignaturesCount = Val(1u, _countof(PRSNames));

        for (Uint32 i = 0; i < SrcPSO.ResourceSignaturesCount; ++i)
            SrcPRSNames[i] = PRSNames[i].c_str();

        Helper.Init(SrcPSO, Val);

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

    } while (!Val.IsComplete());
}


TEST(PSOSerializerTest, SerializePSOCreateInfo)
{
    struct Helper
    {
        void Init(PipelineStateCreateInfo&, ValueIterator&)
        {}

        void Measure(Serializer<SerializerMode::Measure>& Ser, const PipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            EXPECT_TRUE(PSOSerializer<SerializerMode::Measure>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr));
        }

        void Write(Serializer<SerializerMode::Write>& Ser, const PipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            EXPECT_TRUE(PSOSerializer<SerializerMode::Write>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr));
        }

        void Read(Serializer<SerializerMode::Read>& Ser, PipelineStateCreateInfo& CI, TPRSNames& PRSNames, DynamicLinearAllocator* Allocator)
        {
            EXPECT_TRUE(PSOSerializer<SerializerMode::Read>::SerializeCreateInfo(Ser, CI, PRSNames, Allocator));
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

        void Init(GraphicsPipelineStateCreateInfo& CI, ValueIterator& Val)
        {
            auto& GraphicsPipeline{CI.GraphicsPipeline};

            auto& BlendDesc{GraphicsPipeline.BlendDesc};
            BlendDesc.AlphaToCoverageEnable                  = Val.Bool();
            BlendDesc.IndependentBlendEnable                 = Val.Bool();
            BlendDesc.RenderTargets[0].BlendEnable           = Val.Bool();
            BlendDesc.RenderTargets[0].LogicOperationEnable  = Val.Bool();
            BlendDesc.RenderTargets[0].SrcBlend              = Val(BLEND_FACTOR_UNDEFINED, BLEND_FACTOR_NUM_FACTORS - 1);
            BlendDesc.RenderTargets[0].DestBlend             = Val(BLEND_FACTOR_UNDEFINED, BLEND_FACTOR_NUM_FACTORS - 1);
            BlendDesc.RenderTargets[0].BlendOp               = Val(BLEND_OPERATION_UNDEFINED, BLEND_OPERATION_NUM_OPERATIONS - 1);
            BlendDesc.RenderTargets[0].SrcBlendAlpha         = Val(BLEND_FACTOR_UNDEFINED, BLEND_FACTOR_NUM_FACTORS - 1);
            BlendDesc.RenderTargets[0].DestBlendAlpha        = Val(BLEND_FACTOR_UNDEFINED, BLEND_FACTOR_NUM_FACTORS - 1);
            BlendDesc.RenderTargets[0].BlendOpAlpha          = Val(BLEND_OPERATION_UNDEFINED, BLEND_OPERATION_NUM_OPERATIONS - 1);
            BlendDesc.RenderTargets[0].LogicOp               = Val(LOGIC_OP_CLEAR, LOGIC_OP_NUM_OPERATIONS - 1);
            BlendDesc.RenderTargets[0].RenderTargetWriteMask = Val(COLOR_MASK_NONE, COLOR_MASK_ALL);

            GraphicsPipeline.SampleMask = Val(0u, 0xFFFFFFFFu, 0x2FFFF1u);

            auto& RasterizerDesc{GraphicsPipeline.RasterizerDesc};
            RasterizerDesc.FillMode              = Val(FILL_MODE_UNDEFINED, FILL_MODE_NUM_MODES - 1);
            RasterizerDesc.CullMode              = Val(CULL_MODE_UNDEFINED, CULL_MODE_NUM_MODES - 1);
            RasterizerDesc.FrontCounterClockwise = Val.Bool();
            RasterizerDesc.DepthClipEnable       = Val.Bool();
            RasterizerDesc.ScissorEnable         = Val.Bool();
            RasterizerDesc.AntialiasedLineEnable = Val.Bool();
            RasterizerDesc.DepthBias             = Val(-10, 10);
            RasterizerDesc.DepthBiasClamp        = Val(-10.f, 10.f);
            RasterizerDesc.SlopeScaledDepthBias  = Val(-10.f, 10.f);

            auto& DepthStencilDesc{GraphicsPipeline.DepthStencilDesc};
            DepthStencilDesc.DepthEnable                  = Val.Bool();
            DepthStencilDesc.DepthWriteEnable             = Val.Bool();
            DepthStencilDesc.DepthFunc                    = Val(COMPARISON_FUNC_UNKNOWN, COMPARISON_FUNC_NUM_FUNCTIONS - 1);
            DepthStencilDesc.StencilEnable                = Val.Bool();
            DepthStencilDesc.StencilReadMask              = Val(Uint8{0}, Uint8{0xFF});
            DepthStencilDesc.StencilWriteMask             = Val(Uint8{0}, Uint8{0xFF});
            DepthStencilDesc.FrontFace.StencilFailOp      = Val(STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS - 1);
            DepthStencilDesc.FrontFace.StencilDepthFailOp = Val(STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS - 1);
            DepthStencilDesc.FrontFace.StencilPassOp      = Val(STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS - 1);
            DepthStencilDesc.FrontFace.StencilFunc        = Val(COMPARISON_FUNC_UNKNOWN, COMPARISON_FUNC_NUM_FUNCTIONS - 1);
            DepthStencilDesc.BackFace.StencilFailOp       = Val(STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS - 1);
            DepthStencilDesc.BackFace.StencilDepthFailOp  = Val(STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS - 1);
            DepthStencilDesc.BackFace.StencilPassOp       = Val(STENCIL_OP_UNDEFINED, STENCIL_OP_NUM_OPS - 1);
            DepthStencilDesc.BackFace.StencilFunc         = Val(COMPARISON_FUNC_UNKNOWN, COMPARISON_FUNC_NUM_FUNCTIONS - 1);

            auto& InputLayout{GraphicsPipeline.InputLayout};
            InputLayout.NumElements = Val(0u, 4u);

            if (InputLayout.NumElements > 0)
            {
                LayoutElements.resize(InputLayout.NumElements);
                InputLayout.LayoutElements = LayoutElements.data();

                for (size_t i = 0; i < LayoutElements.size(); ++i)
                {
                    auto& Elem          = LayoutElements[i];
                    Elem.InputIndex     = Val(static_cast<Uint32>(i), 16u);
                    Elem.BufferSlot     = Val(static_cast<Uint32>(i / 2), 4u);
                    Elem.NumComponents  = Val(0u, 4u);
                    Elem.ValueType      = Val(VT_UNDEFINED, VT_NUM_TYPES - 1);
                    Elem.IsNormalized   = Val.Bool();
                    Elem.RelativeOffset = Val(0u, (1u << 12), 128u);
                    if (i == 2)
                        Elem.RelativeOffset = LAYOUT_ELEMENT_AUTO_OFFSET;
                    Elem.Stride = Val(0u, (1u << 10), 128u);
                    if (i == 1)
                        Elem.Stride = LAYOUT_ELEMENT_AUTO_STRIDE;
                    Elem.Frequency            = Val(INPUT_ELEMENT_FREQUENCY_UNDEFINED, INPUT_ELEMENT_FREQUENCY_NUM_FREQUENCIES - 1);
                    Elem.InstanceDataStepRate = Val(1u, 128u);
                }
            }

            GraphicsPipeline.PrimitiveTopology = Val(PRIMITIVE_TOPOLOGY_UNDEFINED, PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES - 1);
            GraphicsPipeline.NumViewports      = Val(Uint8{1}, Uint8{8});
            GraphicsPipeline.SubpassIndex      = Val(Uint8{1}, Uint8{8});
            GraphicsPipeline.ShadingRateFlags  = Val(PIPELINE_SHADING_RATE_FLAG_NONE, (PIPELINE_SHADING_RATE_FLAG_LAST << 1) - 1);
            GraphicsPipeline.NumRenderTargets  = Val(Uint8{1}, Uint8{8});
            for (Uint32 i = 0; i < GraphicsPipeline.NumRenderTargets; ++i)
            {
                GraphicsPipeline.RTVFormats[i] = Val(TEX_FORMAT_UNKNOWN, TEX_FORMAT_NUM_FORMATS - 1, i + 1);
            }
            GraphicsPipeline.SmplDesc.Count   = Val(Uint8{0}, Uint8{64});
            GraphicsPipeline.SmplDesc.Quality = Val(Uint8{0}, Uint8{8});
        }

        void Measure(Serializer<SerializerMode::Measure>& Ser, const GraphicsPipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            const char* RPName = SrcRenderPassName.c_str();
            EXPECT_TRUE(PSOSerializer<SerializerMode::Measure>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr, RPName));
        }

        void Write(Serializer<SerializerMode::Write>& Ser, const GraphicsPipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            const char* RPName = SrcRenderPassName.c_str();
            EXPECT_TRUE(PSOSerializer<SerializerMode::Write>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr, RPName));
        }

        void Read(Serializer<SerializerMode::Read>& Ser, GraphicsPipelineStateCreateInfo& CI, TPRSNames& PRSNames, DynamicLinearAllocator* Allocator)
        {
            const char* RPName = nullptr;
            EXPECT_TRUE(PSOSerializer<SerializerMode::Read>::SerializeCreateInfo(Ser, CI, PRSNames, Allocator, RPName));
            EXPECT_EQ(SrcRenderPassName, RPName);
        }
    };
    TestSerializePSOCreateInfo<GraphicsPipelineStateCreateInfo>(Helper{});
}


TEST(PSOSerializerTest, SerializeComputePSOCreateInfo)
{
    struct Helper
    {
        void Init(ComputePipelineStateCreateInfo&, ValueIterator&)
        {}

        void Measure(Serializer<SerializerMode::Measure>& Ser, const ComputePipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            EXPECT_TRUE(PSOSerializer<SerializerMode::Measure>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr));
        }

        void Write(Serializer<SerializerMode::Write>& Ser, const ComputePipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            EXPECT_TRUE(PSOSerializer<SerializerMode::Write>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr));
        }

        void Read(Serializer<SerializerMode::Read>& Ser, ComputePipelineStateCreateInfo& CI, TPRSNames& PRSNames, DynamicLinearAllocator* Allocator)
        {
            EXPECT_TRUE(PSOSerializer<SerializerMode::Read>::SerializeCreateInfo(Ser, CI, PRSNames, Allocator));
        }
    };
    TestSerializePSOCreateInfo<ComputePipelineStateCreateInfo>(Helper{});
}


TEST(PSOSerializerTest, SerializeTilePSOCreateInfo)
{
    struct Helper
    {
        void Init(TilePipelineStateCreateInfo& CI, ValueIterator& Val)
        {
            CI.TilePipeline.SampleCount      = Val(Uint8{1}, Uint8{64});
            CI.TilePipeline.NumRenderTargets = Val(Uint8{1}, Uint8{8});
            for (Uint32 i = 0; i < CI.TilePipeline.NumRenderTargets; ++i)
            {
                CI.TilePipeline.RTVFormats[i] = Val(TEX_FORMAT_UNKNOWN, TEX_FORMAT_NUM_FORMATS - 1, i + 1);
            }
        }

        void Measure(Serializer<SerializerMode::Measure>& Ser, const TilePipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            EXPECT_TRUE(PSOSerializer<SerializerMode::Measure>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr));
        }

        void Write(Serializer<SerializerMode::Write>& Ser, const TilePipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            EXPECT_TRUE(PSOSerializer<SerializerMode::Write>::SerializeCreateInfo(Ser, CI, PRSNames, nullptr));
        }

        void Read(Serializer<SerializerMode::Read>& Ser, TilePipelineStateCreateInfo& CI, TPRSNames& PRSNames, DynamicLinearAllocator* Allocator)
        {
            EXPECT_TRUE(PSOSerializer<SerializerMode::Read>::SerializeCreateInfo(Ser, CI, PRSNames, Allocator));
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

        void Init(RayTracingPipelineStateCreateInfo& CI, ValueIterator& Val)
        {
            CI.RayTracingPipeline.MaxRecursionDepth = Val(Uint8{0}, Uint8{16});
            CI.RayTracingPipeline.ShaderRecordSize  = Val(Uint16{0}, Uint16{128});

            CI.MaxAttributeSize = Val(0u, 128u);
            CI.MaxPayloadSize   = Val(0u, 128u);

            const auto UseShaderRecord = Val.Bool();
            CI.pShaderRecordName       = UseShaderRecord ? ShaderRecordName.c_str() : nullptr;

            CI.GeneralShaderCount       = Val(1u, 4u);
            CI.TriangleHitShaderCount   = Val(0u, 16u);
            CI.ProceduralHitShaderCount = Val(0u, 8u);

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
            auto res =
                PSOSerializer<SerializerMode::Measure>::SerializeCreateInfo(
                    Ser, CI, PRSNames, nullptr,
                    [](Uint32& outIndex, IShader* const& inShader) { outIndex = static_cast<Uint32>(BitCast<size_t>(inShader)); });
            EXPECT_TRUE(res);
        }

        void Write(Serializer<SerializerMode::Write>& Ser, const RayTracingPipelineStateCreateInfo& CI, const TPRSNames& PRSNames)
        {
            auto res =
                PSOSerializer<SerializerMode::Write>::SerializeCreateInfo(
                    Ser, CI, PRSNames, nullptr,
                    [](Uint32& outIndex, IShader* const& inShader) { outIndex = static_cast<Uint32>(BitCast<size_t>(inShader)); });
            EXPECT_TRUE(res);
        }

        void Read(Serializer<SerializerMode::Read>& Ser, RayTracingPipelineStateCreateInfo& CI, TPRSNames& PRSNames, DynamicLinearAllocator* Allocator)
        {
            auto res =
                PSOSerializer<SerializerMode::Read>::SerializeCreateInfo(
                    Ser, CI, PRSNames, Allocator,
                    [](Uint32& inIndex, IShader*& outShader) { outShader = BitCast<IShader*>(inIndex); });
            EXPECT_TRUE(res);
        }
    };
    TestSerializePSOCreateInfo<RayTracingPipelineStateCreateInfo>(Helper{});
}


TEST(PSOSerializerTest, SerializeRenderPassDesc)
{
    ValueIterator Val;
    do
    {
        RenderPassDesc SrcRP;

        RenderPassAttachmentDesc Attachments[8]  = {};
        SubpassDesc              Subpasses[3]    = {};
        SubpassDependencyDesc    Dependencies[4] = {};
        DynamicLinearAllocator   TmpAlloc{GetRawAllocator()};

        SrcRP.AttachmentCount = Val(1u, _countof(Attachments));
        SrcRP.SubpassCount    = Val(1u, _countof(Subpasses));
        SrcRP.DependencyCount = Val(0u, _countof(Dependencies));

        const auto GenAttachmentIndex = [&SrcRP, &Val](Uint32 Offset = 0) {
            auto Idx = Val(Uint32{0}, SrcRP.AttachmentCount);
            Idx      = (Idx + Offset) % SrcRP.AttachmentCount;
            if (Idx == SrcRP.AttachmentCount)
                Idx = ATTACHMENT_UNUSED;
            return Idx;
        };
        const auto GenState = [&Val](Uint32 Step) {
            return Val(RESOURCE_STATE_UNKNOWN, (RESOURCE_STATE_MAX_BIT << 1) - 1, Step);
        };

        for (Uint32 i = 0; i < SrcRP.AttachmentCount; ++i)
        {
            auto& Attachment{Attachments[i]};
            Attachment.Format         = Val(TEX_FORMAT_UNKNOWN, TEX_FORMAT_NUM_FORMATS, i + 1);
            Attachment.SampleCount    = Val(Uint8{1}, Uint8{32});
            Attachment.LoadOp         = Val(ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_LOAD_OP_COUNT - 1);
            Attachment.StoreOp        = Val(ATTACHMENT_STORE_OP_STORE, ATTACHMENT_STORE_OP_COUNT - 1);
            Attachment.StencilLoadOp  = Val(ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_LOAD_OP_COUNT - 1);
            Attachment.StencilStoreOp = Val(ATTACHMENT_STORE_OP_STORE, ATTACHMENT_STORE_OP_COUNT - 1);
            Attachment.InitialState   = GenState(0xEF + i);
            Attachment.FinalState     = GenState(0x47 + i);
        }
        for (Uint32 i = 0; i < SrcRP.SubpassCount; ++i)
        {
            auto& Subpass{Subpasses[i]};
            Subpass.InputAttachmentCount        = Val(0u, 2u);
            Subpass.RenderTargetAttachmentCount = Val(0u, SrcRP.AttachmentCount);
            Subpass.PreserveAttachmentCount     = Val(0u, SrcRP.AttachmentCount);

            const auto HasDepthStencil       = Val.Bool();
            const auto HasShadingRate        = Val.Bool();
            const auto HasResolveAttachments = Val.Bool();

            auto* pInputAttachments        = TmpAlloc.ConstructArray<AttachmentReference>(Subpass.InputAttachmentCount);
            auto* pRenderTargetAttachments = TmpAlloc.ConstructArray<AttachmentReference>(Subpass.RenderTargetAttachmentCount);
            auto* pResolveAttachments      = TmpAlloc.ConstructArray<AttachmentReference>(HasResolveAttachments ? Subpass.RenderTargetAttachmentCount : 0);
            auto* pPreserveAttachments     = TmpAlloc.ConstructArray<Uint32>(Subpass.PreserveAttachmentCount);

            for (Uint32 j = 0; j < Subpass.InputAttachmentCount; ++j)
            {
                pInputAttachments[i].AttachmentIndex = GenAttachmentIndex(0);
                pInputAttachments[i].State           = GenState(0x55 + j + i * 10);
            }
            for (Uint32 j = 0; j < Subpass.RenderTargetAttachmentCount; ++j)
            {
                pRenderTargetAttachments[i].AttachmentIndex = GenAttachmentIndex(1);
                pRenderTargetAttachments[i].State           = GenState(0x49 + j + i * 16);
            }
            if (pResolveAttachments)
            {
                for (Uint32 j = 0; j < Subpass.RenderTargetAttachmentCount; ++j)
                {
                    pResolveAttachments[i].AttachmentIndex = GenAttachmentIndex(2);
                    pResolveAttachments[i].State           = GenState(0x38 + j + i * 9);
                }
            }
            for (Uint32 j = 0; j < Subpass.PreserveAttachmentCount; ++j)
            {
                pPreserveAttachments[j] = GenAttachmentIndex(3);
            }

            Subpass.pInputAttachments        = pInputAttachments;
            Subpass.pRenderTargetAttachments = pRenderTargetAttachments;
            Subpass.pResolveAttachments      = pResolveAttachments;
            Subpass.pPreserveAttachments     = pPreserveAttachments;

            if (HasDepthStencil)
            {
                auto* pDepthStencilAttachment            = TmpAlloc.Construct<AttachmentReference>();
                Subpass.pDepthStencilAttachment          = pDepthStencilAttachment;
                pDepthStencilAttachment->AttachmentIndex = GenAttachmentIndex(4);
                pDepthStencilAttachment->State           = GenState(0x82 + i);
            }
            if (HasShadingRate)
            {
                auto* pShadingRateAttachment                       = TmpAlloc.Construct<ShadingRateAttachment>();
                Subpass.pShadingRateAttachment                     = pShadingRateAttachment;
                pShadingRateAttachment->TileSize[0]                = Val(0u, 32u, 2);
                pShadingRateAttachment->TileSize[1]                = Val(0u, 64u, 4);
                pShadingRateAttachment->Attachment.AttachmentIndex = GenAttachmentIndex(5);
                pShadingRateAttachment->Attachment.State           = GenState(0x63 + i);
            }
        }
        for (Uint32 i = 0; i < SrcRP.DependencyCount; ++i)
        {
            auto& Dep{Dependencies[i]};
            Dep.SrcSubpass    = Val(0u, 10u);
            Dep.DstSubpass    = Val(1u, 8u);
            Dep.SrcStageMask  = Val(PIPELINE_STAGE_FLAG_UNDEFINED, PIPELINE_STAGE_FLAG_DEFAULT, 0x317877 + i);
            Dep.DstStageMask  = Val(PIPELINE_STAGE_FLAG_UNDEFINED, PIPELINE_STAGE_FLAG_DEFAULT, 0x317888 + i);
            Dep.SrcAccessMask = Val(ACCESS_FLAG_NONE, ACCESS_FLAG_DEFAULT, 0x317866 + i);
            Dep.DstAccessMask = Val(ACCESS_FLAG_NONE, ACCESS_FLAG_DEFAULT, 0x317899 + i);
        }

        SrcRP.pAttachments  = Attachments;
        SrcRP.pSubpasses    = Subpasses;
        SrcRP.pDependencies = SrcRP.DependencyCount ? Dependencies : nullptr;

        Serializer<SerializerMode::Measure> MSer;
        EXPECT_TRUE(RPSerializer<SerializerMode::Measure>::SerializeDesc(MSer, SrcRP, nullptr));

        DynamicLinearAllocator Allocator{GetRawAllocator()};
        SerializedData         Data{MSer.GetSize(), GetRawAllocator()};

        Serializer<SerializerMode::Write> WSer{Data};
        EXPECT_TRUE(RPSerializer<SerializerMode::Write>::SerializeDesc(WSer, SrcRP, nullptr));

        EXPECT_EQ(Data.Size(), WSer.GetSize());

        RenderPassDesc DstRP;

        Serializer<SerializerMode::Read> RSer{Data};
        EXPECT_TRUE(RPSerializer<SerializerMode::Read>::SerializeDesc(RSer, DstRP, &Allocator));

        EXPECT_TRUE(RSer.IsEnded());
        EXPECT_EQ(SrcRP, DstRP);

    } while (!Val.IsComplete());
}

void SerializeShaderCreateInfo(bool UseBytecode)
{
    ShaderCreateInfo RefCI;
    RefCI.Desc           = {"Serialized Shader", SHADER_TYPE_COMPUTE, true, "suff"};
    RefCI.EntryPoint     = "Entry_Point";
    RefCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
    RefCI.ShaderCompiler = SHADER_COMPILER_GLSLANG;

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
        EXPECT_TRUE(ShaderSerializer<SerializerMode::Measure>::SerializeCI(Ser, RefCI));
        Data = Ser.AllocateData(GetRawAllocator());
    }

    {
        Serializer<SerializerMode::Write> Ser{Data};
        EXPECT_TRUE(ShaderSerializer<SerializerMode::Write>::SerializeCI(Ser, RefCI));
    }

    ShaderCreateInfo CI;
    {
        Serializer<SerializerMode::Read> Ser{Data};
        EXPECT_TRUE(ShaderSerializer<SerializerMode::Read>::SerializeCI(Ser, CI));
    }

    // clang-format off
    EXPECT_STREQ(CI.Desc.Name,      RefCI.Desc.Name);
    EXPECT_EQ   (CI.Desc,           RefCI.Desc);
    EXPECT_STREQ(CI.EntryPoint,     RefCI.EntryPoint);
    EXPECT_EQ   (CI.SourceLanguage, RefCI.SourceLanguage);
    EXPECT_EQ   (CI.ShaderCompiler, RefCI.ShaderCompiler);
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
