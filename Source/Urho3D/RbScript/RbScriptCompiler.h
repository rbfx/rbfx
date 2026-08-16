// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "RbScriptType.h"

namespace Urho3D
{

enum class RbScriptOpcode
{
    Nop,
    LoadConstant,
    LoadLocal,
    StoreLocal,
    LoadField,
    StoreField,
    LoadMember,
    StoreMember,
    Pop,
    UnaryNot,
    Negate,
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    And,
    Or,
    Call,
    Return,
    Jump,
    JumpIfFalse,
    Emit,
    ArrayNew,
    ArrayGet,
    ArraySet,
    ArrayLength,
    ArrayPush,
    MapNew,
    MapGet,
    MapSet,
    MapContains,
    Halt,
};

enum class RbScriptConstantKind
{
    Null,
    Boolean,
    Integer,
    Float,
    String,
};

struct URHO3D_API RbScriptConstant
{
    RbScriptConstantKind kind{RbScriptConstantKind::Null};
    bool booleanValue{false};
    long long integerValue{0};
    double floatValue{0.0};
    ea::string stringValue;
};

struct URHO3D_API RbScriptInstruction
{
    RbScriptOpcode opcode{RbScriptOpcode::Nop};
    int operand0{0};
    int operand1{0};
    int operand2{0};
    RbScriptSourceSpan span;
};

struct URHO3D_API RbScriptCompiledFunction
{
    ea::string name;
    ea::string scriptName;
    ea::string returnType{"void"};
    unsigned entryPoint{0};
    unsigned parameterCount{0};
    ea::vector<ea::string> parameterNames;
    ea::vector<RbScriptType> parameterTypes;
    ea::vector<ea::string> localNames;
    unsigned localCount{0};
    bool asynchronous{false};
    bool blueprintCallable{false};
};

struct URHO3D_API RbScriptChunk
{
    ea::vector<RbScriptInstruction> instructions;
    ea::vector<RbScriptConstant> constants;
    ea::vector<RbScriptCompiledFunction> functions;
    unsigned entryFunction{0};
};

class URHO3D_API RbScriptCompiler
{
public:
    explicit RbScriptCompiler(const RbScriptTypeRegistry* registry = nullptr);

    RbScriptChunk Compile(const RbScriptModule& module);
    const ea::vector<RbScriptDiagnostic>& GetDiagnostics() const { return diagnostics_; }
    bool HadError() const;

private:
    struct FunctionContext;

    void AddDiagnostic(const ea::string& code, const ea::string& message, const RbScriptSourceSpan& span);
    void IndexFunctions(const RbScriptModule& module);
    void CompileScript(const RbScriptScript& script);
    void CompileFunction(const RbScriptScript& script, const RbScriptFunction& function, unsigned functionIndex);
    void CompileStatement(const RbScriptStatement& statement);
    void CompileStatements(const std::vector<std::unique_ptr<RbScriptStatement>>& statements);
    void CompileExpression(const RbScriptExpression& expression);
    void CompileAssignment(const RbScriptExpression& expression);
    void CompileCall(const RbScriptExpression& expression);
    void CompileBinary(const RbScriptExpression& expression);
    void CompileUnary(const RbScriptExpression& expression);

    int AddConstant(const RbScriptConstant& constant);
    int AddNullConstant();
    unsigned Emit(RbScriptOpcode opcode, int operand0, int operand1, int operand2, const RbScriptSourceSpan& span);
    unsigned EmitJump(RbScriptOpcode opcode, const RbScriptSourceSpan& span);
    void PatchJump(unsigned instruction, unsigned target);
    int FindFunction(const ea::string& name) const;
    ea::string GetCalleeName(const RbScriptExpression& expression) const;
    int FindLocal(const ea::string& name) const;
    int FindOrCreateLocal(const ea::string& name);
    int FindField(const ea::string& name) const;

    const RbScriptTypeRegistry* registry_{};
    RbScriptChunk chunk_;
    ea::vector<RbScriptDiagnostic> diagnostics_;
    ea::unordered_map<ea::string, unsigned> functionIndices_;
    ea::unordered_map<ea::string, unsigned> fieldIndices_;
    FunctionContext* currentFunction_{};
};

} // namespace Urho3D
