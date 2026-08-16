// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "RbScriptCompiler.h"

#include <Urho3D/Math/Color.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>

#include <EASTL/shared_ptr.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

#include <utility>

namespace Urho3D
{

struct RbScriptValue;
struct RbScriptArray;
struct RbScriptMap;
using RbScriptNativeCallHandler = ea::function<bool(const ea::string&, const ea::vector<RbScriptValue>&, RbScriptValue&)>;

enum class RbScriptValueKind
{
    Null,
    Boolean,
    Integer,
    Float,
    String,
    Vector2,
    Vector3,
    Quaternion,
    Color,
    Pointer,
    Array,
    Map,
};

struct URHO3D_API RbScriptValue
{
    RbScriptValueKind kind{RbScriptValueKind::Null};
    bool booleanValue{false};
    long long integerValue{0};
    double floatValue{0.0};
    ea::string stringValue;
    Vector2 vector2Value{Vector2::ZERO};
    Vector3 vector3Value{Vector3::ZERO};
    Quaternion quaternionValue{Quaternion::IDENTITY};
    Color colorValue{Color::WHITE};
    void* pointerValue{nullptr};
    ea::shared_ptr<RbScriptArray> arrayValue;
    ea::shared_ptr<RbScriptMap> mapValue;

    static RbScriptValue Null();
    static RbScriptValue FromBoolean(bool value);
    static RbScriptValue FromInteger(long long value);
    static RbScriptValue FromFloat(double value);
    static RbScriptValue FromString(const ea::string& value);
    static RbScriptValue FromVector2(const Vector2& value);
    static RbScriptValue FromVector3(const Vector3& value);
    static RbScriptValue FromQuaternion(const Quaternion& value);
    static RbScriptValue FromColor(const Color& value);
    static RbScriptValue FromPointer(void* value);
    static RbScriptValue FromArray(const ea::vector<RbScriptValue>& value);
    static RbScriptValue FromMap(const ea::unordered_map<ea::string, RbScriptValue>& value);

    bool IsTruthy() const;
    bool IsNumeric() const;
    double AsFloat() const;
    long long AsInteger() const;
    ea::string ToString() const;
};

struct URHO3D_API RbScriptArray
{
    ea::vector<RbScriptValue> values;
};

struct URHO3D_API RbScriptMap
{
    ea::unordered_map<ea::string, RbScriptValue> values;
};

class URHO3D_API RbScriptVM
{
public:
    RbScriptVM();

    bool Execute(const RbScriptChunk& chunk);
    bool ExecuteFunction(const RbScriptChunk& chunk, const ea::string& functionName,
        const ea::vector<RbScriptValue>& arguments = {});

    /// Start a source-level debugging session without executing the first instruction.
    bool BeginDebug(const RbScriptChunk& chunk, const ea::string& functionName = {},
        const ea::vector<RbScriptValue>& arguments = {});
    /// Execute exactly one bytecode instruction in the active debug session.
    bool StepDebug();
    /// Continue until a breakpoint, completion, or an execution error.
    bool ContinueDebug();
    /// Stop the current debug session and discard its active call frames.
    void StopDebug();
    bool IsDebugging() const { return debugActive_; }
    bool IsDebugPaused() const { return debugPaused_; }
    void SetBreakpoint(unsigned line);
    void RemoveBreakpoint(unsigned line);
    const ea::vector<unsigned>& GetBreakpoints() const { return breakpoints_; }
    unsigned GetCurrentLine() const { return debugCurrentLine_; }
    const ea::unordered_map<ea::string, RbScriptValue>& GetLocals() const { return debugLocals_; }
    const ea::vector<ea::string>& GetCallStack() const { return debugCallStack_; }

    void SetNativeCallHandler(RbScriptNativeCallHandler handler) { nativeCallHandler_ = std::move(handler); }
    const RbScriptValue& GetResult() const { return result_; }
    const ea::vector<RbScriptDiagnostic>& GetDiagnostics() const { return diagnostics_; }
    const ea::vector<RbScriptValue>& GetEmittedEvents() const { return emittedEvents_; }
    bool HadError() const;

    void SetStepLimit(unsigned limit) { stepLimit_ = limit; }
    void SetStackLimit(unsigned limit) { stackLimit_ = limit; }
    void SetCallDepthLimit(unsigned limit) { callDepthLimit_ = limit; }
    unsigned GetExecutedSteps() const { return executedSteps_; }

private:
    struct CallFrame
    {
        unsigned functionIndex{0};
        unsigned instructionPointer{0};
        unsigned returnStackSize{0};
        ea::vector<RbScriptValue> locals;
    };

    void AddError(const ea::string& code, const ea::string& message, const RbScriptSourceSpan& span);
    bool Push(const RbScriptValue& value, const RbScriptSourceSpan& span);
    bool Pop(RbScriptValue& value, const RbScriptSourceSpan& span);
    bool ExecuteInstruction(const RbScriptChunk& chunk, CallFrame& frame, const RbScriptInstruction& instruction);
    bool ExecuteEntry(const RbScriptChunk& chunk, unsigned functionIndex, const ea::vector<RbScriptValue>& arguments);
    bool PrepareExecution(const RbScriptChunk& chunk, unsigned functionIndex, const ea::vector<RbScriptValue>& arguments);
    bool ExecuteOneDebugInstruction();
    bool IsBreakpointAtCurrentInstruction() const;
    void RefreshDebugSnapshot();
    bool ExecuteBinary(RbScriptOpcode opcode, const RbScriptInstruction& instruction);
    bool ExecuteComparison(RbScriptOpcode opcode, const RbScriptInstruction& instruction);
    bool StartCall(const RbScriptChunk& chunk, CallFrame& caller, const RbScriptInstruction& instruction);
    bool ReturnFromCall(CallFrame& frame, const RbScriptInstruction& instruction);
    RbScriptValue ConstantToValue(const RbScriptConstant& constant) const;
    RbScriptValue NullResult() const;

    ea::vector<RbScriptValue> valueStack_;
    ea::vector<RbScriptValue> fieldValues_;
    ea::vector<CallFrame> callStack_;
    ea::vector<RbScriptDiagnostic> diagnostics_;
    ea::vector<RbScriptValue> emittedEvents_;
    RbScriptValue result_;
    unsigned stepLimit_{1000000};
    unsigned stackLimit_{4096};
    unsigned callDepthLimit_{256};
    unsigned executedSteps_{0};
    bool halted_{false};
    const RbScriptChunk* debugChunk_{nullptr};
    ea::vector<unsigned> breakpoints_;
    ea::unordered_map<ea::string, RbScriptValue> debugLocals_;
    ea::vector<ea::string> debugCallStack_;
    unsigned debugCurrentLine_{0};
    bool debugActive_{false};
    bool debugPaused_{false};
    RbScriptNativeCallHandler nativeCallHandler_;
};

} // namespace Urho3D
