//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "Sample.h"

namespace Urho3D
{

class BufferedSoundStream;
class Button;
class CheckBox;
class Microphone;
class Scene;
class Slider;
class SoundSource;

}

/// Sound effects example
/// This sample demonstrates:
///     - Playing sound effects and music
///     - Controlling sound and music master volume
class SoundEffects : public Sample
{
    URHO3D_OBJECT(SoundEffects, Sample);

public:
    /// Construct.
    explicit SoundEffects(Context* context);

    /// Setup before engine initialization. Modifies the engine parameters.
    void Setup() override;
    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:

    SoundSource* musicSource_;

    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    ea::string GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button2']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "</patch>";
    }

private:
    /// Create the UI and subscribes to UI events.
    void CreateUI();
    /// Create a button at position with specified text in it.
    Button* CreateButton(int x, int y, int xSize, int ySize, const ea::string& text);
    /// Create a checkbox at the given position with specified text.
    CheckBox* CreateCheckbox(int x, int y, const ea::string& text);
    /// Create a horizontal slider with specified text above it.
    Slider* CreateSlider(int x, int y, int xSize, int ySize, const ea::string& text);
    /// Handle a sound effect button click.
    void HandlePlaySound(StringHash eventType, VariantMap& eventData);
    /// Handle "play music" button click.
    void HandlePlayMusic(StringHash eventType, VariantMap& eventData);
    /// Handle "stop music" button click.
    void HandleStopMusic(StringHash eventType, VariantMap& eventData);
    /// Handle sound effects volume slider change.
    void HandleSoundVolume(StringHash eventType, VariantMap& eventData);
    /// Handle sound effect L/R pan slider change.
    void HandleSoundPan(StringHash eventType, VariantMap& eventData);
    /// Handle sound effect F/B reach slider change.
    void HandleSoundReach(StringHash eventType, VariantMap& eventData);
    /// Handle sound effect F/B reach slider change.
    void HandleLFE(StringHash eventType, VariantMap& eventData);
    /// Handle music volume slider change.
    void HandleMusicVolume(StringHash eventType, VariantMap& eventData);
    /// Handle starting mic recording.
    void HandleStartMicRecord(StringHash eventType, VariantMap& eventData);
    /// Handle stopping mic recording.
    void HandleStopMicRecord(StringHash eventType, VariantMap& eventData);

    /// Explicit pan value.
    float pan_{ 0.0f };
    /// Explicit reach value.
    float reach_{ 0.0f };
    /// Output to subwoofer.
    bool lfe_{ false };

    SharedPtr<Microphone> activeMic_;
    SharedPtr<BufferedSoundStream> micStream_;
};


