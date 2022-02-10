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

class AnimationController;

/// Behavior that replicates animation over network.
/// TODO: This behavior doesn't really replicate any animation now, it only does essential setup on the server.
class URHO3D_API ReplicatedAnimation : public NetworkBehavior
{
    URHO3D_OBJECT(ReplicatedAnimation, NetworkBehavior);

public:
    static constexpr NetworkCallbackFlags CallbackMask = NetworkCallbackMask::Update;

    explicit ReplicatedAnimation(Context* context, NetworkCallbackFlags derivedClassMask = NetworkCallbackMask::None);
    ~ReplicatedAnimation() override;

    static void RegisterObject(Context* context);

    /// Implement NetworkBehavior.
    /// @{
    void InitializeStandalone() override;
    void InitializeOnServer() override;
    void InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned) override;

    void Update(float replicaTimeStep, float inputTimeStep) override;
    /// @}

private:
    void InitializeCommon();

protected:
    WeakPtr<AnimationController> animationController_;
};

};
