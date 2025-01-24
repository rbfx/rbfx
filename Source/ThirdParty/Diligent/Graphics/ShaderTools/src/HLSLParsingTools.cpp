/*
 *  Copyright 2024 Diligent Graphics LLC
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

#include "HLSLParsingTools.hpp"
#include "HLSLTokenizer.hpp"
#include "GLSLParsingTools.hpp"

namespace Diligent
{

namespace Parsing
{

static std::pair<std::string, TEXTURE_FORMAT> ParseRWTextureDefinition(HLSLTokenizer::TokenListType::const_iterator& Token,
                                                                       HLSLTokenizer::TokenListType::const_iterator  End)
{
    // RWTexture2D<unorm  /*format=rg8*/ float4>  g_RWTex;
    // ^

    ++Token;
    // RWTexture2D<unorm  /*format=rg8*/ float4>  g_RWTex;
    //            ^
    if (Token == End || Token->Literal != "<")
        return {};

    TEXTURE_FORMAT Fmt = TEX_FORMAT_UNKNOWN;
    while (Token != End && Token->Literal != ">")
    {
        ++Token;
        if (Token != End)
        {
            // RWTexture2D< /*format=rg8*/ unorm float4> g_RWTex;
            //                             ^
            // RWTexture2D< unorm /*format=rg8*/ float4> g_RWTex;
            //                                   ^
            // RWTexture2D< unorm float4 /*format=rg8*/> g_RWTex;
            //                                         ^
            std::string FormatStr = ExtractGLSLImageFormatFromComment(Token->Delimiter.begin(), Token->Delimiter.end());
            if (!FormatStr.empty())
            {
                Fmt = ParseGLSLImageFormat(FormatStr);
            }
        }
    }

    // RWTexture2D<unorm  /*format=rg8*/ float4>  g_RWTex;
    //                                         ^
    if (Token == End)
        return {};

    ++Token;
    if (Token == End)
        return {};

    // RWTexture2D<unorm  /*format=rg8*/ float4>  g_RWTex;
    //                                            ^
    if (Token->Type != HLSLTokenType::Identifier)
        return {};

    return {Token->Literal, Fmt};
}

std::unordered_map<HashMapStringKey, TEXTURE_FORMAT> ExtractGLSLImageFormatsFromHLSL(const std::string& HLSLSource)
{
    HLSLTokenizer                      Tokenizer;
    const HLSLTokenizer::TokenListType Tokens = Tokenizer.Tokenize(HLSLSource);

    std::unordered_map<HashMapStringKey, TEXTURE_FORMAT> ImageFormats;

    auto Token      = Tokens.begin();
    int  ScopeLevel = 0;
    while (Token != Tokens.end())
    {
        if (Token->Type == HLSLTokenType::OpenBrace ||
            Token->Type == HLSLTokenType::OpenParen ||
            Token->Type == HLSLTokenType::OpenAngleBracket ||
            Token->Type == HLSLTokenType::OpenSquareBracket)
            ++ScopeLevel;

        if (Token->Type == HLSLTokenType::ClosingBrace ||
            Token->Type == HLSLTokenType::ClosingParen ||
            Token->Type == HLSLTokenType::ClosingAngleBracket ||
            Token->Type == HLSLTokenType::ClosingSquareBracket)
            --ScopeLevel;

        if (ScopeLevel < 0)
        {
            // No matching parenthesis found - stop parsing
            break;
        }

        if (ScopeLevel == 0 &&
            (Token->Type == HLSLTokenType::kw_RWTexture1D ||
             Token->Type == HLSLTokenType::kw_RWTexture1DArray ||
             Token->Type == HLSLTokenType::kw_RWTexture2D ||
             Token->Type == HLSLTokenType::kw_RWTexture2DArray ||
             Token->Type == HLSLTokenType::kw_RWTexture3D))
        {
            auto NameAndFmt = ParseRWTextureDefinition(Token, Tokens.end());
            if (NameAndFmt.second != TEX_FORMAT_UNKNOWN)
            {
                auto it_inserted = ImageFormats.emplace(NameAndFmt);
                if (!it_inserted.second && it_inserted.first->second != NameAndFmt.second)
                {
                    LOG_WARNING_MESSAGE("Different formats are specified for the same RWTexture '", NameAndFmt.first, "'. Note that the parser does not support preprocessing.");
                }
            }
        }
        else
        {
            ++Token;
        }
    }

    return ImageFormats;
}

} // namespace Parsing

} // namespace Diligent
