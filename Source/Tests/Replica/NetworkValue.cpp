//
// Copyright (c) 2017-2021 the rbfx project.
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

#include <Urho3D/Replica/NetworkValue.h>

namespace
{

template <class T>
void Set(NetworkValueVector<T>& dest, NetworkFrame frame, std::initializer_list<T> value)
{
    dest.Set(frame, {value.begin(), static_cast<unsigned>(value.size())});
}

template <class T>
bool IsSame(ea::span<const T> lhs, std::initializer_list<T> rhs)
{
    return ea::identical(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <class T>
bool IsSame(ea::optional<ea::span<const T>> lhs, std::initializer_list<T> rhs)
{
    return lhs && IsSame(*lhs, rhs);
}

template <class T, class Traits>
bool IsSame(Detail::InterpolatedConstSpan<T, Traits> lhs, std::initializer_list<T> rhs)
{
    if (lhs.Size() != rhs.size())
        return false;
    for (unsigned i = 0; i < rhs.size(); ++i)
    {
        if (lhs[i] != rhs.begin()[i])
            return false;
    }
    return true;
}

template <class T, class Traits>
bool IsSame(ea::optional<Detail::InterpolatedConstSpan<T, Traits>> lhs, std::initializer_list<T> rhs)
{
    return lhs && IsSame(*lhs, rhs);
}

}

TEST_CASE("NetworkValue is updated and sampled")
{
    NetworkValue<float> v;
    v.Resize(5);

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{2}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{3}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{4}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{5}));

    v.Set(NetworkFrame{2}, 1000.0f);

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE(v.GetRaw(NetworkFrame{2}) == 1000.0f);
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{3}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{4}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{5}));

    REQUIRE(v.GetClosestRaw(NetworkFrame{1}) == 1000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{2}) == 1000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{5}) == 1000.0f);

    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{1}, 0.5f}) == 1000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.0f}) == 1000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.5f}) == 1000.0f);

    v.Set(NetworkFrame{2}, 2000.0f);

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE(v.GetRaw(NetworkFrame{2}) == 2000.0f);
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{3}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{4}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{5}));

    REQUIRE(v.GetClosestRaw(NetworkFrame{1}) == 2000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{2}) == 2000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{5}) == 2000.0f);

    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{1}, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.0f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.5f}) == 2000.0f);

    v.Set(NetworkFrame{4}, 4000.0f);

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE(v.GetRaw(NetworkFrame{2}) == 2000.0f);
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{3}));
    REQUIRE(v.GetRaw(NetworkFrame{4}) == 4000.0f);
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{5}));

    REQUIRE(v.GetClosestRaw(NetworkFrame{1}) == 2000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{2}) == 2000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{3}) == 2000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{4}) == 4000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{5}) == 4000.0f);

    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{1}, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.0f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.5f}) == 2500.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{3}, 0.0f}) == 3000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{3}, 0.5f}) == 3500.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{4}, 0.0f}) == 4000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{4}, 0.5f}) == 4000.0f);

    v.Set(NetworkFrame{3}, 3000.0f);
    v.Set(NetworkFrame{5}, 5000.0f);
    v.Set(NetworkFrame{6}, 6000.0f);

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE(v.GetRaw(NetworkFrame{2}) == 2000.0f);
    REQUIRE(v.GetRaw(NetworkFrame{3}) == 3000.0f);
    REQUIRE(v.GetRaw(NetworkFrame{4}) == 4000.0f);
    REQUIRE(v.GetRaw(NetworkFrame{5}) == 5000.0f);
    REQUIRE(v.GetRaw(NetworkFrame{6}) == 6000.0f);

    REQUIRE(v.GetClosestRaw(NetworkFrame{5}) == 5000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{6}) == 6000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{7}) == 6000.0f);

    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{1}, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.0f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.5f}) == 2500.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{3}, 0.0f}) == 3000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{3}, 0.5f}) == 3500.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{4}, 0.0f}) == 4000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{4}, 0.5f}) == 4500.0f);

    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{5}, 0.75f}) == 5750.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{6}, 0.0f}) == 6000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{6}, 0.5f}) == 6000.0f);

    v.Set(NetworkFrame{9}, 9000.0f);

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{2}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{3}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{4}));
    REQUIRE(v.GetRaw(NetworkFrame{5}) == 5000.0f);
    REQUIRE(v.GetRaw(NetworkFrame{6}) == 6000.0f);
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{7}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{8}));
    REQUIRE(v.GetRaw(NetworkFrame{9}) == 9000.0f);

    REQUIRE(v.GetClosestRaw(NetworkFrame{4}) == 5000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{5}) == 5000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{6}) == 6000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{7}) == 6000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{8}) == 6000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{9}) == 9000.0f);
    REQUIRE(v.GetClosestRaw(NetworkFrame{10}) == 9000.0f);

    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{4}, 0.5f}) == 5000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{5}, 0.0f}) == 5000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{5}, 0.5f}) == 5500.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{6}, 0.0f}) == 6000.0f);
    REQUIRE(v.SampleValid(NetworkTime{NetworkFrame{6}, 0.5f}) == 6500.0f);
}

TEST_CASE("NetworkValueSampler is smoothly sampled")
{
    const unsigned maxExtrapolation = 10;
    const float smoothing = 5.0f;
    const float snapThreshold = 10000.0f;

    NetworkValue<ValueWithDerivative<float>> v;
    v.Resize(11);
    NetworkValueSampler<ValueWithDerivative<float>> s;
    s.Setup(maxExtrapolation, smoothing, snapThreshold);

    // Interpolation is smooth when past frames are added
    v.Set(NetworkFrame{5}, {5000.0f, 1000.0f});
    v.Set(NetworkFrame{7}, {7000.0f, 1000.0f});

    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(4.0f), 0.5f) == 5000.0f);
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(4.5f), 0.5f) == 5000.0f);
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(5.0f), 0.5f) == 5000.0f);
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(5.5f), 0.5f) == 5500.0f);

    v.Set(NetworkFrame{6}, {6000.0f, 1000.0f});

    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(5.5f), 0.0f) == 5500.0f);
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(6.0f), 0.5f) == 6000.0f);
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(6.5f), 0.5f) == 6500.0f);

    // Extrapolation is smooth when past frames are added
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(7.0f), 0.5f) == 7000.0f);
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(7.5f), 0.5f) == 7500.0f);
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(8.0f), 0.5f) == 8000.0f);
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(8.5f), 0.5f) == 8500.0f);

    v.Set(NetworkFrame{8}, {8000.0f, 1000.0f});

    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(8.5f), 0.0f) == 8500.0f);
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(9.0f), 0.5f) == 9000.0f);

    // Extrapolation is smooth when unexpected past frames are added
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(11.0f), 2.0f) == 11000.0f);

    v.Set(NetworkFrame{10}, {10000.0f, 2000.0f});

    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(11.0f), 0.0f) == 11000.0f);
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(11.5f), 0.5f).value_or(0.0f) == Catch::Approx(13000.0f).margin(200.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(12.0f), 0.5f).value_or(0.0f) == Catch::Approx(14000.0f).margin(40.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(12.5f), 0.5f).value_or(0.0f) == Catch::Approx(15000.0f).margin(6.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(13.0f), 0.5f).value_or(0.0f) == Catch::Approx(16000.0f).margin(1.0f));

    // Transition from extrapolation to interpolation is smooth
    v.Set(NetworkFrame{15}, {15000.0f, 1000.0f});

    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(13.0f), 0.0f).value_or(0.0f) == Catch::Approx(16000.0f).margin(1.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(13.5f), 0.5f).value_or(0.0f) == Catch::Approx(13500.0f).margin(600.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(14.0f), 0.5f).value_or(0.0f) == Catch::Approx(14000.0f).margin(100.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(14.5f), 0.5f).value_or(0.0f) == Catch::Approx(14500.0f).margin(20.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(15.0f), 0.5f).value_or(0.0f) == Catch::Approx(15000.0f).margin(3.0f));

    // Snap threshold is exceeded and value is snapped
    v.Set(NetworkFrame{25}, {25000.0f, 1000.0f});
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(15.5f), 0.5f).value_or(0.0f) == Catch::Approx(15000.0f).margin(0.6f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(16.0f), 0.5f).value_or(0.0f) == Catch::Approx(15000.0f).margin(0.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(16.5f), 0.5f).value_or(0.0f) == Catch::Approx(15000.0f).margin(0.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(17.0f), 0.5f).value_or(0.0f) == Catch::Approx(15000.0f).margin(0.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(17.5f), 0.5f).value_or(0.0f) == Catch::Approx(15000.0f).margin(0.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(18.0f), 0.5f).value_or(0.0f) == Catch::Approx(15000.0f).margin(0.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(18.5f), 0.5f).value_or(0.0f) == Catch::Approx(15000.0f).margin(0.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(19.0f), 0.5f).value_or(0.0f) == Catch::Approx(15000.0f).margin(0.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(20.5f), 1.5f).value_or(0.0f) == Catch::Approx(25000.0f).margin(0.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(25.0f), 4.5f).value_or(0.0f) == Catch::Approx(25000.0f).margin(0.0f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(25.5f), 0.5f).value_or(0.0f) == Catch::Approx(25500.0f).margin(0.0f));
}

TEST_CASE("NetworkValueSampler for Quaternion is smoothly sampled")
{
    const unsigned maxExtrapolation = 0;
    const float smoothing = 5.0f;

    NetworkValue<Quaternion> v;
    v.Resize(10);
    NetworkValueSampler<Quaternion> s;
    s.Setup(maxExtrapolation, smoothing, M_LARGE_VALUE);

    v.Set(NetworkFrame{5}, Quaternion{0.0f});
    v.Set(NetworkFrame{6}, Quaternion{90.0f});
    v.Set(NetworkFrame{7}, Quaternion{180.0f});

    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(5.0f), 0.0f)->Equivalent(Quaternion{0.0f}));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(5.5f), 0.5f)->Equivalent(Quaternion{45.0f}));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(6.0f), 0.5f)->Equivalent(Quaternion{90.0f}));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(6.5f), 0.5f)->Equivalent(Quaternion{135.0f}));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(7.0f), 0.5f)->Equivalent(Quaternion{180.0f}));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(7.5f), 0.5f)->Equivalent(Quaternion{180.0f}));

    v.Set(NetworkFrame{8}, Quaternion{270.0f});
    v.Set(NetworkFrame{9}, Quaternion{360.0f});

    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(7.5f), 0.0f)->Equivalent(Quaternion{180.0f}));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(8.0f), 0.5f)->Equivalent(Quaternion{270.0f}, 0.003f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(8.5f), 0.5f)->Equivalent(Quaternion{315.0f}, 0.0001f));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(9.0f), 0.5f)->Equivalent(Quaternion{360.0f}, 0.00001f));
}

TEST_CASE("NetworkValueSampler for Quaternion is extrapolated")
{
    const unsigned maxExtrapolation = 10;
    const float smoothing = 5.0f;

    NetworkValue<ValueWithDerivative<Quaternion>> v;
    v.Resize(10);
    NetworkValueSampler<ValueWithDerivative<Quaternion>> s;
    s.Setup(maxExtrapolation, smoothing, M_LARGE_VALUE);

    const Vector3 velocity = Quaternion({90.0f}).AngularVelocity();
    v.Set(NetworkFrame{5}, {Quaternion{90.0f}, velocity});

    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(4.0f), 0.0f)->Equivalent(Quaternion{90.0f}));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(4.5f), 0.5f)->Equivalent(Quaternion{90.0f}));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(5.0f), 0.5f)->Equivalent(Quaternion{90.0f}));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(5.5f), 0.5f)->Equivalent(Quaternion{135.0f}));
    REQUIRE(s.UpdateAndSample(v, NetworkTime::FromDouble(6.0f), 0.5f)->Equivalent(Quaternion{180.0f}));
}

TEST_CASE("NetworkValueVector is updated and sampled")
{
    const unsigned size = 2;

    NetworkValueVector<float> v;
    v.Resize(size, 5);

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{2}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{3}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{4}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{5}));

    Set(v, NetworkFrame{2}, {1000.0f, 10000.0f});

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{2}), {1000.0f, 10000.0f}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{3}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{4}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{5}));

    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{1}), {1000.0f, 10000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{2}), {1000.0f, 10000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{5}), {1000.0f, 10000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{1}, 0.5f}), {1000.0f, 10000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.0f}), {1000.0f, 10000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.5f}), {1000.0f, 10000.0f}));

    Set(v, NetworkFrame{2}, {2000.0f, 20000.0f});

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{2}), {2000.0f, 20000.0f}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{3}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{4}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{5}));

    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{1}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{2}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{5}), {2000.0f, 20000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{1}, 0.5f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.0f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.5f}), {2000.0f, 20000.0f}));

    Set(v, NetworkFrame{4}, {4000.0f, 40000.0f});

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{2}), {2000.0f, 20000.0f}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{3}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{4}), {4000.0f, 40000.0f}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{5}));

    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{1}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{2}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{3}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{4}), {4000.0f, 40000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{5}), {4000.0f, 40000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{1}, 0.5f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.0f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.5f}), {2500.0f, 25000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{3}, 0.0f}), {3000.0f, 30000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{3}, 0.5f}), {3500.0f, 35000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{4}, 0.0f}), {4000.0f, 40000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{4}, 0.5f}), {4000.0f, 40000.0f}));

    Set(v, NetworkFrame{3}, {3000.0f, 30000.0f});
    Set(v, NetworkFrame{5}, {5000.0f, 50000.0f});
    Set(v, NetworkFrame{6}, {6000.0f, 60000.0f});

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{2}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{3}), {3000.0f, 30000.0f}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{4}), {4000.0f, 40000.0f}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{5}), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{6}), {6000.0f, 60000.0f}));

    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{5}), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{6}), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{7}), {6000.0f, 60000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{1}, 0.5f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.0f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{2}, 0.5f}), {2500.0f, 25000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{3}, 0.0f}), {3000.0f, 30000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{3}, 0.5f}), {3500.0f, 35000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{4}, 0.0f}), {4000.0f, 40000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{4}, 0.5f}), {4500.0f, 45000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{5}, 0.75f}), {5750.0f, 57500.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{6}, 0.0f}), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{6}, 0.5f}), {6000.0f, 60000.0f}));

    Set(v, NetworkFrame{9}, {9000.0f, 90000.0f});

    REQUIRE_FALSE(v.GetRaw(NetworkFrame{1}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{2}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{3}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{4}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{5}), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{6}), {6000.0f, 60000.0f}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{7}));
    REQUIRE_FALSE(v.GetRaw(NetworkFrame{8}));
    REQUIRE(IsSame(v.GetRaw(NetworkFrame{9}), {9000.0f, 90000.0f}));

    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{4}), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{5}), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{6}), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{7}), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{8}), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{9}), {9000.0f, 90000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(NetworkFrame{10}), {9000.0f, 90000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{4}, 0.5f}), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{5}, 0.0f}), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{5}, 0.5f}), {5500.0f, 55000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{6}, 0.0f}), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{NetworkFrame{6}, 0.5f}), {6500.0f, 65000.0f}));
}
