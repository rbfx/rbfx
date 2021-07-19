//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include <EASTL/list.h>

#include "../Core/Macros.h"
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

static Color LOG_LEVEL_COLORS[] = {
    Color(0.529f, 0.529f, 0.529f),
    Color(0.784f, 0.784f, 0.784f),
    Color(1.f, 1.f, 1.f),
    Color(0.969f, 0.969f, 0.659f),
    Color(0.969f, 0.659f, 0.659f),
    Color(1.f, 1.f, 1.f),
};
static_assert(URHO3D_ARRAYSIZE(LOG_LEVEL_COLORS) == MAX_LOGLEVELS, "Inconsistent number of log levels and log colors.");

static const char* logLevelNames[] =
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    nullptr
};

class File;

/// Stored log message from another thread.
struct StoredLogMessage
{
    /// Construct undefined.
    StoredLogMessage() = default;

    /// Construct with parameters.
    StoredLogMessage(LogLevel level, time_t timestamp, const ea::string& logger, const ea::string& message) :
        level_(level),
        timestamp_(timestamp),
        logger_(logger),
        message_(message)
    {
    }

    /// Message level. -1 for raw messages.
    LogLevel level_{};
    /// Timestamp when message was logged.
    time_t timestamp_{};
    /// Message text.
    ea::string logger_{};
    /// Message text.
    ea::string message_{};
};

class LogImpl;
class Log;

/// Forwards a message to underlying logger. Use %Log::GetLogger to obtain instance of this class.
class URHO3D_API Logger
{
protected:
    explicit Logger(void* logger);

    friend class Log;

public:
    Logger() = default;
    Logger(const Logger& other) = default;
    ///
    template<typename... Args> void Trace(const char* format, Args... args) const   { Write(LOG_TRACE, format, args...); }
    template<typename... Args> void Debug(const char* format, Args... args) const   { Write(LOG_DEBUG, format, args...); }
    template<typename... Args> void Info(const char* format, Args... args) const    { Write(LOG_INFO, format, args...); }
    template<typename... Args> void Warning(const char* format, Args... args) const { Write(LOG_WARNING, format, args...); }
    template<typename... Args> void Error(const char* format, Args... args) const   { Write(LOG_ERROR, format, args...); }
    template<typename... Args> void Write(LogLevel level, const char* format, Args... args) const { Write(level, Format(format, args...)); }

    template<typename... Args> void Trace(const ea::string& message) const   { Write(LOG_TRACE, message.c_str()); }
    template<typename... Args> void Debug(const ea::string& message) const   { Write(LOG_DEBUG, message.c_str()); }
    template<typename... Args> void Info(const ea::string& message) const    { Write(LOG_INFO, message.c_str()); }
    template<typename... Args> void Warning(const ea::string& message) const { Write(LOG_WARNING, message.c_str()); }
    template<typename... Args> void Error(const ea::string& message) const   { Write(LOG_ERROR, message.c_str()); }

    void Write(LogLevel level, const ea::string& message) const;

protected:
    /// Instance of spdlog logger.
    void* logger_ = nullptr;
};

/// Logging subsystem.
class URHO3D_API Log : public Object
{
    URHO3D_OBJECT(Log, Object);

    void SendMessageEvent(LogLevel level, time_t timestamp, const ea::string& logger, const ea::string& message);

public:
    /// Construct.
    explicit Log(Context* context);
    /// Destruct. Close the log file if open.
    ~Log() override;

    /// Open the log file.
    void Open(const ea::string& fileName);
    /// Close the log file.
    void Close();
    /// Set logging level.
    /// @property
    void SetLevel(LogLevel level);
    /// Set whether to timestamp log messages.
    /// @property
    void SetLogFormat(const ea::string& format);
    /// Set quiet mode ie. only print error entries to standard error stream (which is normally redirected to console also). Output to log file is not affected by this mode.
    /// @property
    void SetQuiet(bool quiet);

    /// Return logging level.
    /// @property
    LogLevel GetLevel() const { return level_; }

    /// Return whether log is in quiet mode (only errors printed to standard error stream).
    /// @property
    bool IsQuiet() const { return quiet_; }

    /// Returns a logger with specified name.
    static Logger GetLogger(const ea::string& name);
    /// Returns default logger.
    static Logger GetLogger();

    ///
    void PumpThreadMessages();

private:
    /// Handle end of frame. Process the threaded log messages.
    void HandleEndFrame(StringHash eventType, VariantMap& eventData) { PumpThreadMessages(); }
    /// Return new or existing logger for this Log instance.
    Logger GetOrCreateLogger(const ea::string& name);

    /// Implementation hiding spdlog class types from public headers.
    SharedPtr<LogImpl> impl_;
    /// Log format pattern.
    ea::string formatPattern_{};
    /// Mutex for threaded operation.
    Mutex logMutex_{};
    /// Log messages from other threads.
    ea::list<StoredLogMessage> threadMessages_{};
    /// Logging level.
#ifdef _DEBUG
    LogLevel level_ = LOG_DEBUG;
#else
    LogLevel level_ = LOG_INFO;
#endif
    /// In write flag to prevent recursion.
    bool inWrite_ = false;
    /// Quiet mode flag.
    bool quiet_ = false;
    /// Default logger used to log messages from Urho3D library.
    Logger defaultLogger_;
};

#ifdef URHO3D_LOGGING
#define URHO3D_LOGTRACE(message, ...) Urho3D::Log::GetLogger().Trace(message, ##__VA_ARGS__)
#define URHO3D_LOGDEBUG(message, ...) Urho3D::Log::GetLogger().Debug(message, ##__VA_ARGS__)
#define URHO3D_LOGINFO(message, ...) Urho3D::Log::GetLogger().Info(message, ##__VA_ARGS__)
#define URHO3D_LOGWARNING(message, ...) Urho3D::Log::GetLogger().Warning(message, ##__VA_ARGS__)
#define URHO3D_LOGERROR(message, ...) Urho3D::Log::GetLogger().Error(message, ##__VA_ARGS__)
#define URHO3D_LOGTRACEF(format, ...) Urho3D::Log::GetLogger().Write(Urho3D::LOG_TRACE, Urho3D::ToString(format, ##__VA_ARGS__))
#define URHO3D_LOGDEBUGF(format, ...) Urho3D::Log::GetLogger().Write(Urho3D::LOG_DEBUG, Urho3D::ToString(format, ##__VA_ARGS__))
#define URHO3D_LOGINFOF(format, ...) Urho3D::Log::GetLogger().Write(Urho3D::LOG_INFO, Urho3D::ToString(format, ##__VA_ARGS__))
#define URHO3D_LOGWARNINGF(format, ...) Urho3D::Log::GetLogger().Write(Urho3D::LOG_WARNING, Urho3D::ToString(format, ##__VA_ARGS__))
#define URHO3D_LOGERRORF(format, ...) Urho3D::Log::GetLogger().Write(Urho3D::LOG_ERROR, Urho3D::ToString(format, ##__VA_ARGS__))
#else
#define URHO3D_LOGTRACE(...) ((void)0)
#define URHO3D_LOGDEBUG(...) ((void)0)
#define URHO3D_LOGINFO(...) ((void)0)
#define URHO3D_LOGWARNING(...) ((void)0)
#define URHO3D_LOGERROR(...) ((void)0)
#define URHO3D_LOGTRACEF(...) ((void)0)
#define URHO3D_LOGDEBUGF(...) ((void)0)
#define URHO3D_LOGINFOF(...) ((void)0)
#define URHO3D_LOGWARNINGF(...) ((void)0)
#define URHO3D_LOGERRORF(...) ((void)0)
#endif

}
