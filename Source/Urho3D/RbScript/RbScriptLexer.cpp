// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RbScriptLexer.h"

#include <cctype>

namespace Urho3D
{

namespace
{

bool IsIdentifierStart(char ch)
{
    return ch == '_' || std::isalpha(static_cast<unsigned char>(ch)) != 0;
}

bool IsIdentifierPart(char ch)
{
    return ch == '_' || std::isalnum(static_cast<unsigned char>(ch)) != 0;
}

RbScriptTokenKind KeywordKind(const ea::string& value)
{
    if (value == "true") return RbScriptTokenKind::True;
    if (value == "false") return RbScriptTokenKind::False;
    if (value == "null") return RbScriptTokenKind::Null;
    if (value == "module") return RbScriptTokenKind::Module;
    if (value == "use") return RbScriptTokenKind::Use;
    if (value == "script") return RbScriptTokenKind::Script;
    if (value == "fn") return RbScriptTokenKind::Fn;
    if (value == "return") return RbScriptTokenKind::Return;
    if (value == "if") return RbScriptTokenKind::If;
    if (value == "else") return RbScriptTokenKind::Else;
    if (value == "while") return RbScriptTokenKind::While;
    if (value == "for") return RbScriptTokenKind::For;
    if (value == "in") return RbScriptTokenKind::In;
    if (value == "let") return RbScriptTokenKind::Let;
    if (value == "var") return RbScriptTokenKind::Var;
    if (value == "const") return RbScriptTokenKind::Const;
    if (value == "struct") return RbScriptTokenKind::Struct;
    if (value == "enum") return RbScriptTokenKind::Enum;
    if (value == "class") return RbScriptTokenKind::Class;
    if (value == "signal") return RbScriptTokenKind::Signal;
    if (value == "on") return RbScriptTokenKind::On;
    if (value == "async") return RbScriptTokenKind::Async;
    if (value == "await") return RbScriptTokenKind::Await;
    if (value == "emit") return RbScriptTokenKind::Emit;
    if (value == "match") return RbScriptTokenKind::Match;
    if (value == "break") return RbScriptTokenKind::Break;
    if (value == "continue") return RbScriptTokenKind::Continue;
    if (value == "public") return RbScriptTokenKind::Public;
    if (value == "private") return RbScriptTokenKind::Private;
    if (value == "static") return RbScriptTokenKind::Static;
    return RbScriptTokenKind::Identifier;
}

} // namespace

RbScriptLexer::RbScriptLexer(const ea::string& source, const ea::string& fileName)
    : source_(source)
    , fileName_(fileName)
{
}

ea::vector<RbScriptToken> RbScriptLexer::Tokenize()
{
    offset_ = 0;
    line_ = 1;
    column_ = 1;
    diagnostics_.clear();

    ea::vector<RbScriptToken> tokens;
    while (!IsAtEnd())
    {
        SkipTrivia();
        if (!IsAtEnd())
            ScanToken(tokens);
    }

    RbScriptSourcePosition end{offset_, line_, column_};
    AddToken(tokens, RbScriptTokenKind::EndOfFile, end, {});
    return tokens;
}

char RbScriptLexer::Peek(unsigned lookahead) const
{
    const unsigned index = offset_ + lookahead;
    return index < source_.size() ? source_[index] : '\0';
}

char RbScriptLexer::Advance()
{
    if (IsAtEnd())
        return '\0';

    const char result = source_[offset_++];
    if (result == '\n')
    {
        ++line_;
        column_ = 1;
    }
    else
        ++column_;
    return result;
}

bool RbScriptLexer::Match(char expected)
{
    if (Peek() != expected)
        return false;
    Advance();
    return true;
}

bool RbScriptLexer::IsAtEnd() const
{
    return offset_ >= source_.size();
}

void RbScriptLexer::SkipTrivia()
{
    for (;;)
    {
        const char ch = Peek();
        if (ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f' || ch == '\r' || ch == '\n')
        {
            Advance();
            continue;
        }

        if (ch == '/' && Peek(1) == '/')
        {
            Advance();
            Advance();
            while (!IsAtEnd() && Peek() != '\n')
                Advance();
            continue;
        }

        if (ch == '/' && Peek(1) == '*')
        {
            const RbScriptSourcePosition begin{offset_, line_, column_};
            Advance();
            Advance();
            bool closed = false;
            while (!IsAtEnd())
            {
                if (Peek() == '*' && Peek(1) == '/')
                {
                    Advance();
                    Advance();
                    closed = true;
                    break;
                }
                Advance();
            }
            if (!closed)
            {
                const RbScriptSourcePosition end{offset_, line_, column_};
                AddError("E1002", "Unterminated block comment", begin, end);
            }
            continue;
        }
        break;
    }
}

void RbScriptLexer::ScanToken(ea::vector<RbScriptToken>& tokens)
{
    const RbScriptSourcePosition begin{offset_, line_, column_};
    const char ch = Advance();

    if (IsIdentifierStart(ch))
    {
        ScanIdentifierOrKeyword(tokens, begin);
        return;
    }
    if (std::isdigit(static_cast<unsigned char>(ch)) != 0)
    {
        ScanNumber(tokens, begin);
        return;
    }
    if (ch == '"')
    {
        ScanString(tokens, begin);
        return;
    }

    auto add = [&](RbScriptTokenKind kind) { AddToken(tokens, kind, begin); };
    switch (ch)
    {
    case '{': add(RbScriptTokenKind::LeftBrace); break;
    case '}': add(RbScriptTokenKind::RightBrace); break;
    case '(': add(RbScriptTokenKind::LeftParen); break;
    case ')': add(RbScriptTokenKind::RightParen); break;
    case '[': add(RbScriptTokenKind::LeftBracket); break;
    case ']': add(RbScriptTokenKind::RightBracket); break;
    case ',': add(RbScriptTokenKind::Comma); break;
    case '.': add(RbScriptTokenKind::Dot); break;
    case ':': add(Match(':') ? RbScriptTokenKind::Scope : RbScriptTokenKind::Colon); break;
    case ';': add(RbScriptTokenKind::Semicolon); break;
    case '@': add(RbScriptTokenKind::At); break;
    case '?': add(RbScriptTokenKind::Question); break;
    case '+': add(RbScriptTokenKind::Plus); break;
    case '-': add(Match('>') ? RbScriptTokenKind::Arrow : RbScriptTokenKind::Minus); break;
    case '*': add(RbScriptTokenKind::Star); break;
    case '/': add(RbScriptTokenKind::Slash); break;
    case '%': add(RbScriptTokenKind::Percent); break;
    case '=': add(Match('=') ? RbScriptTokenKind::EqualEqual : RbScriptTokenKind::Equal); break;
    case '!': add(Match('=') ? RbScriptTokenKind::BangEqual : RbScriptTokenKind::Bang); break;
    case '<': add(Match('=') ? RbScriptTokenKind::LessEqual : RbScriptTokenKind::Less); break;
    case '>': add(Match('=') ? RbScriptTokenKind::GreaterEqual : RbScriptTokenKind::Greater); break;
    case '&': add(Match('&') ? RbScriptTokenKind::AndAnd : RbScriptTokenKind::Ampersand); break;
    case '|': add(Match('|') ? RbScriptTokenKind::OrOr : RbScriptTokenKind::Pipe); break;
    default:
        {
            const RbScriptSourcePosition end{offset_, line_, column_};
            AddError("E1001", "Unexpected character in rbscript source", begin, end);
            AddToken(tokens, RbScriptTokenKind::Invalid, begin);
        }
        break;
    }
}

void RbScriptLexer::ScanIdentifierOrKeyword(ea::vector<RbScriptToken>& tokens, RbScriptSourcePosition begin)
{
    while (IsIdentifierPart(Peek()))
        Advance();

    const ea::string text = source_.substr(begin.offset, offset_ - begin.offset);
    AddToken(tokens, KeywordKind(text), begin, text);
}

void RbScriptLexer::ScanNumber(ea::vector<RbScriptToken>& tokens, RbScriptSourcePosition begin)
{
    while (std::isdigit(static_cast<unsigned char>(Peek())) != 0 || Peek() == '_')
        Advance();

    bool floating = false;
    if (Peek() == '.' && std::isdigit(static_cast<unsigned char>(Peek(1))) != 0)
    {
        floating = true;
        Advance();
        while (std::isdigit(static_cast<unsigned char>(Peek())) != 0 || Peek() == '_')
            Advance();
    }

    if (Peek() == 'e' || Peek() == 'E')
    {
        floating = true;
        Advance();
        if (Peek() == '+' || Peek() == '-')
            Advance();
        while (std::isdigit(static_cast<unsigned char>(Peek())) != 0 || Peek() == '_')
            Advance();
    }

    if (Peek() == 'f' || Peek() == 'F')
    {
        floating = true;
        Advance();
    }

    const ea::string text = source_.substr(begin.offset, offset_ - begin.offset);
    AddToken(tokens, floating ? RbScriptTokenKind::FloatLiteral : RbScriptTokenKind::IntegerLiteral, begin, text);
}

void RbScriptLexer::ScanString(ea::vector<RbScriptToken>& tokens, RbScriptSourcePosition begin)
{
    ea::string decoded;
    bool closed = false;
    while (!IsAtEnd())
    {
        const char ch = Advance();
        if (ch == '"')
        {
            closed = true;
            break;
        }
        if (ch == '\n' || ch == '\r')
        {
            const RbScriptSourcePosition end{offset_, line_, column_};
            AddError("E1004", "Newline inside string literal", begin, end);
            break;
        }
        if (ch != '\\')
        {
            decoded += ch;
            continue;
        }

        const char escaped = Advance();
        switch (escaped)
        {
        case 'n': decoded += '\n'; break;
        case 'r': decoded += '\r'; break;
        case 't': decoded += '\t'; break;
        case '0': decoded += '\0'; break;
        case '\\': decoded += '\\'; break;
        case '"': decoded += '"'; break;
        default:
            {
                const RbScriptSourcePosition errorBegin{offset_ - 1, line_, column_ - 1};
                const RbScriptSourcePosition errorEnd{offset_, line_, column_};
                AddError("E1003", "Unknown escape sequence in string literal", errorBegin, errorEnd);
                decoded += escaped;
            }
            break;
        }
    }

    if (!closed)
    {
        const RbScriptSourcePosition end{offset_, line_, column_};
        AddError("E1005", "Unterminated string literal", begin, end);
    }
    AddToken(tokens, RbScriptTokenKind::StringLiteral, begin, decoded);
}

void RbScriptLexer::AddToken(ea::vector<RbScriptToken>& tokens, RbScriptTokenKind kind,
    RbScriptSourcePosition begin, const ea::string& lexeme)
{
    const RbScriptSourcePosition end{offset_, line_, column_};
    RbScriptToken token;
    token.kind = kind;
    token.lexeme = lexeme.empty() && kind != RbScriptTokenKind::StringLiteral
        ? source_.substr(begin.offset, offset_ - begin.offset)
        : lexeme;
    token.span = {begin, end};
    tokens.push_back(token);
}

void RbScriptLexer::AddError(const ea::string& code, const ea::string& message,
    RbScriptSourcePosition begin, RbScriptSourcePosition end)
{
    RbScriptDiagnostic diagnostic;
    diagnostic.severity = RbScriptDiagnosticSeverity::Error;
    diagnostic.file = fileName_;
    diagnostic.code = code;
    diagnostic.message = message;
    diagnostic.span = {begin, end};
    diagnostics_.push_back(diagnostic);
}

void RbScriptLexer::AdvanceLine()
{
    ++line_;
    column_ = 1;
}

} // namespace Urho3D
