// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../RmlUI/RmlSystem.h"

#include "../Core/Context.h"
#include "../Core/Timer.h"
#include "../IO/Log.h"
#include "../Input/Input.h"
#include "../Resource/Localization.h"

#include <SDL_clipboard.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace Detail
{

RmlSystem::RmlSystem(Context* context)
    : context_(context)
{
}

double RmlSystem::GetElapsedTime()
{
    return context_->GetSubsystem<Time>()->GetElapsedTime();
}

int RmlSystem::TranslateString(Rml::String& translated, const Rml::String& input)
{
    Localization* l10n = context_->GetSubsystem<Localization>();
    if (l10n->GetLanguageIndex() > -1)
    {
        translated = l10n->Get(input.c_str()).c_str();
        return 1;
    }
    translated = input;
    return 0;
}

bool RmlSystem::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
    switch (type)
    {
    case Rml::Log::LT_ALWAYS:
    case Rml::Log::LT_ERROR:
    case Rml::Log::LT_ASSERT:
        URHO3D_LOGERROR(message.c_str());
        break;
    case Rml::Log::LT_WARNING:
        URHO3D_LOGWARNING(message.c_str());
        break;
    case Rml::Log::LT_INFO: // There is nothing worthy of "info" status reported by RmlUI.
    case Rml::Log::LT_DEBUG:
        URHO3D_LOGDEBUG(message.c_str());
        break;
    default:
        return false;
    }
    return true;
}

void RmlSystem::SetMouseCursor(const Rml::String& cursor_name)
{
    SystemInterface::SetMouseCursor(cursor_name);
}

void RmlSystem::SetClipboardText(const Rml::String& text)
{
    SDL_SetClipboardText(text.c_str());
}

void RmlSystem::GetClipboardText(Rml::String& text)
{
    text = SDL_GetClipboardText();
}

void RmlSystem::ActivateKeyboard(Rml::Vector2f caret_position, float line_height)
{
    Input* input = context_->GetSubsystem<Input>();
    Time* time = context_->GetSubsystem<Time>();
    input->SetScreenKeyboardVisible(true);
    textInputActivatedFrame_ = time->GetFrameNumber();
}

void RmlSystem::DeactivateKeyboard()
{
    Input* input = context_->GetSubsystem<Input>();
    input->SetScreenKeyboardVisible(false);
}

bool RmlSystem::TextInputActivatedThisFrame() const
{
    Time* time = context_->GetSubsystem<Time>();
    return textInputActivatedFrame_ == time->GetFrameNumber();
}

}   // namespace Detail

}   // namespace Urho3D
