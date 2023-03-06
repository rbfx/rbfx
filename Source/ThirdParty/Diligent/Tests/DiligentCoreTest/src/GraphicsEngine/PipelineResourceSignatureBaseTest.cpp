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

#include "../../../../Graphics/GraphicsEngine/include/PipelineResourceSignatureBase.hpp"
#include "CommonlyUsedStates.h"

#include "gtest/gtest.h"

#include <array>

using namespace Diligent;

namespace
{

TEST(PipelineResourceSignatureBaseTest, Compatibility)
{
    {
        PipelineResourceSignatureDesc NullDesc1;
        PipelineResourceSignatureDesc NullDesc2;
        EXPECT_TRUE(PipelineResourceSignaturesCompatible(NullDesc1, NullDesc2));
        EXPECT_EQ(CalculatePipelineResourceSignatureDescHash(NullDesc1), CalculatePipelineResourceSignatureDescHash(NullDesc2));

        NullDesc2.BindingIndex = 1;
        EXPECT_FALSE(PipelineResourceSignaturesCompatible(NullDesc1, NullDesc2));
        EXPECT_NE(CalculatePipelineResourceSignatureDescHash(NullDesc1), CalculatePipelineResourceSignatureDescHash(NullDesc2));
    }

    constexpr PipelineResourceDesc RefRes[] = //
        {
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "Buff", 2u, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS | PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "Tex", 4u, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER | PIPELINE_RESOURCE_FLAG_RUNTIME_ARRAY} //
        };

    const ImmutableSamplerDesc RefSam[] = //
        {
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "SamA", Sam_LinearMirror},
            {SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "SamB", Sam_LinearWrap} //
        };

    PipelineResourceSignatureDesc RefDesc;
    RefDesc.Name                 = "Ref PRS Desc";
    RefDesc.NumResources         = _countof(RefRes);
    RefDesc.Resources            = RefRes;
    RefDesc.NumImmutableSamplers = _countof(RefSam);
    RefDesc.ImmutableSamplers    = RefSam;

    const auto RefHash = CalculatePipelineResourceSignatureDescHash(RefDesc);

    PipelineResourceSignatureDesc       TestDesc;
    std::array<PipelineResourceDesc, 2> TestRes = {};
    std::array<ImmutableSamplerDesc, 2> TestSam = {};

    auto Test = [&](const auto& ModifyTestDesc) //
    {
        TestDesc = {};

        TestDesc.Name = "Test PRS Desc";

        TestRes[0]      = RefRes[0];
        TestRes[1]      = RefRes[1];
        TestRes[0].Name = "OtherBuff";
        TestRes[1].Name = "OtherTex";
        VERIFY_EXPR(TestRes[0] != RefRes[0]);
        VERIFY_EXPR(TestRes[1] != RefRes[1]);

        TestSam[0]                      = RefSam[0];
        TestSam[1]                      = RefSam[1];
        TestSam[0].SamplerOrTextureName = "OtherSamA";
        TestSam[1].SamplerOrTextureName = "OtherSamB";
        VERIFY_EXPR(TestSam[0] != RefSam[0]);
        VERIFY_EXPR(TestSam[1] != RefSam[1]);

        TestDesc.NumResources         = StaticCast<Uint32>(TestRes.size());
        TestDesc.Resources            = TestRes.data();
        TestDesc.NumImmutableSamplers = StaticCast<Uint32>(TestSam.size());
        TestDesc.ImmutableSamplers    = TestSam.data();
        VERIFY_EXPR(TestDesc.Resources != RefDesc.Resources);
        VERIFY_EXPR(TestDesc.ImmutableSamplers != RefDesc.ImmutableSamplers);

        EXPECT_TRUE(PipelineResourceSignaturesCompatible(TestDesc, RefDesc));
        EXPECT_EQ(RefHash, CalculatePipelineResourceSignatureDescHash(TestDesc));

        ModifyTestDesc();

        EXPECT_FALSE(PipelineResourceSignaturesCompatible(TestDesc, RefDesc));
        EXPECT_NE(RefHash, CalculatePipelineResourceSignatureDescHash(TestDesc));
    };

    Test([&TestDesc]() { TestDesc.NumResources = 1; });
    Test([&TestDesc]() { TestDesc.NumImmutableSamplers = 1; });
    Test([&TestDesc]() { TestDesc.BindingIndex = 1; });
    for (SHADER_TYPE ShaderType = static_cast<SHADER_TYPE>(1u); ShaderType <= SHADER_TYPE_LAST; ShaderType = static_cast<SHADER_TYPE>(ShaderType * 2))
    {
        Test([&TestRes, ShaderType]() { TestRes[0].ShaderStages = ShaderType; });
        Test([&TestRes, ShaderType]() { TestRes[1].ShaderStages = ShaderType; });
        Test([&TestSam, ShaderType]() { TestSam[0].ShaderStages = ShaderType; });
        Test([&TestSam, ShaderType]() { TestSam[1].ShaderStages = ShaderType; });
    }

    for (Uint32 ArrSize = 0; ArrSize < 64; ++ArrSize)
    {
        Test([&TestRes, ArrSize]() { TestRes[0].ArraySize = (TestRes[0].ArraySize != ArrSize) ? ArrSize : 128; });
        Test([&TestRes, ArrSize]() { TestRes[1].ArraySize = (TestRes[1].ArraySize != ArrSize) ? ArrSize : 128; });
    }

    {
        size_t Count = 0;
        for (SHADER_RESOURCE_TYPE ResType = SHADER_RESOURCE_TYPE_UNKNOWN;
             ResType <= SHADER_RESOURCE_TYPE_LAST;
             ResType = static_cast<SHADER_RESOURCE_TYPE>(ResType + 1))
        {
            if (TestRes[0].ResourceType != ResType)
            {
                Test([&TestRes, ResType]() { TestRes[0].ResourceType = ResType; });
                ++Count;
            }

            if (TestRes[1].ResourceType != ResType)
            {
                Test([&TestRes, ResType]() { TestRes[1].ResourceType = ResType; });
                ++Count;
            }
        }
        EXPECT_EQ(Count, static_cast<size_t>(SHADER_RESOURCE_TYPE_LAST) * 2);
    }


    {
        size_t Count = 0;
        for (SHADER_RESOURCE_VARIABLE_TYPE VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(0);
             VarType < SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES;
             VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(VarType + 1))
        {
            if (TestRes[0].VarType != VarType)
            {
                Test([&TestRes, VarType]() { TestRes[0].VarType = VarType; });
                ++Count;
            }

            if (TestRes[1].VarType != VarType)
            {
                Test([&TestRes, VarType]() { TestRes[1].VarType = VarType; });
                ++Count;
            }
        }
        EXPECT_EQ(Count, (static_cast<size_t>(SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES) - 1) * 2);
    }

    for (PIPELINE_RESOURCE_FLAGS ResFlags = static_cast<PIPELINE_RESOURCE_FLAGS>(1u);
         ResFlags <= PIPELINE_RESOURCE_FLAG_LAST;
         ResFlags = static_cast<PIPELINE_RESOURCE_FLAGS>(ResFlags * 2))
    {
        Test([&TestRes, ResFlags]() { TestRes[0].Flags = ResFlags; });
        Test([&TestRes, ResFlags]() { TestRes[1].Flags = ResFlags; });
    }
}

} // namespace
