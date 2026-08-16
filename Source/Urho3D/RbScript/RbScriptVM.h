// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "RbScriptCompiler.h"

#include <Urho3D/Math/Color.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>

#include <utility>

namespace Urho3D
{

struct RbScriptValue;
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

    bool IsTruthy() const;
    bool IsNumeric() const;
    double AsFloat() const;
    long long AsInteger() const;
    ea::string ToString() const;
};

class URHO3D_API RbScriptVM
{
public:
    RbScriptVM();

    bool Execute(const RbScriptChunk& chunk);
    bool ExecuteFunction(const RbScriptChunk& chunk, const ea::string& functionName,
        const ea::vector<RbScriptValue>& arguments = {});
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
    bool ExecuteBinary(RbScriptOpcode opcode, const RbScriptInstruction& instruction);
    bool ExecuteComparison(RbScriptOpcode opcode, const RbScriptInstruction& instruction);
    bool StartCall(const RbScriptChunk& chunk, CallFrame& caller, const RbScriptInstruction& instruction);
    bool ReturnFromCall(CallFrame& frame, const RbScriptInstruction& instruction);
    RbScriptValue ConstantToValue(const RbScriptConstant& constant) const;
    RbScriptValue NullResult() const;

    ea::vector<RbScriptValue> valueStack_;
    ea::vector<CallFrame> callStack_;
    ea::vector<RbScriptDiagnostic> diagnostics_;
    ea::vector<RbScriptValue> emittedEvents_;
    RbScriptValue result_;
    unsigned stepLimit_{1000000};
    unsigned stackLimit_{4096};
    unsigned callDepthLimit_{256};
    unsigned executedSteps_{0};
    bool halted_{false};
    RbScriptNativeCallHandler nativeCallHandler_;
};

} // namespace Urho3D
