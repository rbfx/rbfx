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

using EventListenerVector = ea::vector<ea::unique_ptr<Rml::EventListener>>;

URHO3D_API Rml::EventListener* CreateSingleEventListener(ea::string_view value, Rml::Element* element);

class URHO3D_API PipeEventListener : public Rml::EventListener, public NonCopyable
{
public:
    static Rml::EventListener* CreateInstancer(ea::string_view value, Rml::Element* element);
    explicit PipeEventListener(EventListenerVector&& listeners);

    /// Implement Rml::EventListener
    /// @{
    void ProcessEvent(Rml::Event& event) override;
    void OnDetach(Rml::Element* element) override;
    /// @}

private:
    const EventListenerVector listeners_;
};

class URHO3D_API NavigateEventListener : public Rml::EventListener, public NonCopyable
{
public:
    static Rml::EventListener* CreateInstancer(ea::string_view value, Rml::Element* element);
    explicit NavigateEventListener(const ea::string& group);

    /// Implement Rml::EventListener
    /// @{
    void ProcessEvent(Rml::Event& event) override;
    void OnDetach(Rml::Element* element) override;
    /// @}

private:
    const ea::string group_;
};

class URHO3D_API SoundEventListener : public Rml::EventListener, public NonCopyable
{
public:
    static Rml::EventListener* CreateInstancer(ea::string_view value, Rml::Element* element);
    SoundEventListener(const ea::string& soundResource, float volume);

    /// Implement Rml::EventListener
    /// @{
    void ProcessEvent(Rml::Event& event) override;
    void OnDetach(Rml::Element* element) override;
    /// @}

private:
    /// Sound resource.
    const ea::string soundResource_;
    /// Volume of the sound.
    const float volume_{1.0f};

    /// Node that contains SoundSource component.
    SharedPtr<Node> soundNode_;
    /// Sound player.
    WeakPtr<SoundSource> soundPlayer_;
};

class URHO3D_API CustomEventListener : public Rml::EventListener, public NonCopyable
{
public:
    static Rml::EventListener* CreateInstancer(ea::string_view value, Rml::Element* element);
    CustomEventListener(const ea::string& eventType, const VariantMap& eventData);

    /// Implement Rml::EventListener
    /// @{
    void ProcessEvent(Rml::Event& event) override;
    void OnDetach(Rml::Element* element) override;
    /// @}

private:
    StringHash eventType_;
    VariantMap eventData_;
};

}

};  // namespace Urho3D
