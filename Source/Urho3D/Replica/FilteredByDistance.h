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

/// \file

#pragma once

#include "../Replica/BehaviorNetworkObject.h"

namespace Urho3D
{

/// Behavior that filters NetworkObject by the minimum distance to the client.
/// If the distance is less than the threshold, no relevance is reported.
/// If the distance is greater than the threshold, specified relevance or irrelevance is reported.
class URHO3D_API FilteredByDistance : public NetworkBehavior
{
    URHO3D_OBJECT(FilteredByDistance, NetworkBehavior);

public:
    static constexpr NetworkCallbackFlags CallbackMask = NetworkCallbackMask::GetRelevanceForClient;
    static constexpr float DefaultDistance = 100.0f;

    explicit FilteredByDistance(Context* context);
    ~FilteredByDistance() override;

    static void RegisterObject(Context* context);

    /// Manage attributes.
    /// @{
    void SetRelevant(bool value) { isRelevant_ = value; }
    bool IsRelevant() const { return isRelevant_; }
    void SetUpdatePeriod(unsigned value) { updatePeriod_ = value; }
    unsigned GetUpdatePeriod() const { return updatePeriod_; }
    void SetDistance(float value) { distance_ = value; }
    float GetDistance() const { return distance_; }
    /// @}

    /// Implement NetworkBehavior.
    /// @{
    ea::optional<NetworkObjectRelevance> GetRelevanceForClient(AbstractConnection* connection) override;
    /// @}

private:
    bool isRelevant_{true};
    unsigned updatePeriod_{};
    float distance_{DefaultDistance};
};

};
