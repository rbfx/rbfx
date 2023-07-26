//
// Copyright (c) 2017-2021 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Core/Assert.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/Input/Input.h>

#include <catch2/catch_amalgamated.hpp>

#include <EASTL/optional.h>

#include <ostream>

using namespace Urho3D;

namespace Urho3D
{

class Resource;

}

namespace Tests
{

/// Callback used to create context.
using CreateContextCallback = SharedPtr<Context> (*)();

/// Get or create test context.
SharedPtr<Context> GetOrCreateContext(CreateContextCallback callback);

/// Reset test context created by GetOrCreateContext.
void ResetContext();

/// Create test context with all subsystems ready.
SharedPtr<Context> CreateCompleteContext();

/// Run frame with given time step.
void RunFrame(Context* context, float timeStep, float maxTimeStep = M_LARGE_VALUE);

/// Return resource by name. Creates and adds manual resource if missing.
Resource* GetOrCreateResource(
    Context* context, StringHash type, const ea::string& name, ea::function<SharedPtr<Resource>(Context*)> factory);

void SendMouseMoveEvent(Input* input, IntVector2 pos, IntVector2 delta = IntVector2::ZERO);

void SendKeyEvent(Input* input, StringHash eventId, Scancode scancode, Key key);

void SendDPadEvent(Input* input, HatPosition position, int hatIndex = 0, int joystickId = 0);

void SendJoystickDisconnected(Input* input, int joystickId = 0);

void SendAxisEvent(Input* input, int axis, float value, int joystickId = 0);

/// Return resource by name. Creates and adds manual resource if missing.
template <class T>
T* GetOrCreateResource(Context* context, const ea::string& name, ea::function<SharedPtr<Resource>(Context*)> factory)
{
    return dynamic_cast<T*>(GetOrCreateResource(context, T::GetTypeStatic(), name, factory));
}

/// Helper class to track events in the engine.
/// Events are grouped by frames using specified event.
/// Events during the first tracked frame and after the last tracked frame are ignored.
class FrameEventTracker : public Object
{
    URHO3D_OBJECT(FrameEventTracker, Object);

public:
    explicit FrameEventTracker(Context* context);
    FrameEventTracker(Context* context, StringHash endFrameEventType);

    void TrackEvent(StringHash eventType);
    void TrackEvent(Object* object, StringHash eventType);

    unsigned GetNumFrames() const { return recordedFrames_.size(); }

    template <class T> void SkipFramesUntil(T callback)
    {
        const auto iter = ea::find_if(recordedFrames_.begin(), recordedFrames_.end(), callback);
        recordedFrames_.erase(recordedFrames_.begin(), iter);
    }

    void SkipFramesUntilEvent(StringHash eventType, unsigned hits = 1);
    void ValidatePattern(const ea::vector<ea::vector<StringHash>>& pattern) const;

private:
    struct EventRecord
    {
        StringHash eventType_;
        VariantMap eventData_;
    };

    void HandleEvent(StringHash eventType, VariantMap& eventData);

    bool recordEvents_{};
    ea::vector<EventRecord> currentFrameEvents_;
    ea::vector<ea::vector<EventRecord>> recordedFrames_;
};

/// Helper class to track attribute of serializable at specified event.
class AttributeTracker : public Object
{
    URHO3D_OBJECT(AttributeTracker, Object);

public:
    explicit AttributeTracker(Context* context);
    AttributeTracker(Context* context, StringHash endFrameEventType);

    void Track(Serializable* serializable, const ea::string& attributeName);

    template <class T> void SkipUntil(T callback)
    {
        const auto iter = ea::find_if(recordedValues_.begin(), recordedValues_.end(), callback);
        recordedValues_.erase(recordedValues_.begin(), iter);
    }

    void SkipUntilChanged()
    {
        const auto iter =
            ea::adjacent_find(recordedValues_.begin(), recordedValues_.end(), ea::not_equal_to<Variant>());
        recordedValues_.erase(recordedValues_.begin(), iter);
    }

    const ea::vector<Variant>& GetValues() const { return recordedValues_; }
    unsigned Size() const { return recordedValues_.size(); }
    const Variant& operator[](unsigned i) const { return recordedValues_[i]; }

private:
    ea::vector<ea::pair<WeakPtr<Serializable>, ea::string>> trackers_;
    ea::vector<Variant> recordedValues_;
};

/// Tag to mark object that should be fully registered via T::RegisterObject.
template <class T> struct RegisterObject
{
};

/// Helper class to register and unregister object from context.
class ScopedReflection : public MovableNonCopyable
{
public:
    template <class... Ts>
    ScopedReflection(Context* context, Ts*...)
        : context_(context)
    {
        RegisterTypes<Ts...>();
    }

    ~ScopedReflection()
    {
        for (auto type : registeredTypes_)
            context_->RemoveReflection(type);
    }

private:
    template <class T, class... Ts> void RegisterTypes()
    {
        DispatchRegister(static_cast<T*>(nullptr));
        if constexpr (sizeof...(Ts) > 0)
            RegisterTypes<Ts...>();
    }

    template <class T> void DispatchRegister(T*)
    {
        URHO3D_ASSERT(!context_->IsReflected<T>());
        context_->RegisterFactory<T>();
        registeredTypes_.push_back(T::GetTypeStatic());
    }

    template <class T> void DispatchRegister(RegisterObject<T>*)
    {
        URHO3D_ASSERT(!context_->IsReflected<T>());
        T::RegisterObject(context_);
        registeredTypes_.push_back(T::GetTypeStatic());
    }

    Context* context_{};
    ea::vector<StringHash> registeredTypes_;
};

template <class... Ts> ScopedReflection MakeScopedReflection(Context* context)
{
    return ScopedReflection(context, static_cast<Ts*>(nullptr)...);
}

} // namespace Tests

/// Convert common types to strings
/// @{
namespace Catch
{

#define DEFINE_STRING_MAKER(type, expr) \
    template <> struct StringMaker<type> \
    { \
        static std::string convert(const type& value) \
        { \
            return expr; \
        } \
    }

DEFINE_STRING_MAKER(Variant, value.ToString().c_str());
DEFINE_STRING_MAKER(Vector2, value.ToString().c_str());
DEFINE_STRING_MAKER(Vector3, value.ToString().c_str());
DEFINE_STRING_MAKER(Vector4, value.ToString().c_str());
DEFINE_STRING_MAKER(Quaternion, value.ToString().c_str());
DEFINE_STRING_MAKER(StringHash, value.ToString().c_str());

#undef DEFINE_STRING_MAKER

template <class T> struct StringMaker<ea::optional<T>>
{
    static std::string convert(const ea::optional<T>& value)
    {
        return value ? StringMaker<T>::convert(*value) : "(nullopt)";
    }
};

} // namespace Catch
/// @}
