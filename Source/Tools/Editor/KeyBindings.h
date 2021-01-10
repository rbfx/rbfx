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
#pragma once


#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Signal.h>
#include <Urho3D/Input/InputConstants.h>


namespace Urho3D
{

/// Key-bindable action type.
enum ActionType : unsigned
{
    /// File > Save Project.
    SaveProject,
    /// File > Open or Create Project.
    OpenProject,
    /// File > Exit.
    Exit,
    /// Undo action requested.
    Undo,
    /// Redo action requested.
    Redo,
    /// The End.
    MaxCount
};

/// Key and qualifier combination.
struct KeyCombination
{
    /// Key. KEY_UNKNOWN means action is unbound.
    Key key_ = KEY_UNKNOWN;
    /// Qualifier mask.
    QualifierFlags qualifiers_ = Qualifier::QUAL_NONE;
};

/// Action that may be bound to a key combination.
struct KeyBoundAction : KeyCombination
{
    /// Construct empty action.
    KeyBoundAction() = default;
    /// Construct predefined action.
    KeyBoundAction(ea::string_view title, Key key, QualifierFlags qualifiers)
    {
        title_ = title;
        key_ = key;
        qualifiers_ = qualifiers;
    }

    /// Description of action.
    ea::string title_{};
    /// Textual representation of key combination
    ea::string binding_{};
    /// Set to true when key combination is held down.
    bool isDown_ = false;
    /// Sent when key combination is pressed.
    Signal<void()> onPressed_{};
};

/// Key bindings manager.
class KeyBindings : public Object
{
    URHO3D_OBJECT(KeyBindings, Object);
public:
    /// Construct.
    explicit KeyBindings(Context* context);
    /// Serialize key bindings state.
    bool Serialize(Archive& archive) override;
    /// Renders key bindings tab in settings window.
    void RenderSettingsUI();
    /// Bind handler to action.
    template<typename Receiver>
    void Bind(ActionType actionType, Receiver* receiver, void(Receiver::*handler)()) { actions_[actionType].onPressed_.Subscribe(receiver, handler); }
    /// Bind handler to action.
    template<typename Receiver>
    void Bind(ActionType actionType, Receiver* receiver, bool(Receiver::*handler)()) { actions_[actionType].onPressed_.Subscribe(receiver, handler); }
    /// Get a string representing key combination that is bound to specified action.
    const char* GetKeyCombination(ActionType actionType);

protected:
    /// Handle object initialization.
    void OnApplicationStarted(StringHash, VariantMap&);
    /// Handle input.
    void OnInputEnd(StringHash, VariantMap&);
    /// Returns a name of specified action.
    ea::string KeysToString(QualifierFlags qualifiers, Key key);
    /// Returns mask of currently pressed qualifiers.
    QualifierFlags GetCurrentQualifiers() const;
    /// Sort actions by size of modifier flags.
    void SortActions();

    /// Suppress firing any of key binding actions on the next frame.
    bool ignoreKeyPresses_ = false;
    /// Configured key bindings.
    KeyBoundAction actions_[ActionType::MaxCount]{};
    /// Default key bindings.
    KeyCombination defaults_[ActionType::MaxCount]{};
    /// Order which will be used for checking actions.
    int actionOrder_[ActionType::MaxCount]{};
};

}
