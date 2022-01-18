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

/// \file

#pragma once

#include "../Network/NetworkValue.h"

#include <EASTL/bonus/ring_buffer.h>

namespace Urho3D
{

/// Utility to evaluate client input quality and preferred input buffering.
class URHO3D_API ClientInputStatistics
{
public:
    ClientInputStatistics(unsigned windowSize);

    /// Notify that the input was received for given frame.
    void OnInputReceived(unsigned frame);
    /// Notify that all the input up to specified frame has been consumed.
    void OnInputConsumed(unsigned frame);

    unsigned GetBufferSize() const { return bufferSize_; }

private:
    void ConsumeInputForFrame(unsigned frame);
    void TrackInputLoss();
    void UpdateHistogram();
    unsigned GetMaxRepeatedLoss() const;
    ea::pair<unsigned, unsigned> CalculateBufferSize() const;

    unsigned currentFrame_{};
    NetworkValue<int> receivedFrames_;

    unsigned numLostFramesBeforeCurrent_{};
    ea::ring_buffer<unsigned> numLostFrames_;

    ea::vector<unsigned> histogram_;

    unsigned bufferSize_{};
};

}
