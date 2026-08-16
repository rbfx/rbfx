// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RbScriptDefs.h"

namespace Urho3D
{

ea::string ToString(RbScriptTokenKind kind)
{
    switch (kind)
    {
    case RbScriptTokenKind::EndOfFile: return "end_of_file";
    case RbScriptTokenKind::Invalid: return "invalid";
    case RbScriptTokenKind::Identifier: return "identifier";
    case RbScriptTokenKind::IntegerLiteral: return "integer";
    case RbScriptTokenKind::FloatLiteral: return "float";
    case RbScriptTokenKind::StringLiteral: return "string";
    case RbScriptTokenKind::True: return "true";
    case RbScriptTokenKind::False: return "false";
    case RbScriptTokenKind::Null: return "null";
    case RbScriptTokenKind::Module: return "module";
    case RbScriptTokenKind::Use: return "use";
    case RbScriptTokenKind::Script: return "script";
    case RbScriptTokenKind::Fn: return "fn";
    case RbScriptTokenKind::Return: return "return";
    case RbScriptTokenKind::If: return "if";
    case RbScriptTokenKind::Else: return "else";
    case RbScriptTokenKind::While: return "while";
    case RbScriptTokenKind::For: return "for";
    case RbScriptTokenKind::In: return "in";
    case RbScriptTokenKind::Let: return "let";
    case RbScriptTokenKind::Var: return "var";
    case RbScriptTokenKind::Const: return "const";
    case RbScriptTokenKind::Struct: return "struct";
    case RbScriptTokenKind::Enum: return "enum";
    case RbScriptTokenKind::Class: return "class";
    case RbScriptTokenKind::Signal: return "signal";
    case RbScriptTokenKind::On: return "on";
    case RbScriptTokenKind::Async: return "async";
    case RbScriptTokenKind::Await: return "await";
    case RbScriptTokenKind::Emit: return "emit";
    case RbScriptTokenKind::Match: return "match";
    case RbScriptTokenKind::Break: return "break";
    case RbScriptTokenKind::Continue: return "continue";
    case RbScriptTokenKind::Public: return "public";
    case RbScriptTokenKind::Private: return "private";
    case RbScriptTokenKind::Static: return "static";
    case RbScriptTokenKind::LeftBrace: return "{";
    case RbScriptTokenKind::RightBrace: return "}";
    case RbScriptTokenKind::LeftParen: return "(";
    case RbScriptTokenKind::RightParen: return ")";
    case RbScriptTokenKind::LeftBracket: return "[";
    case RbScriptTokenKind::RightBracket: return "]";
    case RbScriptTokenKind::Comma: return ",";
    case RbScriptTokenKind::Dot: return ".";
    case RbScriptTokenKind::Colon: return ":";
    case RbScriptTokenKind::Semicolon: return ";";
    case RbScriptTokenKind::Scope: return "::";
    case RbScriptTokenKind::Arrow: return "->";
    case RbScriptTokenKind::At: return "@";
    case RbScriptTokenKind::Question: return "?";
    case RbScriptTokenKind::Plus: return "+";
    case RbScriptTokenKind::Minus: return "-";
    case RbScriptTokenKind::Star: return "*";
    case RbScriptTokenKind::Slash: return "/";
    case RbScriptTokenKind::Percent: return "%";
    case RbScriptTokenKind::Equal: return "=";
    case RbScriptTokenKind::EqualEqual: return "==";
    case RbScriptTokenKind::Bang: return "!";
    case RbScriptTokenKind::BangEqual: return "!=";
    case RbScriptTokenKind::Less: return "<";
    case RbScriptTokenKind::LessEqual: return "<=";
    case RbScriptTokenKind::Greater: return ">";
    case RbScriptTokenKind::GreaterEqual: return ">=";
    case RbScriptTokenKind::AndAnd: return "&&";
    case RbScriptTokenKind::OrOr: return "||";
    case RbScriptTokenKind::Ampersand: return "&";
    case RbScriptTokenKind::Pipe: return "|";
    }
    return "invalid";
}

ea::string ToString(RbScriptDiagnosticSeverity severity)
{
    switch (severity)
    {
    case RbScriptDiagnosticSeverity::Info: return "info";
    case RbScriptDiagnosticSeverity::Warning: return "warning";
    case RbScriptDiagnosticSeverity::Error: return "error";
    }
    return "error";
}

} // namespace Urho3D
