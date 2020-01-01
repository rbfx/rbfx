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

static const char* debugLevelAbbreviations[] = {"T", "D", "I", "W", "E", 0};

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

void Console::SetConsoleHeight(unsigned height)
{
    windowSize_.y_ = static_cast<int>(height);
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
    {
        ea::string formattedMessage = Format("[{}] [{}] [{}] : {}", Time::GetTimeStamp(timestamp, "%H:%M:%S"),
            debugLevelAbbreviations[level], logger, row);
        history_.push_back(LogEntry{level, timestamp, logger, formattedMessage});
    }
    if (isAtEnd_)
        ScrollToEnd();

    if (autoVisibleOnError_ && level == LOG_ERROR && !IsVisible())
        SetVisible(true);
}

void Console::RenderContent()
{
    ImVec2 region = ui::GetContentRegionAvail();
    bool showCommandInput = !interpretersPointers_.empty();
    bool copying = (ui::IsKeyDown(SDL_SCANCODE_LCTRL) || ui::IsKeyDown(SDL_SCANCODE_RCTRL)) && ui::IsKeyPressed(SDL_SCANCODE_C);
    if (ui::BeginChild("ConsoleScrollArea", ImVec2(region.x, region.y - (showCommandInput ? 30 : 0)), false,
        ImGuiWindowFlags_HorizontalScrollbar))
    {
        const ImGuiIO& io = ui::GetIO();
        int textStart = 0;
        for (const auto& row : history_)
        {
            int textEnd = textStart + (int) row.message_.length();

            if (!levelVisible_[row.level_])
            {
                textStart = textEnd;
                continue;
            }

            if (loggersHidden_.contains(row.logger_))
            {
                textStart = textEnd;
                continue;
            }

            const char* rowStart = row.message_.c_str();
            const char* rowEnd = row.message_.c_str() + row.message_.length();

            ImVec2 rowSize = ui::CalcTextSize(rowStart, rowEnd);
            ImVec2 rowStartPos = ui::GetCursorScreenPos();
            ImRect rowRect{};
            rowRect.Min = rowStartPos;
            rowRect.Max = rowStartPos + rowSize;
            rowRect.Max.y += ui::GetStyle().ItemSpacing.y;   // So clicking between rows still does a selection
            bool isRowHovered = rowRect.Contains(ui::GetMousePos()) && ui::IsWindowHovered();

            // Perform selection
            if (ui::IsMouseDown(MOUSEB_LEFT))
            {
                if (isRowHovered)
                {
                    ImVec2 pos = rowStartPos;
                    const char* c = rowStart;
                    while (c < rowEnd)
                    {
                        int numBytes = ImTextCountUtf8BytesFromChar(c, rowEnd);
                        ImVec2 charSize = ui::CalcTextSize(c, c + numBytes);
                        if (ImRect(pos, pos + charSize).Contains(ui::GetMousePos()))
                            break;
                        pos.x += charSize.x;
                        c += numBytes;
                    }
                    selection_.y_ = textStart + static_cast<int>(c - rowStart);
                    if (ui::IsMouseClicked(MOUSEB_LEFT))
                        selection_.x_ = selection_.y_;
                }
            }

            // Render selection.
            const char* selectedStart = Clamp(rowStart + (Min(selection_.x_, selection_.y_) - textStart), rowStart, rowEnd);
            const char* selectedEnd = Clamp(rowEnd + (Max(selection_.x_, selection_.y_) - textEnd), rowStart, rowEnd);
            if (selectedStart < selectedEnd)
            {
                if (copying && ui::IsWindowFocused())
                {
                    copyBuffer_.append(selectedStart, selectedEnd - selectedStart);
                    if (selectedEnd == rowEnd && Max(selection_.x_, selection_.y_) > textEnd)
                    {
                        // Append new line if this line is selected to the end and selection on the next line exists.
#if _WIN32
                        const char* eol = "\r\n";
#else
                        const char* eol = "\n";
#endif
                        copyBuffer_.append(eol);
                    }
                }
                ImVec2 sizeUnselected = ui::CalcTextSize(rowStart, selectedStart);
                ImVec2 sizeSelected = ui::CalcTextSize(selectedStart, selectedEnd);
                ImRect selection{};
                selection.Min = rowStartPos;
                selection.Min.x += sizeUnselected.x;
                selection.Max = selection.Min + sizeSelected;
                selection.Max.y += ui::GetStyle().ItemSpacing.y;   // Fill in spaces between lines
                ui::GetWindowDrawList()->AddRectFilled(selection.Min, selection.Max, ui::GetColorU32(ImGuiCol_TextSelectedBg));
            }
            ui::PushStyleColor(ImGuiCol_Text, ToImGui(LOG_LEVEL_COLORS[row.level_]));
            ui::TextUnformatted(rowStart, rowEnd);

            // Find URIs, render underlines and send click events
            if (isRowHovered)
            {
                for (unsigned i = 0; i < row.message_.length();)
                {
                    unsigned uriPos = row.message_.find("://", i);
                    if (uriPos == ea::string::npos)
                        break;

                    char uriTerminator = ' ';
                    const char* uriStart = rowStart + uriPos - 1;
                    const char* uriEnd = rowStart + uriPos + 2;
                    while (uriStart > rowStart && isalnum(*uriStart))
                        --uriStart;

                    if (*uriStart == '\'' || *uriStart == '"')
                        uriTerminator = *uriStart;
                    uriStart++;

                    while (uriEnd < rowEnd && *uriEnd != uriTerminator)
                        ++uriEnd;

                    ImRect uriRect{};
                    uriRect.Min = rowStartPos;
                    uriRect.Min.x += ui::CalcTextSize(rowStart, uriStart).x;
                    uriRect.Max = uriRect.Min + ui::CalcTextSize(uriStart, uriEnd);

                    if (uriRect.Contains(ui::GetMousePos()))
                    {
                        ui::GetWindowDrawList()->AddLine({uriRect.Min.x, uriRect.Max.y}, uriRect.Max, ui::GetColorU32(ImGuiCol_Text));
                        ui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        if (ui::IsMouseClicked(MOUSEB_LEFT) || ui::IsMouseClicked(MOUSEB_RIGHT))
                        {
                            using namespace ConsoleUriClick;
                            VariantMap& newEventData = GetEventDataMap();
                            newEventData[P_PROTOCOL] = ea::string(uriStart, rowStart + uriPos);
                            newEventData[P_ADDRESS] = ea::string(rowStart + uriPos + 3, uriEnd - (rowStart + uriPos + 3));
                            SendEvent(E_CONSOLEURICLICK, newEventData);
                        }
                    }

                    i = uriEnd - rowStart;
                }
            }
            ui::PopStyleColor();
            textStart = textEnd;
        }

        if (scrollToEnd_ > 0)
        {
            ui::SetScrollHereY(1.f);
            scrollToEnd_--;
        }

        isAtEnd_ = ui::GetScrollY() >= ui::GetScrollMaxY();

        if (!copyBuffer_.empty())
        {
            ui::SetClipboardText(copyBuffer_.c_str());
            copyBuffer_.clear();
        }

        ui::SetCursorPosY(ui::GetCursorPosY() + 1);
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
                ScrollToEnd();
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
    if (isAtEnd_)
        ScrollToEnd();
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

    if (isAtEnd_)
        ScrollToEnd();
    levelVisible_[level] = visible;
}

bool Console::GetLevelVisible(LogLevel level) const
{
    if (level < LOG_TRACE || level >= LOG_NONE)
        return false;

    return levelVisible_[level];
}

}
