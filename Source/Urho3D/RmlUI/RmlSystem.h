// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once


#include "../Container/Ptr.h"

#include <RmlUi/Core/SystemInterface.h>


namespace Urho3D
{

class Context;

namespace Detail
{

class URHO3D_API RmlSystem : public RefCounted, public Rml::SystemInterface
{
public:
    /// Construct.
    explicit RmlSystem(Context* context);
    /// Get the number of seconds elapsed since the start of the application.
    double GetElapsedTime() override;
    /// Translate the input string into the translated string.
    int TranslateString(Rml::String& translated, const Rml::String& input) override;
    /// Log the specified message.
    /// @param[in] type Type of log message, ERROR, WARNING, etc.
    /// @param[in] message Message to log.
    /// @return True to continue execution, false to break into the debugger.
    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;

    /// Set mouse cursor.
    /// @param[in] cursor_name Cursor name to activate.
    void SetMouseCursor(const Rml::String& cursor_name) override;

    /// Set clipboard text.
    /// @param[in] text Text to apply to clipboard.
    void SetClipboardText(const Rml::String& text) override;

    /// Get clipboard text.
    /// @param[out] text Retrieved text from clipboard.
    void GetClipboardText(Rml::String& text) override;

    /// Activate keyboard (for touchscreen devices)
    void ActivateKeyboard(Rml::Vector2f caret_position, float line_height) override;

    /// Deactivate keyboard (for touchscreen devices)
    void DeactivateKeyboard() override;

    /// Return true if text input activation was requested on this frame.
    bool TextInputActivatedThisFrame() const;

private:
    /// Pointer to Urho3D context.
    WeakPtr<Context> context_;
    /// Frame on which text input was activated. Used for managing input focus between multiple RmlUi contexts.
    unsigned textInputActivatedFrame_ = 0;
};

}   // namespace Detail

}   // namespace Urho3D
