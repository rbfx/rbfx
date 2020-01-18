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

#pragma once

#include "../Core/Object.h"
#include "../IO/Log.h"

#include <EASTL/bonus/ring_buffer.h>
#include <EASTL/hash_set.h>
#include <EASTL/unique_ptr.h>

namespace Urho3D
{

/// %Console window with log history and command line prompt.
class URHO3D_API Console : public Object
{
    URHO3D_OBJECT(Console, Object);

public:
    /// Construct.
    explicit Console(Context* context);
    /// Destruct.
    ~Console() override;

    /// Show or hide.
    void SetVisible(bool enable);
    /// Toggle visibility.
    void Toggle();
    /// Automatically set console to visible when receiving an error log message.
    void SetAutoVisibleOnError(bool enable) { autoVisibleOnError_ = enable; }
    /// Set the command interpreter.
    void SetCommandInterpreter(const ea::string& interpreter);
    /// Set command history maximum size, 0 disables history.
    void SetNumHistoryRows(unsigned rows);
    /// Set console height.
    void SetConsoleHeight(unsigned height);
    /// Return whether is visible.
    bool IsVisible() const;
    /// Return true when console is set to automatically visible when receiving an error log message.
    bool IsAutoVisibleOnError() const { return autoVisibleOnError_; }
    /// Return the last used command interpreter.
    const ea::string& GetCommandInterpreter() const { return interpreters_[currentInterpreter_]; }
    /// Return history maximum size.
    unsigned GetNumHistoryRows() const { return historyRows_; }
    /// Remove all rows.
    void Clear();
    /// Render contents of the console window. Useful for embedding console into custom UI.
    void RenderContent();
    /// Populate the command line interpreters that could handle the console command.
    void RefreshInterpreters();
    /// Returns a set of loggers that exist in console history.
    StringVector GetLoggers() const;
    /// Set visibility of certain loggers in the console.
    void SetLoggerVisible(const ea::string& loggerName, bool visible);
    /// Get visibility of certain loggers in the console.
    bool GetLoggerVisible(const ea::string& loggerName) const;
    /// Set visibility of certain log levels in the console.
    void SetLevelVisible(LogLevel level, bool visible);
    /// Get visibility of certain log levels in the console.
    bool GetLevelVisible(LogLevel level) const;

private:
    /// Update console size on application window changes.
    void HandleScreenMode(StringHash eventType, VariantMap& eventData);
    /// Handle a log message.
    void HandleLogMessage(StringHash eventType, VariantMap& eventData);
    /// Render system ui.
    void RenderUi(StringHash eventType, VariantMap& eventData);
    /// Scroll console to the end.
    void ScrollToEnd() { scrollToEnd_ = 2; }

    struct LogEntry
    {
        /// Log level.
        LogLevel level_;
        /// Time when event was logged.
        time_t timestamp_;
        /// Name of logger.
        ea::string logger_;
        /// Log message.
        ea::string message_;
    };

    /// Auto visible on error flag.
    bool autoVisibleOnError_ = false;
    /// List of command interpreters.
    ea::vector<ea::string> interpreters_{};
    /// Pointers to c strings in interpreters_ list for efficient UI rendering.
    ea::vector<const char*> interpretersPointers_{};
    /// Last used command interpreter.
    int currentInterpreter_ = 0;
    /// Command history.
    ea::ring_buffer<LogEntry> history_{2000};
    /// Command history maximum rows.
    unsigned historyRows_ = 512;
    /// Is console window open.
    bool isOpen_ = false;
    /// Input box buffer.
    char inputBuffer_[0x1000]{};
    /// Console window size.
    IntVector2 windowSize_{M_MAX_INT, 200};
    /// Number of frames to attempt scrolling to the end. Usually two tries are required to properly complete the action (for some reason).
    int scrollToEnd_ = 0;
    ///Flag indicating that console input should be focused on the next frame.
    bool focusInput_ = false;
    /// Set of loggers to be omitted from rendering.
    ea::hash_set<ea::string> loggersHidden_{};
    /// Log level visibility flags.
    bool levelVisible_[LOG_NONE]{
        false,  // LOG_TRACE
#if URHO3D_DEBUG
        true,   // LOG_DEBUG
#else
        false,  // LOG_DEBUG
#endif
        true,   // LOG_INFO
        true,   // LOG_WARNING
        true    // LOG_ERROR
    };
    /// Current selection in console window. This range denote start and end of selected characters and may span multiple log lines.
    IntVector2 selection_{};
    /// Temporary variable for accumulating selection in order to copy it to clipboard.
    ea::string copyBuffer_{};
    /// When set to true scrollbar of messages panel is at the bottom.
    bool isAtEnd_ = true;
};

}
