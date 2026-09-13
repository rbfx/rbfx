// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Replica/ClientInputStatistics.h"

namespace Urho3D
{

ClientInputStatistics::ClientInputStatistics(unsigned windowSize, unsigned maxInputLoss)
    : maxInputLoss_(maxInputLoss)
{
    numLostFrames_.set_capacity(windowSize);
}

void ClientInputStatistics::OnInputReceived(NetworkFrame frame)
{
    if (!latestInputFrame_)
    {
        latestInputFrame_ = frame;
        return;
    }

    // Skip outdated inputs
    const auto delta = static_cast<int>(frame - *latestInputFrame_);
    latestInputFrame_ = frame;
    if (delta <= 0)
        return;

    const int numLostFrames = ea::min(delta, maxInputLoss_) - 1;
    for (int i = 0; i <= numLostFrames; ++i)
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
    return iter != histogram_.begin() ? static_cast<unsigned>(iter - histogram_.begin() - 1) : 0;
}

}
