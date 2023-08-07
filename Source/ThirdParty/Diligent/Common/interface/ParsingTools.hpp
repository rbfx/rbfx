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

#pragma once

/// \file
/// Parsing tools

#include <cstring>
#include <sstream>

#include "../../Primitives/interface/BasicTypes.h"
#include "../../Primitives/interface/FlagEnum.h"
#include "../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "StringTools.h"

namespace Diligent
{

namespace Parsing
{

/// Returns true if the character is a white space or tab
inline bool IsWhitespace(Char Symbol) noexcept
{
    return Symbol == ' ' || Symbol == '\t';
}

/// Returns true if the character is a new line character
inline bool IsNewLine(Char Symbol) noexcept
{
    return Symbol == '\r' || Symbol == '\n';
}

/// Returns true if the character is a delimiter symbol (white space or new line)
inline bool IsDelimiter(Char Symbol) noexcept
{
    return Symbol == ' ' || Symbol == '\t' || Symbol == '\r' || Symbol == '\n';
}

/// Returns true if the character is a statement separator symbol
inline bool IsStatementSeparator(Char Symbol) noexcept
{
    return Symbol == ';' || Symbol == '}';
}

/// Returns true if the character is a digit between 0 and 9
inline bool IsDigit(Char Symbol) noexcept
{
    return Symbol >= '0' && Symbol <= '9';
}

/// Skips all characters until the end of the line.

/// \param[inout] Pos          - starting position.
/// \param[in]    End          - end of the input string.
/// \param[in]    GoToNextLine - whether to go to the next line.
///
/// \return       If GoToNextLine is true, the position following the
///               new line character at the end of the string.
///               If GoToNextLine is false, the position of the new line
///               character at the end of the string.
///
/// \remarks      CRLF ending (\r\n) is treated as a single new line separator.
template <typename InteratorType>
InteratorType SkipLine(const InteratorType& Start, const InteratorType& End, bool GoToNextLine = false) noexcept
{
    auto Pos = Start;
    while (Pos != End && *Pos != '\0' && !IsNewLine(*Pos))
        ++Pos;
    if (GoToNextLine && Pos != End && IsNewLine(*Pos))
    {
        ++Pos;
        if (Pos[-1] == '\r' && Pos != End && Pos[0] == '\n')
        {
            ++Pos;
            // treat \r\n as a single ending
        }
    }
    return Pos;
}


/// Flags controlling what kind of comments the SkipComment function should skip.
enum SKIP_COMMENT_FLAGS : Uint32
{
    /// Skip no comments
    SKIP_COMMENT_FLAG_NONE = 0u,

    /// Skip single-line comment
    SKIP_COMMENT_FLAG_SINGLE_LINE = 1u << 0u,

    /// Skip multi-line comment
    SKIP_COMMENT_FLAG_MULTILINE = 1u << 1u,

    /// Skip all kinds of comments
    SKIP_COMMENT_FLAG_ALL = SKIP_COMMENT_FLAG_SINGLE_LINE | SKIP_COMMENT_FLAG_MULTILINE
};
DEFINE_FLAG_ENUM_OPERATORS(SKIP_COMMENT_FLAGS)


/// Skips single-line and multi-line comments starting from the given position.

/// \param[inout] Start - starting position.
/// \param[in]    End   - end of the input string.
/// \param[in]    Flags - flags controlling what kind of comments to skip.
///
/// \return     if the comment is found, the position immediately following
///             the end of the comment; starting position otherwise.
///
/// \remarks    In case of an error while parsing the comment (e.g. /* is not
///             closed), the function throws an exception of type
///             std::pair<InteratorType, const char*>, where first is the position
///             of the error, and second is the error description.
template <typename InteratorType>
InteratorType SkipComment(const InteratorType& Start, const InteratorType& End, SKIP_COMMENT_FLAGS Flags = SKIP_COMMENT_FLAG_ALL) noexcept(false)
{
    auto Pos = Start;
    if (Pos == End || *Pos == '\0')
        return Pos;

    //  // Comment       /* Comment
    //  ^                ^
    //  Pos              Pos
    if (*Pos != '/')
        return Pos;

    ++Pos;
    //  // Comment       /* Comment
    //   ^                ^
    //   Pos              Pos
    if (Pos == End ||
        !((*Pos == '/' && (Flags & SKIP_COMMENT_FLAG_SINGLE_LINE) != 0) ||
          (*Pos == '*' && (Flags & SKIP_COMMENT_FLAG_MULTILINE) != 0)))
        return Start;

    if (*Pos == '/')
    {
        ++Pos;
        //  // Comment
        //    ^
        //    Pos

        return SkipLine(Pos, End, true);
        //  // Comment
        //
        //  ^
        //  Pos
    }
    else
    {
        VERIFY_EXPR(*Pos == '*');
        ++Pos;
        //  /* Comment
        //    ^
        while (Pos != End && *Pos != '\0')
        {
            if (*Pos == '*')
            {
                //  /* Comment */
                //             ^
                //             Pos
                ++Pos;

                if (Pos != End && *Pos == '/')
                {
                    //  /* Comment */
                    //              ^
                    //              Pos

                    ++Pos;
                    //  /* Comment */
                    //               ^
                    //              Pos
                    return Pos;
                }
            }
            else
            {
                ++Pos;
            }
        }

        throw std::pair<InteratorType, const char*>{Start, "Unable to find the end of the multiline comment."};
    }
    // Unreachable
}

/// Skips all delimiters starting from the given position.

/// \param[inout] Pos        - starting position.
/// \param[in]    End        - end of the input string.
/// \param[in]    Delimiters - optional string containing custom delimiters.
///
/// \return       position of the first non-delimiter character.
template <typename InteratorType>
InteratorType SkipDelimiters(const InteratorType& Start, const InteratorType& End, const char* Delimiters = nullptr) noexcept
{
    auto Pos = Start;
    if (Delimiters != nullptr)
    {
        while (Pos != End && strchr(Delimiters, *Pos))
            ++Pos;
    }
    else
    {
        while (Pos != End && IsDelimiter(*Pos))
            ++Pos;
    }
    return Pos;
}


/// Skips all comments and all delimiters starting from the given position.

/// \param[inout] Pos          - starting position.
/// \param[in]    End          - end of the input string.
/// \param[in]    Delimiters   - optional string containing custom delimiters.
/// \param[in]    CommentFlags - optional flags controlling what kind of comments to skip.
///
/// \return true  position of the first non-comment non-delimiter character.
///
/// \remarks    In case of a parsing error (which means there is an
///             open multi-line comment /*... ), the function throws an exception
///             of type std::pair<InteratorType, const char*>, where first is the position
///             of the error, and second is the error description.
template <typename IteratorType>
IteratorType SkipDelimitersAndComments(const IteratorType& Start,
                                       const IteratorType& End,
                                       const char*         Delimiters   = nullptr,
                                       SKIP_COMMENT_FLAGS  CommentFlags = SKIP_COMMENT_FLAG_ALL) noexcept(false)
{
    auto Pos = Start;
    while (Pos != End && *Pos != '\0')
    {
        const auto BlockStart = Pos;

        Pos = SkipDelimiters(Pos, End, Delimiters);
        Pos = SkipComment(Pos, End, CommentFlags); // May throw
        if (Pos == BlockStart)
            break;
    };

    return Pos;
}


/// Skips one identifier starting from the given position.

/// \param[inout] Pos - starting position.
/// \param[in]    End - end of the input string.
///
/// \return     position immediately following the last character of identifier.
template <typename IteratorType>
IteratorType SkipIdentifier(const IteratorType& Start, const IteratorType& End) noexcept
{
    if (Start == End)
        return Start;

    auto Pos = Start;
    if (isalpha(*Pos) || *Pos == '_')
        ++Pos;
    else
        return Pos;

    while (Pos != End && (isalnum(*Pos) || *Pos == '_'))
        ++Pos;

    return Pos;
}


/// Skips a floating point number starting from the given position.

/// \param[inout] Pos - starting position.
/// \param[in]    End - end of the input string.
///
/// \return     position immediately following the last character of the number.
template <typename IteratorType>
IteratorType SkipFloatNumber(const IteratorType& Start, const IteratorType& End) noexcept
{
    auto Pos = Start;

#define CHECK_END()                 \
    do                              \
    {                               \
        if (c == End || *c == '\0') \
            return Pos;             \
    } while (false)

    auto c = Pos;
    CHECK_END();

    if (*c == '+' || *c == '-')
        ++c;
    CHECK_END();

    if (*c == '0' && IsNum(c[1]))
    {
        // 01 is invalid
        Pos = c + 1;
        return Pos;
    }

    const auto HasIntegerPart = IsNum(*c);
    if (HasIntegerPart)
    {
        while (c != End && IsNum(*c))
            Pos = ++c;
        CHECK_END();
    }

    const auto HasDecimalPart = (*c == '.');
    if (HasDecimalPart)
    {
        ++c;
        if (HasIntegerPart)
        {
            // . as well as +. or -. are not valid numbers, however 0., +0., and -0. are.
            Pos = c;
        }

        while (c != End && IsNum(*c))
            Pos = ++c;
        CHECK_END();
    }

    const auto HasExponent = (*c == 'e' || *c == 'E');
    if (HasExponent)
    {
        if (!HasIntegerPart)
        {
            // .e, e, e+1, +.e are invalid
            return Pos;
        }

        ++c;
        if (c == End || (*c != '+' && *c != '-'))
        {
            // 10e&
            return Pos;
        }

        ++c;
        if (c == End || !IsNum(*c))
        {
            // 10e+x
            return Pos;
        }

        while (c != End && IsNum(*c))
            Pos = ++c;
    }

    if ((HasDecimalPart || HasExponent) && c != End && Pos > Start && (*c == 'f' || *c == 'F'))
    {
        // 10.f, 10e+3f, 10.e+3f, 10.4e+3f
        Pos = ++c;
    }
#undef CHECK_END

    return Pos;
}


/// Splits string into chunks separated by comments and delimiters.
///
/// \param [in] Start   - start of the string to split.
/// \param [in] End     - end of the string to split.
/// \param [in] Handler - user-provided handler to call for each chunk.
///
/// \remarks    The function starts from the beginning of the strings
///             and splits it into chunks separated by comments and delimiters.
///             For each chunk, it calls the user-provided handler and passes
///             the start of the preceding comments/delimiters part. The handler
///             must then process the text at the current position and move the pointer.
///             It should return true to continue processing, and false to stop it.
///
///             In case of a parsing error, the function throws an exception
///             of type std::pair<InteratorType, const char*>, where first is the position
///             of the error, and second is the error description.
template <typename IteratorType, typename HandlerType>
void SplitString(const IteratorType& Start, const IteratorType& End, HandlerType&& Handler) noexcept(false)
{
    auto Pos = Start;
    while (Pos != End)
    {
        auto DelimStart = Pos;
        Pos             = SkipDelimitersAndComments(Pos, End); // May throw
        auto OrigPos    = Pos;
        if (!Handler(DelimStart, Pos))
            break;
        VERIFY(Pos == End || OrigPos != Pos, "Position has not been updated by the handler.");
    }
}


/// Prints a parsing context around the given position in the string.

/// \param[in]  Start    - start of the string.
/// \param[in]  End      - end of the string.
/// \param[in]  Pos      - position around which to print the context and
///                        which will be highlighted by ^.
/// \param[in]  NumLines - the number of lines above and below.
///
/// \return     string representing the context around the given position.
///
/// \remarks    The context looks like shown below:
///
///                 Lorem ipsum dolor sit amet, consectetur
///                 adipiscing elit, sed do eiusmod tempor
///                 incididunt ut labore et dolore magna aliqua.
///                                      ^
///                 Ut enim ad minim veniam, quis nostrud
///                 exercitation ullamco lab
///
template <typename IteratorType>
std::string GetContext(const IteratorType& Start, const IteratorType& End, IteratorType Pos, size_t NumLines) noexcept
{
    if (Start == End)
        return "";

    auto CtxStart = Pos;
    while (CtxStart > Start && !IsNewLine(CtxStart[-1]))
        --CtxStart;
    const size_t CharPos = Pos - CtxStart; // Position of the character in the line

    Pos = SkipLine(Pos, End);

    std::stringstream Ctx;
    {
        size_t LineAbove = 0;
        while (LineAbove < NumLines && CtxStart > Start)
        {
            VERIFY_EXPR(IsNewLine(CtxStart[-1]));
            if (CtxStart[-1] == '\n' && CtxStart > Start + 1 && CtxStart[-2] == '\r')
                --CtxStart;
            if (CtxStart > Start)
                --CtxStart;
            while (CtxStart > Start && !IsNewLine(CtxStart[-1]))
                --CtxStart;
            ++LineAbove;
        }
        VERIFY_EXPR(CtxStart == Start || IsNewLine(CtxStart[-1]));
        Ctx.write(&*CtxStart, Pos - CtxStart);
    }

    Ctx << std::endl;
    for (size_t i = 0; i < CharPos; ++i)
        Ctx << ' ';
    Ctx << '^';

    {
        auto   CtxEnd    = Pos;
        size_t LineBelow = 0;
        while (LineBelow < NumLines && CtxEnd != End && *CtxEnd != '\0')
        {
            if (*CtxEnd == '\r' && CtxEnd + 1 != End && CtxEnd[+1] == '\n')
                ++CtxEnd;
            if (CtxEnd != End)
                ++CtxEnd;
            CtxEnd = SkipLine(CtxEnd, End);
            ++LineBelow;
        }
        Ctx.write(&*Pos, CtxEnd - Pos);
    }

    return Ctx.str();
}


/// Tokenizes the given string using the C-language syntax

/// \param [in] SourceStart  - start of the source string.
/// \param [in] SourceEnd    - end of the source string.
/// \param [in] CreateToken  - a handler called every time a new token should
///                            be created.
/// \param [in] GetTokenType - a function that should return the token type
///                            for the given literal.
/// \return     Tokenized representation of the source string
///
/// \remarks    In case of a parsing error, the function throws std::runtime_error.
template <typename TokenClass,
          typename ContainerType,
          typename IteratorType,
          typename CreateTokenFuncType,
          typename GetTokenTypeFunctType>
ContainerType Tokenize(const IteratorType&   SourceStart,
                       const IteratorType&   SourceEnd,
                       CreateTokenFuncType   CreateToken,
                       GetTokenTypeFunctType GetTokenType) noexcept(false)
{
    using TokenType = typename TokenClass::TokenType;

    ContainerType Tokens;
    // Push empty node in the beginning of the list to facilitate
    // backwards searching
    Tokens.emplace_back(TokenClass{});

    try
    {
        SplitString(SourceStart, SourceEnd, [&](const auto& DelimStart, auto& Pos) {
            const auto DelimEnd = Pos;

            auto LiteralStart = Pos;
            auto LiteralEnd   = DelimStart;

            auto Type = TokenType::Undefined;

            if (Pos == SourceEnd)
            {
                Tokens.push_back(CreateToken(Type, DelimStart, DelimEnd, LiteralStart, Pos));
                return false;
            }

            auto AddDoubleCharToken = [&](TokenType DoubleCharType) {
                if (!Tokens.empty() && DelimStart == DelimEnd)
                {
                    auto& LastToken = Tokens.back();
                    if (LastToken.CompareLiteral(Pos, Pos + 1))
                    {
                        LastToken.SetType(DoubleCharType);
                        LastToken.ExtendLiteral(Pos, Pos + 1);
                        ++Pos;
                        return true;
                    }
                }

                return false;
            };

#define SINGLE_CHAR_TOKEN(TYPE) \
    Type = TokenType::TYPE;     \
    ++Pos;                      \
    break

            switch (*Pos)
            {
                case '#':
                    Type = TokenType::PreprocessorDirective;
                    ++Pos;
                    while (Pos != SourceEnd && (*Pos == ' ' || *Pos == '\t'))
                        ++Pos;
                    if (Pos == SourceEnd || *Pos == '\0' || *Pos == '\n')
                        throw std::pair<IteratorType, const char*>{LiteralStart, "Missing preprocessor directive."};
                    if (*Pos == '/')
                        throw std::pair<IteratorType, const char*>{LiteralStart, "Comments between # and preprocessor directive are currently not supported."};
                    Pos = SkipIdentifier(Pos, SourceEnd);
                    break;

                case '=':
                {
                    if (!Tokens.empty() && DelimStart == DelimEnd)
                    {
                        auto& LastToken = Tokens.back();
                        // +=, -=, *=, /=, %=, <<=, >>=, &=, |=, ^=
                        for (const char* op : {"+", "-", "*", "/", "%", "<<", ">>", "&", "|", "^"})
                        {
                            if (LastToken.CompareLiteral(op))
                            {
                                LastToken.SetType(TokenType::Assignment);
                                LastToken.ExtendLiteral(Pos, Pos + 1);
                                ++Pos;
                                return Pos != SourceEnd;
                            }
                        }

                        // <=, >=, ==, !=
                        for (const char* op : {"<", ">", "=", "!"})
                        {
                            if (LastToken.CompareLiteral(op))
                            {
                                LastToken.SetType(TokenType::ComparisonOp);
                                LastToken.ExtendLiteral(Pos, Pos + 1);
                                ++Pos;
                                return Pos != SourceEnd;
                            }
                        }
                    }

                    SINGLE_CHAR_TOKEN(Assignment);
                }

                case '|':
                case '&':
                    if (AddDoubleCharToken(TokenType::LogicOp))
                        return Pos != SourceEnd;
                    SINGLE_CHAR_TOKEN(BitwiseOp);

                case '<':
                case '>':
                    // Note: we do not distinguish between comparison operators
                    // and template arguments like in Texture2D<float> at this
                    // point.
                    if (AddDoubleCharToken(TokenType::BitwiseOp))
                        return Pos != SourceEnd;
                    SINGLE_CHAR_TOKEN(ComparisonOp);

                case '+':
                case '-':
                    if (AddDoubleCharToken(TokenType::IncDecOp))
                        return Pos != SourceEnd;
                    else
                    {
                        const auto& LastTokenType = !Tokens.empty() ? Tokens.back().GetType() : TokenType::Undefined;
                        if (!(LastTokenType == TokenType::Identifier ||        // a + 1
                              LastTokenType == TokenType::NumericConstant ||   // 1 + 2
                              LastTokenType == TokenType::ClosingParen ||      // ) + 3
                              LastTokenType == TokenType::ClosingSquareBracket // ] + 4
                              ))
                        {
                            auto NumEnd = SkipFloatNumber(Pos, SourceEnd);
                            if (Pos != NumEnd)
                            {
                                Type = TokenType::NumericConstant;
                                Pos  = NumEnd;
                                break;
                            }
                        }
                    }
                    SINGLE_CHAR_TOKEN(MathOp);

                case ':':
                    if (AddDoubleCharToken(TokenType::DoubleColon))
                        return Pos != SourceEnd;
                    SINGLE_CHAR_TOKEN(Colon);

                case '~':
                case '^':
                    SINGLE_CHAR_TOKEN(BitwiseOp);

                case '*':
                case '/':
                case '%':
                    SINGLE_CHAR_TOKEN(MathOp);

                case '!':
                    SINGLE_CHAR_TOKEN(LogicOp);

                case ',':
                    SINGLE_CHAR_TOKEN(Comma);

                case ';':
                    SINGLE_CHAR_TOKEN(Semicolon);

                case '?':
                    SINGLE_CHAR_TOKEN(QuestionMark);

                    //case '<': SINGLE_CHAR_TOKEN(OpenAngleBracket);
                    //case '>': SINGLE_CHAR_TOKEN(ClosingAngleBracket);

                case '(': SINGLE_CHAR_TOKEN(OpenParen);
                case ')': SINGLE_CHAR_TOKEN(ClosingParen);
                case '{': SINGLE_CHAR_TOKEN(OpenBrace);
                case '}': SINGLE_CHAR_TOKEN(ClosingBrace);
                case '[': SINGLE_CHAR_TOKEN(OpenSquareBracket);
                case ']': SINGLE_CHAR_TOKEN(ClosingSquareBracket);

                case '"':
                {
                    // Skip quotes
                    Type = TokenType::StringConstant;
                    ++LiteralStart;
                    ++Pos;
                    while (Pos != SourceEnd && *Pos != '\0' && *Pos != '"')
                        ++Pos;
                    if (Pos == SourceEnd || *Pos != '"')
                        throw std::pair<IteratorType, const char*>{LiteralStart - 1, "Unable to find matching closing quotes."};

                    LiteralEnd = Pos++;
                    break;
                }

                default:
                {
                    Pos = SkipIdentifier(Pos, SourceEnd);
                    if (LiteralStart != Pos)
                    {
                        Type = GetTokenType(LiteralStart, Pos);
                        if (Type == TokenType::Undefined)
                            Type = TokenType::Identifier;
                    }
                    else
                    {
                        Pos = SkipFloatNumber(Pos, SourceEnd);
                        if (LiteralStart != Pos)
                        {
                            Type = TokenType::NumericConstant;
                        }
                    }

                    if (Type == TokenType::Undefined)
                    {
                        ++Pos; // Add single character
                    }
                }
            }

            if (LiteralEnd == DelimStart)
                LiteralEnd = Pos;

            Tokens.push_back(CreateToken(Type, DelimStart, DelimEnd, LiteralStart, LiteralEnd));
            return Pos != SourceEnd;
        } //
        );
#undef SINGLE_CHAR_TOKEN
    }
    catch (const std::pair<IteratorType, const char*>& ErrInfo)
    {
        constexpr size_t NumContextLines = 2;
        LOG_ERROR_MESSAGE(ErrInfo.second, "\n", GetContext(SourceStart, SourceEnd, ErrInfo.first, NumContextLines));
        LOG_ERROR_AND_THROW("Unable to tokenize string.");
    }

    return Tokens;
}

template <typename TokenClass>
std::ostream& WriteToken(std::ostream& stream, const TokenClass& Token)
{
    Token.OutputDelimiter(stream);

    const auto Type = Token.GetType();
    using TokenType = decltype(Type);
    if (Type == TokenType::StringConstant)
        stream << "\"";
    Token.OutputLiteral(stream);
    if (Type == TokenType::StringConstant)
        stream << "\"";

    return stream;
}

/// Builds source string from tokens
template <typename ContainerType>
std::string BuildSource(const ContainerType& Tokens) noexcept
{
    std::stringstream stream;
    for (const auto& Token : Tokens)
    {
        WriteToken(stream, Token);
    }
    return stream.str();
}


/// Finds a function with the given name in the token scope
template <typename TokenIterType>
TokenIterType FindFunction(const TokenIterType& Start, const TokenIterType& End, const char* Name) noexcept
{
    if (Name == nullptr || *Name == '\0')
    {
        UNEXPECTED("Name must not be null");
        return End;
    }

    int BracketCount = 0;
    for (auto Token = Start; Token != End; ++Token)
    {
        const auto Type = Token->GetType();
        using TokenType = decltype(Type);
        switch (Type)
        {
            case TokenType::OpenBrace:
            case TokenType::OpenParen:
            case TokenType::OpenSquareBracket:
            case TokenType::OpenAngleBracket:
                ++BracketCount;
                break;

            case TokenType::ClosingBrace:
            case TokenType::ClosingParen:
            case TokenType::ClosingSquareBracket:
            case TokenType::ClosingAngleBracket:
                --BracketCount;
                if (BracketCount < 0)
                {
                    LOG_ERROR_MESSAGE("Brackets are not correctly balanced");
                    return End;
                }
                break;

            case TokenType::Identifier:
                if (BracketCount == 0)
                {
                    if (Token->CompareLiteral(Name))
                    {
                        auto NextToken = Token;
                        ++NextToken;
                        if (NextToken != End && NextToken->GetType() == TokenType::OpenParen)
                            return Token;
                    }
                }
                break;

            default:
                // Go to next token
                break;
        }
    }

    return End;
}



/// Searches for the matching bracket.
/// For open brackets, searches in the forward direction.
/// For closing brackets, searches backwards.
///
/// \param [in] Start - start of the string.
/// \param [in] End   - end of the string.
/// \param [in] Pos   - search starting position.
///
/// \return     position of the matching bracket, or End
///             if none is found.
template <typename TokenIterType>
TokenIterType FindMatchingBracket(const TokenIterType& Start,
                                  const TokenIterType& End,
                                  TokenIterType        Pos)
{
    if (Pos == End)
        return Pos;

    const auto OpenBracketType = Pos->GetType();
    using TokenType            = decltype(OpenBracketType);

    auto ClosingBracketType = TokenType::Undefined;
    bool SearchForward      = true;
    switch (OpenBracketType)
    {
        case TokenType::OpenBrace:
            ClosingBracketType = TokenType::ClosingBrace;
            break;

        case TokenType::OpenParen:
            ClosingBracketType = TokenType::ClosingParen;
            break;

        case TokenType::OpenSquareBracket:
            ClosingBracketType = TokenType::ClosingSquareBracket;
            break;

        case TokenType::OpenAngleBracket:
            ClosingBracketType = TokenType::ClosingAngleBracket;
            break;

        case TokenType::ClosingBrace:
            ClosingBracketType = TokenType::OpenBrace;
            SearchForward      = false;
            break;

        case TokenType::ClosingParen:
            ClosingBracketType = TokenType::OpenParen;
            SearchForward      = false;
            break;

        case TokenType::ClosingSquareBracket:
            ClosingBracketType = TokenType::OpenSquareBracket;
            SearchForward      = false;
            break;

        case TokenType::ClosingAngleBracket:
            ClosingBracketType = TokenType::OpenAngleBracket;
            SearchForward      = false;
            break;

        default:
            UNEXPECTED("One of the bracket types is expected");
            return Pos;
    }

    (SearchForward ? ++Pos : --Pos); // Skip open bracket
    int BracketCount = 1;

    auto UpdateBracketCount = [&](TokenType Type) //
    {
        if (Type == OpenBracketType)
            ++BracketCount;
        else if (Type == ClosingBracketType)
        {
            --BracketCount;
        }
        return BracketCount;
    };
    if (SearchForward)
    {
        while (Pos != End)
        {
            if (UpdateBracketCount(Pos->GetType()) == 0)
                return Pos;
            ++Pos;
        }
    }
    else
    {
        while (Pos != Start)
        {
            if (UpdateBracketCount(Pos->GetType()) == 0)
                return Pos;
            --Pos;
        }
    }
    return End;
}


/// Prints a parsing context around the given token.

/// \param[in]  Start            - start of the token range.
/// \param[in]  End              - end of the token range.
/// \param[in]  Pos              - position around which to print the context and
///                                which will be highlighted by ^.
/// \param[in]  NumAdjacentLines - the number of adjacent lines above and
///                                below to print.
///
/// \remarks    The context looks like shown below:
///
///                 Lorem ipsum dolor sit amet, consectetur
///                 adipiscing elit, sed do eiusmod tempor
///                 incididunt ut labore et dolore magna aliqua.
///                                      ^
///                 Ut enim ad minim veniam, quis nostrud
///                 exercitation ullamco lab
///
template <typename TokenIterType>
std::string GetTokenContext(const TokenIterType& Start,
                            const TokenIterType& End,
                            TokenIterType        Pos,
                            size_t               NumAdjacentLines)
{
    if (Start == End)
        return "";

    if (Pos == End)
        --Pos;

    //\n  ++ x ;
    //\n  ++ y ;
    //\n  if ( x != 0 )
    //         ^
    //\n      x += y ;
    //\n
    //\n  if ( y != 0 )
    //\n      x += 2 ;

    auto CountNewLines = [](const auto& Start, const auto& End) //
    {
        size_t NumNewLines = 0;

        auto it = Start;
        while (it != End)
        {
            if (IsNewLine(*it))
            {
                ++NumNewLines;
                auto next_it = it;
                ++next_it;
                if (next_it != End && *next_it == '\r')
                {
                    // Treat \r\n as a single ending
                    it = next_it;
                }
            }
            ++it;
        }
        return NumNewLines;
    };

    std::stringstream Ctx;

    // Find the first token in the current line
    auto   CurrLineStart = Pos;
    size_t NumLinesAbove = 0;
    while (CurrLineStart != Start)
    {
        const auto DelimRange = CurrLineStart->GetDelimiter();
        NumLinesAbove += CountNewLines(DelimRange.first, DelimRange.second);
        if (NumLinesAbove > 0)
            break;
        --CurrLineStart;
    }
    //\n  if( x != 0 )
    //    ^

    // Find the first token in the line NumAdjacentLines above
    auto CtxStart = CurrLineStart;
    while (CtxStart != Start && NumLinesAbove <= NumAdjacentLines)
    {
        --CtxStart;
        const auto DelimRange = CtxStart->GetDelimiter();
        NumLinesAbove += CountNewLines(DelimRange.first, DelimRange.second);
    }
    //\n  ++ x ;
    //    ^
    //\n  ++ y ;
    //\n  if ( x != 0 )

    // Write everything from the top line up to the current line start
    auto Token = CtxStart;
    for (; Token != CurrLineStart; ++Token)
    {
        WriteToken(Ctx, *Token);
    }

    //\n  if ( x != 0 )
    //    ^

    // Accumulate whitespaces preceding current token
    std::string Spaces;
    bool        AccumWhiteSpaces = true;
    while (Token != End)
    {
        const auto DelimRange = Token->GetDelimiter();
        if (Token != CurrLineStart && CountNewLines(DelimRange.first, DelimRange.second) > 0)
            break;

        if (AccumWhiteSpaces)
        {
            for (auto it = DelimRange.first; it != DelimRange.second; ++it)
            {
                if (IsNewLine(*it))
                    Spaces.clear();
                else if (*it == '\t')
                    Spaces.push_back(*it);
                else
                    Spaces.push_back(' ');
            }
        }

        // Accumulate spaces until we encounter current token
        if (Token == Pos)
            AccumWhiteSpaces = false;

        if (AccumWhiteSpaces)
        {
            Spaces.append(Token->GetLiteralLen(), ' ');
        }

        WriteToken(Ctx, *Token);
        ++Token;
    }

    // Write ^ on the line below
    Ctx << std::endl
        << Spaces << '^';

    // Write NumAdjacentLines lines below current line
    size_t NumLinesBelow = 0;
    while (Token != End)
    {
        const auto DelimRange = Token->GetDelimiter();
        NumLinesBelow += CountNewLines(DelimRange.first, DelimRange.second);
        if (NumLinesBelow > NumAdjacentLines)
            break;

        WriteToken(Ctx, *Token);
        ++Token;
    }

    return Ctx.str();
}

} // namespace Parsing

} // namespace Diligent
