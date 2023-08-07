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

#include "../RmlUI/RmlNavigable.h"

#include "../IO/Log.h"
#include "../RmlUI/RmlNavigationManager.h"
#include "../RmlUI/RmlUIComponent.h"

#include <RmlUi/Source/Core/EventDispatcher.h>
#include <RmlUi/Core/StyleSheetSpecification.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Element.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

const Rml::String elementTag{"navigable"};

// Copy behaviour from ElementLabel.
const Rml::StringList matching_tags = {"button", "input", "textarea", "select"};

// Get the first descending element whose tag name matches one of tags.
static Rml::Element* TagMatchRecursive(const Rml::StringList& tags, Rml::Element* element)
{
    const int num_children = element->GetNumChildren();

    for (int i = 0; i < num_children; i++)
    {
        Rml::Element* child = element->GetChild(i);

        for (const Rml::String& tag : tags)
        {
            if (child->GetTagName() == tag)
                return child;
        }

        Rml::Element* matching_element = TagMatchRecursive(tags, child);
        if (matching_element)
            return matching_element;
    }

    return nullptr;
}

bool IsEventNeeded(bool value, NavigableEventMode eventMode)
{
    switch (eventMode)
    {
    case NavigableEventMode::Always:
        return true;
    case NavigableEventMode::OnActivation:
        return value;
    case NavigableEventMode::Never:
    default:
        return false;
    }
}

}

void RmlNavigable::Register()
{
    Rml::StyleSheetSpecification::RegisterProperty("nav-mode-mouse", "trigger", true).AddParser("keyword", "trigger, toggle, sticky");
    Rml::StyleSheetSpecification::RegisterProperty("nav-mode-keyboard", "trigger", true).AddParser("keyword", "trigger, toggle, sticky");
    Rml::StyleSheetSpecification::RegisterProperty("nav-mode-joystick", "trigger", true).AddParser("keyword", "trigger, toggle, sticky");
    Rml::StyleSheetSpecification::RegisterShorthand("nav-mode", "nav-mode-mouse, nav-mode-keyboard, nav-mode-joystick", Rml::ShorthandType::Replicate);

    static Detail::NavigableInstancer navigableInstancer;
    Rml::Factory::RegisterElementInstancer(elementTag, &navigableInstancer);

    Rml::RegisterEventType("navigated", true, false);
    Rml::RegisterEventType("pressed", true, false);
    Rml::RegisterEventType("depressed", true, false);
}

RmlNavigableEventListener::~RmlNavigableEventListener()
{
}

void RmlNavigableEventListener::ProcessEvent(Rml::Event& event)
{
    if (enabled_)
        event.StopPropagation();
}

RmlNavigable::RmlNavigable(const Rml::String& tag, const Rml::String& group)
    : Rml::Element(tag)
    , group_(group)
    , clickBlocker_(Rml::MakeUnique<RmlNavigableEventListener>())
{
    GetEventDispatcher()->AttachEvent(Rml::EventId::Click, clickBlocker_.get(), true);
}

RmlNavigable::~RmlNavigable()
{
    GetEventDispatcher()->DetachEvent(Rml::EventId::Click, clickBlocker_.get(), true);
}

void RmlNavigable::OnResize()
{
    MarkCachesDirty();
    Rml::Element::OnResize();
}

void RmlNavigable::OnLayout()
{
    MarkCachesDirty();
    Rml::Element::OnLayout();
}

void RmlNavigable::OnDpRatioChange()
{
    MarkCachesDirty();
    Rml::Element::OnDpRatioChange();
}

void RmlNavigable::OnStyleSheetChange()
{
    MarkCachesDirty();
    Rml::Element::OnStyleSheetChange();
}

void RmlNavigable::OnAttributeChange(const Rml::ElementAttributes& changedAttributes)
{
    MarkCachesDirty();
    Rml::Element::OnAttributeChange(changedAttributes);
}

void RmlNavigable::OnPropertyChange(const Rml::PropertyIdSet& changedProperties)
{
    MarkCachesDirty();
    Rml::Element::OnPropertyChange(changedProperties);
}

void RmlNavigable::OnPseudoClassChange(const Rml::String& pseudoClass, bool activate)
{
    MarkCachesDirty();
    Rml::Element::OnPseudoClassChange(pseudoClass, activate);
}

void RmlNavigable::OnUpdate()
{
    if (!EnsureInitialized())
        return;

    Refresh();
    UpdateDisabled();
    UpdatePosition();
    firstUpdate_ = false;
}

bool RmlNavigable::EnsureInitialized()
{
    if (owner_)
        return true;

    if (Rml::ElementDocument* ownerDocument = GetOwnerDocument())
    {
        if (auto ownerComponent = RmlUIComponent::FromDocument(ownerDocument))
        {
            RmlNavigationManager& manager = ownerComponent->GetNavigationManager();
            manager.AddNavigable(this);
            owner_ = ownerComponent;
            return true;
        }
    }
    return false;
}

void RmlNavigable::UpdateCaches()
{
    cachesDirty_ = false;

    UpdateHovered();
    UpdateStyle();
    UpdateVisible();
}

void RmlNavigable::Refresh()
{
    if (cachesDirty_)
        UpdateCaches();
}

void RmlNavigable::UpdatePosition()
{
    const auto position = GetAbsoluteOffset(Rml::BoxArea::Border) + GetBox().GetSize(Rml::BoxArea::Border) * 0.5f;
    position_ = {position.x, position.y};
}

void RmlNavigable::UpdateVisible()
{
    cache_.visible_ = true;
    for (Rml::Element* ptr = this; ptr; ptr = ptr->GetParentNode())
    {
        if (!ptr->IsVisible())
        {
            cache_.visible_ = false;
            return;
        }
    }
}

void RmlNavigable::UpdateDisabled()
{
    static const Rml::String disabledTag = "disabled";

    const bool wasDisabled = disabled_;
    disabled_ = IsClassSet(disabledTag);

    if (wasDisabled != disabled_ || firstUpdate_)
    {
        ForEachChild(this, [&](Rml::Element* innerElement)
        {
            innerElement->SetClass(disabledTag, disabled_);
            if (disabled_)
                innerElement->SetAttribute(disabledTag, true);
            else
                innerElement->RemoveAttribute(disabledTag);
        });
    }
}

void RmlNavigable::UpdateHovered()
{
    cache_.hovered_ = IsPseudoClassSet("hover");
}

void RmlNavigable::UpdateStyle()
{
    cache_.mousePressMode_ = static_cast<NavigablePressMode>(GetProperty<int>("nav-mode-mouse"));
    cache_.keyboardPressMode_ = static_cast<NavigablePressMode>(GetProperty<int>("nav-mode-keyboard"));
    cache_.joystickPressMode_ = static_cast<NavigablePressMode>(GetProperty<int>("nav-mode-joystick"));
}

void RmlNavigable::Release()
{
    if (owner_)
        owner_->GetNavigationManager().RemoveNavigable(this);
    Rml::Element::Release();
}

void RmlNavigable::SetNavigated(bool navigated, NavigableEventMode eventMode)
{
    if (navigated)
    {
        this->Focus();

        auto element = TagMatchRecursive(matching_tags, this);
        targetElement_ = (element) ? element->GetObserverPtr() : nullptr;
    }

    if (targetElement_)
    {
        targetElement_->SetPseudoClass("navigated", navigated);
    }

    if (IsEventNeeded(navigated, eventMode))
    {
        Rml::Dictionary parameters;
        DispatchEvent(navigated ? "navigated" : "abandoned", parameters);
    }

    if (!navigated)
    {
        targetElement_ = nullptr;
    }
}

void RmlNavigable::SetPressed(bool pressed, NavigableInputSource inputSource, NavigableEventMode eventMode)
{
    if (auto target = targetElement_.get())
    {
        target->SetPseudoClass("pressed", pressed);
        if (!pressed)
        {
            bool isFocused = target == this->GetOwnerDocument()->GetFocusLeafNode();

            if (!isFocused || !RmlNavigationManager::DoesElementHandleDirectionKeys(target))
            {
                clickBlocker_->enabled_ = false;
                target->Focus();
                target->Click();
                clickBlocker_->enabled_ = true;
            }
            else
            {
                this->Focus();
            }
        }
    }

    if (IsEventNeeded(pressed, eventMode))
    {
        Rml::Dictionary parameters;
        parameters["is_mouse"] = inputSource == NavigableInputSource::Mouse;
        parameters["is_keyboard"] = inputSource == NavigableInputSource::Keyboard;
        parameters["is_joystick"] = inputSource == NavigableInputSource::Joystick;
        DispatchEvent(pressed ? "pressed" : "depressed", parameters);
    }
}

template <class T>
void RmlNavigable::ForEach(Rml::Element* element, const T& func)
{
    func(element);
    for (int i = 0; i < element->GetNumChildren(); ++i)
        ForEach(element->GetChild(i), func);
}

template <class T>
void RmlNavigable::ForEachChild(Rml::Element* element, const T& func)
{
    for (int i = 0; i < element->GetNumChildren(); ++i)
        ForEach(element->GetChild(i), func);
}

namespace Detail
{

Rml::ElementPtr NavigableInstancer::InstanceElement(Rml::Element* parent, const Rml::String& tag, const Rml::XMLAttributes& attributes)
{
    const auto iter = attributes.find("group");
    if (iter == attributes.end())
    {
        URHO3D_LOGWARNING("RmlNavigable element must have 'group' specified");
        return nullptr;
    }

    const auto group = iter->second.Get<Rml::String>();
    auto navigable = new RmlNavigable(tag, group);
    return Rml::ElementPtr(navigable->AsElement());
}

void NavigableInstancer::ReleaseElement(Rml::Element* element)
{
    delete element;
}

}

}
