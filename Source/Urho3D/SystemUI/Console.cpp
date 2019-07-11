//
// Copyright (c) 2017 the Urho3D project.
// Copyright (c) 2008-2015 the Urho3D project.
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

#include <EASTL/sort.h>

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Engine/EngineEvents.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/GraphicsEvents.h"
#include "Urho3D/Input/Input.h"
#include "Urho3D/IO/IOEvents.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "SystemUI.h"
#include "SystemUIEvents.h"
#include "Console.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

Console::Console(Context* context) :
    Object(context)
{
    inputBuffer_[0] = 0;

    SetNumHistoryRows(historyRows_);
    VariantMap dummy;
    HandleScreenMode(StringHash::ZERO, dummy);
    RefreshInterpreters();

    SubscribeToEvent(E_SCREENMODE, URHO3D_HANDLER(Console, HandleScreenMode));
    SubscribeToEvent(E_LOGMESSAGE, URHO3D_HANDLER(Console, HandleLogMessage));
}

Console::~Console()
{
    UnsubscribeFromAllEvents();
}

void Console::SetVisible(bool enable)
{
    isOpen_ = enable;
    if (isOpen_)
    {
        focusInput_ = true;
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Console, RenderUi));
    }
    else
    {
        UnsubscribeFromEvent(E_UPDATE);
        ui::SetWindowFocus(nullptr);
    }
}

void Console::Toggle()
{
    SetVisible(!IsVisible());
}

void Console::SetNumHistoryRows(unsigned rows)
{
    historyRows_ = rows;
    if (history_.size() > rows)
        history_.resize(rows);
}

bool Console::IsVisible() const
{
    return isOpen_;
}

void Console::RefreshInterpreters()
{
    interpreters_.clear();
    interpretersPointers_.clear();

    EventReceiverGroup* group = context_->GetEventReceivers(E_CONSOLECOMMAND);
    if (!group || group->receivers_.empty())
        return;

    ea::string currentInterpreterName;
    if (currentInterpreter_ < interpreters_.size())
        currentInterpreterName = interpreters_[currentInterpreter_];

    for (unsigned i = 0; i < group->receivers_.size(); ++i)
    {
        Object* receiver = group->receivers_[i];
        if (receiver)
            interpreters_.push_back(receiver->GetTypeName());
    }
    ea::quick_sort(interpreters_.begin(), interpreters_.end());

    for (const ea::string& interpreter : interpreters_)
        interpretersPointers_.push_back(interpreter.c_str());

    currentInterpreter_ = interpreters_.index_of(currentInterpreterName);
    if (currentInterpreter_ == interpreters_.size())
        currentInterpreter_ = 0;
}

void Console::HandleLogMessage(StringHash eventType, VariantMap& eventData)
{
    using namespace LogMessage;

    auto level = (LogLevel)eventData[P_LEVEL].GetInt();
    time_t timestamp = eventData[P_TIME].GetUInt();
    const ea::string& logger = eventData[P_LOGGER].GetString();
    const ea::string& message = eventData[P_MESSAGE].GetString();

    // The message may be multi-line, so split to rows in that case
    ea::vector<ea::string> rows = message.split('\n');
    for (const auto& row : rows)
        history_.push_back(LogEntry{level, timestamp, logger, row});
    scrollToEnd_ = true;

    if (autoVisibleOnError_ && level == LOG_ERROR && !IsVisible())
        SetVisible(true);
}

void Console::RenderContent()
{
    auto region = ui::GetContentRegionAvail();
    auto showCommandInput = !interpretersPointers_.empty();
    ui::BeginChild("ConsoleScrollArea", ImVec2(region.x, region.y - (showCommandInput ? 30 : 0)), false,
                   ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& row : history_)
    {
        if (!levelVisible_[row.level_])
            continue;

        if (loggersHidden_.contains(row.logger_))
            continue;

        ImColor color;
        const char* debugLevel;
        switch (row.level_)
        {
        case LOG_TRACE:
            debugLevel = "T";
            color = ImColor(135, 135, 135);
            break;
        case LOG_DEBUG:
            debugLevel = "D";
            color = ImColor(200, 200, 200);
            break;
        case LOG_INFO:
            debugLevel = "I";
            color = IM_COL32_WHITE;
            break;
        case LOG_WARNING:
            debugLevel = "W";
            color = ImColor(247, 247, 168);
            break;
        case LOG_ERROR:
            debugLevel = "E";
            color = ImColor(247, 168, 168);
            break;
        default:
            debugLevel = "?";
            color = IM_COL32_WHITE;
            break;
        }

        ui::TextColored(color, "[%s] [%s] [%s] : %s", Time::GetTimeStamp(row.timestamp_, "%H:%M:%S").c_str(),
            debugLevel, row.logger_.c_str(), row.message_.c_str());
    }

    if (scrollToEnd_)
    {
        ui::SetScrollHereY(1.f);
        scrollToEnd_ = false;
    }

    ui::EndChild();

    if (showCommandInput)
    {
        ui::PushItemWidth(110);
        if (ui::Combo("##ConsoleInterpreter", &currentInterpreter_, &interpretersPointers_.front(),
            interpretersPointers_.size()))
        {
        }
        ui::PopItemWidth();
        ui::SameLine();
        ui::PushItemWidth(region.x - 120);
        if (focusInput_)
        {
            ui::SetKeyboardFocusHere();
            focusInput_ = false;
        }
        if (ui::InputText("##ConsoleInput", inputBuffer_, sizeof(inputBuffer_), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            focusInput_ = true;
            ea::string line(inputBuffer_);
            if (line.length() && currentInterpreter_ < interpreters_.size())
            {
                // Store to history, then clear the lineedit
                URHO3D_LOGINFOF("> %s", line.c_str());
                if (history_.size() > historyRows_)
                    history_.pop_front();
                scrollToEnd_ = true;
                inputBuffer_[0] = 0;

                // Send the command as an event for script subsystem
                using namespace ConsoleCommand;

                VariantMap& newEventData = GetEventDataMap();
                newEventData[P_COMMAND] = line;
                newEventData[P_ID] = interpreters_[currentInterpreter_];
                SendEvent(E_CONSOLECOMMAND, newEventData);
            }
        }
        ui::PopItemWidth();
    }
}

void Console::RenderUi(StringHash eventType, VariantMap& eventData)
{
    Graphics* graphics = GetSubsystem<Graphics>();
    ui::SetNextWindowPos(ImVec2(0, 0));
    bool wasOpen = isOpen_;
    ImVec2 size(graphics->GetWidth(), windowSize_.y_);
    ui::SetNextWindowSize(size);

    ui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    if (ui::Begin("Debug Console", &isOpen_, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings))
    {
        RenderContent();
    }
    else if (wasOpen)
    {
        SetVisible(false);
        ui::SetWindowFocus(nullptr);
        SendEvent(E_CONSOLECLOSED);
    }

    windowSize_.y_ = ui::GetWindowHeight();

    ui::End();

    ui::PopStyleVar();
}

void Console::Clear()
{
    history_.clear();
}

void Console::SetCommandInterpreter(const ea::string& interpreter)
{
    RefreshInterpreters();

    auto index = interpreters_.index_of(interpreter);
    if (index == interpreters_.size())
        index = 0;
    currentInterpreter_ = index;
}

void Console::HandleScreenMode(StringHash eventType, VariantMap& eventData)
{
    Graphics* graphics = GetSubsystem<Graphics>();
    windowSize_.x_ = Clamp(windowSize_.x_, 0, graphics->GetWidth());
    windowSize_.y_ = Clamp(windowSize_.y_, 0, graphics->GetHeight());
}

StringVector Console::GetLoggers() const
{
    ea::hash_set<ea::string> loggers;
    StringVector loggersVector;

    for (const auto& row : history_)
        loggers.insert(row.logger_);

    for (const ea::string& logger : loggers)
        loggersVector.emplace_back(logger);

    ea::quick_sort(loggersVector.begin(), loggersVector.end());
    return loggersVector;
}

void Console::SetLoggerVisible(const ea::string& loggerName, bool visible)
{
    scrollToEnd_ = true;
    if (visible)
        loggersHidden_.erase(loggerName);
    else
        loggersHidden_.insert(loggerName);
}

bool Console::GetLoggerVisible(const ea::string& loggerName) const
{
    return !loggersHidden_.contains(loggerName);
}

void Console::SetLevelVisible(LogLevel level, bool visible)
{
    if (level < LOG_TRACE || level >= LOG_NONE)
        return;

    scrollToEnd_ = true;
    levelVisible_[level] = visible;
}

bool Console::GetLevelVisible(LogLevel level) const
{
    if (level < LOG_TRACE || level >= LOG_NONE)
        return false;

    return levelVisible_[level];
}

}
