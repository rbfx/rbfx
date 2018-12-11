//
// Copyright (c) 2008-2018 the Urho3D project.
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

#include "../Math/MathDefs.h"

#include "../DebugNew.h"

namespace Urho3D
{
    int Wrap(int value, int min, int max)
    {
        int range = max - min;

        if (value < min) {
            value = 1 + (max - (min - value) % (range));
    }
        else if (value > max) {
            value = (-1 + min + (value - min) % (range));
        }
        return value;
}

    float Wrap(float value, float min, float max)
    {
        float range = max - min;

        if (value < min) {
            float delta = (min - value);
            value = max - ((delta / range) - delta);
        }
        else if (value > max) {
            float delta = (value - min);
            value = min + ((delta / range) - delta);
        }
        return value;
    }

    double Wrap(double value, double min, double max)
    {
        double range = max - min;

        if (value < min) {
            double delta = (min - value);
            value = max - ((delta / range) - delta);
        }
        else if (value > max) {
            double delta = (value - min);
            value = min + ((delta / range) - delta);
        }
        return value;
    }

void SinCos(float angle, float& sin, float& cos)
{
    float angleRadians = angle * M_DEGTORAD;
#if defined(HAVE_SINCOSF)
    sincosf(angleRadians, &sin, &cos);
#elif defined(HAVE___SINCOSF)
    __sincosf(angleRadians, &sin, &cos);
#else
    sin = sinf(angleRadians);
    cos = cosf(angleRadians);
#endif
}

}
