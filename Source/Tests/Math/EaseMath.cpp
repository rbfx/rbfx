//
// Copyright (c) 2022-2022 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "../CommonUtils.h"
#include <Urho3D/Math/EaseMath.h>

using namespace Urho3D;

TEST_CASE("BackIn")
{
    CHECK(Equals(-0.0f, BackIn(0.0f)));
    CHECK(Equals(-0.064136565f, BackIn(0.25f)));
    CHECK(Equals(-0.087697506f, BackIn(0.5f)));
    CHECK(Equals(0.1825903f, BackIn(0.75f)));
    CHECK(Equals(1.0f, BackIn(1.0f)));
}

TEST_CASE("BackOut")
{
    CHECK(Equals(0.0f, BackOut(0.0f)));
    CHECK(Equals(0.8174097f, BackOut(0.25f)));
    CHECK(Equals(1.0876975f, BackOut(0.5f)));
    CHECK(Equals(1.0641365f, BackOut(0.75f)));
    CHECK(Equals(1.0f, BackOut(1.0f)));
}

TEST_CASE("BackInOut")
{
    CHECK(Equals(-0.0f, BackInOut(0.0f)));
    CHECK(Equals(-0.09968184f, BackInOut(0.25f)));
    CHECK(Equals(0.5f, BackInOut(0.5f)));
    CHECK(Equals(1.0996819f, BackInOut(0.75f)));
    CHECK(Equals(1.0f, BackInOut(1.0f)));
}

TEST_CASE("BounceOut")
{
    CHECK(Equals(0.0f, BounceOut(0.0f)));
    CHECK(Equals(0.47265625f, BounceOut(0.25f)));
    CHECK(Equals(0.765625f, BounceOut(0.5f)));
    CHECK(Equals(0.97265625f, BounceOut(0.75f)));
    CHECK(Equals(1.0f, BounceOut(1.0f)));
}

TEST_CASE("BounceIn")
{
    CHECK(Equals(0.0f, BounceIn(0.0f)));
    CHECK(Equals(0.02734375f, BounceIn(0.25f)));
    CHECK(Equals(0.234375f, BounceIn(0.5f)));
    CHECK(Equals(0.52734375f, BounceIn(0.75f)));
    CHECK(Equals(1.0f, BounceIn(1.0f)));
}

TEST_CASE("BounceInOut")
{
    CHECK(Equals(0.0f, BounceInOut(0.0f)));
    CHECK(Equals(0.1171875f, BounceInOut(0.25f)));
    CHECK(Equals(0.5f, BounceInOut(0.5f)));
    CHECK(Equals(0.8828125f, BounceInOut(0.75f)));
    CHECK(Equals(1.0f, BounceInOut(1.0f)));
}

TEST_CASE("SineOut")
{
    CHECK(Equals(0.0f, SineOut(0.0f)));
    CHECK(Equals(0.38268346f, SineOut(0.25f)));
    CHECK(Equals(0.70710677f, SineOut(0.5f)));
    CHECK(Equals(0.9238795f, SineOut(0.75f)));
    CHECK(Equals(1.0f, SineOut(1.0f)));
}

TEST_CASE("SineIn")
{
    CHECK(Equals(0.0f, SineIn(0.0f)));
    CHECK(Equals(0.076120496f, SineIn(0.25f)));
    CHECK(Equals(0.29289323f, SineIn(0.5f)));
    CHECK(Equals(0.6173166f, SineIn(0.75f)));
    CHECK(Equals(1.0f, SineIn(1.0f)));
}

TEST_CASE("SineInOut")
{
    CHECK(Equals(-0.0f, SineInOut(0.0f)));
    CHECK(Equals(0.14644662f, SineInOut(0.25f)));
    CHECK(Equals(0.5f, SineInOut(0.5f)));
    CHECK(Equals(0.8535534f, SineInOut(0.75f)));
    CHECK(Equals(1.0f, SineInOut(1.0f)));
}

TEST_CASE("ExponentialOut")
{
    CHECK(Equals(0.0f, ExponentialOut(0.0f)));
    CHECK(Equals(0.8232233f, ExponentialOut(0.25f)));
    CHECK(Equals(0.96875f, ExponentialOut(0.5f)));
    CHECK(Equals(0.9944757f, ExponentialOut(0.75f)));
    CHECK(Equals(1.0f, ExponentialOut(1.0f)));
}

TEST_CASE("ExponentialIn")
{
    CHECK(Equals(0.0f, ExponentialIn(0.0f)));
    CHECK(Equals(0.0045242715f, ExponentialIn(0.25f)));
    CHECK(Equals(0.03025f, ExponentialIn(0.5f)));
    CHECK(Equals(0.17577669f, ExponentialIn(0.75f)));
    CHECK(Equals(0.999f, ExponentialIn(1.0f)));
}

TEST_CASE("ExponentialInOut")
{
    CHECK(Equals(0.00048828125f, ExponentialInOut(0.0f)));
    CHECK(Equals(0.015625f, ExponentialInOut(0.25f)));
    CHECK(Equals(0.5f, ExponentialInOut(0.5f)));
    CHECK(Equals(0.984375f, ExponentialInOut(0.75f)));
    CHECK(Equals(0.9995117f, ExponentialInOut(1.0f)));
}

TEST_CASE("ElasticIn")
{
    CHECK(Equals(0.0f, ElasticIn(0.0f, 2.1f)));
    CHECK(Equals(-0.0034443287f, ElasticIn(0.25f, 2.1f)));
    CHECK(Equals(0.0023353111f, ElasticIn(0.5f, 2.1f)));
    CHECK(Equals(0.12958647f, ElasticIn(0.75f, 2.1f)));
    CHECK(Equals(1.0f, ElasticIn(1.0f, 2.1f)));
}

TEST_CASE("ElasticOut")
{
    CHECK(Equals(0.0f, ElasticOut(0.0f, 2.1f)));
    CHECK(Equals(0.87041354f, ElasticOut(0.25f, 2.1f)));
    CHECK(Equals(0.9976647f, ElasticOut(0.5f, 2.1f)));
    CHECK(Equals(1.0034443f, ElasticOut(0.75f, 2.1f)));
    CHECK(Equals(1.0f, ElasticOut(1.0f, 2.1f)));
}

TEST_CASE("ElasticInOut")
{
    CHECK(Equals(0.0f, ElasticInOut(0.0f, 2.1f)));
    CHECK(Equals(0.0011676556f, ElasticInOut(0.25f, 2.1f)));
    CHECK(Equals(0.5f, ElasticInOut(0.5f, 2.1f)));
    CHECK(Equals(0.99883235f, ElasticInOut(0.75f, 2.1f)));
    CHECK(Equals(1.0f, ElasticInOut(1.0f, 2.1f)));
}
