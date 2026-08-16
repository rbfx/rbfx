// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RbScriptVM.h"

#include <cmath>
#include <string>

namespace Urho3D
{

RbScriptValue RbScriptValue::Null()
{
    return {};
}

RbScriptValue RbScriptValue::FromBoolean(bool value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::Boolean;
    result.booleanValue = value;
    return result;
}

RbScriptValue RbScriptValue::FromInteger(long long value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::Integer;
    result.integerValue = value;
    return result;
}

RbScriptValue RbScriptValue::FromFloat(double value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::Float;
    result.floatValue = value;
    return result;
}

RbScriptValue RbScriptValue::FromString(const ea::string& value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::String;
    result.stringValue = value;
    return result;
}

RbScriptValue RbScriptValue::FromPointer(void* value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::Pointer;
    result.pointerValue = value;
    return result;
}

bool RbScriptValue::IsTruthy() const
{
    switch (kind)
    {
    case RbScriptValueKind::Null: return false;
    case RbScriptValueKind::Boolean: return booleanValue;
    case RbScriptValueKind::Integer: return integerValue != 0;
    case RbScriptValueKind::Float: return floatValue != 0.0;
    case RbScriptValueKind::String: return !stringValue.empty();
    case RbScriptValueKind::Pointer: return pointerValue != nullptr;
    }
    return false;
}

bool RbScriptValue::IsNumeric() const
{
    return kind == RbScriptValueKind::Integer || kind == RbScriptValueKind::Float;
}

double RbScriptValue::AsFloat() const
{
    return kind == RbScriptValueKind::Integer ? static_cast<double>(integerValue) : floatValue;
}

long long RbScriptValue::AsInteger() const
{
    return kind == RbScriptValueKind::Float ? static_cast<long long>(floatValue) : integerValue;
}

ea::string RbScriptValue::ToString() const
{
    switch (kind)
    {
    case RbScriptValueKind::Null: return "null";
    case RbScriptValueKind::Boolean: return booleanValue ? "true" : "false";
    case RbScriptValueKind::Integer: return std::to_string(integerValue).c_str();
    case RbScriptValueKind::Float: return std::to_string(floatValue).c_str();
    case RbScriptValueKind::String: return stringValue;
    case RbScriptValueKind::Pointer: return pointerValue ? "pointer" : "null";
    }
    return {};
}

RbScriptVM::RbScriptVM()
    : result_(RbScriptValue::Null())
{
}

bool RbScriptVM::Execute(const RbScriptChunk& chunk)
{
    valueStack_.clear();
    callStack_.clear();
    diagnostics_.clear();
    emittedEvents_.clear();
    result_ = RbScriptValue::Null();
    executedSteps_ = 0;
    halted_ = false;

    if (chunk.functions.empty())
    {
        AddError("V3001", "Cannot execute an empty rbscript chunk", {});
        return false;
    }
    if (chunk.entryFunction >= chunk.functions.size())
    {
        AddError("V3002", "The rbscript entry function is out of range", {});
        return false;
    }

    const RbScriptCompiledFunction& entry = chunk.functions[chunk.entryFunction];
    CallFrame frame;
    frame.functionIndex = chunk.entryFunction;
    frame.instructionPointer = entry.entryPoint;
    frame.locals.resize(entry.localCount);
    callStack_.push_back(frame);

    while (!halted_ && !callStack_.empty())
    {
        if (executedSteps_++ >= stepLimit_)
        {
            AddError("V3003", "rbscript execution exceeded its step limit", {});
            return false;
        }

        CallFrame& current = callStack_.back();
        if (current.instructionPointer >= chunk.instructions.size())
        {
            AddError("V3004", "Instruction pointer escaped the rbscript chunk", {});
            return false;
        }
        const RbScriptInstruction instruction = chunk.instructions[current.instructionPointer++];
        if (!ExecuteInstruction(chunk, current, instruction))
            return false;
    }
    return !HadError();
}

bool RbScriptVM::HadError() const
{
    for (const RbScriptDiagnostic& diagnostic : diagnostics_)
    {
        if (diagnostic.severity == RbScriptDiagnosticSeverity::Error)
            return true;
    }
    return false;
}

void RbScriptVM::AddError(const ea::string& code, const ea::string& message, const RbScriptSourceSpan& span)
{
    RbScriptDiagnostic diagnostic;
    diagnostic.severity = RbScriptDiagnosticSeverity::Error;
    diagnostic.code = code;
    diagnostic.message = message;
    diagnostic.span = span;
    diagnostics_.push_back(diagnostic);
}

bool RbScriptVM::Push(const RbScriptValue& value, const RbScriptSourceSpan& span)
{
    if (valueStack_.size() >= stackLimit_)
    {
        AddError("V3005", "rbscript value stack overflow", span);
        return false;
    }
    valueStack_.push_back(value);
    return true;
}

bool RbScriptVM::Pop(RbScriptValue& value, const RbScriptSourceSpan& span)
{
    if (valueStack_.empty())
    {
        AddError("V3006", "rbscript value stack underflow", span);
        value = RbScriptValue::Null();
        return false;
    }
    value = valueStack_.back();
    valueStack_.pop_back();
    return true;
}

bool RbScriptVM::ExecuteInstruction(const RbScriptChunk& chunk, CallFrame& frame,
    const RbScriptInstruction& instruction)
{
    switch (instruction.opcode)
    {
    case RbScriptOpcode::Nop:
        return true;

    case RbScriptOpcode::LoadConstant:
        if (instruction.operand0 < 0 || static_cast<unsigned>(instruction.operand0) >= chunk.constants.size())
        {
            AddError("V3007", "Constant index is out of range", instruction.span);
            return false;
        }
        return Push(ConstantToValue(chunk.constants[instruction.operand0]), instruction.span);

    case RbScriptOpcode::LoadLocal:
        if (instruction.operand0 < 0 || static_cast<unsigned>(instruction.operand0) >= frame.locals.size())
        {
            AddError("V3008", "Local index is out of range", instruction.span);
            return false;
        }
        return Push(frame.locals[instruction.operand0], instruction.span);

    case RbScriptOpcode::StoreLocal:
        {
            RbScriptValue value;
            if (!Pop(value, instruction.span))
                return false;
            if (instruction.operand0 < 0 || static_cast<unsigned>(instruction.operand0) >= frame.locals.size())
            {
                AddError("V3008", "Local index is out of range", instruction.span);
                return false;
            }
            frame.locals[instruction.operand0] = value;
            return true;
        }

    case RbScriptOpcode::LoadField:
        return Push(RbScriptValue::Null(), instruction.span);

    case RbScriptOpcode::StoreField:
        {
            RbScriptValue ignored;
            return Pop(ignored, instruction.span);
        }

    case RbScriptOpcode::LoadMember:
        {
            RbScriptValue object;
            if (!Pop(object, instruction.span))
                return false;
            AddError("V3009", "Native member access requires an rbfx binding", instruction.span);
            return Push(RbScriptValue::Null(), instruction.span);
        }

    case RbScriptOpcode::StoreMember:
        {
            RbScriptValue value;
            RbScriptValue object;
            if (!Pop(value, instruction.span) || !Pop(object, instruction.span))
                return false;
            AddError("V3010", "Native member assignment requires an rbfx binding", instruction.span);
            return true;
        }

    case RbScriptOpcode::Pop:
        {
            RbScriptValue ignored;
            return Pop(ignored, instruction.span);
        }

    case RbScriptOpcode::UnaryNot:
        {
            RbScriptValue value;
            if (!Pop(value, instruction.span))
                return false;
            return Push(RbScriptValue::FromBoolean(!value.IsTruthy()), instruction.span);
        }

    case RbScriptOpcode::Negate:
        {
            RbScriptValue value;
            if (!Pop(value, instruction.span))
                return false;
            if (!value.IsNumeric())
            {
                AddError("V3011", "Unary '-' requires a numeric value", instruction.span);
                return false;
            }
            return Push(value.kind == RbScriptValueKind::Integer
                    ? RbScriptValue::FromInteger(-value.integerValue)
                    : RbScriptValue::FromFloat(-value.floatValue), instruction.span);
        }

    case RbScriptOpcode::Add:
    case RbScriptOpcode::Sub:
    case RbScriptOpcode::Mul:
    case RbScriptOpcode::Div:
    case RbScriptOpcode::Mod:
    case RbScriptOpcode::And:
    case RbScriptOpcode::Or:
        return ExecuteBinary(instruction.opcode, instruction);

    case RbScriptOpcode::Equal:
    case RbScriptOpcode::NotEqual:
    case RbScriptOpcode::Less:
    case RbScriptOpcode::LessEqual:
    case RbScriptOpcode::Greater:
    case RbScriptOpcode::GreaterEqual:
        return ExecuteComparison(instruction.opcode, instruction);

    case RbScriptOpcode::Call:
        return StartCall(chunk, frame, instruction);

    case RbScriptOpcode::Return:
        return ReturnFromCall(frame, instruction);

    case RbScriptOpcode::Jump:
        if (instruction.operand0 < 0 || static_cast<unsigned>(instruction.operand0) >= chunk.instructions.size())
        {
            AddError("V3012", "Jump target is out of range", instruction.span);
            return false;
        }
        frame.instructionPointer = static_cast<unsigned>(instruction.operand0);
        return true;

    case RbScriptOpcode::JumpIfFalse:
        {
            RbScriptValue condition;
            if (!Pop(condition, instruction.span))
                return false;
            if (!condition.IsTruthy())
            {
                if (instruction.operand0 < 0 || static_cast<unsigned>(instruction.operand0) >= chunk.instructions.size())
                {
                    AddError("V3012", "Conditional jump target is out of range", instruction.span);
                    return false;
                }
                frame.instructionPointer = static_cast<unsigned>(instruction.operand0);
            }
            return true;
        }

    case RbScriptOpcode::Emit:
        {
            RbScriptValue value;
            if (!Pop(value, instruction.span))
                return false;
            emittedEvents_.push_back(value);
            return true;
        }

    case RbScriptOpcode::Halt:
        {
            if (!valueStack_.empty())
                result_ = valueStack_.back();
            halted_ = true;
            return true;
        }
    }
    AddError("V3013", "Unknown rbscript opcode", instruction.span);
    return false;
}

bool RbScriptVM::ExecuteBinary(RbScriptOpcode opcode, const RbScriptInstruction& instruction)
{
    RbScriptValue right;
    RbScriptValue left;
    if (!Pop(right, instruction.span) || !Pop(left, instruction.span))
        return false;

    if (opcode == RbScriptOpcode::And || opcode == RbScriptOpcode::Or)
    {
        const bool value = opcode == RbScriptOpcode::And
            ? left.IsTruthy() && right.IsTruthy() : left.IsTruthy() || right.IsTruthy();
        return Push(RbScriptValue::FromBoolean(value), instruction.span);
    }

    if (opcode == RbScriptOpcode::Add && (left.kind == RbScriptValueKind::String || right.kind == RbScriptValueKind::String))
        return Push(RbScriptValue::FromString(left.ToString() + right.ToString()), instruction.span);

    if (!left.IsNumeric() || !right.IsNumeric())
    {
        AddError("V3014", "Arithmetic operation requires numeric values", instruction.span);
        return false;
    }

    if (opcode == RbScriptOpcode::Div && right.AsFloat() == 0.0)
    {
        AddError("V3015", "Division by zero in rbscript", instruction.span);
        return false;
    }
    if (opcode == RbScriptOpcode::Mod && right.AsInteger() == 0)
    {
        AddError("V3016", "Modulo by zero in rbscript", instruction.span);
        return false;
    }

    const bool integer = left.kind == RbScriptValueKind::Integer && right.kind == RbScriptValueKind::Integer
        && opcode != RbScriptOpcode::Div;
    if (integer)
    {
        const long long lhs = left.integerValue;
        const long long rhs = right.integerValue;
        switch (opcode)
        {
        case RbScriptOpcode::Add: return Push(RbScriptValue::FromInteger(lhs + rhs), instruction.span);
        case RbScriptOpcode::Sub: return Push(RbScriptValue::FromInteger(lhs - rhs), instruction.span);
        case RbScriptOpcode::Mul: return Push(RbScriptValue::FromInteger(lhs * rhs), instruction.span);
        case RbScriptOpcode::Mod: return Push(RbScriptValue::FromInteger(lhs % rhs), instruction.span);
        default: break;
        }
    }

    const double lhs = left.AsFloat();
    const double rhs = right.AsFloat();
    switch (opcode)
    {
    case RbScriptOpcode::Add: return Push(RbScriptValue::FromFloat(lhs + rhs), instruction.span);
    case RbScriptOpcode::Sub: return Push(RbScriptValue::FromFloat(lhs - rhs), instruction.span);
    case RbScriptOpcode::Mul: return Push(RbScriptValue::FromFloat(lhs * rhs), instruction.span);
    case RbScriptOpcode::Div: return Push(RbScriptValue::FromFloat(lhs / rhs), instruction.span);
    case RbScriptOpcode::Mod: return Push(RbScriptValue::FromFloat(std::fmod(lhs, rhs)), instruction.span);
    default: break;
    }
    AddError("V3017", "Unsupported arithmetic opcode", instruction.span);
    return false;
}

bool RbScriptVM::ExecuteComparison(RbScriptOpcode opcode, const RbScriptInstruction& instruction)
{
    RbScriptValue right;
    RbScriptValue left;
    if (!Pop(right, instruction.span) || !Pop(left, instruction.span))
        return false;

    bool result = false;
    if (opcode == RbScriptOpcode::Equal || opcode == RbScriptOpcode::NotEqual)
    {
        if (left.IsNumeric() && right.IsNumeric())
            result = left.AsFloat() == right.AsFloat();
        else if (left.kind == RbScriptValueKind::String && right.kind == RbScriptValueKind::String)
            result = left.stringValue == right.stringValue;
        else if (left.kind == RbScriptValueKind::Boolean && right.kind == RbScriptValueKind::Boolean)
            result = left.booleanValue == right.booleanValue;
        else if (left.kind == RbScriptValueKind::Null && right.kind == RbScriptValueKind::Null)
            result = true;
        else if (left.kind == RbScriptValueKind::Pointer && right.kind == RbScriptValueKind::Pointer)
            result = left.pointerValue == right.pointerValue;
        if (opcode == RbScriptOpcode::NotEqual)
            result = !result;
        return Push(RbScriptValue::FromBoolean(result), instruction.span);
    }

    if (left.kind == RbScriptValueKind::String && right.kind == RbScriptValueKind::String)
    {
        if (opcode == RbScriptOpcode::Less) result = left.stringValue < right.stringValue;
        if (opcode == RbScriptOpcode::LessEqual) result = left.stringValue <= right.stringValue;
        if (opcode == RbScriptOpcode::Greater) result = left.stringValue > right.stringValue;
        if (opcode == RbScriptOpcode::GreaterEqual) result = left.stringValue >= right.stringValue;
        return Push(RbScriptValue::FromBoolean(result), instruction.span);
    }
    if (!left.IsNumeric() || !right.IsNumeric())
    {
        AddError("V3018", "Ordered comparison requires numeric or string values", instruction.span);
        return false;
    }

    const double lhs = left.AsFloat();
    const double rhs = right.AsFloat();
    if (opcode == RbScriptOpcode::Less) result = lhs < rhs;
    if (opcode == RbScriptOpcode::LessEqual) result = lhs <= rhs;
    if (opcode == RbScriptOpcode::Greater) result = lhs > rhs;
    if (opcode == RbScriptOpcode::GreaterEqual) result = lhs >= rhs;
    return Push(RbScriptValue::FromBoolean(result), instruction.span);
}

bool RbScriptVM::StartCall(const RbScriptChunk& chunk, CallFrame& caller, const RbScriptInstruction& instruction)
{
    const int argumentCount = instruction.operand1;
    if (argumentCount < 0 || static_cast<unsigned>(argumentCount) > valueStack_.size())
    {
        AddError("V3019", "Invalid argument count for rbscript call", instruction.span);
        return false;
    }
    if (instruction.operand0 < 0 || static_cast<unsigned>(instruction.operand0) >= chunk.functions.size())
    {
        AddError("V3020", "Cannot call an unresolved rbscript function", instruction.span);
        valueStack_.resize(valueStack_.size() - static_cast<unsigned>(argumentCount));
        return Push(RbScriptValue::Null(), instruction.span);
    }
    if (callStack_.size() >= callDepthLimit_)
    {
        AddError("V3021", "rbscript call stack overflow", instruction.span);
        return false;
    }

    const unsigned returnStackSize = static_cast<unsigned>(valueStack_.size()) - static_cast<unsigned>(argumentCount);
    const unsigned functionIndex = static_cast<unsigned>(instruction.operand0);
    const RbScriptCompiledFunction& function = chunk.functions[functionIndex];
    if (argumentCount != static_cast<int>(function.parameterCount))
    {
        AddError("V3022", "rbscript call argument count does not match the function signature", instruction.span);
        return false;
    }

    CallFrame callee;
    callee.functionIndex = functionIndex;
    callee.instructionPointer = function.entryPoint;
    callee.returnStackSize = returnStackSize;
    callee.locals.resize(function.localCount);
    for (unsigned i = 0; i < static_cast<unsigned>(argumentCount); ++i)
        callee.locals[i] = valueStack_[returnStackSize + i];
    callStack_.push_back(callee);
    return true;
}

bool RbScriptVM::ReturnFromCall(CallFrame&, const RbScriptInstruction& instruction)
{
    RbScriptValue value;
    if (!Pop(value, instruction.span))
        return false;

    if (callStack_.size() == 1)
    {
        result_ = value;
        halted_ = true;
        return true;
    }

    const unsigned returnStackSize = callStack_.back().returnStackSize;
    callStack_.pop_back();
    valueStack_.resize(returnStackSize);
    return Push(value, instruction.span);
}

RbScriptValue RbScriptVM::ConstantToValue(const RbScriptConstant& constant) const
{
    switch (constant.kind)
    {
    case RbScriptConstantKind::Null: return RbScriptValue::Null();
    case RbScriptConstantKind::Boolean: return RbScriptValue::FromBoolean(constant.booleanValue);
    case RbScriptConstantKind::Integer: return RbScriptValue::FromInteger(constant.integerValue);
    case RbScriptConstantKind::Float: return RbScriptValue::FromFloat(constant.floatValue);
    case RbScriptConstantKind::String: return RbScriptValue::FromString(constant.stringValue);
    }
    return RbScriptValue::Null();
}

RbScriptValue RbScriptVM::NullResult() const
{
    return RbScriptValue::Null();
}

} // namespace Urho3D
