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
#include "../Core/Context.h"
#include "../Core/Timer.h"
#include "../IO/Log.h"
#include "../Input/Input.h"
#include "../Resource/Localization.h"
#include "../RmlUI/RmlSystem.h"

#include <SDL/SDL_clipboard.h>

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
    case Rml::Log::LT_INFO:
        URHO3D_LOGINFO(message.c_str());
        break;
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

void RmlSystem::ActivateKeyboard()
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
