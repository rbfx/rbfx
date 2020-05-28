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

#include "../Precompiled.h"

#include "../Core/CoreEvents.h"
#include "../Core/Profiler.h"
#include "../Core/Timer.h"

#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#elif __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "../DebugNew.h"

namespace Urho3D
{

URHO3D_EVENT(E_ENDFRAMEPRIVATE, EndFramePrivate)
{
}

bool HiresTimer::supported(false);
long long HiresTimer::frequency(1000);

Time::Time(Context* context) :
    Object(context),
    frameNumber_(0),
    timeStep_(0.0f),
    timerPeriod_(0)
{
#ifdef _WIN32
    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency))
    {
        HiresTimer::frequency = frequency.QuadPart;
        HiresTimer::supported = true;
    }
#else
    HiresTimer::frequency = 1000000;
    HiresTimer::supported = true;
#endif
}

Time::~Time()
{
    SetTimerPeriod(0);
}

static unsigned Tick()
{
#ifdef _WIN32
    return (unsigned)GetTickCount();
#elif __EMSCRIPTEN__
    return (unsigned)emscripten_get_now();
#else
    struct timeval time{};
    gettimeofday(&time, nullptr);
    return (unsigned)(time.tv_sec * 1000 + time.tv_usec / 1000);
#endif
}

static long long HiresTick()
{
#ifdef _WIN32
#ifndef UWP
    if (HiresTimer::IsSupported())
    {
#endif
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        return counter.QuadPart;
#ifndef UWP
    }
    else
        return GetTickCount();
#endif
#elif __EMSCRIPTEN__
    return (long long)(emscripten_get_now()*1000.0);
#else
    struct timeval time{};
    gettimeofday(&time, nullptr);
    return time.tv_sec * 1000000LL + time.tv_usec;
#endif
}

void Time::BeginFrame(float timeStep)
{
    ++frameNumber_;
    if (!frameNumber_)
        ++frameNumber_;

    timeStep_ = timeStep;

    {
        URHO3D_PROFILE("BeginFrame");

        // Frame begin event
        using namespace BeginFrame;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_FRAMENUMBER] = frameNumber_;
        eventData[P_TIMESTEP] = timeStep_;
        SendEvent(E_BEGINFRAME, eventData);
    }
}

void Time::EndFrame()
{
    {
        URHO3D_PROFILE("EndFrame");

        // Frame end event
        SendEvent(E_ENDFRAME);

        // Internal frame end event used only by the engine/tools
        SendEvent(E_ENDFRAMEPRIVATE);
    }
}

void Time::SetTimerPeriod(unsigned mSec)
{
#if defined(_WIN32) && !defined(UWP)
    if (timerPeriod_ > 0)
        timeEndPeriod(timerPeriod_);

    timerPeriod_ = mSec;

    if (timerPeriod_ > 0)
        timeBeginPeriod(timerPeriod_);
#endif
}

float Time::GetElapsedTime()
{
    return elapsedTime_.GetMSec(false) / 1000.0f;
}

unsigned Time::GetSystemTime()
{
    return Tick();
}

unsigned Time::GetTimeSinceEpoch()
{
    return (unsigned)time(nullptr);
}

ea::string Time::GetTimeStamp(const char* format)
{
    time_t timestamp = 0;
    time(&timestamp);
    return GetTimeStamp(timestamp, format);
}

ea::string Time::GetTimeStamp(time_t timestamp, const char* format)
{
    if (format == nullptr)
        format = DEFAULT_DATE_TIME_FORMAT;

    char dateTime[128];
    tm* timeInfo = localtime(&timestamp);
    strftime(dateTime, sizeof(dateTime), format, timeInfo);
    return dateTime;
}

void Time::Sleep(unsigned mSec)
{
#ifdef _WIN32
    ::Sleep(mSec);
#else
    timespec time{static_cast<time_t>(mSec / 1000), static_cast<long>((mSec % 1000) * 1000000)};
    nanosleep(&time, nullptr);
#endif
}

float Time::GetFramesPerSecond() const
{
    return 1.0f / timeStep_;
}

Timer::Timer()
{
    Reset();
}

unsigned Timer::GetMSec(bool reset)
{
    unsigned currentTime = Tick();
    unsigned elapsedTime = currentTime - startTime_;
    if (reset)
        startTime_ = currentTime;

    return elapsedTime;
}

void Timer::Reset()
{
    startTime_ = Tick();
}

HiresTimer::HiresTimer()
{
    Reset();
}

long long HiresTimer::GetUSec(bool reset)
{
    long long currentTime = HiresTick();
    long long elapsedTime = currentTime - startTime_;

    // Correct for possible weirdness with changing internal frequency
    if (elapsedTime < 0)
        elapsedTime = 0;

    if (reset)
        startTime_ = currentTime;

    return (elapsedTime * 1000000LL) / frequency;
}

void HiresTimer::Reset()
{
    startTime_ = HiresTick();
}

}
