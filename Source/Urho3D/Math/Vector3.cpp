//
// Copyright (c) 2008-2016 the Urho3D project.
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

#include "../Math/Vector3.h"

#include <cstdio>

#include "../DebugNew.h"

namespace Urho3D
{

template<typename T> const Vector3_<T> Vector3_<T>::ZERO;
template<typename T> const Vector3_<T> Vector3_<T>::LEFT(-1, 0, 0);
template<typename T> const Vector3_<T> Vector3_<T>::RIGHT(1, 0, 0);
template<typename T> const Vector3_<T> Vector3_<T>::UP(0, 1, 0);
template<typename T> const Vector3_<T> Vector3_<T>::DOWN(0, -1, 0);
template<typename T> const Vector3_<T> Vector3_<T>::FORWARD(0, 0, 1);
template<typename T> const Vector3_<T> Vector3_<T>::BACK(0, 0, -1);
template<typename T> const Vector3_<T> Vector3_<T>::ONE(1, 1, 1);

template<>
String Vector3::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%g %g %g", x_, y_, z_);
    return String(tempBuffer);
}

template<>
String DoubleVector3::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%g %g %g", x_, y_, z_);
    return String(tempBuffer);
}

template<>
String IntVector3::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%d %d %d", x_, y_, z_);
    return String(tempBuffer);
}

template<>
String LongLongVector3::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%lld %lld %lld", x_, y_, z_);
    return String(tempBuffer);
}

}
