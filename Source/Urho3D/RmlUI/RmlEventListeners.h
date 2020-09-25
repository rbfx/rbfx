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


#include "../Core/Object.h"

#include <RmlUi/Core/EventListener.h>


namespace Urho3D
{

class Node;
class Sound;
class SoundSource;
class RmlUI;

namespace Detail
{

class URHO3D_API SoundEventListener : public Rml::EventListener
{
public:
    /// Create event instancer if value is acceptable, or return null otherwise.
    static SoundEventListener* CreateInstancer(const ea::string& value, Rml::Element* element);
    /// Initialize from inline event value.
    explicit SoundEventListener(const ea::string& soundResource);
    /// Process event.
    void ProcessEvent(Rml::Event& event) override;
    /// Free this event listener.
    void OnDetach(Rml::Element* element) override;

private:
    /// Node that contains SoundSource component.
    SharedPtr<Node> soundNode_;
    /// Sound player.
    WeakPtr<SoundSource> soundPlayer_;
    /// Sound resource.
    ea::string soundResource_;
};

class URHO3D_API CustomEventListener : public Rml::EventListener
{
public:
    /// Create event instancer if value is acceptable, or return null otherwise.
    static CustomEventListener* CreateInstancer(const ea::string& value, Rml::Element* element);
    /// Initialize from inline event value.
    CustomEventListener(const JSONValue& value);
    /// Process event.
    void ProcessEvent(Rml::Event& event) override;
    /// Free this event listener.
    void OnDetach(Rml::Element* element) override;

private:
    /// Event.
    StringHash event_;
    /// Event arguments.
    VariantMap eventArgs_;
};

}

};  // namespace Urho3D
