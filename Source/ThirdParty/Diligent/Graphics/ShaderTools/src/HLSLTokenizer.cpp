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

#include "HLSLTokenizer.hpp"

namespace Diligent
{

namespace Parsing
{

HLSLTokenizer::HLSLTokenizer()
{
    // Populate HLSL keywords hash map
#define DEFINE_KEYWORD(keyword) m_Keywords.insert(std::make_pair(#keyword, HLSLTokenInfo(HLSLTokenType::kw_##keyword, #keyword)));
    ITERATE_HLSL_KEYWORDS(DEFINE_KEYWORD)
#undef DEFINE_KEYWORD
}

HLSLTokenizer::TokenListType HLSLTokenizer::Tokenize(const String& Source) const
{
    try
    {
        size_t TokenIdx = 0;
        return Parsing::Tokenize<HLSLTokenInfo, TokenListType>(
            Source.begin(), Source.end(),
            [&TokenIdx](HLSLTokenType                      Type,
                        const std::string::const_iterator& DelimStart,
                        const std::string::const_iterator& DelimEnd,
                        const std::string::const_iterator& LiteralStart,
                        const std::string::const_iterator& LiteralEnd) //
            {
                return HLSLTokenInfo::Create(Type, DelimStart, DelimEnd, LiteralStart, LiteralEnd, TokenIdx++);
            },
            [&](const std::string::const_iterator& Start, const std::string::const_iterator& End) //
            {
                auto KeywordIt = m_Keywords.find(HashMapStringKey{std::string{Start, End}});
                if (KeywordIt != m_Keywords.end())
                {
                    VERIFY(std::string(Start, End) == KeywordIt->second.Literal, "Inconsistent literal");
                    return KeywordIt->second.Type;
                }
                return HLSLTokenType::Identifier;
            });
    }
    catch (...)
    {
        return {};
    }
}

} // namespace Parsing

} // namespace Diligent
