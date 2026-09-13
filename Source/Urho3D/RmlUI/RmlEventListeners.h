// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once


#include "Urho3D/Core/Object.h"

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
