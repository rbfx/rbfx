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

#include <deque>

#include "ShaderToolsCommon.hpp"
#include "DefaultShaderSourceStreamFactory.h"
#include "RenderDevice.h"
#include "TestingEnvironment.hpp"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

TEST(ShaderPreprocessTest, Include)
{
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    CreateDefaultShaderSourceStreamFactory("shaders/ShaderPreprocessor", &pShaderSourceFactory);
    ASSERT_NE(pShaderSourceFactory, nullptr);

    {
        std::deque<const char*> Includes{
            "IncludeCommon0.hlsl",
            "IncludeCommon1.hlsl",
            "IncludeBasicTest.hlsl"};

        ShaderCreateInfo ShaderCI{};
        ShaderCI.Desc.Name                  = "TestShader";
        ShaderCI.FilePath                   = "IncludeBasicTest.hlsl";
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

        const auto Result = ProcessShaderIncludes(ShaderCI, [&](const ShaderIncludePreprocessInfo& ProcessInfo) {
            EXPECT_EQ(ProcessInfo.FilePath, Includes.front());
            Includes.pop_front();
        });
        EXPECT_EQ(Result, true);
    }

    {
        std::deque<const char*> Includes{
            "IncludeCommon0.hlsl",
            "IncludeWhiteSpaceTest.hlsl"};

        ShaderCreateInfo ShaderCI{};
        ShaderCI.Desc.Name                  = "TestShader";
        ShaderCI.FilePath                   = "IncludeWhiteSpaceTest.hlsl";
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

        const auto Result = ProcessShaderIncludes(ShaderCI, [&](const ShaderIncludePreprocessInfo& ProcessInfo) {
            EXPECT_EQ(ProcessInfo.FilePath, Includes.front());
            Includes.pop_front();
        });

        EXPECT_EQ(Result, true);
    }

    {
        std::deque<const char*> Includes{
            "IncludeCommon0.hlsl",
            "IncludeCommentsSingleLineTest.hlsl"};

        ShaderCreateInfo ShaderCI{};
        ShaderCI.Desc.Name                  = "TestShader";
        ShaderCI.FilePath                   = "IncludeCommentsSingleLineTest.hlsl";
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

        const auto Result = ProcessShaderIncludes(ShaderCI, [&](const ShaderIncludePreprocessInfo& ProcessInfo) {
            EXPECT_EQ(ProcessInfo.FilePath, Includes.front());
            Includes.pop_front();
        });

        EXPECT_EQ(Result, true);
    }

    {
        std::deque<const char*> Includes{
            "IncludeCommon0.hlsl",
            "IncludeCommentsMultiLineTest.hlsl"};

        ShaderCreateInfo ShaderCI{};
        ShaderCI.Desc.Name                  = "TestShader";
        ShaderCI.FilePath                   = "IncludeCommentsMultiLineTest.hlsl";
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

        const auto Result = ProcessShaderIncludes(ShaderCI, [&](const ShaderIncludePreprocessInfo& ProcessInfo) {
            EXPECT_EQ(ProcessInfo.FilePath, Includes.front());
            Includes.pop_front();
        });

        EXPECT_EQ(Result, true);
    }

    {
        std::deque<const char*> Includes{
            "IncludeCommentsTrickyCasesTest.hlsl"};

        ShaderCreateInfo ShaderCI{};
        ShaderCI.Desc.Name                  = "TestShader";
        ShaderCI.FilePath                   = "IncludeCommentsTrickyCasesTest.hlsl";
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

        const auto Result = ProcessShaderIncludes(ShaderCI, [&](const ShaderIncludePreprocessInfo& ProcessInfo) {
            EXPECT_EQ(ProcessInfo.FilePath, Includes.front());
            Includes.pop_front();
        });

        EXPECT_EQ(Result, true);
    }
}

TEST(ShaderPreprocessTest, InvalidInclude)
{
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    CreateDefaultShaderSourceStreamFactory("shaders/ShaderPreprocessor", &pShaderSourceFactory);
    ASSERT_NE(pShaderSourceFactory, nullptr);

    constexpr size_t TestCount = 12;
    for (size_t TestId = 0; TestId < TestCount; ++TestId)
    {
        String FilePath = "IncludeInvalidCase" + std::to_string(TestId) + ".hlsl";

        ShaderCreateInfo ShaderCI{};
        ShaderCI.Desc.Name                  = "TestShader";
        ShaderCI.FilePath                   = FilePath.c_str();
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

        TestingEnvironment::ErrorScope ExpectedErrors{String{"Failed to process includes in file '"} + FilePath + "'"};

        const auto Result = ProcessShaderIncludes(ShaderCI, {});
        EXPECT_EQ(Result, false);
    }
}

TEST(ShaderPreprocessTest, UnrollIncludes)
{
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    CreateDefaultShaderSourceStreamFactory("shaders/ShaderPreprocessor", &pShaderSourceFactory);
    ASSERT_NE(pShaderSourceFactory, nullptr);

    {
        ShaderCreateInfo ShaderCI{};
        ShaderCI.Desc.Name                  = "TestShader";
        ShaderCI.FilePath                   = "InlineIncludeShaderTest.hlsl";
        ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;

        constexpr char RefString[] =
            "// Start InlineIncludeShaderTest.hlsl\n"
            "// Start InlineIncludeShaderCommon1.hlsl\n"
            "// #include \"InlineIncludeShaderCommon0.hlsl\"\n"
            "\n"
            "#define MACRO\n"
            "// End InlineIncludeShaderCommon1.hlsl\n"
            "\n"
            "// Start InlineIncludeShaderCommon2.hlsl\n"
            "\n"
            "\n"
            "\n"
            "// End InlineIncludeShaderCommon2.hlsl\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "// End InlineIncludeShaderTest.hlsl\n";

        auto UnrolledStr = UnrollShaderIncludes(ShaderCI);
        ASSERT_EQ(RefString, UnrolledStr);
    }
}

TEST(ShaderPreprocessTest, ShaderSourceLanguageDefiniton)
{
    EXPECT_EQ(ParseShaderSourceLanguageDefinition(""), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("abc"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("**/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("abc*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("*abc*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*abc*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/**/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/****/"), SHADER_SOURCE_LANGUAGE_DEFAULT);

    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANG*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANG=1*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGE*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGE   */"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGEx*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGE   x*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGE=*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGE=   */"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGE   =*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGE=X*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGE = X*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGE  =   2*/"), SHADER_SOURCE_LANGUAGE_GLSL);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*  $SHADER_SOURCE_LANGUAGE  =   2  */"), SHADER_SOURCE_LANGUAGE_GLSL);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/**$SHADER_SOURCE_LANGUAGE  =   3**/"), SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/***$SHADER_SOURCE_LANGUAGE  =   3***/"), SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGE=9*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);
    EXPECT_EQ(ParseShaderSourceLanguageDefinition("/*$SHADER_SOURCE_LANGUAGE=11*/"), SHADER_SOURCE_LANGUAGE_DEFAULT);

    for (Uint32 lang = SHADER_SOURCE_LANGUAGE_DEFAULT; lang < SHADER_SOURCE_LANGUAGE_COUNT; ++lang)
    {
        const auto Lang = static_cast<SHADER_SOURCE_LANGUAGE>(lang);
        {
            std::string Source;
            AppendShaderSourceLanguageDefinition(Source, Lang);
            EXPECT_EQ(ParseShaderSourceLanguageDefinition(Source), Lang);
        }
    }
}

} // namespace
