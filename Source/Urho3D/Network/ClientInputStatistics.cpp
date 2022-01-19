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

ClientInputStatistics::ClientInputStatistics(unsigned windowSize, unsigned maxInputLoss)
    : maxInputLoss_(maxInputLoss)
{
    numLostFrames_.set_capacity(windowSize);
}

void ClientInputStatistics::OnInputReceived(unsigned frame)
{
    if (!latestInputFrame_)
    {
        latestInputFrame_ = frame;
        return;
    }

    // Skip outdated inputs
    const auto delta = RoundToInt(NetworkTime{frame} - NetworkTime{*latestInputFrame_});
    latestInputFrame_ = frame;
    if (delta <= 0)
        return;

    for (int i = 0; i < ea::min(delta, maxInputLoss_); ++i)
        numLostFrames_.push_back(i);

    UpdateHistogram();
    bufferSize_ = GetMaxRepeatedLoss();
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

}
