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

#include "../Graphics/AnimationTrack.h"
#include "../IO/ArchiveSerialization.h"

#include "../DebugNew.h"

namespace Urho3D
{

void AnimationTrack::Sample(float time, float duration, bool isLooped, unsigned& frameIndex, Transform& value) const
{
    float blendFactor{};
    unsigned nextFrameIndex{};
    GetKeyFrames(time, duration, isLooped, frameIndex, nextFrameIndex, blendFactor);

    const AnimationKeyFrame& keyFrame = keyFrames_[frameIndex];
    const AnimationKeyFrame& nextKeyFrame = keyFrames_[nextFrameIndex];

    if (blendFactor >= M_EPSILON)
    {
        if (channelMask_ & CHANNEL_POSITION)
            value.position_ = keyFrame.position_.Lerp(nextKeyFrame.position_, blendFactor);
        if (channelMask_ & CHANNEL_ROTATION)
            value.rotation_ = keyFrame.rotation_.Slerp(nextKeyFrame.rotation_, blendFactor);
        if (channelMask_ & CHANNEL_SCALE)
            value.scale_ = keyFrame.scale_.Lerp(nextKeyFrame.scale_, blendFactor);
    }
    else
    {
        if (channelMask_ & CHANNEL_POSITION)
            value.position_ = keyFrame.position_;
        if (channelMask_ & CHANNEL_ROTATION)
            value.rotation_ = keyFrame.rotation_;
        if (channelMask_ & CHANNEL_SCALE)
            value.scale_ = keyFrame.scale_;
    }
}

}
