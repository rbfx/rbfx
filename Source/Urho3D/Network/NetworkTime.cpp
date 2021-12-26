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

#include "../Core/Format.h"
#include "../Network/NetworkTime.h"

namespace Urho3D
{

ea::string NetworkTime::ToString() const
{
    return Format("#{}:{:.2f}", frame_, subFrame_);
}

void NetworkTime::Normalize()
{
    while (subFrame_ < 0.0)
    {
        --frame_;
        subFrame_ += 1.0;
    }

    while (subFrame_ >= 1.0)
    {
        ++frame_;
        subFrame_ -= 1.0;
    }
}

void NetworkTime::AddDelta(double delta)
{
    const int deltaInt = static_cast<int>(delta);
    const float deltaFract = delta - deltaInt;
    frame_ += deltaInt;
    subFrame_ += deltaFract;
    Normalize();
}

double NetworkTime::GetDelta(const NetworkTime& origin) const
{
    const auto deltaInt = static_cast<int>(origin.frame_ - frame_);
    const double deltaFract = origin.subFrame_ - subFrame_;
    return deltaInt + deltaFract;
}

}
