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

#include <unordered_map>
#include <unordered_set>
#include <array>
#include <vector>

#include "HashUtils.hpp"
#include "XXH128Hasher.hpp"
#include "GraphicsTypesOutputInserters.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(Common_HashUtils, HashMapStringKey)
{
    {
        const char* Str = "Test String";

        HashMapStringKey Key1{Str};
        EXPECT_TRUE(Key1);
        EXPECT_EQ(Key1.GetStr(), Str);
        EXPECT_STREQ(Key1.GetStr(), Str);

        HashMapStringKey Key2{Str, true};
        EXPECT_NE(Key2.GetStr(), Str);
        EXPECT_STREQ(Key2.GetStr(), Str);

        EXPECT_EQ(Key1, Key1);
        EXPECT_EQ(Key2, Key2);
        EXPECT_EQ(Key1, Key2);

        HashMapStringKey Key3{std::string{Str}};
        EXPECT_NE(Key3.GetStr(), Str);
        EXPECT_STREQ(Key3.GetStr(), Str);

        EXPECT_EQ(Key3, Key1);
        EXPECT_EQ(Key3, Key2);
        EXPECT_EQ(Key3, Key3);
    }

    {
        const char*      Str1 = "Test String 1";
        const char*      Str2 = "Test String 2";
        HashMapStringKey Key1{Str1};
        HashMapStringKey Key2{Str2, true};
        EXPECT_NE(Key1, Key2);

        HashMapStringKey Key3{std::move(Key1)};
        EXPECT_NE(Key1, Key2);
        EXPECT_NE(Key2, Key1);

        HashMapStringKey Key4{std::move(Key2)};
        EXPECT_EQ(Key1, Key2);
        EXPECT_EQ(Key2, Key1);
        EXPECT_NE(Key3, Key4);
    }

    {
        std::unordered_map<HashMapStringKey, int> TestMap;

        const char* Str1 = "String1";
        const char* Str2 = "String2";
        const char* Str3 = "String3";
        const int   Val1 = 1;
        const int   Val2 = 2;

        auto it_ins = TestMap.emplace(HashMapStringKey{Str1, true}, Val1);
        EXPECT_TRUE(it_ins.second);
        EXPECT_NE(it_ins.first->first.GetStr(), Str1);
        EXPECT_STREQ(it_ins.first->first.GetStr(), Str1);

        it_ins = TestMap.emplace(Str2, Val2);
        EXPECT_TRUE(it_ins.second);
        EXPECT_EQ(it_ins.first->first, Str2);

        auto it = TestMap.find(Str1);
        ASSERT_NE(it, TestMap.end());
        EXPECT_EQ(it->second, Val1);
        EXPECT_NE(it->first.GetStr(), Str1);
        EXPECT_STREQ(it->first.GetStr(), Str1);

        it = TestMap.find(Str2);
        ASSERT_NE(it, TestMap.end());
        EXPECT_EQ(it->second, Val2);
        EXPECT_EQ(it->first.GetStr(), Str2);

        it = TestMap.find(Str3);
        EXPECT_EQ(it, TestMap.end());

        it = TestMap.find(HashMapStringKey{std::string{Str3}});
        EXPECT_EQ(it, TestMap.end());
    }

    {
        HashMapStringKey Key1;
        EXPECT_FALSE(Key1);

        HashMapStringKey Key2{"Key2", true};
        Key1 = std::move(Key2);
        EXPECT_TRUE(Key1);
        EXPECT_FALSE(Key2);
        EXPECT_STREQ(Key1.GetStr(), "Key2");

        HashMapStringKey Key3{"Key3", true};
        Key1 = Key3.Clone();
        EXPECT_TRUE(Key1);
        EXPECT_TRUE(Key3);
        EXPECT_NE(Key1.GetStr(), Key3.GetStr());
        EXPECT_STREQ(Key1.GetStr(), "Key3");

        Key1.Clear();
        EXPECT_FALSE(Key1);
        EXPECT_EQ(Key1.GetStr(), nullptr);

        Key2 = HashMapStringKey{"Key2"};
        Key1 = Key2.Clone();
        EXPECT_TRUE(Key1);
        EXPECT_TRUE(Key2);
        EXPECT_EQ(Key1.GetStr(), Key2.GetStr());
    }
}

TEST(Common_HashUtils, ComputeHashRaw)
{
    {
        std::array<Uint8, 16> Data{};
        for (Uint8 i = 0; i < Data.size(); ++i)
            Data[i] = 1u + i * 3u;

        std::unordered_set<size_t> Hashes;
        for (size_t start = 0; start < Data.size() - 1; ++start)
        {
            for (size_t size = 1; size <= Data.size() - start; ++size)
            {
                auto Hash = ComputeHashRaw(&Data[start], size);
                EXPECT_NE(Hash, size_t{0});
                auto inserted = Hashes.insert(Hash).second;
                EXPECT_TRUE(inserted) << Hash;
            }
        }
    }

    {
        std::array<Uint8, 16> RefData = {1, 3, 5, 7, 11, 13, 21, 35, 2, 4, 8, 10, 22, 40, 60, 82};
        for (size_t size = 1; size <= RefData.size(); ++size)
        {
            auto RefHash = ComputeHashRaw(RefData.data(), size);
            for (size_t offset = 0; offset < RefData.size() - size; ++offset)
            {
                std::array<Uint8, RefData.size()> Data{};
                std::copy(RefData.begin(), RefData.begin() + size, Data.begin() + offset);
                auto Hash = ComputeHashRaw(&Data[offset], size);
                EXPECT_EQ(RefHash, Hash) << offset << " " << size;
            }
        }
    }
}


template <typename Type>
class StdHasherTestHelper
{
public:
    StdHasherTestHelper(const char* StructName, bool ZeroOut = false) :
        m_StructName{StructName}
    {
        if (ZeroOut)
            memset(&m_Desc, 0, sizeof(m_Desc));
        EXPECT_TRUE(m_Hashes.insert(m_Hasher(m_Desc)).second);
        EXPECT_TRUE(m_Descs.insert(m_Desc).second);
    }

    void Add(const char* Msg)
    {
        EXPECT_TRUE(m_Hashes.insert(m_Hasher(m_Desc)).second) << Msg;
        EXPECT_TRUE(m_Descs.insert(m_Desc).second) << Msg;

        EXPECT_FALSE(m_Desc == m_LastDesc) << Msg;
        EXPECT_TRUE(m_Desc != m_LastDesc) << Msg;
        m_LastDesc = m_Desc;
        EXPECT_TRUE(m_Desc == m_LastDesc) << Msg;
        EXPECT_FALSE(m_Desc != m_LastDesc) << Msg;
    }

    template <typename MemberType>
    void Add(MemberType& Member, const char* MemberName, MemberType Value)
    {
        Member = Value;
        std::stringstream ss;
        ss << m_StructName << '.' << MemberName << '=' << Value;
        Add(ss.str().c_str());
    }

    template <typename MemberType>
    typename std::enable_if<std::is_enum<MemberType>::value, void>::type
    AddRange(MemberType& Member, const char* MemberName, MemberType StartValue, MemberType EndValue)
    {
        for (typename std::underlying_type<MemberType>::type i = StartValue; i < EndValue; ++i)
        {
            Add(Member, MemberName, static_cast<MemberType>(i));
        }
    }

    template <typename MemberType>
    typename std::enable_if<std::is_floating_point<MemberType>::value || std::is_integral<MemberType>::value, void>::type
    AddRange(MemberType& Member, const char* MemberName, MemberType StartValue, MemberType EndValue, MemberType Step = MemberType{1})
    {
        for (auto i = StartValue; i <= EndValue; i += Step)
        {
            Add(Member, MemberName, static_cast<MemberType>(i));
            if (i == EndValue)
                break;
        }
    }

    void AddBool(bool& Member, const char* MemberName)
    {
        Add(Member, MemberName, !Member);
    }

    template <typename MemberType>
    void AddFlags(MemberType& Member, const char* MemberName, MemberType StartValue, MemberType EndValue)
    {
        for (Uint64 i = StartValue; i <= EndValue; i *= 2)
        {
            Add(Member, MemberName, static_cast<MemberType>(i));
            if (i == EndValue)
                break;
        }
    }

    template <typename MemberType>
    void AddStrings(MemberType& Member, const char* MemberName, const std::vector<const char*>& Strings)
    {
        for (const auto* Str : Strings)
        {
            Add(Member, MemberName, Str);
        }
    }

    Type& Get()
    {
        return m_Desc;
    }
    operator Type&()
    {
        return Get();
    }

private:
    const char* const m_StructName;

    Type m_Desc;
    Type m_LastDesc;

    std::hash<Type>            m_Hasher;
    std::unordered_set<size_t> m_Hashes;
    std::unordered_set<Type>   m_Descs;
};


template <typename Type>
class XXH128HasherTestHelper
{
public:
    XXH128HasherTestHelper(const char* StructName, bool ZeroOut = false) :
        m_StructName{StructName}
    {
        if (ZeroOut)
            memset(&m_Desc, 0, sizeof(m_Desc));
    }

    void Add(const char* Msg)
    {
        XXH128State Hasher;
        Hasher.Update(m_Desc);
        EXPECT_TRUE(m_Hashes.insert(Hasher.Digest()).second) << Msg;
    }

    template <typename MemberType>
    void Add(MemberType& Member, const char* MemberName, MemberType Value)
    {
        Member = Value;
        std::stringstream ss;
        ss << m_StructName << '.' << MemberName << '=' << Value;
        Add(ss.str().c_str());
    }

    template <typename MemberType>
    typename std::enable_if<std::is_enum<MemberType>::value, void>::type
    AddRange(MemberType& Member, const char* MemberName, MemberType StartValue, MemberType EndValue)
    {
        for (typename std::underlying_type<MemberType>::type i = StartValue; i < EndValue; ++i)
        {
            Add(Member, MemberName, static_cast<MemberType>(i));
        }
    }

    template <typename MemberType>
    typename std::enable_if<std::is_floating_point<MemberType>::value || std::is_integral<MemberType>::value, void>::type
    AddRange(MemberType& Member, const char* MemberName, MemberType StartValue, MemberType EndValue, MemberType Step = MemberType{1})
    {
        for (auto i = StartValue; i <= EndValue; i += Step)
        {
            Add(Member, MemberName, static_cast<MemberType>(i));
            if (i == EndValue)
                break;
        }
    }

    void AddBool(bool& Member, const char* MemberName)
    {
        Add(Member, MemberName, !Member);
    }

    template <typename MemberType>
    void AddFlags(MemberType& Member, const char* MemberName, MemberType StartValue, MemberType EndValue)
    {
        for (Uint64 i = StartValue; i <= EndValue; i *= 2)
        {
            Add(Member, MemberName, static_cast<MemberType>(i));
            if (i == EndValue)
                break;
        }
    }

    template <typename MemberType>
    void AddStrings(MemberType& Member, const char* MemberName, const std::vector<const char*>& Strings)
    {
        for (const auto* Str : Strings)
        {
            Add(Member, MemberName, Str);
        }
    }

    Type& Get()
    {
        return m_Desc;
    }
    operator Type&()
    {
        return Get();
    }

private:
    const char* const m_StructName;

    Type m_Desc;

    std::unordered_set<XXH128Hash> m_Hashes;
};

#define DEFINE_HELPER(Type, ...) \
    HelperType<Type> Helper { #Type, ##__VA_ARGS__ }
#define TEST_VALUE(Member, ...)   Helper.Add(Helper.Get().Member, #Member, __VA_ARGS__)
#define TEST_RANGE(Member, ...)   Helper.AddRange(Helper.Get().Member, #Member, __VA_ARGS__)
#define TEST_BOOL(Member, ...)    Helper.AddBool(Helper.Get().Member, #Member)
#define TEST_FLAGS(Member, ...)   Helper.AddFlags(Helper.Get().Member, #Member, __VA_ARGS__)
#define TEST_STRINGS(Member, ...) Helper.AddStrings(Helper.Get().Member, #Member, {__VA_ARGS__})

template <template <typename T> class HelperType>
void TestSamplerDescHasher()
{
    ASSERT_SIZEOF64(SamplerDesc, 56, "Did you add new members to SamplerDesc? Please update the tests.");
    DEFINE_HELPER(SamplerDesc, true);

    TEST_RANGE(MinFilter, static_cast<FILTER_TYPE>(1), FILTER_TYPE_NUM_FILTERS);
    TEST_RANGE(MagFilter, static_cast<FILTER_TYPE>(1), FILTER_TYPE_NUM_FILTERS);
    TEST_RANGE(MipFilter, static_cast<FILTER_TYPE>(1), FILTER_TYPE_NUM_FILTERS);

    TEST_RANGE(AddressU, static_cast<TEXTURE_ADDRESS_MODE>(1), TEXTURE_ADDRESS_NUM_MODES);
    TEST_RANGE(AddressV, static_cast<TEXTURE_ADDRESS_MODE>(1), TEXTURE_ADDRESS_NUM_MODES);
    TEST_RANGE(AddressW, static_cast<TEXTURE_ADDRESS_MODE>(1), TEXTURE_ADDRESS_NUM_MODES);

    TEST_FLAGS(Flags, static_cast<SAMPLER_FLAGS>(1), SAMPLER_FLAG_LAST);
    TEST_BOOL(UnnormalizedCoords);
    TEST_RANGE(MipLODBias, -10.125f, +10.f, 0.25f);

    TEST_RANGE(MaxAnisotropy, 1u, 16u);
    TEST_RANGE(ComparisonFunc, static_cast<COMPARISON_FUNCTION>(1), COMPARISON_FUNC_NUM_FUNCTIONS);
    TEST_RANGE(BorderColor[0], 1.f, 10.f, 0.25f);
    TEST_RANGE(BorderColor[1], 1.f, 10.f, 0.25f);
    TEST_RANGE(BorderColor[2], 1.f, 10.f, 0.25f);
    TEST_RANGE(BorderColor[3], 1.f, 10.f, 0.25f);
    TEST_RANGE(MinLOD, -10.125f, +10.f, 0.25f);
    TEST_RANGE(MaxLOD, -10.125f, +10.f, 0.25f);
}

TEST(Common_HashUtils, SamplerDescStdHash)
{
    TestSamplerDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, SamplerDescXXH128Hash)
{
    TestSamplerDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestStencilOpDescHasher()
{
    ASSERT_SIZEOF(StencilOpDesc, 4, "Did you add new members to StencilOpDesc? Please update the tests.");
    DEFINE_HELPER(StencilOpDesc, true);

    TEST_RANGE(StencilFailOp, static_cast<STENCIL_OP>(1), STENCIL_OP_NUM_OPS);
    TEST_RANGE(StencilDepthFailOp, static_cast<STENCIL_OP>(1), STENCIL_OP_NUM_OPS);
    TEST_RANGE(StencilPassOp, static_cast<STENCIL_OP>(1), STENCIL_OP_NUM_OPS);
    TEST_RANGE(StencilFunc, static_cast<COMPARISON_FUNCTION>(1), COMPARISON_FUNC_NUM_FUNCTIONS);
}

TEST(Common_HashUtils, StencilOpDescStdHash)
{
    TestStencilOpDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, StencilOpDescXXH128Hash)
{
    TestStencilOpDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestDepthStencilStateDescHasher()
{
    ASSERT_SIZEOF(DepthStencilStateDesc, 14, "Did you add new members to StencilOpDesc? Please update the tests.");
    DEFINE_HELPER(DepthStencilStateDesc, true);

    TEST_BOOL(DepthEnable);
    TEST_BOOL(DepthWriteEnable);
    TEST_RANGE(DepthFunc, static_cast<COMPARISON_FUNCTION>(1), COMPARISON_FUNC_NUM_FUNCTIONS);
    TEST_BOOL(StencilEnable);
    TEST_RANGE(StencilReadMask, Uint8{1u}, Uint8{255u});
    TEST_RANGE(StencilWriteMask, Uint8{1u}, Uint8{255u});
}

TEST(Common_HashUtils, DepthStencilStateDescStdHash)
{
    TestDepthStencilStateDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, DepthStencilStateDescXXH128Hash)
{
    TestDepthStencilStateDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestRasterizerStateDesc()
{
    ASSERT_SIZEOF(RasterizerStateDesc, 20, "Did you add new members to RasterizerStateDesc? Please update the tests.");
    DEFINE_HELPER(RasterizerStateDesc, true);

    TEST_RANGE(FillMode, static_cast<FILL_MODE>(1), FILL_MODE_NUM_MODES);
    TEST_RANGE(CullMode, static_cast<CULL_MODE>(1), CULL_MODE_NUM_MODES);
    TEST_BOOL(FrontCounterClockwise);
    TEST_BOOL(DepthClipEnable);
    TEST_BOOL(ScissorEnable);
    TEST_BOOL(AntialiasedLineEnable);
    TEST_RANGE(DepthBias, -33, +32, 2);
    TEST_RANGE(DepthBiasClamp, -32.125f, +32.f, 0.25f);
    TEST_RANGE(SlopeScaledDepthBias, -16.0625f, +16.f, 0.125f);
}

TEST(Common_HashUtils, RasterizerStateDescStdHash)
{
    TestRasterizerStateDesc<StdHasherTestHelper>();
}

TEST(Common_HashUtils, RasterizerStateDescXXH128Hash)
{
    TestRasterizerStateDesc<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestBlendStateDescHasher()
{
    ASSERT_SIZEOF(BlendStateDesc, 82, "Did you add new members to RasterizerStateDesc? Please update the tests.");
    DEFINE_HELPER(BlendStateDesc, true);

    TEST_BOOL(AlphaToCoverageEnable);
    TEST_BOOL(IndependentBlendEnable);

    for (Uint32 rt = 0; rt < DILIGENT_MAX_RENDER_TARGETS; ++rt)
    {
        TEST_BOOL(RenderTargets[rt].BlendEnable);
        TEST_BOOL(RenderTargets[rt].LogicOperationEnable);
        TEST_RANGE(RenderTargets[rt].SrcBlend, static_cast<BLEND_FACTOR>(1), BLEND_FACTOR_NUM_FACTORS);
        TEST_RANGE(RenderTargets[rt].DestBlend, static_cast<BLEND_FACTOR>(1), BLEND_FACTOR_NUM_FACTORS);
        TEST_RANGE(RenderTargets[rt].BlendOp, static_cast<BLEND_OPERATION>(1), BLEND_OPERATION_NUM_OPERATIONS);
        TEST_RANGE(RenderTargets[rt].SrcBlendAlpha, static_cast<BLEND_FACTOR>(1), BLEND_FACTOR_NUM_FACTORS);
        TEST_RANGE(RenderTargets[rt].DestBlendAlpha, static_cast<BLEND_FACTOR>(1), BLEND_FACTOR_NUM_FACTORS);
        TEST_RANGE(RenderTargets[rt].BlendOpAlpha, static_cast<BLEND_OPERATION>(1), BLEND_OPERATION_NUM_OPERATIONS);
        TEST_RANGE(RenderTargets[rt].LogicOp, static_cast<LOGIC_OPERATION>(1), LOGIC_OP_NUM_OPERATIONS);
        TEST_RANGE(RenderTargets[rt].RenderTargetWriteMask, static_cast<COLOR_MASK>(1), static_cast<COLOR_MASK>(COLOR_MASK_ALL + 1));
    }
}

TEST(Common_HashUtils, BlendStateDescStdHash)
{
    TestBlendStateDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, BlendStateDescXXH128Hash)
{
    TestBlendStateDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestTextureViewDescHasher()
{
    ASSERT_SIZEOF64(TextureViewDesc, 40, "Did you add new members to TextureViewDesc? Please update the tests.");
    DEFINE_HELPER(TextureViewDesc);

    TEST_RANGE(ViewType, static_cast<TEXTURE_VIEW_TYPE>(1), TEXTURE_VIEW_NUM_VIEWS);
    TEST_RANGE(TextureDim, static_cast<RESOURCE_DIMENSION>(1), RESOURCE_DIM_NUM_DIMENSIONS);
    TEST_RANGE(Format, static_cast<TEXTURE_FORMAT>(1), TEX_FORMAT_NUM_FORMATS);
    TEST_RANGE(MostDetailedMip, 1u, 32u);
    TEST_RANGE(NumMipLevels, 1u, 32u);
    TEST_RANGE(FirstArraySlice, 1u, 32u);
    TEST_RANGE(NumArraySlices, 1u, 2048u);
    TEST_FLAGS(AccessFlags, static_cast<UAV_ACCESS_FLAG>(1u), UAV_ACCESS_FLAG_LAST);
    TEST_FLAGS(Flags, static_cast<TEXTURE_VIEW_FLAGS>(1u), TEXTURE_VIEW_FLAG_LAST);
    TEST_RANGE(Swizzle.R, static_cast<TEXTURE_COMPONENT_SWIZZLE>(1), TEXTURE_COMPONENT_SWIZZLE_COUNT);
    TEST_RANGE(Swizzle.G, static_cast<TEXTURE_COMPONENT_SWIZZLE>(1), TEXTURE_COMPONENT_SWIZZLE_COUNT);
    TEST_RANGE(Swizzle.B, static_cast<TEXTURE_COMPONENT_SWIZZLE>(1), TEXTURE_COMPONENT_SWIZZLE_COUNT);
    TEST_RANGE(Swizzle.A, static_cast<TEXTURE_COMPONENT_SWIZZLE>(1), TEXTURE_COMPONENT_SWIZZLE_COUNT);
}

TEST(Common_HashUtils, TextureViewDescStdHash)
{
    TestTextureViewDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, TextureViewDescXXH128Hash)
{
    TestTextureViewDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestSampleDescHasher()
{
    ASSERT_SIZEOF(SampleDesc, 2, "Did you add new members to SampleDesc? Please update the tests.");
    DEFINE_HELPER(SampleDesc);

    TEST_RANGE(Count, Uint8{2u}, Uint8{255u});
    TEST_RANGE(Quality, Uint8{1u}, Uint8{255u});
}

TEST(Common_HashUtils, SampleDescStdHash)
{
    TestSampleDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, SampleDescXXH128Hash)
{
    TestSampleDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestShaderResourceVariableDescHasher()
{
    ASSERT_SIZEOF64(ShaderResourceVariableDesc, 16, "Did you add new members to ShaderResourceVariableDesc? Please update the tests.");
    DEFINE_HELPER(ShaderResourceVariableDesc);

    TEST_STRINGS(Name, "Name1", "Name2", "Name3");
    TEST_FLAGS(ShaderStages, static_cast<SHADER_TYPE>(1), SHADER_TYPE_LAST);
    TEST_RANGE(Type, static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(1), SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES);
    TEST_FLAGS(Flags, static_cast<SHADER_VARIABLE_FLAGS>(1), SHADER_VARIABLE_FLAG_LAST);
}

TEST(Common_HashUtils, ShaderResourceVariableStdHasher)
{
    TestShaderResourceVariableDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, ShaderResourceVariableXXH128Hasher)
{
    TestShaderResourceVariableDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestImmutableSamplerDescHasher()
{
    ASSERT_SIZEOF64(ImmutableSamplerDesc, 16 + sizeof(SamplerDesc), "Did you add new members to ImmutableSamplerDesc? Please update the tests.");
    DEFINE_HELPER(ImmutableSamplerDesc);

    TEST_FLAGS(ShaderStages, static_cast<SHADER_TYPE>(1), SHADER_TYPE_LAST);
    TEST_STRINGS(SamplerOrTextureName, "Name1", "Name2", "Name3");
}

TEST(Common_HashUtils, ImmutableSamplerDescStdHash)
{
    TestImmutableSamplerDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, ImmutableSamplerDescXXH128Hash)
{
    TestImmutableSamplerDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestPipelineResourceDescHasher()
{
    ASSERT_SIZEOF64(PipelineResourceDesc, 24, "Did you add new members to PipelineResourceDesc? Please update the tests.");
    DEFINE_HELPER(PipelineResourceDesc);
    Helper.Get().VarType = static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(0);

    TEST_STRINGS(Name, "Name1", "Name2", "Name3");
    TEST_FLAGS(ShaderStages, static_cast<SHADER_TYPE>(1), SHADER_TYPE_LAST);
    TEST_RANGE(ArraySize, 2u, 2048u);
    TEST_RANGE(ResourceType, static_cast<SHADER_RESOURCE_TYPE>(1), static_cast<SHADER_RESOURCE_TYPE>(SHADER_RESOURCE_TYPE_LAST + 1));
    TEST_RANGE(VarType, static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(1), SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES);
    TEST_FLAGS(Flags, static_cast<PIPELINE_RESOURCE_FLAGS>(1), PIPELINE_RESOURCE_FLAG_LAST);
}

TEST(Common_HashUtils, PipelineResourceDescStdHash)
{
    TestPipelineResourceDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, PipelineResourceDescXXH128Hash)
{
    TestPipelineResourceDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestPipelineResourceLayoutDescHasher()
{
    ASSERT_SIZEOF64(PipelineResourceLayoutDesc, 40, "Did you add new members to PipelineResourceLayoutDesc? Please update the tests.");
    DEFINE_HELPER(PipelineResourceLayoutDesc);

    TEST_RANGE(DefaultVariableType, static_cast<SHADER_RESOURCE_VARIABLE_TYPE>(1), SHADER_RESOURCE_VARIABLE_TYPE_NUM_TYPES);
    TEST_FLAGS(DefaultVariableMergeStages, static_cast<SHADER_TYPE>(1), SHADER_TYPE_LAST);

    constexpr ShaderResourceVariableDesc Vars[] =
        {
            {SHADER_TYPE_VERTEX, "Var1", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE, SHADER_VARIABLE_FLAG_NO_DYNAMIC_BUFFERS},
            {SHADER_TYPE_PIXEL, "Var2", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, SHADER_VARIABLE_FLAG_GENERAL_INPUT_ATTACHMENT},
        };
    Helper.Get().Variables = Vars;
    TEST_VALUE(NumVariables, 1u);
    TEST_VALUE(NumVariables, 2u);

    constexpr ImmutableSamplerDesc ImtblSamplers[] = //
        {
            {SHADER_TYPE_VERTEX, "Sam1", SamplerDesc{}},
            {SHADER_TYPE_PIXEL, "Sam2", SamplerDesc{}} //
        };
    Helper.Get().ImmutableSamplers = ImtblSamplers;
    TEST_VALUE(NumImmutableSamplers, 1u);
    TEST_VALUE(NumImmutableSamplers, 2u);
}

TEST(Common_HashUtils, PipelineResourceLayoutDescStdHash)
{
    TestPipelineResourceLayoutDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, PipelineResourceLayoutDescXXH128Hash)
{
    TestPipelineResourceLayoutDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestRenderPassAttachmentDescHasher()
{
    ASSERT_SIZEOF(RenderPassAttachmentDesc, 16, "Did you add new members to RenderPassAttachmentDesc? Please update the tests.");
    DEFINE_HELPER(RenderPassAttachmentDesc);

    TEST_RANGE(Format, static_cast<TEXTURE_FORMAT>(1), TEX_FORMAT_NUM_FORMATS);
    TEST_RANGE(SampleCount, Uint8{2u}, Uint8{32u});
    TEST_RANGE(LoadOp, static_cast<ATTACHMENT_LOAD_OP>(1), ATTACHMENT_LOAD_OP_COUNT);
    TEST_RANGE(StoreOp, static_cast<ATTACHMENT_STORE_OP>(1), ATTACHMENT_STORE_OP_COUNT);
    TEST_RANGE(StencilLoadOp, static_cast<ATTACHMENT_LOAD_OP>(1), ATTACHMENT_LOAD_OP_COUNT);
    TEST_RANGE(StencilStoreOp, static_cast<ATTACHMENT_STORE_OP>(1), ATTACHMENT_STORE_OP_COUNT);
    TEST_FLAGS(InitialState, static_cast<RESOURCE_STATE>(1), RESOURCE_STATE_MAX_BIT);
    TEST_FLAGS(FinalState, static_cast<RESOURCE_STATE>(1), RESOURCE_STATE_MAX_BIT);
}

TEST(Common_HashUtils, RenderPassAttachmentDescStdHash)
{
    TestRenderPassAttachmentDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, RenderPassAttachmentXXH128Hasher)
{
    TestRenderPassAttachmentDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestAttachmentReferenceHasher()
{
    ASSERT_SIZEOF(AttachmentReference, 8, "Did you add new members to AttachmentReference? Please update the tests.");
    DEFINE_HELPER(AttachmentReference);

    TEST_RANGE(AttachmentIndex, 1u, 8u);
    TEST_FLAGS(State, static_cast<RESOURCE_STATE>(1), RESOURCE_STATE_MAX_BIT);
}

TEST(Common_HashUtils, AttachmentReferenceStdHash)
{
    TestAttachmentReferenceHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, AttachmentReferenceXXH128Hash)
{
    TestAttachmentReferenceHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestShadingRateAttachmentHasher()
{
    ASSERT_SIZEOF(ShadingRateAttachment, 16, "Did you add new members to ShadingRateAttachment? Please update the tests.");
    DEFINE_HELPER(ShadingRateAttachment);

    TEST_VALUE(Attachment, AttachmentReference{1, RESOURCE_STATE_RENDER_TARGET});
    TEST_VALUE(Attachment, AttachmentReference{2, RESOURCE_STATE_UNORDERED_ACCESS});

    TEST_RANGE(TileSize[0], 1u, 32u);
    TEST_RANGE(TileSize[1], 1u, 32u);
}

TEST(Common_HashUtils, ShadingRateAttachmentStdHash)
{
    TestShadingRateAttachmentHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, ShadingRateAttachmentXXH128Hash)
{
    TestShadingRateAttachmentHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestSubpassDescHasher()
{
    ASSERT_SIZEOF64(SubpassDesc, 72, "Did you add new members to SubpassDesc? Please update the tests.");
    DEFINE_HELPER(SubpassDesc);

    constexpr AttachmentReference Inputs[] =
        {
            {1, RESOURCE_STATE_INPUT_ATTACHMENT},
            {3, RESOURCE_STATE_INPUT_ATTACHMENT},
            {5, RESOURCE_STATE_INPUT_ATTACHMENT},
        };
    Helper.Get().pInputAttachments = Inputs;
    TEST_VALUE(InputAttachmentCount, 1u);
    TEST_VALUE(InputAttachmentCount, 2u);
    TEST_VALUE(InputAttachmentCount, 3u);

    constexpr AttachmentReference RenderTargets[] =
        {
            {2, RESOURCE_STATE_RENDER_TARGET},
            {4, RESOURCE_STATE_UNORDERED_ACCESS},
            {6, RESOURCE_STATE_COMMON},
        };
    Helper.Get().pRenderTargetAttachments = RenderTargets;
    TEST_VALUE(RenderTargetAttachmentCount, 1u);
    TEST_VALUE(RenderTargetAttachmentCount, 2u);
    TEST_VALUE(RenderTargetAttachmentCount, 3u);

    constexpr AttachmentReference ResolveTargets[] =
        {
            {7, RESOURCE_STATE_RENDER_TARGET},
            {8, RESOURCE_STATE_UNORDERED_ACCESS},
            {9, RESOURCE_STATE_COMMON},
        };
    Helper.Get().pResolveAttachments = ResolveTargets;
    TEST_VALUE(RenderTargetAttachmentCount, 1u);
    TEST_VALUE(RenderTargetAttachmentCount, 2u);
    TEST_VALUE(RenderTargetAttachmentCount, 3u);

    constexpr AttachmentReference DepthStencil{10, RESOURCE_STATE_DEPTH_WRITE};
    TEST_VALUE(pDepthStencilAttachment, &DepthStencil);

    constexpr Uint32 Preserves[]      = {3, 4, 7};
    Helper.Get().pPreserveAttachments = Preserves;
    TEST_VALUE(PreserveAttachmentCount, 1u);
    TEST_VALUE(PreserveAttachmentCount, 2u);
    TEST_VALUE(PreserveAttachmentCount, 3u);

    constexpr ShadingRateAttachment SRA{{5, RESOURCE_STATE_SHADING_RATE}, 32, 64};
    TEST_VALUE(pShadingRateAttachment, &SRA);
}

TEST(Common_HashUtils, SubpassDescStdHash)
{
    TestSubpassDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, SubpassDescXXH128Hash)
{
    TestSubpassDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestSubpassDependencyDescHasher()
{
    ASSERT_SIZEOF64(SubpassDependencyDesc, 24, "Did you add new members to SubpassDependencyDesc? Please update the tests.");
    DEFINE_HELPER(SubpassDependencyDesc);

    TEST_RANGE(SrcSubpass, 1u, 32u);
    TEST_RANGE(DstSubpass, 1u, 32u);
    TEST_FLAGS(SrcStageMask, static_cast<PIPELINE_STAGE_FLAGS>(1), PIPELINE_STAGE_FLAG_DEFAULT);
    TEST_FLAGS(DstStageMask, static_cast<PIPELINE_STAGE_FLAGS>(1), PIPELINE_STAGE_FLAG_DEFAULT);
    TEST_FLAGS(SrcAccessMask, static_cast<ACCESS_FLAGS>(1), ACCESS_FLAG_DEFAULT);
    TEST_FLAGS(DstAccessMask, static_cast<ACCESS_FLAGS>(1), ACCESS_FLAG_DEFAULT);
}

TEST(Common_HashUtils, SubpassDependencyDescStdHash)
{
    TestSubpassDependencyDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, SubpassDependencyDescXXH128Hash)
{
    TestSubpassDependencyDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestRenderPassDescHasher()
{
    ASSERT_SIZEOF64(RenderPassDesc, 56, "Did you add new members to RenderPassDesc? Please update the tests.");
    DEFINE_HELPER(RenderPassDesc);

    RenderPassAttachmentDesc Attachments[3];
    Helper.Get().pAttachments = Attachments;
    TEST_VALUE(AttachmentCount, 1u);
    TEST_VALUE(AttachmentCount, 2u);
    TEST_VALUE(AttachmentCount, 3u);

    SubpassDesc Subpasses[3];
    Helper.Get().pSubpasses = Subpasses;
    TEST_VALUE(SubpassCount, 1u);
    TEST_VALUE(SubpassCount, 2u);
    TEST_VALUE(SubpassCount, 3u);

    SubpassDependencyDesc Deps[3];
    Helper.Get().pDependencies = Deps;
    TEST_VALUE(DependencyCount, 1u);
    TEST_VALUE(DependencyCount, 2u);
    TEST_VALUE(DependencyCount, 3u);
}

TEST(Common_HashUtils, RenderPassDescStdHash)
{
    TestRenderPassDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, RenderPassDescXXH128Hash)
{
    TestRenderPassDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestLayoutElementHasher()
{
    ASSERT_SIZEOF64(LayoutElement, 40, "Did you add new members to LayoutElement? Please update the tests.");
    DEFINE_HELPER(LayoutElement);
    Helper.Get().ValueType = VT_UNDEFINED;
    Helper.Get().Frequency = INPUT_ELEMENT_FREQUENCY_UNDEFINED;

    TEST_STRINGS(HLSLSemantic, "ATTRIB1", "ATTRIB2", "ATTRIB3");
    TEST_RANGE(InputIndex, 1u, 32u);
    TEST_RANGE(BufferSlot, 1u, 32u);
    TEST_RANGE(NumComponents, 1u, 8u);
    TEST_RANGE(ValueType, static_cast<VALUE_TYPE>(1), VT_NUM_TYPES);
    TEST_BOOL(IsNormalized);
    TEST_RANGE(RelativeOffset, 1u, 1024u, 32u);
    TEST_RANGE(Stride, 16u, 1024u, 32u);
    TEST_RANGE(Frequency, static_cast<INPUT_ELEMENT_FREQUENCY>(1), INPUT_ELEMENT_FREQUENCY_NUM_FREQUENCIES);
    TEST_RANGE(InstanceDataStepRate, 2u, 64u);
}

TEST(Common_HashUtils, LayoutElementStdHash)
{
    TestLayoutElementHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, LayoutElementXXH128Hash)
{
    TestLayoutElementHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestInputLayoutDescHasher()
{
    ASSERT_SIZEOF64(InputLayoutDesc, 16, "Did you add new members to InputLayoutDesc? Please update the tests.");
    DEFINE_HELPER(InputLayoutDesc);

    constexpr LayoutElement LayoutElems[] =
        {
            LayoutElement{0, 0, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
            LayoutElement{1, 0, 4, VT_UINT32, False, INPUT_ELEMENT_FREQUENCY_PER_VERTEX},
            LayoutElement{2, 1, 3, VT_UINT16, False, INPUT_ELEMENT_FREQUENCY_PER_VERTEX},
            LayoutElement{3, 3, 3, VT_UINT8, True, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE},
            LayoutElement{4, 5, 1, VT_INT8, True, INPUT_ELEMENT_FREQUENCY_PER_VERTEX},
        };
    Helper.Get().LayoutElements = LayoutElems;
    TEST_RANGE(NumElements, 1u, Uint32{_countof(LayoutElems)}, 1u);
}

TEST(Common_HashUtils, InputLayoutDescStdHash)
{
    TestInputLayoutDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, InputLayoutDescXXH128Hash)
{
    TestInputLayoutDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestGraphicsPipelineDescHasher()
{
    DEFINE_HELPER(GraphicsPipelineDesc);
    Helper.Get().PrimitiveTopology = PRIMITIVE_TOPOLOGY_UNDEFINED;

    TEST_FLAGS(SampleMask, 1u, 0xFFFFFFFFu);

    Helper.Get().BlendDesc.AlphaToCoverageEnable = True;
    Helper.Add("BlendDesc");

    Helper.Get().RasterizerDesc.ScissorEnable = True;
    Helper.Add("RasterizerDesc");

    Helper.Get().DepthStencilDesc.StencilEnable = True;
    Helper.Add("DepthStencilDesc");

    constexpr LayoutElement LayoutElems[] = {LayoutElement{0, 0, 4, VT_FLOAT32, False, INPUT_ELEMENT_FREQUENCY_PER_INSTANCE}};
    Helper.Get().InputLayout              = {LayoutElems, 1};
    Helper.Add("InputLayout");

    TEST_RANGE(PrimitiveTopology, static_cast<PRIMITIVE_TOPOLOGY>(1), PRIMITIVE_TOPOLOGY_NUM_TOPOLOGIES);
    TEST_RANGE(NumRenderTargets, Uint8{1u}, Uint8{8u});
    TEST_RANGE(NumViewports, Uint8{2u}, Uint8{32u});
    TEST_RANGE(SubpassIndex, Uint8{1u}, Uint8{8u});
    TEST_FLAGS(ShadingRateFlags, static_cast<PIPELINE_SHADING_RATE_FLAGS>(1), PIPELINE_SHADING_RATE_FLAG_LAST);

    for (Uint8 i = 1; i < MAX_RENDER_TARGETS; ++i)
    {
        Helper.Get().NumRenderTargets = i;
        TEST_RANGE(RTVFormats[i - 1], TEX_FORMAT_UNKNOWN, TEX_FORMAT_NUM_FORMATS);
    }

    TEST_RANGE(DSVFormat, static_cast<TEXTURE_FORMAT>(1), TEX_FORMAT_NUM_FORMATS);

    Helper.Get().SmplDesc.Count = 4;
    Helper.Add("SmplDesc");

    //IRenderPass* pRenderPass DEFAULT_INITIALIZER(nullptr);

    TEST_RANGE(NodeMask, 2u, 64u);
}

TEST(Common_HashUtils, GraphicsPipelineDescStdHash)
{
    TestGraphicsPipelineDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, GraphicsPipelineDescXXH128Hash)
{
    TestGraphicsPipelineDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestRayTracingPipelineDescHasher()
{
    DEFINE_HELPER(RayTracingPipelineDesc);

    TEST_RANGE(ShaderRecordSize, Uint16{32u}, Uint16{48000u}, Uint16{1024u});
    TEST_RANGE(MaxRecursionDepth, Uint8{1u}, Uint8{32u});
}

TEST(Common_HashUtils, RayTracingPipelineDescStdHash)
{
    TestRayTracingPipelineDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, RayTracingPipelineDescXXH128Hash)
{
    TestRayTracingPipelineDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestPipelineStateDescHasher()
{
    DEFINE_HELPER(PipelineStateDesc);

    TEST_RANGE(PipelineType, static_cast<PIPELINE_TYPE>(1), PIPELINE_TYPE_COUNT);
    TEST_RANGE(SRBAllocationGranularity, 2u, 64u);
    TEST_FLAGS(ImmediateContextMask, Uint64{2u}, Uint64{1u} << Uint64{63u});

    Helper.Get().ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
    Helper.Add("ResourceLayout");
}

TEST(Common_HashUtils, PipelineStateDescStdHash)
{
    TestPipelineStateDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, PipelineStateDescXXH128Hash)
{
    TestPipelineStateDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestPipelineResourceSignatureDescHasher()
{
    ASSERT_SIZEOF64(PipelineResourceSignatureDesc, 56, "Did you add new members to PipelineResourceSignatureDesc? Please update the tests.");
    DEFINE_HELPER(PipelineResourceSignatureDesc);

    constexpr PipelineResourceDesc Resources[] = //
        {
            {SHADER_TYPE_VERTEX, "Res1", 1, SHADER_RESOURCE_TYPE_CONSTANT_BUFFER, SHADER_RESOURCE_VARIABLE_TYPE_STATIC, PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS},
            {SHADER_TYPE_PIXEL, "Res2", 2, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC, PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER} //
        };

    constexpr ImmutableSamplerDesc ImtblSamplers[] = //
        {
            {SHADER_TYPE_VERTEX, "Sam1", SamplerDesc{}},
            {SHADER_TYPE_PIXEL, "Sam2", SamplerDesc{}} //
        };

    Helper.Get().Resources = Resources;
    TEST_VALUE(NumResources, 1u);
    TEST_VALUE(NumResources, 2u);

    Helper.Get().ImmutableSamplers = ImtblSamplers;
    TEST_VALUE(NumImmutableSamplers, 1u);
    TEST_VALUE(NumImmutableSamplers, 2u);

    TEST_RANGE(BindingIndex, Uint8{1u}, Uint8{8u});
    TEST_BOOL(UseCombinedTextureSamplers);

    Helper.Get().UseCombinedTextureSamplers = true;
    Helper.AddStrings(Helper.Get().CombinedSamplerSuffix, "CombinedSamplerSuffix", {"_Sampler", "_sam", "_Samp"});
}

TEST(Common_HashUtils, PipelineResourceSignatureDescStdHash)
{
    TestPipelineResourceSignatureDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, PipelineResourceSignatureDescXXH128Hash)
{
    TestPipelineResourceSignatureDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestShaderDescHasher()
{
    ASSERT_SIZEOF64(ShaderDesc, 24, "Did you add new members to ShaderDesc? Please update the tests.");
    DEFINE_HELPER(ShaderDesc);

    TEST_FLAGS(ShaderType, static_cast<SHADER_TYPE>(1), SHADER_TYPE_LAST);
    TEST_BOOL(UseCombinedTextureSamplers);
    TEST_STRINGS(CombinedSamplerSuffix, "_sampler1", "_sampler2", "_sampler3");
}

TEST(Common_HashUtils, ShaderDescStdHash)
{
    TestShaderDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, ShaderDescXXH128Hash)
{
    TestShaderDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestVersionHasher()
{
    ASSERT_SIZEOF64(Version, 8, "Did you add new members to Version? Please update the tests.");
    DEFINE_HELPER(Version);

    TEST_RANGE(Minor, 1u, 1024u);
    TEST_RANGE(Major, 1u, 1024u);
}

TEST(Common_HashUtils, VersionStdHash)
{
    TestVersionHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, VersionXXH128Hash)
{
    TestVersionHasher<XXH128HasherTestHelper>();
}

TEST(XXH128HasherTest, ShaderCreateInfo)
{
    ASSERT_SIZEOF64(ShaderCreateInfo, 144, "Did you add new members to ShaderCreateInfo? Please update the tests.");
    XXH128HasherTestHelper<ShaderCreateInfo> Helper{"ShaderCreateInfo"};

    TEST_STRINGS(Source, "Source1", "Source2", "Source3");
    TEST_RANGE(SourceLength, size_t{1}, size_t{5});

    Helper.Get().Source       = nullptr;
    constexpr uint32_t Data[] = {1, 2, 3, 4};
    Helper.Get().ByteCode     = Data;
    TEST_RANGE(ByteCodeSize, size_t{1}, size_t{8});

    constexpr char Source[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
    Helper.Get().ByteCode    = nullptr;
    Helper.Get().Source      = Source;
    TEST_RANGE(SourceLength, size_t{1}, sizeof(Source));

    TEST_STRINGS(EntryPoint, "Entry1", "Entry2", "Entry3");

    constexpr ShaderMacro Macros[] = {
        {"Macro1", "Def1"},
        {"Macro2", "Def2"},
        {"Macro3", "Def3"},
        {},
    };
    TEST_VALUE(Macros, Macros);
    TEST_BOOL(Desc.UseCombinedTextureSamplers);

    TEST_STRINGS(Desc.CombinedSamplerSuffix, "_sampler1", "_sampler2", "_sampler3");

    TEST_FLAGS(Desc.ShaderType, static_cast<SHADER_TYPE>(1), SHADER_TYPE_LAST);
    TEST_RANGE(SourceLanguage, static_cast<SHADER_SOURCE_LANGUAGE>(1), SHADER_SOURCE_LANGUAGE_COUNT);
    TEST_RANGE(ShaderCompiler, static_cast<SHADER_COMPILER>(1), SHADER_COMPILER_COUNT);

    TEST_RANGE(HLSLVersion.Minor, 1u, 10u);
    TEST_RANGE(HLSLVersion.Major, 1u, 10u);
    TEST_RANGE(GLSLVersion.Minor, 1u, 10u);
    TEST_RANGE(GLSLVersion.Major, 1u, 10u);
    TEST_RANGE(GLESSLVersion.Minor, 1u, 10u);
    TEST_RANGE(GLESSLVersion.Major, 1u, 10u);
    TEST_RANGE(MSLVersion.Minor, 1u, 10u);
    TEST_RANGE(MSLVersion.Major, 1u, 10u);

    TEST_FLAGS(CompileFlags, static_cast<SHADER_COMPILE_FLAGS>(1), SHADER_COMPILE_FLAG_LAST);
}



template <template <typename T> class HelperType>
void TestPipelineStateCIHasher()
{
    DEFINE_HELPER(PipelineStateCreateInfo);

    TEST_FLAGS(Flags, static_cast<PSO_CREATE_FLAGS>(1), PSO_CREATE_FLAG_LAST);
    TEST_RANGE(PSODesc.PipelineType, static_cast<PIPELINE_TYPE>(1), PIPELINE_TYPE_COUNT);

    std::array<IPipelineResourceSignature*, MAX_RESOURCE_SIGNATURES> ppSignatures{};
    Helper.Get().ppResourceSignatures = ppSignatures.data();
    TEST_RANGE(ResourceSignaturesCount, 1u, MAX_RESOURCE_SIGNATURES);
}

TEST(Common_HashUtils, PipelineStateCIStdHash)
{
    TestPipelineStateCIHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, PipelineStateCIXXH128Hash)
{
    TestPipelineStateCIHasher<XXH128HasherTestHelper>();
}



template <template <typename T> class HelperType>
void TestGraphicsPipelineStateCIHasher()
{
    DEFINE_HELPER(GraphicsPipelineStateCreateInfo);

    TEST_FLAGS(Flags, static_cast<PSO_CREATE_FLAGS>(1), PSO_CREATE_FLAG_LAST);
    TEST_FLAGS(GraphicsPipeline.SampleMask, 1u, 0xFFFFFFFFu);
}

TEST(Common_HashUtils, GraphicsPipelineStateCIStdHash)
{
    TestGraphicsPipelineStateCIHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, GraphicsPipelineStateCIXXH128Hash)
{
    TestGraphicsPipelineStateCIHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestComputePipelineStateCIHasher()
{
    DEFINE_HELPER(ComputePipelineStateCreateInfo);

    TEST_FLAGS(Flags, static_cast<PSO_CREATE_FLAGS>(1), PSO_CREATE_FLAG_LAST);
}

TEST(Common_HashUtils, ComputePipelineStateCIStdHash)
{
    TestComputePipelineStateCIHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, ComputePipelineStateCIXXH128Hash)
{
    TestComputePipelineStateCIHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestRTPipelineStateCIHasher()
{
    DEFINE_HELPER(RayTracingPipelineStateCreateInfo);

    TEST_FLAGS(Flags, static_cast<PSO_CREATE_FLAGS>(1), PSO_CREATE_FLAG_LAST);
    TEST_RANGE(RayTracingPipeline.ShaderRecordSize, Uint16{32u}, Uint16{48000u}, Uint16{1024u});

    std::array<RayTracingGeneralShaderGroup, 8>       pGeneralShaders{};
    std::array<RayTracingTriangleHitShaderGroup, 8>   pTriangleHitShaders{};
    std::array<RayTracingProceduralHitShaderGroup, 8> pProceduralHitShaders{};

    Helper.Get().pGeneralShaders       = pGeneralShaders.data();
    Helper.Get().pTriangleHitShaders   = pTriangleHitShaders.data();
    Helper.Get().pProceduralHitShaders = pProceduralHitShaders.data();

    TEST_RANGE(GeneralShaderCount, 1u, static_cast<Uint32>(pGeneralShaders.size()));
    TEST_RANGE(TriangleHitShaderCount, 1u, static_cast<Uint32>(pTriangleHitShaders.size()));
    TEST_RANGE(ProceduralHitShaderCount, 1u, static_cast<Uint32>(pProceduralHitShaders.size()));
    TEST_STRINGS(pShaderRecordName, "Name1", "Name2", "Name3");
    TEST_RANGE(MaxAttributeSize, 1u, 128u);
    TEST_RANGE(MaxPayloadSize, 1u, 128u);
}

TEST(Common_HashUtils, RTPipelineStateCIStdHash)
{
    TestRTPipelineStateCIHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, RTPipelineStateCIXXH128Hash)
{
    TestRTPipelineStateCIHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestTilePipelineDescHasher()
{
    DEFINE_HELPER(TilePipelineDesc);

    TEST_RANGE(NumRenderTargets, Uint8{1u}, Uint8{8u});
    TEST_RANGE(SampleCount, Uint8{2u}, Uint8{32u});
    for (Uint8 i = 1; i < MAX_RENDER_TARGETS; ++i)
    {
        Helper.Get().NumRenderTargets = i;
        TEST_RANGE(RTVFormats[i - 1], TEX_FORMAT_UNKNOWN, TEX_FORMAT_NUM_FORMATS);
    }
}

TEST(Common_HashUtils, TilePipelineDescStdHash)
{
    TestTilePipelineDescHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, TilePipelineDescXXH128Hash)
{
    TestTilePipelineDescHasher<XXH128HasherTestHelper>();
}


template <template <typename T> class HelperType>
void TestTilePipelineStateCIHasher()
{
    DEFINE_HELPER(TilePipelineStateCreateInfo);

    TEST_FLAGS(Flags, static_cast<PSO_CREATE_FLAGS>(1), PSO_CREATE_FLAG_LAST);
    TEST_RANGE(TilePipeline.SampleCount, Uint8{2u}, Uint8{32u});
}

TEST(Common_HashUtils, TilePipelineStateCIStdHash)
{
    TestTilePipelineStateCIHasher<StdHasherTestHelper>();
}

TEST(Common_HashUtils, TilePipelineStateCIXXH128Hash)
{
    TestTilePipelineStateCIHasher<XXH128HasherTestHelper>();
}

} // namespace
