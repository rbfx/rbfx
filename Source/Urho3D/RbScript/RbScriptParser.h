// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "RbScriptAst.h"

namespace Urho3D
{

/// Recursive-descent parser for the rbscript source language.
class URHO3D_API RbScriptParser
{
public:
    explicit RbScriptParser(const ea::vector<RbScriptToken>& tokens, const ea::string& fileName = {});

    /// Parse a complete module. Diagnostics are retained on the returned module and parser.
    RbScriptModule ParseModule();
    const ea::vector<RbScriptDiagnostic>& GetDiagnostics() const { return diagnostics_; }

private:
    const RbScriptToken& Current() const;
    const RbScriptToken& Previous() const;
    bool IsAtEnd() const;
    bool Check(RbScriptTokenKind kind) const;
    bool Match(RbScriptTokenKind kind);
    const RbScriptToken& Consume(RbScriptTokenKind kind, const ea::string& code, const ea::string& message);
    void SynchronizeTopLevel();
    void SynchronizeStatement();
    void AddError(const ea::string& code, const ea::string& message, const RbScriptToken& token);

    ea::vector<ea::string> ParseQualifiedName();
    ea::vector<ea::string> ParseAttributes();
    RbScriptScript ParseScript();
    RbScriptField ParseField(const ea::vector<ea::string>& attributes);
    RbScriptFunction ParseFunction(const ea::vector<ea::string>& attributes, bool eventHandler, bool asynchronous);
    RbScriptParameter ParseParameter();
    std::unique_ptr<RbScriptStatement> ParseStatement();
    std::vector<std::unique_ptr<RbScriptStatement>> ParseBlock();

    std::unique_ptr<RbScriptExpression> ParseExpression();
    std::unique_ptr<RbScriptExpression> ParseAssignment();
    std::unique_ptr<RbScriptExpression> ParseLogicalOr();
    std::unique_ptr<RbScriptExpression> ParseLogicalAnd();
    std::unique_ptr<RbScriptExpression> ParseEquality();
    std::unique_ptr<RbScriptExpression> ParseComparison();
    std::unique_ptr<RbScriptExpression> ParseTerm();
    std::unique_ptr<RbScriptExpression> ParseFactor();
    std::unique_ptr<RbScriptExpression> ParseUnary();
    std::unique_ptr<RbScriptExpression> ParsePostfix();
    std::unique_ptr<RbScriptExpression> ParsePrimary();

    ea::vector<RbScriptToken> tokens_;
    ea::string fileName_;
    ea::vector<RbScriptDiagnostic> diagnostics_;
    unsigned current_{0};
};

} // namespace Urho3D
