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

#pragma once

#include <unordered_map>
#include <list>

#include "ParsingTools.hpp"
#include "HLSLKeywords.h"
#include "HashUtils.hpp"

namespace Diligent
{

namespace Parsing
{

// clang-format off
enum class HLSLTokenType
{
    Undefined,
#define ADD_KEYWORD(keyword)kw_##keyword,
    ITERATE_HLSL_KEYWORDS(ADD_KEYWORD)
#undef ADD_KEYWORD
    PreprocessorDirective,
    Operator,
    OpenBrace,
    ClosingBrace,
    OpenParen,
    ClosingParen,
    OpenSquareBracket,
    ClosingSquareBracket,
    OpenAngleBracket,
    ClosingAngleBracket,
    Identifier,
    NumericConstant,
    StringConstant,
    Semicolon,
    Comma,
    Colon,
    DoubleColon,
    QuestionMark,
    TextBlock,
    Assignment,
    ComparisonOp,
    LogicOp,
    BitwiseOp,
    IncDecOp,
    MathOp
};
// clang-format on

struct HLSLTokenInfo
{
    using TokenType = HLSLTokenType;

    TokenType Type = TokenType::Undefined;
    String    Literal;
    String    Delimiter;
    size_t    Idx = ~size_t{0};

    HLSLTokenInfo() {}

    HLSLTokenInfo(TokenType   _Type,
                  std::string _Literal,
                  std::string _Delimiter = "",
                  size_t      _Idx       = ~size_t{0}) :
        Type{_Type},
        Literal{std::move(_Literal)},
        Delimiter{std::move(_Delimiter)},
        Idx{_Idx}
    {}

    void SetType(TokenType _Type)
    {
        Type = _Type;
    }

    TokenType GetType() const { return Type; }

    bool CompareLiteral(const char* Str)
    {
        return Literal == Str;
    }

    bool CompareLiteral(const std::string::const_iterator& Start,
                        const std::string::const_iterator& End)
    {
        const size_t Len = End - Start;
        if (strncmp(Literal.c_str(), &*Start, Len) != 0)
            return false;
        return Literal.length() == Len;
    }

    void ExtendLiteral(const std::string::const_iterator& Start,
                       const std::string::const_iterator& End)
    {
        Literal.append(Start, End);
    }

    bool IsBuiltInType() const
    {
        static_assert(static_cast<int>(TokenType::kw_bool) == 1 && static_cast<int>(TokenType::kw_void) == 191,
                      "If you updated built-in types, double check that all types are defined between bool and void");
        return Type >= TokenType::kw_bool && Type <= TokenType::kw_void;
    }

    bool IsFlowControl() const
    {
        static_assert(static_cast<int>(TokenType::kw_break) == 192 && static_cast<int>(TokenType::kw_while) == 202,
                      "If you updated control flow keywords, double check that all keywords are defined between break and while");
        return Type >= TokenType::kw_break && Type <= TokenType::kw_while;
    }

    static HLSLTokenInfo Create(TokenType                          _Type,
                                const std::string::const_iterator& DelimStart,
                                const std::string::const_iterator& DelimEnd,
                                const std::string::const_iterator& LiteralStart,
                                const std::string::const_iterator& LiteralEnd,
                                size_t                             Idx)
    {
        return HLSLTokenInfo{_Type, std::string{LiteralStart, LiteralEnd}, std::string{DelimStart, DelimEnd}, Idx};
    }

    size_t GetDelimiterLen() const
    {
        return Delimiter.length();
    }
    size_t GetLiteralLen() const
    {
        return Literal.length();
    }
    const std::pair<const char*, const char*> GetDelimiter() const
    {
        return {Delimiter.c_str(), Delimiter.c_str() + GetDelimiterLen()};
    }
    const std::pair<const char*, const char*> GetLiteral() const
    {
        return {Literal.c_str(), Literal.c_str() + GetLiteralLen()};
    }

    std::ostream& OutputDelimiter(std::ostream& os) const
    {
        os << Delimiter;
        return os;
    }
    std::ostream& OutputLiteral(std::ostream& os) const
    {
        os << Literal;
        return os;
    }
};

class HLSLTokenizer
{
public:
    HLSLTokenizer();

    const HLSLTokenInfo* FindKeyword(const String& Keyword) const
    {
        auto it = m_Keywords.find(HashMapStringKey{Keyword});
        return it != m_Keywords.end() ? &it->second : nullptr;
    }

    using TokenListType = std::list<HLSLTokenInfo>;
    TokenListType Tokenize(const String& Source) const;

private:
    // HLSL keyword -> token info hash map
    // Example: "Texture2D" -> TokenInfo{TokenType::Texture2D, "Texture2D"}
    std::unordered_map<HashMapStringKey, HLSLTokenInfo> m_Keywords;
};

} // namespace Parsing

} // namespace Diligent
