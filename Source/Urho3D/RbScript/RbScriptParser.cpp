// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RbScriptParser.h"

namespace Urho3D
{

namespace
{

RbScriptSourceSpan SpanFrom(const RbScriptSourceSpan& begin, const RbScriptSourceSpan& end)
{
    return {begin.begin, end.end};
}

ea::string JoinName(const ea::vector<ea::string>& parts, const char* separator = "::")
{
    ea::string result;
    for (unsigned i = 0; i < parts.size(); ++i)
    {
        if (i)
            result += separator;
        result += parts[i];
    }
    return result;
}

std::unique_ptr<RbScriptExpression> MakeExpression(RbScriptExpressionKind kind,
    const RbScriptSourceSpan& span, const ea::string& text = {})
{
    auto result = std::make_unique<RbScriptExpression>();
    result->kind = kind;
    result->span = span;
    result->text = text;
    return result;
}

std::unique_ptr<RbScriptStatement> MakeStatement(RbScriptStatementKind kind, const RbScriptSourceSpan& span)
{
    auto result = std::make_unique<RbScriptStatement>();
    result->kind = kind;
    result->span = span;
    return result;
}

} // namespace

bool RbScriptModule::IsValid() const
{
    for (const RbScriptDiagnostic& diagnostic : diagnostics)
    {
        if (diagnostic.severity == RbScriptDiagnosticSeverity::Error)
            return false;
    }
    return !name.empty() || !scripts.empty();
}

RbScriptParser::RbScriptParser(const ea::vector<RbScriptToken>& tokens, const ea::string& fileName)
    : tokens_(tokens)
    , fileName_(fileName)
{
}

RbScriptModule RbScriptParser::ParseModule()
{
    current_ = 0;
    diagnostics_.clear();
    RbScriptModule module;

    if (Match(RbScriptTokenKind::Module))
    {
        const ea::vector<ea::string> parts = ParseQualifiedName();
        module.name = JoinName(parts);
        Consume(RbScriptTokenKind::Semicolon, "E2001", "Expected ';' after module declaration");
    }
    else
        AddError("E2000", "Expected module declaration", Current());

    while (!IsAtEnd())
    {
        if (Match(RbScriptTokenKind::Use))
        {
            const ea::vector<ea::string> parts = ParseQualifiedName();
            module.imports.push_back(JoinName(parts));
            Consume(RbScriptTokenKind::Semicolon, "E2002", "Expected ';' after use declaration");
        }
        else if (Check(RbScriptTokenKind::Script))
            module.scripts.push_back(ParseScript());
        else
        {
            AddError("E2003", "Expected 'use' or 'script' at module scope", Current());
            SynchronizeTopLevel();
        }
    }

    module.diagnostics = diagnostics_;
    return module;
}

const RbScriptToken& RbScriptParser::Current() const
{
    static const RbScriptToken endToken{RbScriptTokenKind::EndOfFile, {}, {}};
    return current_ < tokens_.size() ? tokens_[current_] : endToken;
}

const RbScriptToken& RbScriptParser::Previous() const
{
    static const RbScriptToken invalidToken{RbScriptTokenKind::Invalid, {}, {}};
    return current_ > 0 && current_ - 1 < tokens_.size() ? tokens_[current_ - 1] : invalidToken;
}

bool RbScriptParser::IsAtEnd() const
{
    return Check(RbScriptTokenKind::EndOfFile);
}

bool RbScriptParser::Check(RbScriptTokenKind kind) const
{
    return Current().kind == kind;
}

bool RbScriptParser::Match(RbScriptTokenKind kind)
{
    if (!Check(kind))
        return false;
    if (current_ < tokens_.size())
        ++current_;
    return true;
}

const RbScriptToken& RbScriptParser::Consume(RbScriptTokenKind kind, const ea::string& code, const ea::string& message)
{
    if (Check(kind))
    {
        const RbScriptToken& token = Current();
        ++current_;
        return token;
    }

    AddError(code, message, Current());
    return Current();
}

void RbScriptParser::SynchronizeTopLevel()
{
    while (!IsAtEnd() && !Check(RbScriptTokenKind::Script) && !Check(RbScriptTokenKind::Use))
        ++current_;
}

void RbScriptParser::SynchronizeStatement()
{
    while (!IsAtEnd())
    {
        if (Match(RbScriptTokenKind::Semicolon))
            return;
        if (Check(RbScriptTokenKind::RightBrace) || Check(RbScriptTokenKind::If)
            || Check(RbScriptTokenKind::While) || Check(RbScriptTokenKind::Return))
            return;
        ++current_;
    }
}

void RbScriptParser::AddError(const ea::string& code, const ea::string& message, const RbScriptToken& token)
{
    RbScriptDiagnostic diagnostic;
    diagnostic.severity = RbScriptDiagnosticSeverity::Error;
    diagnostic.file = fileName_;
    diagnostic.code = code;
    diagnostic.message = message;
    diagnostic.span = token.span;
    diagnostics_.push_back(diagnostic);
}

ea::vector<ea::string> RbScriptParser::ParseQualifiedName()
{
    ea::vector<ea::string> parts;
    const RbScriptToken& first = Consume(RbScriptTokenKind::Identifier, "E2010", "Expected an identifier");
    if (first.kind == RbScriptTokenKind::Identifier)
        parts.push_back(first.lexeme);

    while (Match(RbScriptTokenKind::Scope) || Match(RbScriptTokenKind::Dot))
    {
        const RbScriptToken& part = Consume(RbScriptTokenKind::Identifier, "E2011", "Expected an identifier after namespace separator");
        if (part.kind == RbScriptTokenKind::Identifier)
            parts.push_back(part.lexeme);
    }

    if (Match(RbScriptTokenKind::Less))
    {
        ea::string generic = JoinName(parts) + "<";
        bool firstArgument = true;
        while (!Check(RbScriptTokenKind::Greater) && !IsAtEnd())
        {
            if (!firstArgument)
                Consume(RbScriptTokenKind::Comma, "E2012", "Expected ',' between generic type arguments");
            const ea::vector<ea::string> argument = ParseQualifiedName();
            if (!firstArgument)
                generic += ",";
            generic += JoinName(argument);
            firstArgument = false;
        }
        Consume(RbScriptTokenKind::Greater, "E2013", "Expected '>' after generic type arguments");
        generic += ">";
        parts.clear();
        parts.push_back(generic);
    }
    return parts;
}

ea::vector<ea::string> RbScriptParser::ParseAttributes()
{
    ea::vector<ea::string> attributes;
    while (Match(RbScriptTokenKind::LeftBracket))
    {
        Consume(RbScriptTokenKind::LeftBracket, "E2020", "Expected second '[' in attribute");
        const RbScriptToken& name = Consume(RbScriptTokenKind::Identifier, "E2021", "Expected attribute name");
        if (name.kind == RbScriptTokenKind::Identifier)
            attributes.push_back(name.lexeme);
        while (!Check(RbScriptTokenKind::RightBracket) && !IsAtEnd())
            ++current_;
        Consume(RbScriptTokenKind::RightBracket, "E2022", "Expected ']' after attribute");
        Consume(RbScriptTokenKind::RightBracket, "E2023", "Expected second ']' after attribute");
    }
    return attributes;
}

RbScriptScript RbScriptParser::ParseScript()
{
    RbScriptScript script;
    const RbScriptToken& begin = Consume(RbScriptTokenKind::Script, "E2030", "Expected 'script'");
    const RbScriptToken& name = Consume(RbScriptTokenKind::Identifier, "E2031", "Expected script name");
    script.name = name.lexeme;
    script.span.begin = begin.span.begin;

    Consume(RbScriptTokenKind::Colon, "E2032", "Expected ':' before script base type");
    script.baseType = JoinName(ParseQualifiedName());
    Consume(RbScriptTokenKind::LeftBrace, "E2033", "Expected '{' to begin script body");

    while (!Check(RbScriptTokenKind::RightBrace) && !IsAtEnd())
    {
        const ea::vector<ea::string> attributes = ParseAttributes();
        if (Match(RbScriptTokenKind::Async))
        {
            if (Match(RbScriptTokenKind::Fn))
                script.functions.push_back(ParseFunction(attributes, false, true));
            else
                AddError("E2034", "Expected 'fn' after 'async'", Current());
        }
        else if (Match(RbScriptTokenKind::Fn))
            script.functions.push_back(ParseFunction(attributes, false, false));
        else if (Match(RbScriptTokenKind::On))
            script.functions.push_back(ParseFunction(attributes, true, false));
        else if (!IsAtEnd())
            script.fields.push_back(ParseField(attributes));
        else
            break;
    }

    const RbScriptToken& end = Consume(RbScriptTokenKind::RightBrace, "E2035", "Expected '}' after script body");
    script.span.end = end.span.end;
    return script;
}

RbScriptField RbScriptParser::ParseField(const ea::vector<ea::string>& attributes)
{
    RbScriptField field;
    field.attributes = attributes;
    const RbScriptToken& begin = Current();
    field.span.begin = begin.span.begin;

    if (Match(RbScriptTokenKind::Var) || Match(RbScriptTokenKind::Let) || Match(RbScriptTokenKind::Const))
    {
        const RbScriptToken& name = Consume(RbScriptTokenKind::Identifier, "E2040", "Expected field name");
        field.name = name.lexeme;
        Consume(RbScriptTokenKind::Colon, "E2041", "Expected ':' after field name");
        field.typeName = JoinName(ParseQualifiedName());
    }
    else
    {
        const RbScriptToken& first = Consume(RbScriptTokenKind::Identifier, "E2042", "Expected field type or name");
        if (Check(RbScriptTokenKind::Identifier))
        {
            field.typeName = first.lexeme;
            field.name = Consume(RbScriptTokenKind::Identifier, "E2043", "Expected field name").lexeme;
        }
        else if (Match(RbScriptTokenKind::Colon))
        {
            field.name = first.lexeme;
            field.typeName = JoinName(ParseQualifiedName());
        }
        else
        {
            field.name = first.lexeme;
            field.typeName = "Variant";
            AddError("E2044", "Expected field type and name", Current());
        }
    }

    if (Match(RbScriptTokenKind::Equal))
        field.initializer = ParseExpression();
    const RbScriptToken& end = Consume(RbScriptTokenKind::Semicolon, "E2045", "Expected ';' after field declaration");
    field.span.end = end.span.end;
    return field;
}

RbScriptFunction RbScriptParser::ParseFunction(const ea::vector<ea::string>& attributes, bool eventHandler, bool asynchronous)
{
    RbScriptFunction function;
    function.attributes = attributes;
    function.eventHandler = eventHandler;
    function.asynchronous = asynchronous;
    const RbScriptToken& begin = Current();
    function.span.begin = begin.span.begin;

    if (eventHandler)
        function.name = JoinName(ParseQualifiedName());
    else
        function.name = Consume(RbScriptTokenKind::Identifier, "E2050", "Expected function name").lexeme;

    Consume(RbScriptTokenKind::LeftParen, "E2051", "Expected '(' after function name");
    while (!Check(RbScriptTokenKind::RightParen) && !IsAtEnd())
    {
        function.parameters.push_back(ParseParameter());
        if (!Match(RbScriptTokenKind::Comma))
            break;
    }
    Consume(RbScriptTokenKind::RightParen, "E2052", "Expected ')' after function parameters");

    if (Match(RbScriptTokenKind::Arrow))
        function.returnType = JoinName(ParseQualifiedName());
    else if (eventHandler)
        function.returnType = "void";

    Consume(RbScriptTokenKind::LeftBrace, "E2053", "Expected '{' to begin function body");
    function.body = ParseBlock();
    const RbScriptToken& end = Consume(RbScriptTokenKind::RightBrace, "E2054", "Expected '}' after function body");
    function.span.end = end.span.end;
    return function;
}

RbScriptParameter RbScriptParser::ParseParameter()
{
    RbScriptParameter parameter;
    const RbScriptToken& first = Consume(RbScriptTokenKind::Identifier, "E2060", "Expected parameter type or name");
    parameter.span.begin = first.span.begin;

    if (Check(RbScriptTokenKind::Identifier))
    {
        parameter.typeName = first.lexeme;
        parameter.name = Consume(RbScriptTokenKind::Identifier, "E2061", "Expected parameter name").lexeme;
    }
    else if (Match(RbScriptTokenKind::Colon))
    {
        parameter.name = first.lexeme;
        parameter.typeName = JoinName(ParseQualifiedName());
    }
    else
    {
        parameter.name = first.lexeme;
        parameter.typeName = "Variant";
        AddError("E2062", "Expected parameter type and name", Current());
    }

    if (Match(RbScriptTokenKind::Equal))
        parameter.defaultValue = ParseExpression();
    parameter.span.end = Previous().span.end;
    return parameter;
}

std::unique_ptr<RbScriptStatement> RbScriptParser::ParseStatement()
{
    if (Match(RbScriptTokenKind::Semicolon))
        return MakeStatement(RbScriptStatementKind::Empty, Previous().span);

    if (Match(RbScriptTokenKind::LeftBrace))
    {
        auto statement = MakeStatement(RbScriptStatementKind::Block, Previous().span);
        statement->body = ParseBlock();
        const RbScriptToken& end = Consume(RbScriptTokenKind::RightBrace, "E2070", "Expected '}' after block");
        statement->span.end = end.span.end;
        return statement;
    }

    if (Match(RbScriptTokenKind::Return))
    {
        const RbScriptSourceSpan span = Previous().span;
        auto statement = MakeStatement(RbScriptStatementKind::Return, span);
        if (!Check(RbScriptTokenKind::Semicolon))
            statement->expression = ParseExpression();
        const RbScriptToken& end = Consume(RbScriptTokenKind::Semicolon, "E2071", "Expected ';' after return");
        statement->span.end = end.span.end;
        return statement;
    }

    if (Match(RbScriptTokenKind::If))
    {
        const RbScriptSourceSpan span = Previous().span;
        auto statement = MakeStatement(RbScriptStatementKind::If, span);
        Consume(RbScriptTokenKind::LeftParen, "E2072", "Expected '(' after if");
        statement->expression = ParseExpression();
        Consume(RbScriptTokenKind::RightParen, "E2073", "Expected ')' after if condition");
        if (Match(RbScriptTokenKind::LeftBrace))
        {
            statement->body = ParseBlock();
            Consume(RbScriptTokenKind::RightBrace, "E2074", "Expected '}' after if body");
        }
        else
            statement->body.push_back(ParseStatement());
        if (Match(RbScriptTokenKind::Else))
        {
            if (Match(RbScriptTokenKind::LeftBrace))
            {
                statement->elseBody = ParseBlock();
                Consume(RbScriptTokenKind::RightBrace, "E2075", "Expected '}' after else body");
            }
            else
                statement->elseBody.push_back(ParseStatement());
        }
        statement->span.end = Previous().span.end;
        return statement;
    }

    if (Match(RbScriptTokenKind::While))
    {
        const RbScriptSourceSpan span = Previous().span;
        auto statement = MakeStatement(RbScriptStatementKind::While, span);
        Consume(RbScriptTokenKind::LeftParen, "E2076", "Expected '(' after while");
        statement->expression = ParseExpression();
        Consume(RbScriptTokenKind::RightParen, "E2077", "Expected ')' after while condition");
        if (Match(RbScriptTokenKind::LeftBrace))
        {
            statement->body = ParseBlock();
            Consume(RbScriptTokenKind::RightBrace, "E2078", "Expected '}' after while body");
        }
        else
            statement->body.push_back(ParseStatement());
        statement->span.end = Previous().span.end;
        return statement;
    }

    if (Match(RbScriptTokenKind::Break))
    {
        const RbScriptSourceSpan span = Previous().span;
        const RbScriptToken& end = Consume(RbScriptTokenKind::Semicolon, "E2079", "Expected ';' after break");
        auto statement = MakeStatement(RbScriptStatementKind::Break, span);
        statement->span.end = end.span.end;
        return statement;
    }

    if (Match(RbScriptTokenKind::Continue))
    {
        const RbScriptSourceSpan span = Previous().span;
        const RbScriptToken& end = Consume(RbScriptTokenKind::Semicolon, "E2080", "Expected ';' after continue");
        auto statement = MakeStatement(RbScriptStatementKind::Continue, span);
        statement->span.end = end.span.end;
        return statement;
    }

    if (Match(RbScriptTokenKind::Var) || Match(RbScriptTokenKind::Let) || Match(RbScriptTokenKind::Const))
    {
        const RbScriptSourceSpan span = Previous().span;
        auto statement = MakeStatement(RbScriptStatementKind::VariableDeclaration, span);
        const RbScriptToken& name = Consume(RbScriptTokenKind::Identifier, "E2081", "Expected local variable name");
        statement->name = name.lexeme;
        Consume(RbScriptTokenKind::Colon, "E2082", "Expected ':' after local variable name");
        statement->typeName = JoinName(ParseQualifiedName());
        if (Match(RbScriptTokenKind::Equal))
            statement->expression = ParseExpression();
        const RbScriptToken& end = Consume(RbScriptTokenKind::Semicolon, "E2083", "Expected ';' after local variable");
        statement->span.end = end.span.end;
        return statement;
    }

    if (Match(RbScriptTokenKind::Emit))
    {
        const RbScriptSourceSpan span = Previous().span;
        auto statement = MakeStatement(RbScriptStatementKind::Emit, span);
        statement->expression = ParseExpression();
        const RbScriptToken& end = Consume(RbScriptTokenKind::Semicolon, "E2084", "Expected ';' after emit");
        statement->span.end = end.span.end;
        return statement;
    }

    const RbScriptSourceSpan span = Current().span;
    auto statement = MakeStatement(RbScriptStatementKind::Expression, span);
    statement->expression = ParseExpression();
    const RbScriptToken& end = Consume(RbScriptTokenKind::Semicolon, "E2085", "Expected ';' after expression");
    statement->span.end = end.span.end;
    return statement;
}

std::vector<std::unique_ptr<RbScriptStatement>> RbScriptParser::ParseBlock()
{
    std::vector<std::unique_ptr<RbScriptStatement>> statements;
    while (!Check(RbScriptTokenKind::RightBrace) && !IsAtEnd())
    {
        const unsigned oldCurrent = current_;
        statements.push_back(ParseStatement());
        if (current_ == oldCurrent)
        {
            AddError("E2090", "Parser made no progress in statement", Current());
            SynchronizeStatement();
        }
    }
    return statements;
}

std::unique_ptr<RbScriptExpression> RbScriptParser::ParseExpression()
{
    return ParseAssignment();
}

std::unique_ptr<RbScriptExpression> RbScriptParser::ParseAssignment()
{
    auto expression = ParseLogicalOr();
    if (Match(RbScriptTokenKind::Equal))
    {
        const RbScriptToken& op = Previous();
        auto result = MakeExpression(RbScriptExpressionKind::Binary, SpanFrom(expression->span, op.span), "=");
        result->children.push_back(std::move(expression));
        result->children.push_back(ParseAssignment());
        result->span.end = result->children.back()->span.end;
        return result;
    }
    return expression;
}

std::unique_ptr<RbScriptExpression> RbScriptParser::ParseLogicalOr()
{
    auto expression = ParseLogicalAnd();
    while (Match(RbScriptTokenKind::OrOr))
    {
        const RbScriptToken& op = Previous();
        auto result = MakeExpression(RbScriptExpressionKind::Binary, expression->span, op.lexeme);
        result->children.push_back(std::move(expression));
        result->children.push_back(ParseLogicalAnd());
        result->span = SpanFrom(result->children.front()->span, result->children.back()->span);
        expression = std::move(result);
    }
    return expression;
}

std::unique_ptr<RbScriptExpression> RbScriptParser::ParseLogicalAnd()
{
    auto expression = ParseEquality();
    while (Match(RbScriptTokenKind::AndAnd))
    {
        const RbScriptToken& op = Previous();
        auto result = MakeExpression(RbScriptExpressionKind::Binary, expression->span, op.lexeme);
        result->children.push_back(std::move(expression));
        result->children.push_back(ParseEquality());
        result->span = SpanFrom(result->children.front()->span, result->children.back()->span);
        expression = std::move(result);
    }
    return expression;
}

std::unique_ptr<RbScriptExpression> RbScriptParser::ParseEquality()
{
    auto expression = ParseComparison();
    while (Check(RbScriptTokenKind::EqualEqual) || Check(RbScriptTokenKind::BangEqual))
    {
        const RbScriptToken& op = Current();
        ++current_;
        auto result = MakeExpression(RbScriptExpressionKind::Binary, expression->span, op.lexeme);
        result->children.push_back(std::move(expression));
        result->children.push_back(ParseComparison());
        result->span = SpanFrom(result->children.front()->span, result->children.back()->span);
        expression = std::move(result);
    }
    return expression;
}

std::unique_ptr<RbScriptExpression> RbScriptParser::ParseComparison()
{
    auto expression = ParseTerm();
    while (Check(RbScriptTokenKind::Less) || Check(RbScriptTokenKind::LessEqual)
        || Check(RbScriptTokenKind::Greater) || Check(RbScriptTokenKind::GreaterEqual))
    {
        const RbScriptToken& op = Current();
        ++current_;
        auto result = MakeExpression(RbScriptExpressionKind::Binary, expression->span, op.lexeme);
        result->children.push_back(std::move(expression));
        result->children.push_back(ParseTerm());
        result->span = SpanFrom(result->children.front()->span, result->children.back()->span);
        expression = std::move(result);
    }
    return expression;
}

std::unique_ptr<RbScriptExpression> RbScriptParser::ParseTerm()
{
    auto expression = ParseFactor();
    while (Check(RbScriptTokenKind::Plus) || Check(RbScriptTokenKind::Minus))
    {
        const RbScriptToken& op = Current();
        ++current_;
        auto result = MakeExpression(RbScriptExpressionKind::Binary, expression->span, op.lexeme);
        result->children.push_back(std::move(expression));
        result->children.push_back(ParseFactor());
        result->span = SpanFrom(result->children.front()->span, result->children.back()->span);
        expression = std::move(result);
    }
    return expression;
}

std::unique_ptr<RbScriptExpression> RbScriptParser::ParseFactor()
{
    auto expression = ParseUnary();
    while (Check(RbScriptTokenKind::Star) || Check(RbScriptTokenKind::Slash) || Check(RbScriptTokenKind::Percent))
    {
        const RbScriptToken& op = Current();
        ++current_;
        auto result = MakeExpression(RbScriptExpressionKind::Binary, expression->span, op.lexeme);
        result->children.push_back(std::move(expression));
        result->children.push_back(ParseUnary());
        result->span = SpanFrom(result->children.front()->span, result->children.back()->span);
        expression = std::move(result);
    }
    return expression;
}

std::unique_ptr<RbScriptExpression> RbScriptParser::ParseUnary()
{
    if (Check(RbScriptTokenKind::Bang) || Check(RbScriptTokenKind::Minus) || Check(RbScriptTokenKind::Plus))
    {
        const RbScriptToken& op = Current();
        ++current_;
        auto result = MakeExpression(RbScriptExpressionKind::Unary, op.span, op.lexeme);
        result->children.push_back(ParseUnary());
        result->span = SpanFrom(op.span, result->children.back()->span);
        return result;
    }
    return ParsePostfix();
}

std::unique_ptr<RbScriptExpression> RbScriptParser::ParsePostfix()
{
    auto expression = ParsePrimary();
    for (;;)
    {
        if (Match(RbScriptTokenKind::Dot) || Match(RbScriptTokenKind::Scope))
        {
            const RbScriptToken& member = Consume(RbScriptTokenKind::Identifier, "E2100", "Expected member name");
            auto result = MakeExpression(RbScriptExpressionKind::Member, SpanFrom(expression->span, member.span), member.lexeme);
            result->children.push_back(std::move(expression));
            expression = std::move(result);
        }
        else if (Match(RbScriptTokenKind::LeftParen))
        {
            const RbScriptSourceSpan begin = expression->span;
            auto result = MakeExpression(RbScriptExpressionKind::Call, begin);
            result->children.push_back(std::move(expression));
            while (!Check(RbScriptTokenKind::RightParen) && !IsAtEnd())
            {
                result->children.push_back(ParseExpression());
                if (!Match(RbScriptTokenKind::Comma))
                    break;
            }
            const RbScriptToken& end = Consume(RbScriptTokenKind::RightParen, "E2101", "Expected ')' after call arguments");
            result->span = SpanFrom(begin, end.span);
            expression = std::move(result);
        }
        else if (Match(RbScriptTokenKind::LeftBracket))
        {
            const RbScriptSourceSpan begin = expression->span;
            auto result = MakeExpression(RbScriptExpressionKind::Index, begin);
            result->children.push_back(std::move(expression));
            result->children.push_back(ParseExpression());
            const RbScriptToken& end = Consume(RbScriptTokenKind::RightBracket, "E2104", "Expected ']' after index expression");
            result->span = SpanFrom(begin, end.span);
            expression = std::move(result);
        }
        else
            break;
    }
    return expression;
}

std::unique_ptr<RbScriptExpression> RbScriptParser::ParsePrimary()
{
    const RbScriptToken& token = Current();
    if (Match(RbScriptTokenKind::LeftBracket))
    {
        auto result = MakeExpression(RbScriptExpressionKind::ArrayLiteral, token.span);
        while (!Check(RbScriptTokenKind::RightBracket) && !IsAtEnd())
        {
            result->children.push_back(ParseExpression());
            if (!Match(RbScriptTokenKind::Comma))
                break;
        }
        const RbScriptToken& end = Consume(RbScriptTokenKind::RightBracket, "E2105", "Expected ']' after array literal");
        result->span = SpanFrom(token.span, end.span);
        return result;
    }
    if (Match(RbScriptTokenKind::LeftBrace))
    {
        auto result = MakeExpression(RbScriptExpressionKind::MapLiteral, token.span);
        while (!Check(RbScriptTokenKind::RightBrace) && !IsAtEnd())
        {
            result->children.push_back(ParseExpression());
            Consume(RbScriptTokenKind::Colon, "E2106", "Expected ':' between map key and value");
            result->children.push_back(ParseExpression());
            if (!Match(RbScriptTokenKind::Comma))
                break;
        }
        const RbScriptToken& end = Consume(RbScriptTokenKind::RightBrace, "E2107", "Expected '}' after map literal");
        result->span = SpanFrom(token.span, end.span);
        return result;
    }
    if (Match(RbScriptTokenKind::IntegerLiteral))
        return MakeExpression(RbScriptExpressionKind::IntegerLiteral, token.span, token.lexeme);
    if (Match(RbScriptTokenKind::FloatLiteral))
        return MakeExpression(RbScriptExpressionKind::FloatLiteral, token.span, token.lexeme);
    if (Match(RbScriptTokenKind::StringLiteral))
        return MakeExpression(RbScriptExpressionKind::StringLiteral, token.span, token.lexeme);
    if (Match(RbScriptTokenKind::True) || Match(RbScriptTokenKind::False))
        return MakeExpression(RbScriptExpressionKind::BooleanLiteral, token.span, token.lexeme);
    if (Match(RbScriptTokenKind::Null))
        return MakeExpression(RbScriptExpressionKind::NullLiteral, token.span, token.lexeme);
    if (Match(RbScriptTokenKind::Identifier))
        return MakeExpression(RbScriptExpressionKind::Identifier, token.span, token.lexeme);
    if (Match(RbScriptTokenKind::LeftParen))
    {
        auto expression = ParseExpression();
        Consume(RbScriptTokenKind::RightParen, "E2102", "Expected ')' after expression");
        return expression;
    }

    AddError("E2103", "Expected an expression", token);
    if (!IsAtEnd())
        ++current_;
    return MakeExpression(RbScriptExpressionKind::Invalid, token.span);
}

} // namespace Urho3D
