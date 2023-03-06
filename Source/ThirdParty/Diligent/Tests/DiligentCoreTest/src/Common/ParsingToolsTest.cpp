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

#include "ParsingTools.hpp"

#include "gtest/gtest.h"

#include "TestingEnvironment.hpp"

using namespace Diligent;
using namespace Diligent::Parsing;
using namespace Diligent::Testing;

namespace
{

TEST(Common_ParsingTools, SkipLine)
{
    auto Test = [](const char* Str, bool EndReached = false, const char* Expected = nullptr) {
        const auto* StrEnd = Str + strlen(Str);
        const auto* Pos    = SkipLine(Str, StrEnd);
        EXPECT_EQ(Pos == StrEnd, EndReached);
        if (Expected == nullptr)
            Expected = EndReached ? "" : "Correct";
        EXPECT_STREQ(Pos, Expected);
    };

    Test("", true);
    Test("abc def ", true);

    Test("abc def \n"
         "Correct",
         false,
         "\n"
         "Correct");

    Test("abc def \r"
         "Correct",
         false,
         "\r"
         "Correct");
}

TEST(Common_ParsingTools, SkipLine_GoToNext)
{
    auto Test = [](const char* Str, bool EndReached = false, const char* Expected = nullptr) {
        const auto* StrEnd = Str + strlen(Str);
        const auto* Pos    = SkipLine(Str, StrEnd, true);
        EXPECT_EQ(Pos == StrEnd, EndReached);
        if (Expected == nullptr)
            Expected = EndReached ? "" : "Correct";
        EXPECT_STREQ(Pos, Expected);
    };

    Test("", true);
    Test("\n", true);
    Test("\r\n", true);
    Test("abc def ", true);
    Test("abc def ", true);

    Test("\n"
         "Correct");
    Test("\r"
         "Correct");
    Test("\r\n"
         "Correct");
}

TEST(Common_ParsingTools, SkipComment)
{
    auto Test = [](const char* Str, bool CommentFound = true, bool EndReached = false) {
        const auto* StrEnd = Str + strlen(Str);
        const auto* Pos    = SkipComment(Str, StrEnd);
        EXPECT_EQ(Pos == StrEnd, EndReached);
        const auto Expected = EndReached ? "" : (CommentFound ? "Correct" : Str);
        EXPECT_STREQ(Pos, Expected);
    };

    Test("", false, true);
    Test("Correct", false);
    Test("/", false);
    Test("/Correct", false);

    Test("//", true, true);
    Test("// Single-line comment", true, true);
    Test("// Single-line comment\n", true, true);

    Test("// Single-line comment\n"
         "Correct");

    Test("// Single-line comment // \n"
         "Correct");

    Test("// Single-line comment /* */ \n"
         "Correct");

    Test("/**/Correct");
    Test("/* abc */Correct");
    Test("/** abc */Correct");
    Test("/* abc **/Correct");
    Test("/*/* abc ** /* **/Correct");

    Test("/*\n"
         "/* abc **\r\n"
         "/****** ***** ***\r"
         " /* **/Correct");

    auto TestFlags = [](const char* Str, SKIP_COMMENT_FLAGS Flags, const char* Expected) {
        const auto* StrEnd = Str + strlen(Str);
        const auto* Pos    = SkipComment(Str, StrEnd, Flags);
        EXPECT_STREQ(Pos, Expected);
    };
    TestFlags("/*\n"
              "/* abc **\r\n*/"
              "//Correct",
              SKIP_COMMENT_FLAG_MULTILINE, "//Correct");
    TestFlags("// abc /* def */\n"
              "/*Correct*/",
              SKIP_COMMENT_FLAG_SINGLE_LINE, "/*Correct*/");
    TestFlags("//Correct",
              SKIP_COMMENT_FLAG_MULTILINE, "//Correct");
    TestFlags("/*Correct*/",
              SKIP_COMMENT_FLAG_SINGLE_LINE, "/*Correct*/");
}


TEST(Common_ParsingTools, SkipCommentErrors)
{
    auto Test = [](const char* Str) {
        try
        {
            SkipComment(Str, Str + strlen(Str));
        }
        catch (const std::pair<const char*, const char*>& err_info)
        {
            EXPECT_STREQ(err_info.first, Str);
            return;
        }
        ADD_FAILURE() << "SkipComment must throw an exception";
    };

    Test("/*");
    Test("/*/");
    Test("/* abc ");
    Test("/* abc *");

    Test("/* abc *\n"
         "***\n");

    Test("/*   *");
    Test("/*   /");

    Test("/*   \n"
         "   ");
    Test("/*   \n"
         "   ***");
    Test("/*   \n"
         "   /***");
}

TEST(Common_ParsingTools, SkipDelimiters)
{
    auto Test = [](const char* Str, bool EndReached = false, const char* Expected = nullptr) {
        const auto* StrEnd = Str + strlen(Str);
        const auto* Pos    = SkipDelimiters(Str, StrEnd);
        EXPECT_EQ(Pos == StrEnd, EndReached);
        if (Expected == nullptr)
            Expected = EndReached ? "" : "Correct";
        EXPECT_STREQ(Pos, Expected);
    };

    Test("", true);
    Test(" ", true);
    Test("\t", true);
    Test("\r", true);
    Test("\n", true);
    Test("\t \r \n ", true);

    Test("Correct");
    Test(" Correct");
    Test("\tCorrect");
    Test("\rCorrect");
    Test("\nCorrect");
    Test("\t \r \n Correct");

    auto TestCustomDelimiters = [](const char* Str, const char* Delimiters, const char* Expected) {
        const auto* StrEnd = Str + strlen(Str);
        const auto* Pos    = SkipDelimiters(Str, StrEnd, Delimiters);
        EXPECT_STREQ(Pos, Expected);
    };
    TestCustomDelimiters(" \t \r \n Correct", " ", "\t \r \n Correct");
    TestCustomDelimiters(" \t \r \n Correct", " \t", "\r \n Correct");
    TestCustomDelimiters(" \t \r \n Correct", " \t\r", "\n Correct");
}

TEST(Common_ParsingTools, SkipDelimitersAndComments)
{
    auto Test = [](const char* Str, bool EndReached = false) {
        const auto* StrEnd = Str + strlen(Str);
        const auto* Pos    = SkipDelimitersAndComments(Str, StrEnd);
        EXPECT_EQ(Pos == StrEnd, EndReached);
        const auto* Expected = EndReached ? "" : "Correct";
        EXPECT_STREQ(Pos, Expected);
    };

    Test("", true);
    Test(" ", true);
    Test("\t", true);
    Test("\r", true);
    Test("\n", true);
    Test("\t \r \n ", true);
    Test("// Comment", true);

    Test("// Comment line 1\n"
         "/// Comment line 2\r"
         "//// Comment line 3\r\n",
         true);

    Test("/* Comment */\n",
         true);

    Test("/* Comment line 1\n"
         "Comment line 2\r"
         "Comment line 3\r\n*/",
         true);

    Test(" \t \r \n // Comment\n"
         " \t \r \n "
         "Correct");

    Test(" \t \r \n \n"
         "/* Comment */\n"
         " \t \r \n "
         "Correct");

    Test(" \t // Comment 1\n"
         " /* Comment 2 \n"
         "Comment 3 /* /* **** \r"
         "Comment 4*/ // Comment 5"
         " \n //\r\n"
         " \t \r \n"
         "Correct");

    auto TestCustom = [](const char* Str, const char* Delimiters, SKIP_COMMENT_FLAGS CommentFlags, const char* Expected) {
        const auto* StrEnd = Str + strlen(Str);
        const auto* Pos    = SkipDelimitersAndComments(Str, StrEnd, Delimiters, CommentFlags);
        EXPECT_STREQ(Pos, Expected);
    };

    TestCustom(" \t // Comment 1\n"
               " \t \r \n"
               " /* Correct */",
               nullptr, SKIP_COMMENT_FLAG_SINGLE_LINE, "/* Correct */");
    TestCustom(" /* Comment 2 \n"
               "Comment 3 /* /* **** \r"
               "Comment 4*/ // Correct",
               nullptr, SKIP_COMMENT_FLAG_MULTILINE, "// Correct");
    TestCustom(" \t // Comment 1\n"
               " /* Comment 2 \n"
               "\t Comment 3 /* /* **** \r"
               "Comment 4*/\n"
               " // Correct",
               " \t", SKIP_COMMENT_FLAG_ALL, "\n // Correct");
    TestCustom(" \t // Comment 1\n"
               " /* Comment 2 \n"
               "\t Comment 3 /* /* **** \r"
               "Comment 4*/\r"
               " /* Correct */",
               " \t\n", SKIP_COMMENT_FLAG_ALL, "\r /* Correct */");
}

TEST(Common_ParsingTools, SkipIdentifier)
{
    auto Test = [](const char* Str, const char* Expected, bool EndReached = false) {
        const auto* StrEnd = Str + strlen(Str);
        const auto* Pos    = SkipIdentifier(Str, StrEnd);
        EXPECT_EQ(Pos == StrEnd, EndReached);
        if (Expected == nullptr)
            Expected = Str;
        EXPECT_STREQ(Pos, Expected);
    };

    Test("", nullptr, true);
    Test(" ", nullptr, false);
    Test("3abc", nullptr);
    Test("*", nullptr);
    Test("_", "", true);
    Test("_3", "", true);
    Test("_a", "", true);
    Test("_a1b2c3", "", true);
    Test("_?", "?");
    Test("_3+1", "+1");
    Test("_a = 10", " = 10");
    Test("_a1b2c3[5]", "[5]");
}

TEST(Common_ParsingTools, SplitString)
{
    static const char* TestStr = R"(
Lorem ipsum //dolor sit amet, consectetur
adipiscing elit, /* sed do eiusmod tempor incididunt 
ut labore et dolore magna*/ aliqua.   Ut 
// enim ad minim veniam, quis nostrud exercitation 
/// ullamco laboris nisi /* ut aliquip ex ea commodo consequat*/.
   Duis aute  irure //dolor in //reprehenderit in voluptate   velit esse 
/* cillum dolore eu fugiat 
/* nulla /* pariatur. 
*/ /*Excepteur 
*/ 
sint occaecat //cupidatat non proident.
)";

    std::vector<std::string> Chunks = {"Lorem", "ipsum", "adipiscing", "elit", ",", "aliqua.", "Ut", "Duis", "aute", "irure", "sint", "occaecat", ""};
    auto                     ref_it = Chunks.begin();

    const auto* StrEnd = TestStr + strlen(TestStr);
    SplitString(TestStr, StrEnd,
                [&](const char* DelimStart, const char*& Pos) //
                {
                    if (ref_it == Chunks.end())
                    {
                        ADD_FAILURE() << "Unexpected string " << Pos;
                        return false;
                    }

                    if (strncmp(Pos, ref_it->c_str(), ref_it->length()) != 0)
                    {
                        ADD_FAILURE() << Pos << " != " << *ref_it;
                        return false;
                    }

                    Pos += ref_it->length();
                    ++ref_it;
                    return true;
                });

    EXPECT_EQ(ref_it, Chunks.end());
}

TEST(Common_ParsingTools, GetContext)
{
    static const char* TestStr =
        "A12345678\n"
        "B12345678\n"
        "C12345678\n"
        "D12345678\n"
        "E12345678\n"
        "F12345678\n";
    const auto StrEnd = TestStr + strlen(TestStr);

    {
        static const char* RefStr =
            "A12345678\n"
            "^";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 0, 0).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "A12345678\n"
            "        ^";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 8, 0).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "A12345678\n"
            "         ^";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 9, 0).c_str(), RefStr);
    }


    {
        static const char* RefStr =
            "F12345678\n"
            "^";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 50, 0).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "F12345678\n"
            "        ^";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 58, 0).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "F12345678\n"
            "         ^";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 59, 0).c_str(), RefStr);
    }


    {
        static const char* RefStr =
            "A12345678\n"
            "^\n"
            "B12345678";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 0, 1).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "A12345678\n"
            "        ^\n"
            "B12345678";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 8, 1).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "A12345678\n"
            "         ^\n"
            "B12345678";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 9, 1).c_str(), RefStr);
    }


    {
        static const char* RefStr =
            "B12345678\n"
            "C12345678\n"
            "^\n"
            "D12345678";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 20, 1).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "B12345678\n"
            "C12345678\n"
            "        ^\n"
            "D12345678";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 28, 1).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "B12345678\n"
            "C12345678\n"
            "         ^\n"
            "D12345678";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 29, 1).c_str(), RefStr);
    }


    {
        static const char* RefStr =
            "E12345678\n"
            "F12345678\n"
            "^\n";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 50, 1).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "E12345678\n"
            "F12345678\n"
            "        ^\n";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 58, 1).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "E12345678\n"
            "F12345678\n"
            "         ^\n";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 59, 1).c_str(), RefStr);
    }

    {
        static const char* EmptyStr = "";
        EXPECT_STREQ(GetContext(EmptyStr, EmptyStr, EmptyStr, 0).c_str(), EmptyStr);
        EXPECT_STREQ(GetContext(EmptyStr, EmptyStr, EmptyStr, 1).c_str(), EmptyStr);
    }
}


TEST(Common_ParsingTools, GetContext_CRLF)
{
    static const char* TestStr =
        "A1234567\r\n"
        "B1234567\r\n"
        "C1234567\r\n"
        "D1234567\r\n"
        "E1234567\r\n"
        "F1234567\r\n";
    const auto StrEnd = TestStr + strlen(TestStr);

    {
        static const char* RefStr =
            "B1234567\r\n"
            "C1234567\n"
            "^\r\n"
            "D1234567";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 20, 1).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "B1234567\r\n"
            "C1234567\n"
            "       ^\r\n"
            "D1234567";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 27, 1).c_str(), RefStr);
    }
    {
        static const char* RefStr =
            "B1234567\r\n"
            "C1234567\n"
            "        ^\r\n"
            "D1234567";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 28, 1).c_str(), RefStr);
    }
}


TEST(Common_ParsingTools, GetContext_EmptyLines)
{
    static const char* TestStr =
        "\n"
        "A12345678\n"
        "B12345678\n"
        "\n";
    const auto StrEnd = TestStr + strlen(TestStr);

    {
        static const char* RefStr =
            "\n"
            "A12345678\n"
            "^\n"
            "B12345678";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 1, 1).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "A12345678\n"
            "B12345678\n"
            "^\n";
        EXPECT_STREQ(GetContext(TestStr, StrEnd, TestStr + 11, 1).c_str(), RefStr);
    }
}



enum class TestTokenType
{
    Undefined,
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
    MathOp,
    Keyword1,
    Keyword2,
    Keyword3,
    kw_void
};

struct TestToken
{
    using TokenType = TestTokenType;

    TokenType   Type = TestTokenType::Undefined;
    std::string Literal;
    std::string Delimiter;

    TestToken() {}
    TestToken(TokenType   _Type,
              std::string _Literal,
              std::string _Delimiter = "") :
        Type{_Type},
        Literal{std::move(_Literal)},
        Delimiter{std::move(_Delimiter)}
    {}

    void SetType(TokenType _Type)
    {
        Type = _Type;
    }

    TokenType GetType() const { return Type; }

    bool CompareLiteral(const char* Str) const
    {
        return Literal == Str;
    }

    bool CompareLiteral(const char* Start, const char* End) const
    {
        return Literal == std::string{Start, End};
    }

    void ExtendLiteral(const char* Start, const char* End)
    {
        Literal.append(Start, End);
    }

    static TestToken Create(TokenType _Type, const char* DelimStart, const char* DelimEnd, const char* LiteralStart, const char* LiteralEnd)
    {
        return TestToken{_Type, std::string{LiteralStart, LiteralEnd}, std::string{DelimStart, DelimEnd}};
    }

    static TokenType FindType(const char* IdentifierStart, const char* IdentifierEnd)
    {
        if (strncmp(IdentifierStart, "Keyword1", IdentifierEnd - IdentifierStart) == 0)
            return TokenType::Keyword1;

        if (strncmp(IdentifierStart, "Keyword2", IdentifierEnd - IdentifierStart) == 0)
            return TokenType::Keyword2;

        if (strncmp(IdentifierStart, "Keyword3", IdentifierEnd - IdentifierStart) == 0)
            return TokenType::Keyword3;

        if (strncmp(IdentifierStart, "void", IdentifierEnd - IdentifierStart) == 0)
            return TokenType::kw_void;

        return TokenType::Identifier;
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

bool FindTokenSequence(const std::vector<TestToken>& Tokens, const std::vector<TestToken>& Sequence)
{
    for (auto start_it = Tokens.begin(); start_it != Tokens.end(); ++start_it)
    {
        auto token_it = start_it;
        auto ref_it   = Sequence.begin();
        while (ref_it != Sequence.end() && token_it != Tokens.end())
        {
            if (ref_it->Type != token_it->Type || ref_it->Literal != token_it->Literal)
                break;
            ++ref_it;
            ++token_it;
        }
        if (ref_it == Sequence.end())
            return true;
    }
    return false;
}

TEST(Common_ParsingTools, Tokenizer_Preprocessor)
{
    static const char* TestStr = R"(
// Comment
#include <Include1.h>

// # not a definition

/* Comment */
#define MACRO

/*
#not 
#a
#definition
*/

void main()
{
}
// Comment
/* Comment */
)";

    const auto Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, TestStr + strlen(TestStr), TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::PreprocessorDirective, "#include"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::PreprocessorDirective, "#define"}}));
}

TEST(Common_ParsingTools, Tokenizer_Operators)
{
    static const char* TestStr = R"(

/* Comment */
void main()
{
    // Binary operators
    a0 + a1; // Comment 2
    b0 - b1; /* Comment 3*/
/**/c0 * c1;
    d0 / d1;
    e0 % e1;
    f0 << f1;
    g0 >> g1;
    h0 & h1;
    i0 | i1;
    j0 ^ j1;

    k0 < k1;
    l0 > l1;
    m0 = m1;

    // Unary operators
    !n0;
    ~o0;

    // Assignment operators
    A0 += A1;
    B0 -= B1;
    C0 *= C1;
    D0 /= D1;
    E0 %= E1;
    F0 <<= F1;
    G0 >>= G1;
    H0 &= H1;
    I0 |= I1;
    J0 ^= J1;

    K0 <= K1;
    L0 >= L1;
    M0 == M1;
    N0 != N1;

    P0++; ++P1;
    Q0--; --Q1;
}
)";

    const auto Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, TestStr + strlen(TestStr), TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "a0"}, {TestTokenType::MathOp, "+"}, {TestTokenType::Identifier, "a1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "b0"}, {TestTokenType::MathOp, "-"}, {TestTokenType::Identifier, "b1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "c0"}, {TestTokenType::MathOp, "*"}, {TestTokenType::Identifier, "c1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "d0"}, {TestTokenType::MathOp, "/"}, {TestTokenType::Identifier, "d1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "e0"}, {TestTokenType::MathOp, "%"}, {TestTokenType::Identifier, "e1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "f0"}, {TestTokenType::BitwiseOp, "<<"}, {TestTokenType::Identifier, "f1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "g0"}, {TestTokenType::BitwiseOp, ">>"}, {TestTokenType::Identifier, "g1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "h0"}, {TestTokenType::BitwiseOp, "&"}, {TestTokenType::Identifier, "h1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "i0"}, {TestTokenType::BitwiseOp, "|"}, {TestTokenType::Identifier, "i1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "j0"}, {TestTokenType::BitwiseOp, "^"}, {TestTokenType::Identifier, "j1"}}));

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "k0"}, {TestTokenType::ComparisonOp, "<"}, {TestTokenType::Identifier, "k1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "l0"}, {TestTokenType::ComparisonOp, ">"}, {TestTokenType::Identifier, "l1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "m0"}, {TestTokenType::Assignment, "="}, {TestTokenType::Identifier, "m1"}}));

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::LogicOp, "!"}, {TestTokenType::Identifier, "n0"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::BitwiseOp, "~"}, {TestTokenType::Identifier, "o0"}}));

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "A0"}, {TestTokenType::Assignment, "+="}, {TestTokenType::Identifier, "A1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "B0"}, {TestTokenType::Assignment, "-="}, {TestTokenType::Identifier, "B1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "C0"}, {TestTokenType::Assignment, "*="}, {TestTokenType::Identifier, "C1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "D0"}, {TestTokenType::Assignment, "/="}, {TestTokenType::Identifier, "D1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "E0"}, {TestTokenType::Assignment, "%="}, {TestTokenType::Identifier, "E1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "F0"}, {TestTokenType::Assignment, "<<="}, {TestTokenType::Identifier, "F1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "G0"}, {TestTokenType::Assignment, ">>="}, {TestTokenType::Identifier, "G1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "H0"}, {TestTokenType::Assignment, "&="}, {TestTokenType::Identifier, "H1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "I0"}, {TestTokenType::Assignment, "|="}, {TestTokenType::Identifier, "I1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "J0"}, {TestTokenType::Assignment, "^="}, {TestTokenType::Identifier, "J1"}}));

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "K0"}, {TestTokenType::ComparisonOp, "<="}, {TestTokenType::Identifier, "K1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "L0"}, {TestTokenType::ComparisonOp, ">="}, {TestTokenType::Identifier, "L1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "M0"}, {TestTokenType::ComparisonOp, "=="}, {TestTokenType::Identifier, "M1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "N0"}, {TestTokenType::ComparisonOp, "!="}, {TestTokenType::Identifier, "N1"}}));

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "P0"}, {TestTokenType::IncDecOp, "++"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::IncDecOp, "++"}, {TestTokenType::Identifier, "P1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "Q0"}, {TestTokenType::IncDecOp, "--"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::IncDecOp, "--"}, {TestTokenType::Identifier, "Q1"}}));
}

TEST(Common_ParsingTools, Tokenizer_Brackets)
{
    static const char* TestStr = R"(
// Comment
struct MyStruct
{
    int a;
};

void main(int argument [[annotation]])
{
    function(argument1, argument2);
    array[size];
}
)";

    const auto Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, TestStr + strlen(TestStr), TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::OpenSquareBracket, "["}, {TestTokenType::Identifier, "annotation"}, {TestTokenType::ClosingSquareBracket, "]"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::OpenBrace, "{"}, {TestTokenType::Identifier, "int"}, {TestTokenType::Identifier, "a"}, {TestTokenType::Semicolon, ";"}, {TestTokenType::ClosingBrace, "}"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "function"}, {TestTokenType::OpenParen, "("}, {TestTokenType::Identifier, "argument1"}, {TestTokenType::Comma, ","}, {TestTokenType::Identifier, "argument2"}, {TestTokenType::ClosingParen, ")"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "array"}, {TestTokenType::OpenSquareBracket, "["}, {TestTokenType::Identifier, "size"}, {TestTokenType::ClosingSquareBracket, "]"}}));
}

TEST(Common_ParsingTools, Tokenizer_StringConstant)
{
    static const char* TestStr = R"(
void main()
{
    const char* String = "string constant";
}
)";

    const auto Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, TestStr + strlen(TestStr), TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "String"}, {TestTokenType::Assignment, "="}, {TestTokenType::StringConstant, "string constant"}, {TestTokenType::Semicolon, ";"}}));
}

TEST(Common_ParsingTools, Tokenizer_FloatNumber)
{
    static const char* TestStr = R"(
void main()
{
    float Number1 = 10;
    float Number2 = 20.0;
    float Number3 = 30.0e+1;
    float Number4 = 40.0e+2f;
    float Number5 = 50.f;
    float Number6 = .123f;
}
)";

    const auto Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, TestStr + strlen(TestStr), TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "Number1"}, {TestTokenType::Assignment, "="}, {TestTokenType::NumericConstant, "10"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "Number2"}, {TestTokenType::Assignment, "="}, {TestTokenType::NumericConstant, "20.0"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "Number3"}, {TestTokenType::Assignment, "="}, {TestTokenType::NumericConstant, "30.0e+1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "Number4"}, {TestTokenType::Assignment, "="}, {TestTokenType::NumericConstant, "40.0e+2f"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "Number5"}, {TestTokenType::Assignment, "="}, {TestTokenType::NumericConstant, "50.f"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "Number6"}, {TestTokenType::Assignment, "="}, {TestTokenType::NumericConstant, ".123f"}}));
}

TEST(Common_ParsingTools, Tokenizer_UnknownIdentifier)
{
    static const char* TestStr = R"(
void main()
{
    @ Unknown;
}
)";

    const auto Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, TestStr + strlen(TestStr), TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Undefined, "@"}, {TestTokenType::Identifier, "Unknown"}}));
}


TEST(Common_ParsingTools, Tokenizer_Colon)
{
    static const char* TestStr = R"(
// Comment

/* Comment */

void main()
{
    a : b;
    // /*
    /* " */
    C /* abc */ :: /* def */ D; // test
    /////****
    e ? F;
}
// Comment
/* Comment */
)";

    const auto Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, TestStr + strlen(TestStr), TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "a"}, {TestTokenType::Colon, ":"}, {TestTokenType::Identifier, "b"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "C"}, {TestTokenType::DoubleColon, "::"}, {TestTokenType::Identifier, "D"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "e"}, {TestTokenType::QuestionMark, "?"}, {TestTokenType::Identifier, "F"}}));
}


TEST(Common_ParsingTools, Tokenizer_Keywords)
{
    static const char* TestStr = R"(
void main()
{
    Keyword1 Id Keyword2(Keyword3);
}
)";

    const auto Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, TestStr + strlen(TestStr), TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Keyword1, "Keyword1"}, {TestTokenType::Identifier, "Id"}, {TestTokenType::Keyword2, "Keyword2"}, {TestTokenType::OpenParen, "("}, {TestTokenType::Keyword3, "Keyword3"}, {TestTokenType::ClosingParen, ")"}}));
}


TEST(Common_ParsingTools, Tokenizer_PlusMinus)
{
    static const char* TestStr = R"(
+1.0;
-2.0;
a1 + a2;
b1 - b2;
c1+3;
3.5+c2;
d1-10.0;
-20.0+d2;

e1 + +4.1;
e2 + -4.2;
e3 - +4.3;
e4 - -4.4;

d1 + + 5.1;
d2 + - 5.2;
d3 - + 5.3;
d4 - - 5.4;
11+12;
13-14;
15 + 16;
17 - 18;
e1[+19]+20;
e2[-21]-22;
func1(+23)+24;
func2(-25)-26;
)";

    const auto Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, TestStr + strlen(TestStr), TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "+1.0"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "-2.0"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "a1"}, {TestTokenType::MathOp, "+"}, {TestTokenType::Identifier, "a2"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "b1"}, {TestTokenType::MathOp, "-"}, {TestTokenType::Identifier, "b2"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "c1"}, {TestTokenType::MathOp, "+"}, {TestTokenType::NumericConstant, "3"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "3.5"}, {TestTokenType::MathOp, "+"}, {TestTokenType::Identifier, "c2"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "d1"}, {TestTokenType::MathOp, "-"}, {TestTokenType::NumericConstant, "10.0"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "-20.0"}, {TestTokenType::MathOp, "+"}, {TestTokenType::Identifier, "d2"}}));

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "e1"}, {TestTokenType::MathOp, "+"}, {TestTokenType::NumericConstant, "+4.1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "e2"}, {TestTokenType::MathOp, "+"}, {TestTokenType::NumericConstant, "-4.2"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "e3"}, {TestTokenType::MathOp, "-"}, {TestTokenType::NumericConstant, "+4.3"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "e4"}, {TestTokenType::MathOp, "-"}, {TestTokenType::NumericConstant, "-4.4"}}));

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "d1"}, {TestTokenType::MathOp, "+"}, {TestTokenType::MathOp, "+"}, {TestTokenType::NumericConstant, "5.1"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "d2"}, {TestTokenType::MathOp, "+"}, {TestTokenType::MathOp, "-"}, {TestTokenType::NumericConstant, "5.2"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "d3"}, {TestTokenType::MathOp, "-"}, {TestTokenType::MathOp, "+"}, {TestTokenType::NumericConstant, "5.3"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::Identifier, "d4"}, {TestTokenType::MathOp, "-"}, {TestTokenType::MathOp, "-"}, {TestTokenType::NumericConstant, "5.4"}}));

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "11"}, {TestTokenType::MathOp, "+"}, {TestTokenType::NumericConstant, "12"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "13"}, {TestTokenType::MathOp, "-"}, {TestTokenType::NumericConstant, "14"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "15"}, {TestTokenType::MathOp, "+"}, {TestTokenType::NumericConstant, "16"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "17"}, {TestTokenType::MathOp, "-"}, {TestTokenType::NumericConstant, "18"}}));

    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "+19"}, {TestTokenType::ClosingSquareBracket, "]"}, {TestTokenType::MathOp, "+"}, {TestTokenType::NumericConstant, "20"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "-21"}, {TestTokenType::ClosingSquareBracket, "]"}, {TestTokenType::MathOp, "-"}, {TestTokenType::NumericConstant, "22"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "+23"}, {TestTokenType::ClosingParen, ")"}, {TestTokenType::MathOp, "+"}, {TestTokenType::NumericConstant, "24"}}));
    EXPECT_TRUE(FindTokenSequence(Tokens, {{TestTokenType::NumericConstant, "-25"}, {TestTokenType::ClosingParen, ")"}, {TestTokenType::MathOp, "-"}, {TestTokenType::NumericConstant, "26"}}));
}

TEST(Common_ParsingTools, Tokenizer_Errors)
{
    auto TestError = [](const char* Str, const char* Error) {
        TestingEnvironment::ErrorScope ExpectedErrors{"Unable to tokenize string", Error};
        try
        {
            const auto Tokens = Tokenize<TestToken, std::vector<TestToken>>(Str, Str + strlen(Str), TestToken::Create, TestToken::FindType);
            (void)Tokens;
        }
        catch (const std::runtime_error&)
        {
            return;
        }
        ADD_FAILURE() << "Tokenize must throw an exception";
    };


    TestError(R"(
void main()
{
    /* Open comment
}
)",
              R"(Unable to find the end of the multiline comment.
void main()
{
    /* Open comment
    ^
})");


    TestError(R"(
void main()
{
    char* String = "Missing quotes
}
)",
              R"(Unable to find matching closing quotes.
void main()
{
    char* String = "Missing quotes
                   ^
})");


    TestError(R"(
#
void main()
{
}
)",
              R"(Missing preprocessor directive.

#
^
void main()
{)");


    TestError(R"(
#/*comment*/ define Macro
void main()
{
}
)",
              R"(Comments between # and preprocessor directive are currently not supported.

#/*comment*/ define Macro
^
void main()
{)");


    TestError(R"(
void main()
{
}
#)",
              R"(Missing preprocessor directive.
{
}
#
^)");
}


TEST(Common_ParsingTools, Tokenizer_FindFunction)
{
    static const char* TestStr = R"(
//NotAFunction0();

struct Test
{
    void NotAFunction1();
};

MACRO(NotAFunction2())

int array[NotAFunction3(10)];

//array<NotAFunction4()>

void main()
{
    Keyword1 Id Keyword2(Keyword3);
}

void NotAFunction5
)";

    const auto Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, TestStr + strlen(TestStr), TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    EXPECT_EQ(FindFunction(Tokens.begin(), Tokens.end(), "NotAFunction0"), Tokens.end());
    EXPECT_EQ(FindFunction(Tokens.begin(), Tokens.end(), "NotAFunction1"), Tokens.end());
    EXPECT_EQ(FindFunction(Tokens.begin(), Tokens.end(), "NotAFunction2"), Tokens.end());
    EXPECT_EQ(FindFunction(Tokens.begin(), Tokens.end(), "NotAFunction3"), Tokens.end());
    EXPECT_EQ(FindFunction(Tokens.begin(), Tokens.end(), "NotAFunction4"), Tokens.end());
    EXPECT_EQ(FindFunction(Tokens.begin(), Tokens.end(), "NotAFunction5"), Tokens.end());
    auto main_it = FindFunction(Tokens.begin(), Tokens.end(), "main");
    ASSERT_NE(main_it, Tokens.end());
}

TEST(Common_ParsingTools, FindMatchingBracket)
{
    static const char* TestStr = R"(
([(<{}>)])
{[(<{}>)]}
<[(<{}>)]>
[[(<{}>)]]
)]}><{[(
)";

    const auto* StrEnd = TestStr + strlen(TestStr);
    const auto  Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, StrEnd, TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    {
        auto TestMatchingBracket = [&](size_t StartIdx, size_t EndIdx, TestTokenType OpenBracketType, TestTokenType ClosingBracketType) //
        {
            ASSERT_EQ(Tokens[StartIdx].GetType(), OpenBracketType);
            ASSERT_EQ(Tokens[EndIdx].GetType(), ClosingBracketType);

            auto it = FindMatchingBracket(Tokens.begin(), Tokens.end(), Tokens.begin() + StartIdx);
            ASSERT_NE(it, Tokens.end());
            EXPECT_EQ(static_cast<size_t>(it - Tokens.begin()), EndIdx);
            it = FindMatchingBracket(Tokens.begin(), Tokens.end(), it);
            ASSERT_NE(it, Tokens.end());
            EXPECT_EQ(static_cast<size_t>(it - Tokens.begin()), StartIdx);
        };

        TestMatchingBracket(1, 10, TestTokenType::OpenParen, TestTokenType::ClosingParen);
        TestMatchingBracket(11, 20, TestTokenType::OpenBrace, TestTokenType::ClosingBrace);
        // Angle brackets are not currently detected
        //TestMatchingBracket(21, 30, TestTokenType::OpenAngleBracket, TestTokenType::ClosingAngleBracket);
        TestMatchingBracket(31, 40, TestTokenType::OpenSquareBracket, TestTokenType::ClosingSquareBracket);
    }

    {
        auto TestNoBracket = [&](size_t StartIdx, TestTokenType BracketType) //
        {
            ASSERT_EQ(Tokens[StartIdx].GetType(), BracketType);
            auto it = FindMatchingBracket(Tokens.begin(), Tokens.end(), Tokens.begin() + StartIdx);
            EXPECT_EQ(it, Tokens.end());
        };
        TestNoBracket(41, TestTokenType::ClosingParen);
        TestNoBracket(42, TestTokenType::ClosingSquareBracket);
        TestNoBracket(43, TestTokenType::ClosingBrace);
        //TestNoBracket(44, TestTokenType::ClosingAngleBracket);
        //TestNoBracket(45, TestTokenType::OpenAngleBracket);
        TestNoBracket(46, TestTokenType::OpenBrace);
        TestNoBracket(47, TestTokenType::OpenSquareBracket);
        TestNoBracket(48, TestTokenType::OpenParen);
    }
}


TEST(Common_ParsingTools, GetTokenContext)
{
    static const char* TestStr =
        "A1 A2 A3 A4 A5\n"  // 1
        "B1 B2 B3 B4 B5\n"  // 6
        "C1 C2 C3 C4 C5\n"  // 11
        "D1 D2 D3 D4 D5\n"  // 16
        "E1 E2 E3 E4 E5\n"  // 21
        "F1 F2 F3 F4 F5\n"; // 26

    const auto* StrEnd = TestStr + strlen(TestStr);
    const auto  Tokens = Tokenize<TestToken, std::vector<TestToken>>(TestStr, StrEnd, TestToken::Create, TestToken::FindType);
    EXPECT_STREQ(BuildSource(Tokens).c_str(), TestStr);

    {
        static const char* RefStr =
            "A1 A2 A3 A4 A5\n"
            "^";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 1, 0).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "A1 A2 A3 A4 A5\n"
            "            ^";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 5, 0).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "\n"
            "C1 C2 C3 C4 C5\n"
            "^";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 11, 0).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "\n"
            "C1 C2 C3 C4 C5\n"
            "      ^";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 13, 0).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "\n"
            "C1 C2 C3 C4 C5\n"
            "            ^";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 15, 0).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "\n"
            "F1 F2 F3 F4 F5\n"
            "^";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 26, 0).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "\n"
            "F1 F2 F3 F4 F5\n"
            "            ^";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 30, 0).c_str(), RefStr);
    }



    {
        static const char* RefStr =
            "A1 A2 A3 A4 A5\n"
            "^\n"
            "B1 B2 B3 B4 B5";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 1, 1).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "A1 A2 A3 A4 A5\n"
            "            ^\n"
            "B1 B2 B3 B4 B5";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 5, 1).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "\n"
            "B1 B2 B3 B4 B5\n"
            "C1 C2 C3 C4 C5\n"
            "^\n"
            "D1 D2 D3 D4 D5";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 11, 1).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "\n"
            "B1 B2 B3 B4 B5\n"
            "C1 C2 C3 C4 C5\n"
            "      ^\n"
            "D1 D2 D3 D4 D5";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 13, 1).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "\n"
            "B1 B2 B3 B4 B5\n"
            "C1 C2 C3 C4 C5\n"
            "            ^\n"
            "D1 D2 D3 D4 D5";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 15, 1).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "\n"
            "E1 E2 E3 E4 E5\n"
            "F1 F2 F3 F4 F5\n"
            "^\n";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 26, 1).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "\n"
            "E1 E2 E3 E4 E5\n"
            "F1 F2 F3 F4 F5\n"
            "            ^\n";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.begin() + 30, 1).c_str(), RefStr);
    }

    {
        static const char* RefStr =
            "\n"
            "E1 E2 E3 E4 E5\n"
            "F1 F2 F3 F4 F5\n"
            "\n"
            "^";
        EXPECT_STREQ(GetTokenContext(Tokens.begin(), Tokens.end(), Tokens.end(), 2).c_str(), RefStr);
    }

    {
        std::vector<TestToken> Empty;
        EXPECT_STREQ(GetTokenContext(Empty.begin(), Empty.end(), Empty.begin(), 2).c_str(), "");
    }
}

} // namespace
