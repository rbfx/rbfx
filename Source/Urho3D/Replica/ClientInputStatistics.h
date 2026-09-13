// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Replica/NetworkValue.h"

#include <EASTL/bonus/ring_buffer.h>

namespace Urho3D
{

/// Utility to evaluate client input quality and preferred input buffering.
class URHO3D_API ClientInputStatistics
{
public:
    ClientInputStatistics(unsigned windowSize, unsigned maxInputLoss);

    /// Notify that the input was received for given frame.
    void OnInputReceived(NetworkFrame frame);

    unsigned GetRecommendedBufferSize() const { return bufferSize_; }

private:
    void UpdateHistogram();
    unsigned GetMaxRepeatedLoss() const;

    const int maxInputLoss_;

    ea::optional<NetworkFrame> latestInputFrame_{};
    ea::ring_buffer<unsigned> numLostFrames_;

    ea::vector<unsigned> histogram_;
    unsigned bufferSize_{};
};

}
