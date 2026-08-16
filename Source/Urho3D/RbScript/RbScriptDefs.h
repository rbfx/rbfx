// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Container/Str.h>
#include <EASTL/vector.h>

#include <cstddef>

namespace Urho3D
{

enum class RbScriptTokenKind
{
    EndOfFile,
    Invalid,
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,

    True,
    False,
    Null,
    Module,
    Use,
    Script,
    Fn,
    Return,
    If,
    Else,
    While,
    For,
    In,
    Let,
    Var,
    Const,
    Struct,
    Enum,
    Class,
    Signal,
    On,
    Async,
    Await,
    Emit,
    Match,
    Break,
    Continue,
    Public,
    Private,
    Static,

    LeftBrace,
    RightBrace,
    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    Comma,
    Dot,
    Colon,
    Semicolon,
    Scope,
    Arrow,
    At,
    Question,

    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Equal,
    EqualEqual,
    Bang,
    BangEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    AndAnd,
    OrOr,
    Ampersand,
    Pipe,
};

struct URHO3D_API RbScriptSourcePosition
{
    unsigned offset{0};
    unsigned line{1};
    unsigned column{1};
};

struct URHO3D_API RbScriptSourceSpan
{
    RbScriptSourcePosition begin;
    RbScriptSourcePosition end;
};

enum class RbScriptDiagnosticSeverity
{
    Info,
    Warning,
    Error,
};

struct URHO3D_API RbScriptDiagnostic
{
    RbScriptDiagnosticSeverity severity{RbScriptDiagnosticSeverity::Error};
    ea::string file;
    ea::string code;
    ea::string message;
    RbScriptSourceSpan span;
};

struct URHO3D_API RbScriptToken
{
    RbScriptTokenKind kind{RbScriptTokenKind::Invalid};
    ea::string lexeme;
    RbScriptSourceSpan span;
};

URHO3D_API ea::string ToString(RbScriptTokenKind kind);
URHO3D_API ea::string ToString(RbScriptDiagnosticSeverity severity);

} // namespace Urho3D
