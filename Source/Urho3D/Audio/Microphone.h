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

#pragma once

#include "../Core/Object.h"
#include "../Core/Mutex.h"

#include <SDL/SDL.h>

namespace Urho3D
{

class BufferedSoundStream;

/// Microphone audio input device. Uses are for speech recognition or network speech, not intended for high quality recording usage.
class URHO3D_API Microphone : public Object
{
    URHO3D_OBJECT(Microphone, Object);
    friend class Audio;
public:
    /// Construct.
    Microphone(Context*);
    /// Destruct.
    virtual ~Microphone();
    /// Register factory.
    static void RegisterObject(Context*);

    /// Gets direct access to the data.
    ea::vector<int16_t>& GetData() { return buffer_; }
    /// Gets direct access to the data.
    const ea::vector<int16_t>& GetData() const { return buffer_; }

    /// Copies the current data into the destination array.
    void CopyData(ea::vector<int16_t>& dest) const;

    /// Wipes the buffer clean.
    void ClearData();

    /// Appends data to the internal buffer.
    void Update(unsigned char* rawData, int rawDataLen);

    /// Returns the frequency of the microphone's recording.
    /// @property
    unsigned GetFrequency() const { return frequency_; }

    /// Returns the user friendly name of this microphone.
    /// @property
    ea::string GetName() const { return name_; }

    /// Returns true if the device is actively recording.
    /// @property
    bool IsEnabled() const { return enabled_ && micID_ != 0; }
    /// Starts or stops recording.
    /// @property
    void SetEnabled(bool state);

    /// Returns the minimum volume to wake the device (absolute value)
    /// @property
    unsigned GetWakeThreshold() const { return wakeThreshold_; }
    /// Minimum volume to wake this device.
    /// @property
    void SetWakeThreshold(unsigned value) { wakeThreshold_ = value; }

    /// Returns true if this device is in sleep state.
    /// @property
    bool IsSleeping() const { return isSleeping_; }

    /// Gets the linked stream target if any.
    SharedPtr<BufferedSoundStream> GetLinked() const;
    /// Links the given stream object to automatically fill with microphone data.
    void Link(SharedPtr<BufferedSoundStream>);
    /// Unlinks the stream object, typically do this in E_RECORDINGENDED.
    void Unlink();

private:
    /// Initializes the SDL audio device.
    void Init(const ea::string& name, SDL_AudioDeviceID id, int bufferSize, unsigned frequency, unsigned which);
    /// Audio calls this to check if the SDL thread has appended data to us.
    void CheckDirtiness();

    /// Target to auto-copy data into.
    SharedPtr<BufferedSoundStream> linkedStream_;
    /// Named identifier of the microphone.
    ea::string name_;
    /// Stored copy of data contained.
    ea::vector<int16_t> buffer_;
    /// SDL identifier for the mic.
    SDL_AudioDeviceID micID_;
    /// Last seen index by SDL.
    unsigned which_;
    /// Lock object for thread safety, most operations on speech are going to want to be threaded.
    mutable Mutex lock_;
    /// HZ freq of the mic.
    unsigned frequency_;
    /// Signal threshold above which to "wake" the microphone. Not very effective.
    unsigned wakeThreshold_ = UINT_MAX;
    /// Whether to active capture data or not (data still comes in, but is ignored not copied).
    bool enabled_;
    /// Whether the microphone has failed to meet wake thresholds.
    bool isSleeping_ = false;
    /// Cleanliness state of the data.
    bool isDirty_ = false;
};

}
