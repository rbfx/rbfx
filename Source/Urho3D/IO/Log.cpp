//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/ProcessUtils.h"
#include "../Core/Profiler.h"
#include "../Core/Thread.h"
#include "../Core/Timer.h"
#include "../IO/File.h"
#include "../IO/IOEvents.h"
#include "../IO/Log.h"

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>

#include <mutex>
#include <cstdio>

#ifdef __ANDROID__
#include <android/log.h>
#include <spdlog/sinks/android_sink.h>
#endif
#if defined(IOS) || defined(TVOS)
extern "C" void SDL_IOS_LogMessage(const char* message);
#endif

#include "../DebugNew.h"
#include "Log.h"


namespace Urho3D
{

static Log* logInstance = nullptr;

#if defined(IOS) || defined(TVOS)
template<typename Mutex>
class IOSSink : public spdlog::sinks::base_sink<Mutex>
{
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        SDL_IOS_LogMessage(ea::string(msg.payload.begin(), msg.payload.end()).c_str());
    }

    void flush_() override { }
};

using IOSSink_mt = IOSSink<std::mutex>;
using IOSSink_st = IOSSink<spdlog::details::null_mutex>;
#endif

static spdlog::level::level_enum ConvertLogLevel(LogLevel level)
{
    switch (level)
    {
    case LOG_TRACE:
        return spdlog::level::trace;
    case LOG_DEBUG:
        return spdlog::level::debug;
    case LOG_INFO:
        return spdlog::level::info;
    case LOG_WARNING:
        return spdlog::level::warn;
    case LOG_ERROR:
        return spdlog::level::err;
    case LOG_NONE:
        return spdlog::level::off;
    default:
        assert(false);
        return spdlog::level::off;
    }
}

static LogLevel ConvertLogLevel(spdlog::level::level_enum level)
{
    switch (level)
    {
    case spdlog::level::trace:
        return LOG_TRACE;
    case spdlog::level::debug:
        return LOG_DEBUG;
    case spdlog::level::info:
        return LOG_INFO;
    case spdlog::level::warn:
        return LOG_WARNING;
    case spdlog::level::err:
        return LOG_ERROR;
    case spdlog::level::off:
        return LOG_NONE;
    default:
        assert(false);
        return LOG_NONE;
    }
}

template<typename Mutex>
class MessageForwarderSink : public spdlog::sinks::base_sink<Mutex>
{
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        time_t time = std::chrono::system_clock::to_time_t(msg.time);
        logInstance->SendMessageEvent(ConvertLogLevel(msg.level), time,
            ea::string(msg.logger_name->c_str()), ea::string(msg.payload.data(), static_cast<unsigned int>(msg.payload.size())));
    }

    void flush_() override { }
};

using MessageForwarderSink_mt = MessageForwarderSink<std::mutex>;
using MessageForwarderSink_st = MessageForwarderSink<spdlog::details::null_mutex>;

Logger::Logger(void* logger)
    : logger_(logger)
{
}

void Logger::WriteFormatted(LogLevel level, const ea::string& message)
{
    if (logger_ == nullptr)
        return;

    auto* logger = reinterpret_cast<spdlog::logger*>(logger_);

    switch (level)
    {
    case LOG_TRACE:
        logger->trace(message.c_str());
        break;
    case LOG_DEBUG:
        logger->debug(message.c_str());
        break;
    case LOG_INFO:
        logger->info(message.c_str());
        break;
    case LOG_WARNING:
        logger->warn(message.c_str());
        break;
    case LOG_ERROR:
        logger->error(message.c_str());
        break;
    case LOG_NONE:
    case MAX_LOGLEVELS:
        logger->warn("(Unknown log level used!) %s", message.c_str());
        break;
    }
}

class LogImpl : public Object
{
    URHO3D_OBJECT(LogImpl, Object);
public:
    explicit LogImpl(Context* context) : Object(context)
    {
        sinkProxy_ = std::make_shared<spdlog::sinks::dist_sink_mt>();
#if defined(__ANDROID__)
        platformSink_ = std::make_shared<spdlog::sinks::android_sink_mt>("Urho3D");
#elif defined(IOS) || defined(TVOS)
        platformSink_ = std::make_shared<IOSSink_mt>();
#else
#if _WIN32
        platformSink_ = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
        platformSink_ = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
#endif
#endif
        sinkProxy_->add_sink(platformSink_);
        sinkProxy_->add_sink(std::make_shared<MessageForwarderSink_mt>());
    }

#ifdef __ANDROID__
    /// Android adb logcat sink
    std::shared_ptr<spdlog::sinks::android_sink_mt> platformSink_;
#elif defined(IOS) || defined(TVOS)
    /// IOS sink.
    std::shared_ptr<IOSSink_mt> platformSink_;
#else
    /// File sink. Only for desktops.
    std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSink_;
    /// STDOUT sink.
#if _WIN32
    std::shared_ptr<spdlog::sinks::wincolor_stdout_sink_mt> platformSink_;
#else
    std::shared_ptr<spdlog::sinks::ansicolor_stdout_sink_mt> platformSink_;
#endif
#endif
    /// Sink that forwards messages to all other sinks.
    std::shared_ptr<spdlog::sinks::dist_sink_mt> sinkProxy_;
};

Log::Log(Context* context) :
    Object(context),
    impl_(new LogImpl(context))
{
    logInstance = this;

    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Log, HandleEndFrame));
}

Log::~Log()
{
    logInstance = nullptr;
}

void Log::Open(const ea::string& fileName)
{
#if !defined(MOBILE) && !defined(WEB)
    if (fileName.empty())
        return;

    if (fileName == NULL_DEVICE)
        return;

    Close();

    impl_->fileSink_ = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fileName.c_str());
    impl_->fileSink_->set_pattern(formatPattern_.c_str());
    impl_->sinkProxy_->add_sink(impl_->fileSink_);
#endif
}

void Log::Close()
{
#if !defined(MOBILE) && !defined(WEB)
    if (impl_->fileSink_)
    {
        impl_->sinkProxy_->remove_sink(impl_->fileSink_);
        impl_->fileSink_ = nullptr;
    }
#endif
}

void Log::SetLevel(LogLevel level)
{
    if (level < LOG_TRACE || level > LOG_NONE)
    {
        URHO3D_LOGERRORF("Attempted to set erroneous log level %d", level);
        return;
    }

    level_ = level;
    spdlog::set_level(ConvertLogLevel(level));
}

void Log::SetQuiet(bool quiet)
{
    quiet_ = quiet;
    impl_->platformSink_->set_level(ConvertLogLevel(quiet ? LOG_NONE : level_));
}

void Log::SetLogFormat(const ea::string& format)
{
    formatPattern_ = format;

    if (impl_->platformSink_)
        impl_->platformSink_->set_pattern(format.c_str());

#if !defined(MOBILE) && !defined(WEB)
    // May not be opened yet if patter is set from Application::Setup().
    if (impl_->fileSink_)
        impl_->fileSink_->set_pattern(format.c_str());
#endif
}

Logger Log::GetLogger(const char* name)
{
    if (name == nullptr)
        name = "main";

    if (logInstance != nullptr)
    {
        MutexLock lock(logInstance->logMutex_);
        std::shared_ptr<spdlog::logger> logger(spdlog::get(name));

        if (!logger)
        {
            logger = std::make_shared<spdlog::logger>(ea::string(name), logInstance->impl_->sinkProxy_);
            spdlog::register_logger(logger);
        }

        return Logger(reinterpret_cast<void*>(spdlog::get(name).get()));
    }
    else
        return Logger(reinterpret_cast<void*>(spdlog::get(name).get()));
}

void Log::SendMessageEvent(LogLevel level, time_t timestamp, const ea::string& logger, const ea::string& message)
{
    // No-op if illegal level
    if (level < LOG_TRACE || level >= LOG_NONE)
        return;

    // If not in the main thread, store message for later processing
    if (!Thread::IsMainThread())
    {
        if (logInstance)
        {
            MutexLock lock(logMutex_);
            threadMessages_.push_back(StoredLogMessage(level, timestamp, logger, message));
        }

        return;
    }

    if (inWrite_)
        return;

    inWrite_ = true;

    using namespace LogMessage;

    VariantMap& eventData = logInstance->GetEventDataMap();
    eventData[P_LEVEL] = level;
    eventData[P_TIME] = (unsigned)timestamp;
    eventData[P_LOGGER] = logger;
    eventData[P_MESSAGE] = message;
    SendEvent(E_LOGMESSAGE, eventData);

    inWrite_ = false;
}

void Log::PumpThreadMessages()
{
    // If the MainThreadID is not valid, processing this loop can potentially be endless
    if (!Thread::IsMainThread())
    {
        static bool threadErrorDisplayed = false;
        if (!threadErrorDisplayed)
        {
            fprintf(stderr, "Thread::mainThreadID is not setup correctly! Threaded log handling disabled\n");
            threadErrorDisplayed = true;
        }
        return;
    }

    MutexLock lock(logMutex_);

    // Process messages accumulated from other threads (if any)
    while (!threadMessages_.empty())
    {
        const StoredLogMessage& stored = threadMessages_.front();
        SendMessageEvent(stored.level_, stored.timestamp_, stored.logger_, stored.message_);
        threadMessages_.pop_front();
    }
}

}
