//
// Copyright (c) 2008-2018 the Urho3D project.
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

#include "../Container/List.h"
#include "../Core/Mutex.h"
#include "../Core/Object.h"
#include "../Core/StringUtils.h"

namespace Urho3D
{

#if WIN32
static const char* NULL_DEVICE = "NUL";
#else
static const char* NULL_DEVICE = "/dev/null";
#endif

enum LogLevel
{
    /// Fictional message level to indicate a stored raw message.
    LOG_RAW = -1,
    /// Trace message level.
    LOG_TRACE = 0,
    /// Debug message level. By default only shown in debug mode.
    LOG_DEBUG = 1,
    /// Informative message level.
    LOG_INFO = 2,
    /// Warning message level.
    LOG_WARNING = 3,
    /// Error message level.
    LOG_ERROR = 4,
    /// Disable all log messages.
    LOG_NONE = 5,
    /// Number of log levels
    MAX_LOGLEVELS,
};

class File;

/// Stored log message from another thread.
struct StoredLogMessage
{
    /// Construct undefined.
    StoredLogMessage() = default;

    /// Construct with parameters.
    StoredLogMessage(const String& message, LogLevel level, bool error) :
        message_(message),
        level_(level),
        error_(error)
    {
    }

    /// Message text.
    String message_;
    /// Message level. -1 for raw messages.
    LogLevel level_{};
    /// Error flag for raw messages.
    bool error_{};
};

/// Logging subsystem.
class URHO3D_API Log : public Object
{
    URHO3D_OBJECT(Log, Object);

public:
    /// Construct.
    explicit Log(Context* context);
    /// Destruct. Close the log file if open.
    ~Log() override;

    /// Open the log file.
    void Open(const String& fileName);
    /// Close the log file.
    void Close();
    /// Set logging level.
    void SetLevel(LogLevel level);
    /// Set whether to timestamp log messages.
    void SetTimeStampFormat(const String& format) { timeStampFormat_ = format; }
    /// Set quiet mode ie. only print error entries to standard error stream (which is normally redirected to console also). Output to log file is not affected by this mode.
    void SetQuiet(bool quiet);

    /// Return logging level.
    LogLevel GetLevel() const { return level_; }

    /// Return whether log messages are timestamped.
    const String& GetTimeStampFormat() const { return timeStampFormat_; }

    /// Return last log message.
    String GetLastMessage() const { return lastMessage_; }

    /// Return whether log is in quiet mode (only errors printed to standard error stream).
    bool IsQuiet() const { return quiet_; }

    /// Write to the log. If logging level is higher than the level of the message, the message is ignored.
    static void Write(LogLevel level, const String& message);
    /// Write raw output to the log.
    static void WriteRaw(const String& message, bool error = false);
    /// Return instance of opened log file.
    const File* GetLogFile() const { return logFile_; }

private:
    /// Handle end of frame. Process the threaded log messages.
    void HandleEndFrame(StringHash eventType, VariantMap& eventData);

    /// Mutex for threaded operation.
    Mutex logMutex_;
    /// Log messages from other threads.
    List<StoredLogMessage> threadMessages_;
    /// Log file.
    SharedPtr<File> logFile_;
    /// Last log message.
    String lastMessage_;
    /// Format of timestamp that will be prepended to log messages.
    String timeStampFormat_;
    /// Logging level.
    LogLevel level_;
    /// In write flag to prevent recursion.
    bool inWrite_;
    /// Quiet mode flag.
    bool quiet_;
};

#ifdef URHO3D_LOGGING
#define URHO3D_LOGTRACE(message, ...) Urho3D::Log::Write(Urho3D::LOG_TRACE, Urho3D::Format(message, ##__VA_ARGS__))
#define URHO3D_LOGDEBUG(message, ...) Urho3D::Log::Write(Urho3D::LOG_DEBUG, Urho3D::Format(message, ##__VA_ARGS__))
#define URHO3D_LOGINFO(message, ...) Urho3D::Log::Write(Urho3D::LOG_INFO, Urho3D::Format(message, ##__VA_ARGS__))
#define URHO3D_LOGWARNING(message, ...) Urho3D::Log::Write(Urho3D::LOG_WARNING, Urho3D::Format(message, ##__VA_ARGS__))
#define URHO3D_LOGERROR(message, ...) Urho3D::Log::Write(Urho3D::LOG_ERROR, Urho3D::Format(message, ##__VA_ARGS__))
#define URHO3D_LOGRAW(message, ...) Urho3D::Log::WriteRaw(Urho3D::Format(message, ##__VA_ARGS__))
#define URHO3D_LOGTRACEF(format, ...) Urho3D::Log::Write(Urho3D::LOG_TRACE, Urho3D::ToString(format, ##__VA_ARGS__))
#define URHO3D_LOGDEBUGF(format, ...) Urho3D::Log::Write(Urho3D::LOG_DEBUG, Urho3D::ToString(format, ##__VA_ARGS__))
#define URHO3D_LOGINFOF(format, ...) Urho3D::Log::Write(Urho3D::LOG_INFO, Urho3D::ToString(format, ##__VA_ARGS__))
#define URHO3D_LOGWARNINGF(format, ...) Urho3D::Log::Write(Urho3D::LOG_WARNING, Urho3D::ToString(format, ##__VA_ARGS__))
#define URHO3D_LOGERRORF(format, ...) Urho3D::Log::Write(Urho3D::LOG_ERROR, Urho3D::ToString(format, ##__VA_ARGS__))
#define URHO3D_LOGRAWF(format, ...) Urho3D::Log::WriteRaw(Urho3D::ToString(format, ##__VA_ARGS__))
#else
#define URHO3D_LOGTRACE(...) ((void)0)
#define URHO3D_LOGDEBUG(...) ((void)0)
#define URHO3D_LOGINFO(...) ((void)0)
#define URHO3D_LOGWARNING(...) ((void)0)
#define URHO3D_LOGERROR(...) ((void)0)
#define URHO3D_LOGRAW(...) ((void)0)
#define URHO3D_LOGTRACEF(...) ((void)0)
#define URHO3D_LOGDEBUGF(...) ((void)0)
#define URHO3D_LOGINFOF(...) ((void)0)
#define URHO3D_LOGWARNINGF(...) ((void)0)
#define URHO3D_LOGERRORF(...) ((void)0)
#define URHO3D_LOGRAWF(...) ((void)0)
#endif

}
