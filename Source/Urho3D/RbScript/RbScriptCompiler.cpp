// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RbScriptCompiler.h"

#include <cstdlib>

namespace Urho3D
{

struct RbScriptCompiler::FunctionContext
{
    ea::unordered_map<ea::string, int> locals;
    ea::vector<unsigned> loopStarts;
    ea::vector<ea::vector<unsigned>> loopBreaks;
    unsigned functionIndex{0};
};

namespace
{

RbScriptConstant MakeNullConstant()
{
    return {};
}

RbScriptConstant MakeBooleanConstant(bool value)
{
    RbScriptConstant constant;
    constant.kind = RbScriptConstantKind::Boolean;
    constant.booleanValue = value;
    return constant;
}

RbScriptConstant MakeIntegerConstant(long long value)
{
    RbScriptConstant constant;
    constant.kind = RbScriptConstantKind::Integer;
    constant.integerValue = value;
    return constant;
}

RbScriptConstant MakeFloatConstant(double value)
{
    RbScriptConstant constant;
    constant.kind = RbScriptConstantKind::Float;
    constant.floatValue = value;
    return constant;
}

RbScriptConstant MakeStringConstant(const ea::string& value)
{
    RbScriptConstant constant;
    constant.kind = RbScriptConstantKind::String;
    constant.stringValue = value;
    return constant;
}

ea::string RemoveNumericSeparators(const ea::string& value)
{
    ea::string result;
    result.reserve(value.size());
    for (char ch : value)
    {
        if (ch != '_')
            result += ch;
    }
    return result;
}

} // namespace

RbScriptCompiler::RbScriptCompiler(const RbScriptTypeRegistry* registry)
    : registry_(registry)
{
}

RbScriptChunk RbScriptCompiler::Compile(const RbScriptModule& module)
{
    chunk_ = {};
    diagnostics_.clear();
    functionIndices_.clear();
    fieldIndices_.clear();
    currentFunction_ = nullptr;

    if (!module.IsValid())
    {
        AddDiagnostic("C3001", "Cannot compile an invalid rbscript module", {});
        return chunk_;
    }

    IndexFunctions(module);
    for (const RbScriptScript& script : module.scripts)
        CompileScript(script);

    if (!chunk_.functions.empty())
        chunk_.entryFunction = 0;
    return chunk_;
}

bool RbScriptCompiler::HadError() const
{
    for (const RbScriptDiagnostic& diagnostic : diagnostics_)
    {
        if (diagnostic.severity == RbScriptDiagnosticSeverity::Error)
            return true;
    }
    return false;
}

void RbScriptCompiler::AddDiagnostic(const ea::string& code, const ea::string& message,
    const RbScriptSourceSpan& span)
{
    RbScriptDiagnostic diagnostic;
    diagnostic.severity = RbScriptDiagnosticSeverity::Error;
    diagnostic.code = code;
    diagnostic.message = message;
    diagnostic.span = span;
    diagnostics_.push_back(diagnostic);
}

void RbScriptCompiler::IndexFunctions(const RbScriptModule& module)
{
    for (const RbScriptScript& script : module.scripts)
    {
        for (const RbScriptFunction& function : script.functions)
        {
            const ea::string qualifiedName = script.name + "::" + function.name;
            const unsigned index = static_cast<unsigned>(chunk_.functions.size());
            functionIndices_[qualifiedName] = index;
            if (functionIndices_.find(function.name) == functionIndices_.end())
                functionIndices_[function.name] = index;
            RbScriptCompiledFunction compiled;
            compiled.name = function.name;
            compiled.scriptName = script.name;
            compiled.returnType = function.returnType;
            compiled.parameterCount = static_cast<unsigned>(function.parameters.size());
            compiled.asynchronous = function.asynchronous;
            for (const ea::string& attribute : function.attributes)
            {
                if (attribute == "blueprint_callable")
                    compiled.blueprintCallable = true;
            }
            for (const RbScriptParameter& parameter : function.parameters)
            {
                compiled.parameterNames.push_back(parameter.name);
                compiled.parameterTypes.push_back(registry_ ? registry_->Resolve(parameter.typeName) : RbScriptType{});
            }
            chunk_.functions.push_back(compiled);
        }
    }
}

void RbScriptCompiler::CompileScript(const RbScriptScript& script)
{
    fieldIndices_.clear();
    for (unsigned i = 0; i < script.fields.size(); ++i)
        fieldIndices_[script.fields[i].name] = i;

    for (unsigned i = 0; i < script.functions.size(); ++i)
    {
        const RbScriptFunction& function = script.functions[i];
        const int index = FindFunction(script.name + "::" + function.name);
        if (index >= 0)
            CompileFunction(script, function, static_cast<unsigned>(index));
    }
}

void RbScriptCompiler::CompileFunction(const RbScriptScript& script, const RbScriptFunction& function,
    unsigned functionIndex)
{
    FunctionContext context;
    context.functionIndex = functionIndex;
    for (unsigned i = 0; i < function.parameters.size(); ++i)
        context.locals[function.parameters[i].name] = static_cast<int>(i);

    currentFunction_ = &context;
    RbScriptCompiledFunction& compiled = chunk_.functions[functionIndex];
    compiled.entryPoint = static_cast<unsigned>(chunk_.instructions.size());
    compiled.parameterCount = static_cast<unsigned>(function.parameters.size());
    compiled.parameterNames.clear();
    compiled.parameterTypes.clear();
    for (const RbScriptParameter& parameter : function.parameters)
    {
        compiled.parameterNames.push_back(parameter.name);
        compiled.parameterTypes.push_back(registry_ ? registry_->Resolve(parameter.typeName) : RbScriptType{});
    }
    const unsigned functionStart = compiled.entryPoint;
    CompileStatements(function.body);

    if (chunk_.instructions.size() == functionStart || chunk_.instructions.back().opcode != RbScriptOpcode::Return)
    {
        const int nullIndex = AddNullConstant();
        Emit(RbScriptOpcode::LoadConstant, nullIndex, 0, 0, function.span);
        Emit(RbScriptOpcode::Return, 0, 0, 0, function.span);
    }
    compiled.localCount = static_cast<unsigned>(context.locals.size());
    compiled.localNames.resize(compiled.localCount);
    for (const auto& local : context.locals)
    {
        if (local.second >= 0 && static_cast<unsigned>(local.second) < compiled.localNames.size())
            compiled.localNames[local.second] = local.first;
    }
    currentFunction_ = nullptr;
}

void RbScriptCompiler::CompileStatements(const std::vector<std::unique_ptr<RbScriptStatement>>& statements)
{
    for (const std::unique_ptr<RbScriptStatement>& statement : statements)
    {
        if (statement)
            CompileStatement(*statement);
    }
}

void RbScriptCompiler::CompileStatement(const RbScriptStatement& statement)
{
    switch (statement.kind)
    {
    case RbScriptStatementKind::Empty:
    case RbScriptStatementKind::Block:
        CompileStatements(statement.body);
        break;

    case RbScriptStatementKind::VariableDeclaration:
        {
            const int local = FindOrCreateLocal(statement.name);
            if (statement.expression)
                CompileExpression(*statement.expression);
            else
                Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, statement.span);
            Emit(RbScriptOpcode::StoreLocal, local, 0, 0, statement.span);
        }
        break;

    case RbScriptStatementKind::Expression:
        if (statement.expression)
        {
            CompileExpression(*statement.expression);
            Emit(RbScriptOpcode::Pop, 0, 0, 0, statement.span);
        }
        break;

    case RbScriptStatementKind::Return:
        if (statement.expression)
            CompileExpression(*statement.expression);
        else
            Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, statement.span);
        Emit(RbScriptOpcode::Return, 0, 0, 0, statement.span);
        break;

    case RbScriptStatementKind::If:
        if (statement.expression)
            CompileExpression(*statement.expression);
        else
            Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, statement.span);
        {
            const unsigned falseJump = EmitJump(RbScriptOpcode::JumpIfFalse, statement.span);
            CompileStatements(statement.body);
            if (!statement.elseBody.empty())
            {
                const unsigned endJump = EmitJump(RbScriptOpcode::Jump, statement.span);
                PatchJump(falseJump, static_cast<unsigned>(chunk_.instructions.size()));
                CompileStatements(statement.elseBody);
                PatchJump(endJump, static_cast<unsigned>(chunk_.instructions.size()));
            }
            else
                PatchJump(falseJump, static_cast<unsigned>(chunk_.instructions.size()));
        }
        break;

    case RbScriptStatementKind::While:
        {
            const unsigned loopStart = static_cast<unsigned>(chunk_.instructions.size());
            currentFunction_->loopStarts.push_back(loopStart);
            currentFunction_->loopBreaks.emplace_back();
            if (statement.expression)
                CompileExpression(*statement.expression);
            else
                Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, statement.span);
            const unsigned exitJump = EmitJump(RbScriptOpcode::JumpIfFalse, statement.span);
            CompileStatements(statement.body);
            Emit(RbScriptOpcode::Jump, static_cast<int>(loopStart), 0, 0, statement.span);
            const unsigned loopEnd = static_cast<unsigned>(chunk_.instructions.size());
            PatchJump(exitJump, loopEnd);
            for (unsigned instruction : currentFunction_->loopBreaks.back())
                PatchJump(instruction, loopEnd);
            currentFunction_->loopStarts.pop_back();
            currentFunction_->loopBreaks.pop_back();
        }
        break;

    case RbScriptStatementKind::Break:
        if (currentFunction_->loopBreaks.empty())
            AddDiagnostic("C3002", "'break' is only valid inside a while loop", statement.span);
        else
        {
            const unsigned jump = EmitJump(RbScriptOpcode::Jump, statement.span);
            currentFunction_->loopBreaks.back().push_back(jump);
        }
        break;

    case RbScriptStatementKind::Continue:
        if (currentFunction_->loopStarts.empty())
            AddDiagnostic("C3003", "'continue' is only valid inside a while loop", statement.span);
        else
            Emit(RbScriptOpcode::Jump, static_cast<int>(currentFunction_->loopStarts.back()), 0, 0, statement.span);
        break;

    case RbScriptStatementKind::Emit:
        if (statement.expression && statement.expression->kind == RbScriptExpressionKind::Identifier
            && FindLocal(statement.expression->text) < 0 && FindField(statement.expression->text) < 0)
            Emit(RbScriptOpcode::LoadConstant, AddConstant(MakeStringConstant(statement.expression->text)), 0, 0,
                statement.expression->span);
        else if (statement.expression)
            CompileExpression(*statement.expression);
        else
            Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, statement.span);
        Emit(RbScriptOpcode::Emit, 0, 0, 0, statement.span);
        break;
    }
}

void RbScriptCompiler::CompileExpression(const RbScriptExpression& expression)
{
    switch (expression.kind)
    {
    case RbScriptExpressionKind::Identifier:
        {
            const int local = FindLocal(expression.text);
            if (local >= 0)
                Emit(RbScriptOpcode::LoadLocal, local, 0, 0, expression.span);
            else
            {
                const int field = FindField(expression.text);
                if (field >= 0)
                    Emit(RbScriptOpcode::LoadField, field, 0, 0, expression.span);
                else
                {
                    AddDiagnostic("C3004", "Unknown identifier '" + expression.text + "'", expression.span);
                    Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, expression.span);
                }
            }
        }
        break;

    case RbScriptExpressionKind::IntegerLiteral:
        Emit(RbScriptOpcode::LoadConstant,
            AddConstant(MakeIntegerConstant(std::strtoll(RemoveNumericSeparators(expression.text).c_str(), nullptr, 0))),
            0, 0, expression.span);
        break;

    case RbScriptExpressionKind::FloatLiteral:
        {
            ea::string value = RemoveNumericSeparators(expression.text);
            if (!value.empty() && (value.back() == 'f' || value.back() == 'F'))
                value.pop_back();
            Emit(RbScriptOpcode::LoadConstant,
                AddConstant(MakeFloatConstant(std::strtod(value.c_str(), nullptr))), 0, 0, expression.span);
        }
        break;

    case RbScriptExpressionKind::StringLiteral:
        Emit(RbScriptOpcode::LoadConstant, AddConstant(MakeStringConstant(expression.text)), 0, 0, expression.span);
        break;

    case RbScriptExpressionKind::BooleanLiteral:
        Emit(RbScriptOpcode::LoadConstant,
            AddConstant(MakeBooleanConstant(expression.text == "true")), 0, 0, expression.span);
        break;

    case RbScriptExpressionKind::NullLiteral:
        Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, expression.span);
        break;

    case RbScriptExpressionKind::Unary:
        CompileUnary(expression);
        break;
    case RbScriptExpressionKind::Binary:
        if (expression.text == "=")
            CompileAssignment(expression);
        else
            CompileBinary(expression);
        break;
    case RbScriptExpressionKind::Call:
        CompileCall(expression);
        break;
    case RbScriptExpressionKind::Member:
        if (!expression.children.empty())
            CompileExpression(*expression.children.front());
        Emit(RbScriptOpcode::LoadMember, AddConstant(MakeStringConstant(expression.text)), 0, 0, expression.span);
        break;

    case RbScriptExpressionKind::ArrayLiteral:
        for (const std::unique_ptr<RbScriptExpression>& child : expression.children)
            CompileExpression(*child);
        Emit(RbScriptOpcode::ArrayNew, static_cast<int>(expression.children.size()), 0, 0, expression.span);
        break;

    case RbScriptExpressionKind::MapLiteral:
        if (expression.children.size() % 2 != 0)
        {
            AddDiagnostic("C3012", "Map literal must contain key/value pairs", expression.span);
            Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, expression.span);
            break;
        }
        for (const std::unique_ptr<RbScriptExpression>& child : expression.children)
            CompileExpression(*child);
        Emit(RbScriptOpcode::MapNew, static_cast<int>(expression.children.size() / 2), 0, 0, expression.span);
        break;

    case RbScriptExpressionKind::Index:
        if (expression.children.size() == 2)
        {
            CompileExpression(*expression.children[0]);
            CompileExpression(*expression.children[1]);
            Emit(RbScriptOpcode::ArrayGet, 0, 0, 0, expression.span);
        }
        else
            Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, expression.span);
        break;

    case RbScriptExpressionKind::Invalid:
        Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, expression.span);
        break;
    }
}

void RbScriptCompiler::CompileAssignment(const RbScriptExpression& expression)
{
    if (expression.children.size() != 2)
    {
        AddDiagnostic("C3005", "Malformed assignment expression", expression.span);
        Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, expression.span);
        return;
    }

    const RbScriptExpression& target = *expression.children.front();
    const RbScriptExpression& value = *expression.children.back();
    if (target.kind == RbScriptExpressionKind::Identifier)
    {
        CompileExpression(value);
        const int local = FindLocal(target.text);
        if (local >= 0)
        {
            Emit(RbScriptOpcode::StoreLocal, local, 0, 0, expression.span);
            Emit(RbScriptOpcode::LoadLocal, local, 0, 0, expression.span);
            return;
        }
        const int field = FindField(target.text);
        if (field >= 0)
        {
            Emit(RbScriptOpcode::StoreField, field, 0, 0, expression.span);
            Emit(RbScriptOpcode::LoadField, field, 0, 0, expression.span);
            return;
        }
    }
    else if (target.kind == RbScriptExpressionKind::Index && target.children.size() == 2)
    {
        CompileExpression(*target.children[0]);
        CompileExpression(*target.children[1]);
        CompileExpression(value);
        Emit(RbScriptOpcode::ArraySet, 0, 0, 0, expression.span);
        return;
    }
    else if (target.kind == RbScriptExpressionKind::Member && target.children.size() == 1)
    {
        CompileExpression(*target.children.front());
        CompileExpression(value);
        Emit(RbScriptOpcode::StoreMember, AddConstant(MakeStringConstant(target.text)), 0, 0, expression.span);
        return;
    }

    AddDiagnostic("C3006", "Only local, script field and collection assignments are supported", target.span);
    Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, expression.span);
}

void RbScriptCompiler::CompileCall(const RbScriptExpression& expression)
{
    if (expression.children.empty())
    {
        AddDiagnostic("C3007", "Malformed function call", expression.span);
        Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, expression.span);
        return;
    }

    const RbScriptExpression& calleeExpression = *expression.children.front();
    if (calleeExpression.kind == RbScriptExpressionKind::Member && calleeExpression.children.size() == 1)
    {
        const ea::string& method = calleeExpression.text;
        const unsigned argumentCount = expression.children.size() - 1;
        if ((method == "push" && argumentCount == 1) || (method == "length" && argumentCount == 0)
            || (method == "contains" && argumentCount == 1) || (method == "get" && argumentCount == 1)
            || (method == "set" && argumentCount == 2))
        {
            CompileExpression(*calleeExpression.children.front());
            for (unsigned i = 1; i < expression.children.size(); ++i)
                CompileExpression(*expression.children[i]);
            RbScriptOpcode opcode = RbScriptOpcode::Nop;
            if (method == "push") opcode = RbScriptOpcode::ArrayPush;
            else if (method == "length") opcode = RbScriptOpcode::ArrayLength;
            else if (method == "contains") opcode = RbScriptOpcode::MapContains;
            else if (method == "get") opcode = RbScriptOpcode::MapGet;
            else if (method == "set") opcode = RbScriptOpcode::MapSet;
            Emit(opcode, 0, 0, 0, expression.span);
            return;
        }
    }

    const ea::string callee = GetCalleeName(calleeExpression);
    const int function = FindFunction(callee);
    if (function < 0 && (!registry_ || !registry_->FindFunction(callee)))
        AddDiagnostic("C3008", "Unknown function '" + callee + "'", expression.span);

    for (unsigned i = 1; i < expression.children.size(); ++i)
        CompileExpression(*expression.children[i]);
    const int calleeNameConstant = function < 0 ? AddConstant(MakeStringConstant(callee)) : -1;
    Emit(RbScriptOpcode::Call, function, static_cast<int>(expression.children.size() - 1), calleeNameConstant, expression.span);
}

void RbScriptCompiler::CompileBinary(const RbScriptExpression& expression)
{
    if (expression.children.size() != 2)
    {
        AddDiagnostic("C3009", "Malformed binary expression", expression.span);
        Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, expression.span);
        return;
    }
    CompileExpression(*expression.children[0]);
    CompileExpression(*expression.children[1]);

    static const ea::unordered_map<ea::string, RbScriptOpcode> opcodes = {
        {"+", RbScriptOpcode::Add}, {"-", RbScriptOpcode::Sub}, {"*", RbScriptOpcode::Mul},
        {"/", RbScriptOpcode::Div}, {"%", RbScriptOpcode::Mod}, {"==", RbScriptOpcode::Equal},
        {"!=", RbScriptOpcode::NotEqual}, {"<", RbScriptOpcode::Less}, {"<=", RbScriptOpcode::LessEqual},
        {">", RbScriptOpcode::Greater}, {">=", RbScriptOpcode::GreaterEqual},
        {"&&", RbScriptOpcode::And}, {"||", RbScriptOpcode::Or},
    };
    const auto it = opcodes.find(expression.text);
    if (it == opcodes.end())
    {
        AddDiagnostic("C3010", "Unsupported binary operator '" + expression.text + "'", expression.span);
        Emit(RbScriptOpcode::Pop, 0, 0, 0, expression.span);
        Emit(RbScriptOpcode::Pop, 0, 0, 0, expression.span);
        Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, expression.span);
    }
    else
        Emit(it->second, 0, 0, 0, expression.span);
}

void RbScriptCompiler::CompileUnary(const RbScriptExpression& expression)
{
    if (expression.children.size() != 1)
    {
        AddDiagnostic("C3011", "Malformed unary expression", expression.span);
        Emit(RbScriptOpcode::LoadConstant, AddNullConstant(), 0, 0, expression.span);
        return;
    }
    CompileExpression(*expression.children.front());
    if (expression.text == "!")
        Emit(RbScriptOpcode::UnaryNot, 0, 0, 0, expression.span);
    else if (expression.text == "-")
        Emit(RbScriptOpcode::Negate, 0, 0, 0, expression.span);
    else if (expression.text != "+")
        AddDiagnostic("C3012", "Unsupported unary operator '" + expression.text + "'", expression.span);
}

int RbScriptCompiler::AddConstant(const RbScriptConstant& constant)
{
    for (unsigned i = 0; i < chunk_.constants.size(); ++i)
    {
        const RbScriptConstant& existing = chunk_.constants[i];
        if (existing.kind != constant.kind)
            continue;
        if (existing.booleanValue == constant.booleanValue && existing.integerValue == constant.integerValue
            && existing.floatValue == constant.floatValue && existing.stringValue == constant.stringValue)
            return static_cast<int>(i);
    }
    chunk_.constants.push_back(constant);
    return static_cast<int>(chunk_.constants.size() - 1);
}

int RbScriptCompiler::AddNullConstant()
{
    return AddConstant(MakeNullConstant());
}

unsigned RbScriptCompiler::Emit(RbScriptOpcode opcode, int operand0, int operand1, int operand2,
    const RbScriptSourceSpan& span)
{
    RbScriptInstruction instruction;
    instruction.opcode = opcode;
    instruction.operand0 = operand0;
    instruction.operand1 = operand1;
    instruction.operand2 = operand2;
    instruction.span = span;
    chunk_.instructions.push_back(instruction);
    return static_cast<unsigned>(chunk_.instructions.size() - 1);
}

unsigned RbScriptCompiler::EmitJump(RbScriptOpcode opcode, const RbScriptSourceSpan& span)
{
    return Emit(opcode, -1, 0, 0, span);
}

void RbScriptCompiler::PatchJump(unsigned instruction, unsigned target)
{
    if (instruction < chunk_.instructions.size())
        chunk_.instructions[instruction].operand0 = static_cast<int>(target);
}

int RbScriptCompiler::FindFunction(const ea::string& name) const
{
    const auto it = functionIndices_.find(name);
    return it == functionIndices_.end() ? -1 : static_cast<int>(it->second);
}

ea::string RbScriptCompiler::GetCalleeName(const RbScriptExpression& expression) const
{
    if (expression.kind == RbScriptExpressionKind::Identifier)
        return expression.text;
    if (expression.kind == RbScriptExpressionKind::Member && !expression.children.empty())
        return GetCalleeName(*expression.children.front()) + "::" + expression.text;
    return {};
}

int RbScriptCompiler::FindLocal(const ea::string& name) const
{
    if (!currentFunction_)
        return -1;
    const auto it = currentFunction_->locals.find(name);
    return it == currentFunction_->locals.end() ? -1 : it->second;
}

int RbScriptCompiler::FindOrCreateLocal(const ea::string& name)
{
    const int existing = FindLocal(name);
    if (existing >= 0)
        return existing;
    const int index = currentFunction_ ? static_cast<int>(currentFunction_->locals.size()) : -1;
    if (currentFunction_)
        currentFunction_->locals[name] = index;
    return index;
}

int RbScriptCompiler::FindField(const ea::string& name) const
{
    const auto it = fieldIndices_.find(name);
    return it == fieldIndices_.end() ? -1 : static_cast<int>(it->second);
}

} // namespace Urho3D
