//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Precompiled.h"

#include "../Audio/Sound.h"
#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/Shader.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Viewport.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Technique.h"
#include "../Graphics/Renderer.h"
#include "../Input/Input.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ResourceEvents.h"
#include "../Scene/Scene.h"
#ifdef URHO3D_SYSTEMUI
#include "../SystemUI/SystemUI.h"
#endif
#include "../RmlUI/RmlUI.h"
#include "../RmlUI/RmlRenderer.h"
#include "../RmlUI/RmlSystem.h"
#include "../RmlUI/RmlFile.h"
#include "../RmlUI/RmlEventListeners.h"
#include "../RmlUI/RmlCanvasComponent.h"
#include "../RmlUI/RmlNavigable.h"
#include "../RmlUI/RmlSerializableInspector.h"
#include "../RmlUI/RmlUIComponent.h"
#include "Urho3D/RenderAPI/RenderContext.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include <atomic>
#include <EASTL/fixed_vector.h>
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>

#include "../DebugNew.h"

namespace Urho3D
{

static MouseButton MakeTouchIDMask(int id)
{
    return static_cast<MouseButton>(1u << static_cast<MouseButtonFlags::Integer>(id)); // NOLINT(misc-misplaced-widening-cast)
}

static int MouseButtonUrho3DToRml(MouseButton button);
static int ModifiersUrho3DToRml(QualifierFlags modifier);

namespace Detail
{

/// Event instancer that translates some inline events to native Urho3D events.
class RmlEventListenerInstancer : public Rml::EventListenerInstancer
{
public:
    /// Create an instance of inline event listener, if applicable.
    Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* element) override
    {
        return PipeEventListener::CreateInstancer(value, element);
    }
};

class RmlContextInstancer : public Rml::ContextInstancer
{
public:
    /// Create instance of RmlContext.
    Rml::ContextPtr InstanceContext(
        const Rml::String& name, Rml::RenderManager* render_manager, Rml::TextInputHandler* text_input_handler) override
    {
        return Rml::ContextPtr(new Detail::RmlContext(name, render_manager, text_input_handler));
    }
    /// Free instance of RmlContext.
    void ReleaseContext(Rml::Context* context) override { delete static_cast<Detail::RmlContext*>(context); }

protected:
    /// RmlContextInstancer is static, nothing to release.
    void Release() override {}
};

class RmlPlugin : public Rml::Plugin
{
public:
    /// Subscribe to events.
    int GetEventClasses() override { return EVT_DOCUMENT; }
    /// Handle documents being closed from within RmlUi middleware.
    void OnDocumentUnload(Rml::ElementDocument* document) override
    {
        RmlContext* rmlContext = static_cast<RmlContext*>(document->GetContext());
        if (RmlUI* ui = rmlContext->GetOwnerSubsystem())
            ui->OnDocumentUnload(document);
    }
};

class VariantMapDefinition final : public Rml::VariableDefinition
{
public:
    VariantMapDefinition(Rml::DataTypeRegister* typeRegister)
        : Rml::VariableDefinition(Rml::DataVariableType::Struct)
        , register_(typeRegister)
    {
    }

    virtual Rml::DataVariable Child(void* ptr, const Rml::DataAddressEntry& address)
    {
        const auto& name = address.name;
        if (name.empty())
        {
            Rml::Log::Message(Rml::Log::LT_WARNING, "Expected a struct member name but none given.");
            return {};
        }

        auto& value = *static_cast<VariantMap*>(ptr);

        auto it = value.find(name);
        if (it == value.end())
        {
            Rml::Log::Message(Rml::Log::LT_WARNING, "Member %s not found in data struct.", name.c_str());
            return {};
        }

        return {register_->GetDefinition<Variant>(), &it->second};
    }

private:
    Rml::DataTypeRegister* register_;
};

class VariantDefinition final : public Rml::VariableDefinition
{
public:
    VariantDefinition(Rml::DataTypeRegister* typeRegister)
        : Rml::VariableDefinition(Rml::DataVariableType::Scalar)
        , register_(typeRegister)
    {
    }

    bool Get(void* ptr, Rml::Variant& variant) override
    {
        auto& value = *static_cast<const Variant*>(ptr);
        return ToRmlUi(value, variant);
    }

    bool Set(void* ptr, const Rml::Variant& variant) override
    {
        auto& value = *static_cast<Variant*>(ptr);
        return FromRmlUi(variant, value);
    }

    int Size(void* ptr) override
    {
        const auto& value = *static_cast<const Variant*>(ptr);
        switch (value.GetType())
        {
        case VAR_RESOURCEREFLIST: return static_cast<int>(value.GetResourceRefList().names_.size());
        case VAR_STRINGVECTOR: return static_cast<int>(value.GetStringVector().size());
        case VAR_VARIANTVECTOR: return  static_cast<int>(value.GetVariantVector().size());
        default: return 0;
        }
    }

    static bool ValidateIndex(int index, size_t size)
    {
        if (index < 0 || static_cast<size_t>(index) >= size)
        {
            Rml::Log::Message(Rml::Log::LT_WARNING, "Data array index %d out of bounds.", index);
            return false;
        }
        return true;
    }

    Rml::DataVariable Child(void* ptr, const Rml::DataAddressEntry& address) override
    {
        auto& value = *static_cast<Variant*>(ptr);

        if (address.name.empty())
        {
            const int index = address.index;
            switch (value.GetType())
            {
            case VAR_VARIANTVECTOR:
            {
                auto* vector = value.GetVariantVectorPtr();
                return (vector && ValidateIndex(index, vector->size())) ? Rml::DataVariable(this, &(*vector)[index])
                                                                        : Rml::DataVariable();
            }
            case VAR_RESOURCEREFLIST:
            {
                auto& vector = value.GetResourceRefList();
                return (ValidateIndex(index, vector.names_.size())) ? Rml::DataVariable(
                           register_->GetDefinition<Rml::String>(), const_cast<Rml::String*>(&(vector.names_[index])))
                                                                    : Rml::DataVariable();
            }
            case VAR_STRINGVECTOR:
            {
                auto* vector = value.GetStringVectorPtr();
                return (vector && ValidateIndex(index, vector->size()))
                    ? Rml::DataVariable(register_->GetDefinition<eastl::string>(), &(*vector)[index])
                    : Rml::DataVariable();
            }
            default:
                Rml::Log::Message(
                    Rml::Log::LT_WARNING, "Index operation is not valid on type %s.", value.GetTypeName().c_str());
                return {};
            }
        }
        else
        {
            const auto& name = address.name;
            switch (value.GetType())
            {
            case VAR_VARIANTMAP:
            {
                auto* valuePtr = value.GetVariantMapPtr();
                if (!valuePtr)
                    return {};

                auto it = valuePtr->find(name);
                if (it == valuePtr->end())
                {
                    Rml::Log::Message(Rml::Log::LT_WARNING, "Member %s not found in VariantMap.", name.c_str());
                    return {};
                }

                return {this, &it->second};
            }
            default:
                Rml::Log::Message(Rml::Log::LT_WARNING, "Get property %s operation is not valid on type %s.",
                    name.c_str(), value.GetTypeName().c_str());
                return {};
            }
        }
    }

    Rml::DataTypeRegister* register_;
};

} // namespace Detail

/// Number of instances of RmlUI. Used to initialize and release RmlUi library.
static std::atomic<int> rmlInstanceCounter;

/// A standalone object which creates event instances for RmlUi.
static Detail::RmlEventListenerInstancer RmlEventListenerInstancerInstance;

/// A standalone object which creates Context instances for RmlUi.
static Detail::RmlContextInstancer RmlContextInstancerInstance;

/// A standalone object which acts as a bridge between our classes and RmlUi middleware.
static Detail::RmlPlugin RmlPluginInstance;

/// Map engine keys to RmlUi keys. Note that top bit is cleared from key constants when they are used as array index.
static const ea::unordered_map<unsigned, uint16_t> keyMap{
    { KEY_SPACE, Rml::Input::KI_SPACE },
    { KEY_0, Rml::Input::KI_0 },
    { KEY_1, Rml::Input::KI_1 },
    { KEY_2, Rml::Input::KI_2 },
    { KEY_3, Rml::Input::KI_3 },
    { KEY_4, Rml::Input::KI_4 },
    { KEY_5, Rml::Input::KI_5 },
    { KEY_6, Rml::Input::KI_6 },
    { KEY_7, Rml::Input::KI_7 },
    { KEY_8, Rml::Input::KI_8 },
    { KEY_9, Rml::Input::KI_9 },
    { KEY_A, Rml::Input::KI_A },
    { KEY_B, Rml::Input::KI_B },
    { KEY_C, Rml::Input::KI_C },
    { KEY_D, Rml::Input::KI_D },
    { KEY_E, Rml::Input::KI_E },
    { KEY_F, Rml::Input::KI_F },
    { KEY_G, Rml::Input::KI_G },
    { KEY_H, Rml::Input::KI_H },
    { KEY_I, Rml::Input::KI_I },
    { KEY_J, Rml::Input::KI_J },
    { KEY_K, Rml::Input::KI_K },
    { KEY_L, Rml::Input::KI_L },
    { KEY_M, Rml::Input::KI_M },
    { KEY_N, Rml::Input::KI_N },
    { KEY_O, Rml::Input::KI_O },
    { KEY_P, Rml::Input::KI_P },
    { KEY_Q, Rml::Input::KI_Q },
    { KEY_R, Rml::Input::KI_R },
    { KEY_S, Rml::Input::KI_S },
    { KEY_T, Rml::Input::KI_T },
    { KEY_U, Rml::Input::KI_U },
    { KEY_V, Rml::Input::KI_V },
    { KEY_W, Rml::Input::KI_W },
    { KEY_X, Rml::Input::KI_X },
    { KEY_Y, Rml::Input::KI_Y },
    { KEY_Z, Rml::Input::KI_Z },
    { KEY_SEMICOLON, Rml::Input::KI_OEM_1 },           // US standard keyboard; the ';:' key.
    { KEY_EQUALS, Rml::Input::KI_OEM_PLUS },           // Any region; the '=+' key.
    { KEY_COMMA, Rml::Input::KI_OEM_COMMA },           // Any region; the ',<' key.
    { KEY_MINUS, Rml::Input::KI_OEM_MINUS },           // Any region; the '-_' key.
    { KEY_PERIOD, Rml::Input::KI_OEM_PERIOD },         // Any region; the '.>' key.
    { KEY_SLASH, Rml::Input::KI_OEM_2 },               // Any region; the '/?' key.
    { KEY_LEFTBRACKET, Rml::Input::KI_OEM_4 },         // US standard keyboard; the '[{' key.
    { KEY_BACKSLASH, Rml::Input::KI_OEM_5 },           // US standard keyboard; the '\|' key.
    { KEY_RIGHTBRACKET, Rml::Input::KI_OEM_6 },        // US standard keyboard; the ']}' key.
    { KEY_KP_0, Rml::Input::KI_NUMPAD0 },
    { KEY_KP_1, Rml::Input::KI_NUMPAD1 },
    { KEY_KP_2, Rml::Input::KI_NUMPAD2 },
    { KEY_KP_3, Rml::Input::KI_NUMPAD3 },
    { KEY_KP_4, Rml::Input::KI_NUMPAD4 },
    { KEY_KP_5, Rml::Input::KI_NUMPAD5 },
    { KEY_KP_6, Rml::Input::KI_NUMPAD6 },
    { KEY_KP_7, Rml::Input::KI_NUMPAD7 },
    { KEY_KP_8, Rml::Input::KI_NUMPAD8 },
    { KEY_KP_9, Rml::Input::KI_NUMPAD9 },
    { KEY_KP_ENTER, Rml::Input::KI_NUMPADENTER },
    { KEY_KP_MULTIPLY, Rml::Input::KI_MULTIPLY },      // Asterisk on the numeric keypad.
    { KEY_KP_PLUS, Rml::Input::KI_ADD },               // Plus on the numeric keypad.
    { KEY_KP_SPACE, Rml::Input::KI_SEPARATOR },
    { KEY_KP_MINUS, Rml::Input::KI_SUBTRACT },         // Minus on the numeric keypad.
    { KEY_KP_DECIMAL, Rml::Input::KI_DECIMAL },        // Period on the numeric keypad.
    { KEY_KP_DIVIDE, Rml::Input::KI_DIVIDE },          // Forward Slash on the numeric keypad.
    { KEY_BACKSPACE, Rml::Input::KI_BACK },            // Backspace key.
    { KEY_TAB, Rml::Input::KI_TAB },                   // Tab key.
    { KEY_CLEAR, Rml::Input::KI_CLEAR },
    { KEY_RETURN, Rml::Input::KI_RETURN },
    { KEY_PAUSE, Rml::Input::KI_PAUSE },
    { KEY_CAPSLOCK, Rml::Input::KI_CAPITAL },          // Capslock key.
    { KEY_ESCAPE, Rml::Input::KI_ESCAPE },             // Escape key.
    { KEY_PAGEUP, Rml::Input::KI_PRIOR },              // Page Up key.
    { KEY_PAGEDOWN, Rml::Input::KI_NEXT },             // Page Down key.
    { KEY_END, Rml::Input::KI_END },
    { KEY_HOME, Rml::Input::KI_HOME },
    { KEY_LEFT, Rml::Input::KI_LEFT },                 // Left Arrow key.
    { KEY_UP, Rml::Input::KI_UP },                     // Up Arrow key.
    { KEY_RIGHT, Rml::Input::KI_RIGHT },               // Right Arrow key.
    { KEY_DOWN, Rml::Input::KI_DOWN },                 // Down Arrow key.
    { KEY_SELECT, Rml::Input::KI_SELECT },
    { KEY_PRINTSCREEN, Rml::Input::KI_SNAPSHOT },      // Print Screen key.
    { KEY_INSERT, Rml::Input::KI_INSERT },
    { KEY_DELETE, Rml::Input::KI_DELETE },
    { KEY_HELP, Rml::Input::KI_HELP },
    { KEY_LGUI, Rml::Input::KI_LWIN },                 // Left Windows key.
    { KEY_RGUI, Rml::Input::KI_RWIN },                 // Right Windows key.
    { KEY_APPLICATION, Rml::Input::KI_APPS },          // Applications key.
    { KEY_POWER, Rml::Input::KI_POWER },
    { KEY_SLEEP, Rml::Input::KI_SLEEP },
    { KEY_F1, Rml::Input::KI_F1 },
    { KEY_F2, Rml::Input::KI_F2 },
    { KEY_F3, Rml::Input::KI_F3 },
    { KEY_F4, Rml::Input::KI_F4 },
    { KEY_F5, Rml::Input::KI_F5 },
    { KEY_F6, Rml::Input::KI_F6 },
    { KEY_F7, Rml::Input::KI_F7 },
    { KEY_F8, Rml::Input::KI_F8 },
    { KEY_F9, Rml::Input::KI_F9 },
    { KEY_F10, Rml::Input::KI_F10 },
    { KEY_F11, Rml::Input::KI_F11 },
    { KEY_F12, Rml::Input::KI_F12 },
    { KEY_F13, Rml::Input::KI_F13 },
    { KEY_F14, Rml::Input::KI_F14 },
    { KEY_F15, Rml::Input::KI_F15 },
    { KEY_F16, Rml::Input::KI_F16 },
    { KEY_F17, Rml::Input::KI_F17 },
    { KEY_F18, Rml::Input::KI_F18 },
    { KEY_F19, Rml::Input::KI_F19 },
    { KEY_F20, Rml::Input::KI_F20 },
    { KEY_F21, Rml::Input::KI_F21 },
    { KEY_F22, Rml::Input::KI_F22 },
    { KEY_F23, Rml::Input::KI_F23 },
    { KEY_F24, Rml::Input::KI_F24 },
    { KEY_NUMLOCKCLEAR, Rml::Input::KI_NUMLOCK },      // Numlock key.
    { KEY_SCROLLLOCK, Rml::Input::KI_SCROLL },         // Scroll Lock key.
    { KEY_LSHIFT, Rml::Input::KI_LSHIFT },
    { KEY_RSHIFT, Rml::Input::KI_RSHIFT },
    { KEY_LCTRL, Rml::Input::KI_LCONTROL },
    { KEY_RCTRL, Rml::Input::KI_RCONTROL },
    { KEY_LALT, Rml::Input::KI_LMENU },
    { KEY_RALT, Rml::Input::KI_RMENU },
    { KEY_MUTE, Rml::Input::KI_VOLUME_MUTE },
    { KEY_VOLUMEDOWN, Rml::Input::KI_VOLUME_DOWN },
    { KEY_VOLUMEUP, Rml::Input::KI_VOLUME_UP },
};

RmlUI::RmlUI(Context* context, const char* name)
    : Object(context)
    , name_(name)
{
    // Initializing first instance of RmlUI, initialize backend library as well.
    if (rmlInstanceCounter.fetch_add(1) == 0)
    {
        legacyRenderer_ = ea::make_unique<Detail::RmlRenderer>(context_);
        Rml::SetRenderInterface(legacyRenderer_->GetAdaptedInterface());
        Rml::SetSystemInterface(new Detail::RmlSystem(context_));
        Rml::SetFileInterface(new Detail::RmlFile(context_));
        Rml::Initialise();
        Rml::Factory::RegisterEventListenerInstancer(&RmlEventListenerInstancerInstance);
        Rml::Factory::RegisterContextInstancer(&RmlContextInstancerInstance);
        Rml::RegisterPlugin(&RmlPluginInstance);

        RmlNavigable::Register();
    }
    rmlContext_ = static_cast<Detail::RmlContext*>(Rml::CreateContext(name_.c_str(), ToRmlUi(GetDesiredCanvasSize())));
    rmlContext_->SetOwnerSubsystem(this);

    if (auto* ui = GetSubsystem<RmlUI>())
        ui->siblingSubsystems_.push_back(WeakPtr(this));

    Input* input = context_->GetSubsystem<Input>();
    URHO3D_ASSERT(input);
    SubscribeToEvent(input, E_MOUSEBUTTONDOWN, &RmlUI::HandleMouseButtonDown);
    SubscribeToEvent(input, E_MOUSEBUTTONUP, &RmlUI::HandleMouseButtonUp);
    SubscribeToEvent(input, E_MOUSEMOVE, &RmlUI::HandleMouseMove);
    SubscribeToEvent(input, E_MOUSEWHEEL, &RmlUI::HandleMouseWheel);
    SubscribeToEvent(input, E_TOUCHBEGIN, &RmlUI::HandleTouchBegin);
    SubscribeToEvent(input, E_TOUCHEND, &RmlUI::HandleTouchEnd);
    SubscribeToEvent(input, E_TOUCHMOVE, &RmlUI::HandleTouchMove);
    SubscribeToEvent(input, E_KEYDOWN, &RmlUI::HandleKeyDown);
    SubscribeToEvent(input, E_KEYUP, &RmlUI::HandleKeyUp);
    SubscribeToEvent(E_TEXTINPUT, &RmlUI::HandleTextInput);
    SubscribeToEvent(E_DROPFILE, &RmlUI::HandleDropFile);

    SubscribeToEvent(E_SCREENMODE, &RmlUI::HandleScreenMode);
    SubscribeToEvent(E_POSTUPDATE, &RmlUI::HandlePostUpdate);
    SubscribeToEvent(E_ENDALLVIEWSRENDER, &RmlUI::HandleEndAllViewsRender);

    SubscribeToEvent(E_FILECHANGED, &RmlUI::HandleResourceReloaded);
}

RmlUI::~RmlUI()
{
    if (auto* ui = GetSubsystem<RmlUI>())
        ui->siblingSubsystems_.erase_first(WeakPtr(this));

    if (rmlContext_ != nullptr)
    {
        if (!Rml::RemoveContext(rmlContext_->GetName()))
            URHO3D_LOGERROR("Removal of RmlUI context {} failed.", rmlContext_->GetName());
    }
    rmlContext_ = nullptr;

    if (rmlInstanceCounter.fetch_sub(1) == 1)
    {
        // Freeing last instance of RmlUI, deinitialize backend library.
        Rml::Factory::RegisterEventListenerInstancer(nullptr); // Set to a static object instance because there is no getter to delete it.
        auto* renderer = Rml::GetRenderInterface();
        auto* system = Rml::GetSystemInterface();
        auto* file = Rml::GetFileInterface();
        Rml::ReleaseTextures();
        Rml::Shutdown();
        delete renderer;
        delete system;
        delete file;
    }
}

Rml::ElementDocument* RmlUI::LoadDocument(const ea::string& path)
{
    return rmlContext_->LoadDocument(path);
}

void RmlUI::SetDebuggerVisible(bool visible)
{
    if (!debuggerInitialized_)
    {
        Rml::Debugger::Initialise(rmlContext_);
        debuggerInitialized_ = true;
    }
    Rml::Debugger::SetVisible(visible);
}

bool RmlUI::LoadFont(const ea::string& resourceName, bool fallback)
{
    return Rml::LoadFontFace(resourceName, fallback);
}

void RmlUI::ReloadFonts()
{
    auto cache = GetSubsystem<ResourceCache>();

    ea::vector<ea::string> fonts;
    for (const char* pattern : {"*.ttf", "*.otf"})
        cache->Scan(fonts, "Fonts/", pattern, SCAN_FILES | SCAN_RECURSIVE | SCAN_APPEND);

    for (const ea::string& font : fonts)
        LoadFont(Format("Fonts/{}", font));
}

Rml::Context* RmlUI::GetRmlContext() const
{
    return rmlContext_;
}

void RmlUI::HandleScreenMode(StringHash, VariantMap& eventData)
{
    assert(rmlContext_ != nullptr);
    RmlCanvasResizedArgs args;
    args.oldSize_ = ToIntVector2(rmlContext_->GetDimensions());
    args.newSize_ = GetDesiredCanvasSize();
    rmlContext_->SetDimensions(ToRmlUi(args.newSize_));
    canvasResizedEvent_(this, args);
}

void RmlUI::HandleMouseButtonDown(StringHash, VariantMap& eventData)
{
    using namespace MouseButtonDown;
    int button = MouseButtonUrho3DToRml(static_cast<MouseButton>(eventData[P_BUTTON].GetInt()));
    int modifiers = ModifiersUrho3DToRml(static_cast<QualifierFlags>(eventData[P_QUALIFIERS].GetInt()));

    // Manage focus between multiple RmlUI contexts. If no element or root element is hovered - unfocus focused input element.
    if (!IsHovered() && IsInputCapturedInternal())
    {
        Detail::RmlSystem* rmlSystem = static_cast<Detail::RmlSystem*>(Rml::GetSystemInterface());
        bool isTextInputActive = rmlSystem->TextInputActivatedThisFrame();
        rmlContext_->GetFocusElement()->Blur();
        if (isTextInputActive)
        {
            Input* input = GetSubsystem<Input>();
            input->SetScreenKeyboardVisible(true);
        }
        // Do not process click as it is clearly not meant for this context.
        return;
    }
    rmlContext_->ProcessMouseButtonDown(button, modifiers);
}

void RmlUI::HandleMouseButtonUp(StringHash, VariantMap& eventData)
{
    using namespace MouseButtonUp;
    int button = MouseButtonUrho3DToRml(static_cast<MouseButton>(eventData[P_BUTTON].GetInt()));
    int modifiers = ModifiersUrho3DToRml(static_cast<QualifierFlags>(eventData[P_QUALIFIERS].GetInt()));
    rmlContext_->ProcessMouseButtonUp(button, modifiers);
}

void RmlUI::HandleMouseMove(StringHash, VariantMap& eventData)
{
    using namespace MouseMove;
    int modifiers = ModifiersUrho3DToRml(static_cast<QualifierFlags>(eventData[P_QUALIFIERS].GetInt()));
    IntVector2 pos(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
    mouseMoveEvent_(this, pos);
    if (pos.x_ >= 0 && pos.y_ >= 0)
        rmlContext_->ProcessMouseMove(pos.x_, pos.y_, modifiers);
}

void RmlUI::HandleMouseWheel(StringHash, VariantMap& eventData)
{
    using namespace MouseWheel;
    auto* input = GetSubsystem<Input>();
    if (input->IsMouseGrabbed())
        return;
    int modifiers = ModifiersUrho3DToRml(static_cast<QualifierFlags>(eventData[P_QUALIFIERS].GetInt()));
    rmlContext_->ProcessMouseWheel(-eventData[P_WHEEL].GetInt(), modifiers);
}

void RmlUI::HandleTouchBegin(StringHash, VariantMap& eventData)
{
    using namespace TouchBegin;
    auto* input = GetSubsystem<Input>();
    if (input->IsMouseGrabbed())
        return;
    const MouseButton touchId = MakeTouchIDMask(eventData[P_TOUCHID].GetInt());
    int modifiers = ModifiersUrho3DToRml(input->GetQualifiers());
    int button = MouseButtonUrho3DToRml(touchId);
    IntVector2 pos(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
    mouseMoveEvent_(this, pos);
    if (pos.x_ >= 0 && pos.y_ >= 0)
        rmlContext_->ProcessMouseMove(pos.x_, pos.y_, modifiers);
    rmlContext_->ProcessMouseButtonDown(button, modifiers);
}

void RmlUI::HandleTouchEnd(StringHash, VariantMap& eventData)
{
    using namespace TouchEnd;
    auto* input = GetSubsystem<Input>();
    const MouseButton touchId = MakeTouchIDMask(eventData[P_TOUCHID].GetInt());
    int modifiers = ModifiersUrho3DToRml(input->GetQualifiers());
    int button = MouseButtonUrho3DToRml(touchId);
    rmlContext_->ProcessMouseMove(eventData[P_X].GetInt(), eventData[P_Y].GetInt(), modifiers);
    rmlContext_->ProcessMouseButtonUp(button, modifiers);
}

void RmlUI::HandleTouchMove(StringHash, VariantMap& eventData)
{
    using namespace TouchMove;
    auto* input = GetSubsystem<Input>();
    // const MouseButton touchId = MakeTouchIDMask(eventData[P_TOUCHID].GetInt());
    int modifiers = ModifiersUrho3DToRml(input->GetQualifiers());
    IntVector2 pos(eventData[P_X].GetInt(), eventData[P_Y].GetInt());
    mouseMoveEvent_(this, pos);
    if (pos.x_ >= 0 && pos.y_ >= 0)
        rmlContext_->ProcessMouseMove(pos.x_, pos.y_, modifiers);
}

void RmlUI::HandleKeyDown(StringHash, VariantMap& eventData)
{
    using namespace KeyDown;
    auto it = keyMap.find(eventData[P_KEY].GetUInt());
    if (it == keyMap.end())
        return;
    Rml::Input::KeyIdentifier key = static_cast<Rml::Input::KeyIdentifier>(it->second);
    int modifiers = ModifiersUrho3DToRml((QualifierFlags)eventData[P_QUALIFIERS].GetInt());
    rmlContext_->ProcessKeyDown(key, modifiers);
    if (key == Rml::Input::KI_RETURN || key == Rml::Input::KI_NUMPADENTER)
        rmlContext_->ProcessTextInput('\n');
}

void RmlUI::HandleKeyUp(StringHash, VariantMap& eventData)
{
    using namespace KeyUp;
    auto it = keyMap.find(eventData[P_KEY].GetUInt());
    if (it == keyMap.end())
        return;
    Rml::Input::KeyIdentifier key = static_cast<Rml::Input::KeyIdentifier>(it->second);
    int modifiers = ModifiersUrho3DToRml((QualifierFlags)eventData[P_QUALIFIERS].GetInt());
    rmlContext_->ProcessKeyUp(key, modifiers);
}

void RmlUI::HandleTextInput(StringHash, VariantMap& eventData)
{
    using namespace TextInput;
    rmlContext_->ProcessTextInput(eventData[P_TEXT].GetString().c_str());
}

void RmlUI::HandlePostUpdate(StringHash, VariantMap& eventData)
{
    Update(eventData[PostUpdate::P_TIMESTEP].GetFloat());
}

void RmlUI::HandleDropFile(StringHash, VariantMap& eventData)
{
    using namespace DropFile;
    auto* input = GetSubsystem<Input>();

    // Sending the UI variant of the event only makes sense if the OS cursor is visible (not locked to window center)
    if (!input->IsMouseVisible())
        return;

    if (auto* element = rmlContext_->GetHoverElement())
    {
        Rml::Dictionary args;
        args["path"] = eventData[P_FILENAME].GetString().c_str();
        element->DispatchEvent("dropfile", args);
    }
}

void RmlUI::HandleEndAllViewsRender(StringHash, VariantMap& eventData)
{
    if (isRendering_)
        Render();
}

void RmlUI::SetScale(float scale)
{
    rmlContext_->SetDensityIndependentPixelRatio(scale);
}

float RmlUI::GetScale() const
{
    return rmlContext_->GetDensityIndependentPixelRatio();
}

void RmlUI::SetRenderTarget(RenderSurface* target, const Color& clearColor)
{
    renderSurface_ = target;
    clearColor_ = clearColor;
    RmlCanvasResizedArgs args;
    args.oldSize_ = ToIntVector2(rmlContext_->GetDimensions());
    args.newSize_ = GetDesiredCanvasSize();
    rmlContext_->SetDimensions(ToRmlUi(args.newSize_));
    canvasResizedEvent_(this, args);
}

void RmlUI::SetRenderTarget(Texture2D* target, const Color& clearColor)
{
    SetRenderTarget(target ? target->GetRenderSurface() : nullptr, clearColor);
}

void RmlUI::SetRenderTarget(std::nullptr_t, const Color& clearColor)
{
    SetRenderTarget(static_cast<RenderSurface*>(nullptr), clearColor);
}

/// Try to convert variant from RmlUI to Urho3D.
bool FromRmlUi(const Rml::Variant& src, Variant& dst)
{
    switch (src.GetType())
    {
    case Rml::Variant::NONE: dst = Variant{}; return true;
    case Rml::Variant::BOOL: dst = src.Get<bool>(); return true;
    case Rml::Variant::INT: dst = src.Get<int>(); return true;
    case Rml::Variant::INT64: dst = static_cast<long long>(src.Get<int64_t>()); return true;
    case Rml::Variant::FLOAT: dst = src.Get<float>(); return true;
    case Rml::Variant::DOUBLE: dst = src.Get<double>(); return true;
    case Rml::Variant::STRING: dst = src.Get<Rml::String>(); return true;
    case Rml::Variant::VOIDPTR: dst = src.Get<void*>(); return true;
    case Rml::Variant::VECTOR2: dst = ToVector2(src.Get<Rml::Vector2f>()); return true;
    case Rml::Variant::VECTOR3: dst = ToVector3(src.Get<Rml::Vector3f>()); return true;
    case Rml::Variant::VECTOR4: dst = ToVector4(src.Get<Rml::Vector4f>()); return true;
    case Rml::Variant::COLOURF: dst = ToColor(src.Get<Rml::Colourf>()); return true;
    case Rml::Variant::COLOURB: dst = ToColor(src.Get<Rml::Colourb>()); return true;
    case Rml::Variant::BYTE: dst = src.Get<Rml::byte>(); return true;
    case Rml::Variant::CHAR: dst = src.Get<char>(); return true;
    case Rml::Variant::UINT: dst = src.Get<unsigned>(); return true;
    case Rml::Variant::UINT64: dst = static_cast<unsigned long long>(src.Get<uint64_t>()); return true;
    case Rml::Variant::SCRIPTINTERFACE: break;
    case Rml::Variant::TRANSFORMPTR: break;
    case Rml::Variant::TRANSITIONLIST: break;
    case Rml::Variant::ANIMATIONLIST: break;
    case Rml::Variant::DECORATORSPTR: break;
    case Rml::Variant::FONTEFFECTSPTR: break;
    }
    dst = src.Get<ea::string>();
    return true;
}

void RegisterVariantDefinition(Rml::DataTypeRegister* typeRegister)
{
    auto variantDef = Rml::MakeUnique<Detail::VariantDefinition>(typeRegister);
    auto* variantPtr = variantDef.get();
    typeRegister->RegisterDefinition(Rml::Family<Variant>::Id(), ea::move(variantDef));

    typeRegister->RegisterDefinition(
        Rml::Family<VariantVector>::Id(), ea::move(Rml::MakeUnique<Rml::ArrayDefinition<VariantVector>>(variantPtr)));

    typeRegister->RegisterDefinition(
        Rml::Family<VariantMap>::Id(), Rml::MakeUnique<Detail::VariantMapDefinition>(typeRegister));
}

/// Try to convert variant from Urho3D to RmlUI.
bool ToRmlUi(const Variant& src, Rml::Variant& dst)
{
    switch (src.GetType())
    {
    case VAR_NONE: dst = {}; return true;
    case VAR_BOOL: dst = src.Get<bool>(); return true;
    case VAR_INT: dst = src.Get<int>(); return true;
    case VAR_INT64: dst = static_cast<int64_t>(src.Get<long long>()); return true;
    case VAR_FLOAT: dst = src.Get<float>(); return true;
    case VAR_DOUBLE: dst = src.Get<double>(); return true;
    case VAR_STRING: dst = src.Get<ea::string>(); return true;
    case VAR_VOIDPTR: dst = src.Get<void*>(); return true;
    case VAR_VECTOR2: dst = ToRmlUi(src.Get<Vector2>()); return true;
    case VAR_VECTOR3: dst = ToRmlUi(src.Get<Vector3>()); return true;
    case VAR_VECTOR4: dst = ToRmlUi(src.Get<Vector4>()); return true;
    case VAR_COLOR: dst = ToRmlUi(src.Get<Color>()); return true;
    case VAR_QUATERNION: break;
    case VAR_BUFFER: break;
    case VAR_RESOURCEREF: break;
    case VAR_RESOURCEREFLIST: break;
    case VAR_VARIANTVECTOR: break;
    case VAR_VARIANTMAP: break;
    case VAR_INTRECT: break;
    case VAR_INTVECTOR2: break;
    case VAR_PTR: break;
    case VAR_MATRIX3: break;
    case VAR_MATRIX3X4: break;
    case VAR_MATRIX4: break;
    case VAR_STRINGVECTOR: break;
    case VAR_RECT: break;
    case VAR_INTVECTOR3: break;
    case VAR_CUSTOM: break;
    case VAR_VARIANTCURVE: break;
    case VAR_STRINGVARIANTMAP: break;
    }
    auto string = src.ToString();
    if (string.empty())
    {
        URHO3D_LOGERROR("This variant type conversion is not supported: {}", Variant::GetTypeNameList()[src.GetType()]);
        return false;
    }
    dst = string;
    return true;
}

IntVector2 RmlUI::GetDesiredCanvasSize() const
{
    RenderSurface* renderSurface = renderSurface_;
    if (renderSurface)
        return { renderSurface->GetWidth(), renderSurface->GetHeight() };
    else if (Graphics* graphics = GetSubsystem<Graphics>())
        return { graphics->GetWidth(), graphics->GetHeight() };

    // Irrelevant. Canvas will soon be resized once actual screen mode comes in.
    return {512, 512};
}

bool RmlUI::IsHovered() const
{
    Rml::Element* hover = rmlContext_->GetHoverElement();
    return hover != nullptr && hover != rmlContext_->GetRootElement();
}

bool RmlUI::IsInputCaptured() const
{
    if (IsInputCapturedInternal())
        return true;
    for (RmlUI* other : siblingSubsystems_)
    {
        if (other->IsInputCapturedInternal())
            return true;
    }
    return false;
}

bool RmlUI::IsInputCapturedInternal() const
{
    if (Rml::Element* element = rmlContext_->GetFocusElement())
    {
        const ea::string& tag = element->GetTagName();
        return tag == "input" || tag == "textarea" || tag == "select";
    }
    return false;
}

void RmlUI::Render()
{
    auto renderDevice = GetSubsystem<RenderDevice>();
    if (!renderDevice)
        return;

    URHO3D_PROFILE("RenderUI");

    RenderContext* renderContext = renderDevice->GetRenderContext();
    if (!renderSurface_)
    {
        renderContext->SetSwapChainRenderTargets();
    }
    else
    {
        const RenderTargetView renderTargets[] = {renderSurface_->GetView()};
        renderContext->SetRenderTargets(ea::nullopt, renderTargets);
        if (clearColor_.a_ > 0)
            renderContext->ClearRenderTarget(0, clearColor_);
    }
    renderContext->SetFullViewport();

    if (auto rmlRenderer = dynamic_cast<Detail::RmlRenderer*>(Rml::GetRenderInterface()))
    {
        rmlRenderer->BeginRendering();
        rmlContext_->Render();
        rmlRenderer->EndRendering();
    }
}

void RmlUI::OnDocumentUnload(Rml::ElementDocument* document)
{
    documentClosedEvent_(this, document);
}

void RmlUI::Update(float timeStep)
{
    (void)timeStep;
    URHO3D_PROFILE("UpdateUI");

    if (rmlContext_)
        rmlContext_->Update();
}

void RmlUI::HandleResourceReloaded(StringHash eventType, VariantMap& eventData)
{
    (void)eventType;
    using namespace FileChanged;
    const ea::string& resourceName = eventData[P_RESOURCENAME].GetString();
    Detail::RmlFile* file = static_cast<Detail::RmlFile*>(Rml::GetFileInterface());
    if (file->IsResourceLoaded(resourceName))
    {
        file->ClearLoadedResources();

        Rml::ReleaseTextures();
        Rml::Factory::ClearStyleSheetCache();
        Rml::Factory::ClearTemplateCache();

        ea::fixed_vector<Rml::ElementDocument*, 64> unloadingDocuments;
        for (int i = 0; i < rmlContext_->GetNumDocuments(); i++)
        {
            Rml::ElementDocument* document = rmlContext_->GetDocument(i);
            if (!document->GetSourceURL().empty())
                unloadingDocuments.push_back(document);
        }

        for (Rml::ElementDocument* document : unloadingDocuments)
            ReloadDocument(document);
    }
}

Rml::ElementDocument* RmlUI::ReloadDocument(Rml::ElementDocument* document)
{
    assert(document != nullptr);
    assert(document->GetContext() == rmlContext_);

    // Keep some properties of the old document
    const Vector2 oldPosition = ToVector2(document->GetAbsoluteOffset(Rml::BoxArea::Border));
    const Rml::ModalFlag oldModal = document->IsModal() ? Rml::ModalFlag::Modal : Rml::ModalFlag::None;
    const bool oldVisible = document->IsVisible();

    const Rml::Element* oldFocusedElement = rmlContext_->GetFocusElement();
    const Rml::FocusFlag focus = oldFocusedElement && oldFocusedElement->GetOwnerDocument() == document
        ? Rml::FocusFlag::Document
        : Rml::FocusFlag::Auto;

    const Rml::Property* oldLeftProperty = document->GetProperty(Rml::PropertyId::Left);
    const Rml::Property* oldTopProperty = document->GetProperty(Rml::PropertyId::Top);
    const Rml::Property* oldWidthProperty = document->GetProperty(Rml::PropertyId::Width);
    const Rml::Property* oldHeightProperty = document->GetProperty(Rml::PropertyId::Height);

    // Try to reload document
    Rml::ElementDocument* newDocument = rmlContext_->LoadDocument(document->GetSourceURL());
    if (!newDocument)
        return nullptr;

    // Setup persistent properties
    if (oldLeftProperty)
        newDocument->SetProperty(Rml::PropertyId::Left, *oldLeftProperty);
    if (oldTopProperty)
        newDocument->SetProperty(Rml::PropertyId::Top, *oldTopProperty);
    if (oldWidthProperty)
        newDocument->SetProperty(Rml::PropertyId::Width, *oldWidthProperty);
    if (oldHeightProperty)
        newDocument->SetProperty(Rml::PropertyId::Height, *oldHeightProperty);
    newDocument->UpdateDocument();

    if (oldVisible)
        newDocument->Show(oldModal, focus);

    RmlDocumentReloadedArgs args;
    args.unloadedDocument_ = document;
    args.loadedDocument_ = newDocument;
    documentReloaded_(this, args);

    document->Close();

    return newDocument;
}

static int MouseButtonUrho3DToRml(MouseButton button)
{
    int rmlButton = -1;
    switch (button)
    {
    case MOUSEB_LEFT:   rmlButton = 0; break;
    case MOUSEB_MIDDLE: rmlButton = 2; break;
    case MOUSEB_RIGHT:  rmlButton = 1; break;
    case MOUSEB_X1:     rmlButton = 3; break;
    case MOUSEB_X2:     rmlButton = 4; break;
    default:                           break;
    }
    return rmlButton;
}

static int ModifiersUrho3DToRml(QualifierFlags modifier)
{
    int rmlModifiers = 0;
    if (modifier & QUAL_ALT)
        rmlModifiers |= Rml::Input::KeyModifier::KM_ALT;
    if (modifier & QUAL_CTRL)
        rmlModifiers |= Rml::Input::KeyModifier::KM_CTRL;
    if (modifier & QUAL_SHIFT)
        rmlModifiers |= Rml::Input::KeyModifier::KM_SHIFT;
    return rmlModifiers;
}

void RegisterRmlUILibrary(Context* context)
{
    context->AddFactoryReflection<RmlUI>();
    RmlUIComponent::RegisterObject(context);
    RmlCanvasComponent::RegisterObject(context);
    RmlSerializableInspector::RegisterObject(context);
}

}
