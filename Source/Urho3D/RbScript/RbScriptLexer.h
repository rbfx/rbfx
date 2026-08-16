// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "RbScriptDefs.h"

namespace Urho3D
{

/// Converts rbscript source text into tokens while retaining precise source spans.
class URHO3D_API RbScriptLexer
{
public:
    explicit RbScriptLexer(const ea::string& source, const ea::string& fileName = {});

    /// Tokenize the complete source. The returned sequence always ends with EndOfFile.
    ea::vector<RbScriptToken> Tokenize();

    /// Return diagnostics emitted by the last Tokenize call.
    const ea::vector<RbScriptDiagnostic>& GetDiagnostics() const { return diagnostics_; }
    /// Return the original source text.
    const ea::string& GetSource() const { return source_; }
    /// Return the source file name used in diagnostics.
    const ea::string& GetFileName() const { return fileName_; }

private:
    char Peek(unsigned lookahead = 0) const;
    char Advance();
    bool Match(char expected);
    bool IsAtEnd() const;
    void SkipTrivia();
    void ScanToken(ea::vector<RbScriptToken>& tokens);
    void ScanIdentifierOrKeyword(ea::vector<RbScriptToken>& tokens, RbScriptSourcePosition begin);
    void ScanNumber(ea::vector<RbScriptToken>& tokens, RbScriptSourcePosition begin);
    void ScanString(ea::vector<RbScriptToken>& tokens, RbScriptSourcePosition begin);
    void AddToken(ea::vector<RbScriptToken>& tokens, RbScriptTokenKind kind,
        RbScriptSourcePosition begin, const ea::string& lexeme = {});
    void AddError(const ea::string& code, const ea::string& message,
        RbScriptSourcePosition begin, RbScriptSourcePosition end);
    void AdvanceLine();

    ea::string source_;
    ea::string fileName_;
    ea::vector<RbScriptDiagnostic> diagnostics_;
    unsigned offset_{0};
    unsigned line_{1};
    unsigned column_{1};
};

} // namespace Urho3D
