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

#include "Microphone.h"

#include "../Core/Context.h"
#include "../IO/Log.h"

#include "AudioEvents.h"
#include "BufferedSoundStream.h"

#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>

namespace Urho3D
{

Microphone::Microphone(Context* ctx) : Object(ctx),
    micID_(0),
    which_(0),
    frequency_(0u),
    enabled_(false)
{

}

Microphone::~Microphone()
{
    if (micID_ != 0)
        SDL_CloseAudioDevice(micID_);
    micID_ = 0;
}

void Microphone::RegisterObject(Context* ctx)
{
    ctx->RegisterFactory<Microphone>();
}

void Microphone::CopyData(ea::vector<int16_t>& dest) const
{
    MutexLock lockHandle(lock_);

    dest.resize(buffer_.size());
    memcpy(dest.data(), buffer_.data(), buffer_.size() * sizeof(int16_t));
}

void Microphone::ClearData()
{
    MutexLock lockHandle(lock_);
    buffer_.clear();
}

void Microphone::Init(const ea::string& name, SDL_AudioDeviceID id, int bufferSize, unsigned frequency, unsigned which)
{
    MutexLock lockHandle(lock_);

    name_ = name;
    micID_ = id;
    which_ = which;
    frequency_ = frequency;
    buffer_.reserve(bufferSize);

    SetEnabled(true);
}

void Microphone::SetEnabled(bool state) 
{ 
    if (enabled_ != state)
    {
        MutexLock lockHandle(lock_);

        if (state == true)
            buffer_.clear();
        enabled_ = state;

        if (state)
        {
            auto& eventData = GetEventDataMap();
            eventData[RecordingStarted::P_MICROPHONE] = this;
            SendEvent(E_RECORDINGSTARTED, eventData);
        }

        SDL_PauseAudioDevice(micID_, state ? SDL_FALSE : SDL_TRUE);

        if (!state)
        {
            if (state)
            {
                auto& eventData = GetEventDataMap();
                eventData[RecordingEnded::P_MICROPHONE] = this;
                eventData[RecordingEnded::P_DATALENGTH] = (int)buffer_.size();
                eventData[RecordingEnded::P_CLEARDATA] = false;

                SendEvent(E_RECORDINGENDED, eventData);

                if (eventData[RecordingEnded::P_CLEARDATA].GetBool())
                    buffer_.clear();
            }
        }
    }
}

void Microphone::Update(unsigned char* rawData, int rawDataLen)
{
    if (!enabled_)
        return;

    if (wakeThreshold_ != UINT_MAX)
    {
        int16_t* data = (int16_t*)rawData;
        const int sampleCt = rawDataLen / sizeof(int16_t);

        bool needsWake = false;
        for (int i = 0; i < sampleCt; ++i)
            needsWake |= abs(data[i]) > wakeThreshold_;

        if (!needsWake)
        {
            isSleeping_ = true;
            return;
        }
        isSleeping_ = false;
    }

    MutexLock lockHandle(lock_);

    unsigned sz = buffer_.size();
    buffer_.resize(sz + rawDataLen / sizeof(int16_t));

    auto buffPtr = buffer_.data();
    memcpy(buffPtr + sz, rawData, rawDataLen);

    if (linkedStream_)
        linkedStream_->AddData(rawData, rawDataLen);

    isDirty_ = true;
    isSleeping_ = false;
}

void Microphone::CheckDirtiness()
{
    if (!enabled_)
        return;

    MutexLock lockHandle(lock_);

    if (isDirty_)
    {
        auto& data = GetEventDataMap();

        data[RecordingUpdated::P_MICROPHONE] = this;
        data[RecordingUpdated::P_DATALENGTH] = (int)buffer_.size();
        data[RecordingUpdated::P_CLEARDATA] = false;
        SendEvent(E_RECORDINGUPDATED, data);

        // This is generally not done, using an event-data param as a return value. Unsure about how I feel about it.
        if (data[RecordingUpdated::P_CLEARDATA].GetBool())
            buffer_.clear();

        isDirty_ = false;
    }
}

SharedPtr<BufferedSoundStream> Microphone::GetLinked() const
{
    return linkedStream_;
}

void Microphone::Link(SharedPtr<BufferedSoundStream> stream)
{
    linkedStream_ = stream;
    if (linkedStream_)
        linkedStream_->SetFormat(GetFrequency(), true, false);
}

void Microphone::Unlink()
{
    linkedStream_.Reset();
}

}
