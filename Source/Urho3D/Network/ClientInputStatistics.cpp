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

#include "../Network/ClientInputStatistics.h"

namespace Urho3D
{

ClientInputStatistics::ClientInputStatistics(unsigned windowSize)
{
    receivedFrames_.Resize(windowSize * 2);
    numLostFrames_.set_capacity(windowSize);
}

void ClientInputStatistics::OnInputReceived(unsigned frame)
{
    // Skip outdated inputs
    if (NetworkTime{frame} - NetworkTime{currentFrame_} <= 0.0)
        return;

    receivedFrames_.Set(frame, 0);
}

void ClientInputStatistics::OnInputConsumed(unsigned frame)
{
    ConsumeInputForFrame(frame);
    TrackInputLoss();
    UpdateHistogram();

    const auto [growSize, shrinkSize] = CalculateBufferSize();
    if (bufferSize_ <= growSize)
        bufferSize_ = growSize;
    if (bufferSize_ >= shrinkSize)
        bufferSize_ = shrinkSize;
}

void ClientInputStatistics::ConsumeInputForFrame(unsigned frame)
{
    currentFrame_ = frame;

    if (receivedFrames_.GetRaw(currentFrame_).has_value())
        numLostFramesBeforeCurrent_ = 0;
    else
        ++numLostFramesBeforeCurrent_;
}

void ClientInputStatistics::TrackInputLoss()
{
    if (numLostFrames_.full())
        numLostFrames_.pop_front();
    numLostFrames_.push_back(numLostFramesBeforeCurrent_);
}

void ClientInputStatistics::UpdateHistogram()
{
    histogram_.clear();
    for (unsigned numLost : numLostFrames_)
    {
        if (histogram_.size() <= numLost)
            histogram_.resize(numLost + 1);
        ++histogram_[numLost];
    }
}

unsigned ClientInputStatistics::GetMaxRepeatedLoss() const
{
    const auto isRepeated = [](unsigned x) { return x >= 2; };
    const auto iter = ea::find_if(histogram_.rbegin(), histogram_.rend(), isRepeated).base();
    return iter != histogram_.begin() ? iter - histogram_.begin() - 1 : 0;
}

ea::pair<unsigned, unsigned> ClientInputStatistics::CalculateBufferSize() const
{
    if (histogram_.size() <= 1)
        return {0, 0};

    const unsigned maxLoss = histogram_.size() - 1;
    const unsigned maxRepeatedLoss = GetMaxRepeatedLoss();
    return {ea::min(maxLoss - 1, maxRepeatedLoss), maxLoss};
}

}
