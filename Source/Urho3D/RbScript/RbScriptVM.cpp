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

RbScriptValue RbScriptValue::FromVector2(const Vector2& value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::Vector2;
    result.vector2Value = value;
    return result;
}

RbScriptValue RbScriptValue::FromVector3(const Vector3& value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::Vector3;
    result.vector3Value = value;
    return result;
}

RbScriptValue RbScriptValue::FromQuaternion(const Quaternion& value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::Quaternion;
    result.quaternionValue = value;
    return result;
}

RbScriptValue RbScriptValue::FromColor(const Color& value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::Color;
    result.colorValue = value;
    return result;
}

RbScriptValue RbScriptValue::FromPointer(void* value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::Pointer;
    result.pointerValue = value;
    return result;
}

RbScriptValue RbScriptValue::FromArray(const ea::vector<RbScriptValue>& value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::Array;
    result.arrayValue = ea::make_shared<RbScriptArray>();
    result.arrayValue->values = value;
    return result;
}

RbScriptValue RbScriptValue::FromMap(const ea::unordered_map<ea::string, RbScriptValue>& value)
{
    RbScriptValue result;
    result.kind = RbScriptValueKind::Map;
    result.mapValue = ea::make_shared<RbScriptMap>();
    result.mapValue->values = value;
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
    case RbScriptValueKind::Vector2:
    case RbScriptValueKind::Vector3:
    case RbScriptValueKind::Quaternion:
    case RbScriptValueKind::Color:
        return true;
    case RbScriptValueKind::Pointer: return pointerValue != nullptr;
    case RbScriptValueKind::Array: return arrayValue && !arrayValue->values.empty();
    case RbScriptValueKind::Map: return mapValue && !mapValue->values.empty();
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
    case RbScriptValueKind::Vector2: return "Vector2";
    case RbScriptValueKind::Vector3: return "Vector3";
    case RbScriptValueKind::Quaternion: return "Quaternion";
    case RbScriptValueKind::Color: return "Color";
    case RbScriptValueKind::Pointer: return pointerValue ? "pointer" : "null";
    case RbScriptValueKind::Array: return arrayValue ? ea::string("Array[") + std::to_string(arrayValue->values.size()).c_str() + "]" : "Array[]";
    case RbScriptValueKind::Map: return mapValue ? ea::string("Map{") + std::to_string(mapValue->values.size()).c_str() + "}" : "Map{}";
    }
    return {};
}

RbScriptVM::RbScriptVM()
    : result_(RbScriptValue::Null())
{
}

bool RbScriptVM::Execute(const RbScriptChunk& chunk)
{
    return ExecuteEntry(chunk, chunk.entryFunction, {});
}

bool RbScriptVM::ExecuteFunction(const RbScriptChunk& chunk, const ea::string& functionName,
    const ea::vector<RbScriptValue>& arguments)
{
    for (unsigned i = 0; i < chunk.functions.size(); ++i)
    {
        if (chunk.functions[i].name == functionName
            || chunk.functions[i].scriptName + "::" + chunk.functions[i].name == functionName)
            return ExecuteEntry(chunk, i, arguments);
    }

    diagnostics_.clear();
    AddError("V3024", "The requested rbscript function was not found: '" + functionName + "'", {});
    return false;
}

bool RbScriptVM::ExecuteEntry(const RbScriptChunk& chunk, unsigned functionIndex,
    const ea::vector<RbScriptValue>& arguments)
{
    debugActive_ = false;
    debugPaused_ = false;
    debugChunk_ = nullptr;
    debugCurrentLine_ = 0;
    debugLocals_.clear();
    debugCallStack_.clear();
    if (!PrepareExecution(chunk, functionIndex, arguments))
        return false;

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

bool RbScriptVM::PrepareExecution(const RbScriptChunk& chunk, unsigned functionIndex,
    const ea::vector<RbScriptValue>& arguments)
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
    if (functionIndex >= chunk.functions.size())
    {
        AddError("V3002", "The rbscript entry function is out of range", {});
        return false;
    }

    const RbScriptCompiledFunction& entry = chunk.functions[functionIndex];
    if (arguments.size() != entry.parameterCount)
    {
        AddError("V3025", "Entry function argument count does not match its signature", {});
        return false;
    }

    CallFrame frame;
    frame.functionIndex = functionIndex;
    frame.instructionPointer = entry.entryPoint;
    frame.locals.resize(entry.localCount);
    for (unsigned i = 0; i < arguments.size(); ++i)
        frame.locals[i] = arguments[i];
    callStack_.push_back(frame);
    return true;
}

bool RbScriptVM::BeginDebug(const RbScriptChunk& chunk, const ea::string& functionName,
    const ea::vector<RbScriptValue>& arguments)
{
    unsigned functionIndex = chunk.entryFunction;
    if (!functionName.empty())
    {
        bool found = false;
        for (unsigned i = 0; i < chunk.functions.size(); ++i)
        {
            if (chunk.functions[i].name == functionName
                || chunk.functions[i].scriptName + "::" + chunk.functions[i].name == functionName)
            {
                functionIndex = i;
                found = true;
                break;
            }
        }
        if (!found)
        {
            diagnostics_.clear();
            AddError("V3024", "The requested rbscript function was not found: '" + functionName + "'", {});
            return false;
        }
    }

    StopDebug();
    debugChunk_ = &chunk;
    debugActive_ = true;
    if (!PrepareExecution(chunk, functionIndex, arguments))
    {
        debugActive_ = false;
        debugChunk_ = nullptr;
        return false;
    }
    debugPaused_ = true;
    RefreshDebugSnapshot();
    return true;
}

bool RbScriptVM::StepDebug()
{
    if (!debugActive_ || !debugChunk_ || halted_ || callStack_.empty())
        return false;
    debugPaused_ = false;
    const bool success = ExecuteOneDebugInstruction();
    if (debugActive_ && success)
        debugPaused_ = true;
    return success;
}

bool RbScriptVM::ContinueDebug()
{
    if (!debugActive_ || !debugChunk_ || halted_ || callStack_.empty())
        return false;

    const bool skipCurrentBreakpoint = debugPaused_ && executedSteps_ > 0;
    debugPaused_ = false;
    bool skip = skipCurrentBreakpoint;
    while (debugActive_ && !halted_ && !callStack_.empty())
    {
        if (!skip && IsBreakpointAtCurrentInstruction())
        {
            debugPaused_ = true;
            RefreshDebugSnapshot();
            return true;
        }
        skip = false;
        if (!ExecuteOneDebugInstruction())
            return false;
    }
    return !HadError();
}

void RbScriptVM::StopDebug()
{
    debugActive_ = false;
    debugPaused_ = false;
    debugChunk_ = nullptr;
    callStack_.clear();
    valueStack_.clear();
    debugCurrentLine_ = 0;
    debugLocals_.clear();
    debugCallStack_.clear();
}

void RbScriptVM::SetBreakpoint(unsigned line)
{
    if (line == 0)
        return;
    for (unsigned existing : breakpoints_)
    {
        if (existing == line)
            return;
    }
    breakpoints_.push_back(line);
}

void RbScriptVM::RemoveBreakpoint(unsigned line)
{
    for (auto it = breakpoints_.begin(); it != breakpoints_.end(); ++it)
    {
        if (*it == line)
        {
            breakpoints_.erase(it);
            return;
        }
    }
}

bool RbScriptVM::ExecuteOneDebugInstruction()
{
    if (!debugChunk_ || callStack_.empty())
        return false;
    if (executedSteps_++ >= stepLimit_)
    {
        AddError("V3003", "rbscript execution exceeded its step limit", {});
        debugActive_ = false;
        return false;
    }

    CallFrame& current = callStack_.back();
    if (current.instructionPointer >= debugChunk_->instructions.size())
    {
        AddError("V3004", "Instruction pointer escaped the rbscript chunk", {});
        debugActive_ = false;
        return false;
    }
    const RbScriptInstruction instruction = debugChunk_->instructions[current.instructionPointer++];
    if (!ExecuteInstruction(*debugChunk_, current, instruction))
    {
        debugActive_ = false;
        RefreshDebugSnapshot();
        return false;
    }
    RefreshDebugSnapshot();
    if (halted_ || callStack_.empty())
    {
        debugActive_ = false;
        debugPaused_ = false;
    }
    return true;
}

bool RbScriptVM::IsBreakpointAtCurrentInstruction() const
{
    if (!debugChunk_ || callStack_.empty())
        return false;
    const CallFrame& frame = callStack_.back();
    if (frame.instructionPointer >= debugChunk_->instructions.size())
        return false;
    const unsigned line = debugChunk_->instructions[frame.instructionPointer].span.begin.line;
    if (line == 0)
        return false;
    for (unsigned breakpoint : breakpoints_)
    {
        if (breakpoint == line)
            return true;
    }
    return false;
}

void RbScriptVM::RefreshDebugSnapshot()
{
    debugCurrentLine_ = 0;
    debugLocals_.clear();
    debugCallStack_.clear();
    if (!debugChunk_)
        return;

    for (const CallFrame& frame : callStack_)
    {
        if (frame.functionIndex >= debugChunk_->functions.size())
            continue;
        const RbScriptCompiledFunction& function = debugChunk_->functions[frame.functionIndex];
        debugCallStack_.push_back(function.scriptName + "::" + function.name);
    }
    if (callStack_.empty())
        return;

    const CallFrame& current = callStack_.back();
    if (current.functionIndex >= debugChunk_->functions.size())
        return;
    const RbScriptCompiledFunction& function = debugChunk_->functions[current.functionIndex];
    if (current.instructionPointer < debugChunk_->instructions.size())
        debugCurrentLine_ = debugChunk_->instructions[current.instructionPointer].span.begin.line;
    for (unsigned i = 0; i < current.locals.size(); ++i)
    {
        ea::string name;
        if (i < function.localNames.size() && !function.localNames[i].empty())
            name = function.localNames[i];
        else
            name = "local" + ea::string(std::to_string(i).c_str());
        debugLocals_[name] = current.locals[i];
    }
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
        if (instruction.operand0 < 0)
        {
            AddError("V3017", "Field index is invalid", instruction.span);
            return false;
        }
        if (static_cast<unsigned>(instruction.operand0) >= fieldValues_.size())
            fieldValues_.resize(static_cast<unsigned>(instruction.operand0) + 1);
        return Push(fieldValues_[instruction.operand0], instruction.span);

    case RbScriptOpcode::StoreField:
        {
            RbScriptValue value;
            if (!Pop(value, instruction.span))
                return false;
            if (instruction.operand0 < 0)
            {
                AddError("V3017", "Field index is invalid", instruction.span);
                return false;
            }
            if (static_cast<unsigned>(instruction.operand0) >= fieldValues_.size())
                fieldValues_.resize(static_cast<unsigned>(instruction.operand0) + 1);
            fieldValues_[instruction.operand0] = value;
            return true;
        }

    case RbScriptOpcode::LoadMember:
        {
            RbScriptValue object;
            if (!Pop(object, instruction.span))
                return false;
            if (instruction.operand0 < 0 || static_cast<unsigned>(instruction.operand0) >= chunk.constants.size()
                || chunk.constants[instruction.operand0].kind != RbScriptConstantKind::String)
            {
                AddError("V3009", "Collection member name constant is invalid", instruction.span);
                return false;
            }
            const ea::string& member = chunk.constants[instruction.operand0].stringValue;
            if (object.kind == RbScriptValueKind::Array && object.arrayValue && member == "length")
                return Push(RbScriptValue::FromInteger(object.arrayValue->values.size()), instruction.span);
            if (object.kind == RbScriptValueKind::Map && object.mapValue)
            {
                const auto it = object.mapValue->values.find(member);
                return Push(it != object.mapValue->values.end() ? it->second : RbScriptValue::Null(), instruction.span);
            }
            AddError("V3009", "Member access requires an rbscript collection or an rbfx binding", instruction.span);
            return Push(RbScriptValue::Null(), instruction.span);
        }

    case RbScriptOpcode::StoreMember:
        {
            RbScriptValue value;
            RbScriptValue object;
            if (!Pop(value, instruction.span) || !Pop(object, instruction.span))
                return false;
            if (instruction.operand0 < 0 || static_cast<unsigned>(instruction.operand0) >= chunk.constants.size()
                || chunk.constants[instruction.operand0].kind != RbScriptConstantKind::String)
            {
                AddError("V3010", "Collection member name constant is invalid", instruction.span);
                return false;
            }
            const ea::string& member = chunk.constants[instruction.operand0].stringValue;
            if (object.kind == RbScriptValueKind::Map && object.mapValue)
            {
                object.mapValue->values[member] = value;
                return Push(value, instruction.span);
            }
            AddError("V3010", "Member assignment requires a map or an rbfx binding", instruction.span);
            return false;
        }

    case RbScriptOpcode::ArrayNew:
        {
            if (instruction.operand0 < 0 || static_cast<unsigned>(instruction.operand0) > valueStack_.size())
            {
                AddError("V3030", "ArrayNew element count is invalid", instruction.span);
                return false;
            }
            ea::vector<RbScriptValue> values(static_cast<unsigned>(instruction.operand0));
            for (int i = instruction.operand0 - 1; i >= 0; --i)
            {
                if (!Pop(values[static_cast<unsigned>(i)], instruction.span))
                    return false;
            }
            return Push(RbScriptValue::FromArray(values), instruction.span);
        }

    case RbScriptOpcode::ArrayGet:
        {
            RbScriptValue index;
            RbScriptValue array;
            if (!Pop(index, instruction.span) || !Pop(array, instruction.span))
                return false;
            if (array.kind == RbScriptValueKind::Map && array.mapValue && index.kind == RbScriptValueKind::String)
            {
                const auto it = array.mapValue->values.find(index.stringValue);
                return Push(it != array.mapValue->values.end() ? it->second : RbScriptValue::Null(), instruction.span);
            }
            if (array.kind != RbScriptValueKind::Array || !array.arrayValue || !index.IsNumeric())
            {
                AddError("V3031", "Index access requires an array/numeric index or map/string key", instruction.span);
                return false;
            }
            const long long position = index.AsInteger();
            if (position < 0 || static_cast<unsigned long long>(position) >= array.arrayValue->values.size())
                return Push(RbScriptValue::Null(), instruction.span);
            return Push(array.arrayValue->values[static_cast<unsigned>(position)], instruction.span);
        }

    case RbScriptOpcode::ArraySet:
        {
            RbScriptValue value;
            RbScriptValue index;
            RbScriptValue array;
            if (!Pop(value, instruction.span) || !Pop(index, instruction.span) || !Pop(array, instruction.span))
                return false;
            if (array.kind == RbScriptValueKind::Map && array.mapValue && index.kind == RbScriptValueKind::String)
            {
                array.mapValue->values[index.stringValue] = value;
                return Push(value, instruction.span);
            }
            if (array.kind != RbScriptValueKind::Array || !array.arrayValue || !index.IsNumeric())
            {
                AddError("V3032", "Index assignment requires an array/numeric index or map/string key", instruction.span);
                return false;
            }
            const long long position = index.AsInteger();
            if (position < 0 || static_cast<unsigned long long>(position) >= array.arrayValue->values.size())
            {
                AddError("V3033", "ArraySet index is out of range", instruction.span);
                return false;
            }
            array.arrayValue->values[static_cast<unsigned>(position)] = value;
            return Push(value, instruction.span);
        }

    case RbScriptOpcode::ArrayLength:
        {
            RbScriptValue array;
            if (!Pop(array, instruction.span))
                return false;
            if (array.kind != RbScriptValueKind::Array || !array.arrayValue)
            {
                AddError("V3034", "ArrayLength requires an array", instruction.span);
                return false;
            }
            return Push(RbScriptValue::FromInteger(array.arrayValue->values.size()), instruction.span);
        }

    case RbScriptOpcode::ArrayPush:
        {
            RbScriptValue value;
            RbScriptValue array;
            if (!Pop(value, instruction.span) || !Pop(array, instruction.span))
                return false;
            if (array.kind != RbScriptValueKind::Array || !array.arrayValue)
            {
                AddError("V3035", "ArrayPush requires an array", instruction.span);
                return false;
            }
            array.arrayValue->values.push_back(value);
            return Push(array, instruction.span);
        }

    case RbScriptOpcode::MapNew:
        {
            if (instruction.operand0 < 0 || static_cast<unsigned>(instruction.operand0 * 2) > valueStack_.size())
            {
                AddError("V3036", "MapNew entry count is invalid", instruction.span);
                return false;
            }
            ea::unordered_map<ea::string, RbScriptValue> values;
            for (int i = instruction.operand0 - 1; i >= 0; --i)
            {
                RbScriptValue value;
                RbScriptValue key;
                if (!Pop(value, instruction.span) || !Pop(key, instruction.span))
                    return false;
                if (key.kind != RbScriptValueKind::String)
                {
                    AddError("V3037", "Map keys must be strings", instruction.span);
                    return false;
                }
                values[key.stringValue] = value;
            }
            return Push(RbScriptValue::FromMap(values), instruction.span);
        }

    case RbScriptOpcode::MapGet:
        {
            RbScriptValue key;
            RbScriptValue map;
            if (!Pop(key, instruction.span) || !Pop(map, instruction.span))
                return false;
            if (map.kind != RbScriptValueKind::Map || !map.mapValue || key.kind != RbScriptValueKind::String)
            {
                AddError("V3038", "MapGet requires a map and string key", instruction.span);
                return false;
            }
            const auto it = map.mapValue->values.find(key.stringValue);
            return Push(it != map.mapValue->values.end() ? it->second : RbScriptValue::Null(), instruction.span);
        }

    case RbScriptOpcode::MapSet:
        {
            RbScriptValue value;
            RbScriptValue key;
            RbScriptValue map;
            if (!Pop(value, instruction.span) || !Pop(key, instruction.span) || !Pop(map, instruction.span))
                return false;
            if (map.kind != RbScriptValueKind::Map || !map.mapValue || key.kind != RbScriptValueKind::String)
            {
                AddError("V3039", "MapSet requires a map and string key", instruction.span);
                return false;
            }
            map.mapValue->values[key.stringValue] = value;
            return Push(value, instruction.span);
        }

    case RbScriptOpcode::MapContains:
        {
            RbScriptValue key;
            RbScriptValue map;
            if (!Pop(key, instruction.span) || !Pop(map, instruction.span))
                return false;
            if (map.kind != RbScriptValueKind::Map || !map.mapValue || key.kind != RbScriptValueKind::String)
            {
                AddError("V3040", "MapContains requires a map and string key", instruction.span);
                return false;
            }
            return Push(RbScriptValue::FromBoolean(map.mapValue->values.find(key.stringValue) != map.mapValue->values.end()), instruction.span);
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
    if (instruction.operand0 < 0)
    {
        if (!nativeCallHandler_ || instruction.operand2 < 0
            || static_cast<unsigned>(instruction.operand2) >= chunk.constants.size()
            || chunk.constants[instruction.operand2].kind != RbScriptConstantKind::String)
        {
            AddError("V3020", "Cannot call an unresolved rbscript function", instruction.span);
            valueStack_.resize(valueStack_.size() - static_cast<unsigned>(argumentCount));
            return false;
        }

        const unsigned argumentBase = static_cast<unsigned>(valueStack_.size()) - static_cast<unsigned>(argumentCount);
        ea::vector<RbScriptValue> arguments;
        arguments.reserve(static_cast<unsigned>(argumentCount));
        for (unsigned i = 0; i < static_cast<unsigned>(argumentCount); ++i)
            arguments.push_back(valueStack_[argumentBase + i]);

        RbScriptValue result;
        const ea::string& functionName = chunk.constants[instruction.operand2].stringValue;
        if (!nativeCallHandler_(functionName, arguments, result))
        {
            AddError("V3023", "Native rbscript function failed: '" + functionName + "'", instruction.span);
            valueStack_.resize(argumentBase);
            return false;
        }
        valueStack_.resize(argumentBase);
        return Push(result, instruction.span);
    }
    if (static_cast<unsigned>(instruction.operand0) >= chunk.functions.size())
    {
        AddError("V3020", "Cannot call an rbscript function outside the chunk", instruction.span);
        return false;
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
