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

#include <Urho3D/Network/NetworkValue.h>

namespace
{

#if 0
template <class T>
void Append(NetworkValueVector<T>& dest, unsigned frame, std::initializer_list<T> value)
{
    dest.Append(frame, {value.begin(), static_cast<unsigned>(value.size())});
}

template <class T>
void Replace(NetworkValueVector<T>& dest, unsigned frame, std::initializer_list<T> value)
{
    dest.Replace(frame, {value.begin(), static_cast<unsigned>(value.size())});
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

template <class T>
bool IsSame(InterpolatedConstSpan<T> lhs, std::initializer_list<T> rhs)
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

template <class T>
bool IsSame(ea::optional<InterpolatedConstSpan<T>> lhs, std::initializer_list<T> rhs)
{
    return lhs && IsSame(*lhs, rhs);
}
#endif

}

TEST_CASE("NetworkValue is updated and sampled")
{
    NetworkValue<float> v;
    v.Resize(5);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE_FALSE(v.GetRaw(2));
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE_FALSE(v.GetRaw(5));

    v.Set(2, 1000.0f);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(v.GetRaw(2) == 1000.0f);
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE_FALSE(v.GetRaw(5));

    REQUIRE(v.GetClosestRaw(1) == 1000.0f);
    REQUIRE(v.GetClosestRaw(2) == 1000.0f);
    REQUIRE(v.GetClosestRaw(5) == 1000.0f);

    REQUIRE(v.SampleValid(NetworkTime{1, 0.5f}) == 1000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.0f}) == 1000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.5f}) == 1000.0f);

    v.Set(2, 2000.0f);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(v.GetRaw(2) == 2000.0f);
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE_FALSE(v.GetRaw(5));

    REQUIRE(v.GetClosestRaw(1) == 2000.0f);
    REQUIRE(v.GetClosestRaw(2) == 2000.0f);
    REQUIRE(v.GetClosestRaw(5) == 2000.0f);

    REQUIRE(v.SampleValid(NetworkTime{1, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.0f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.5f}) == 2000.0f);

    v.Set(4, 4000.0f);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(v.GetRaw(2) == 2000.0f);
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE(v.GetRaw(4) == 4000.0f);
    REQUIRE_FALSE(v.GetRaw(5));

    REQUIRE(v.GetClosestRaw(1) == 2000.0f);
    REQUIRE(v.GetClosestRaw(2) == 2000.0f);
    REQUIRE(v.GetClosestRaw(3) == 2000.0f);
    REQUIRE(v.GetClosestRaw(4) == 4000.0f);
    REQUIRE(v.GetClosestRaw(5) == 4000.0f);

    REQUIRE(v.SampleValid(NetworkTime{1, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.0f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{3, 0.0f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{3, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{4, 0.0f}) == 4000.0f);
    REQUIRE(v.SampleValid(NetworkTime{4, 0.5f}) == 4000.0f);

    v.Set(3, 3000.0f);
    v.Set(5, 5000.0f);
    v.Set(6, 6000.0f);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(v.GetRaw(2) == 2000.0f);
    REQUIRE(v.GetRaw(3) == 3000.0f);
    REQUIRE(v.GetRaw(4) == 4000.0f);
    REQUIRE(v.GetRaw(5) == 5000.0f);
    REQUIRE(v.GetRaw(6) == 6000.0f);

    REQUIRE(v.GetClosestRaw(5) == 5000.0f);
    REQUIRE(v.GetClosestRaw(6) == 6000.0f);
    REQUIRE(v.GetClosestRaw(7) == 6000.0f);

    REQUIRE(v.SampleValid(NetworkTime{1, 0.5f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.0f}) == 2000.0f);
    REQUIRE(v.SampleValid(NetworkTime{2, 0.5f}) == 2500.0f);
    REQUIRE(v.SampleValid(NetworkTime{3, 0.0f}) == 3000.0f);
    REQUIRE(v.SampleValid(NetworkTime{3, 0.5f}) == 3500.0f);
    REQUIRE(v.SampleValid(NetworkTime{4, 0.0f}) == 4000.0f);
    REQUIRE(v.SampleValid(NetworkTime{4, 0.5f}) == 4500.0f);

    REQUIRE(v.SampleValid(NetworkTime{5, 0.75f}) == 5750.0f);
    REQUIRE(v.SampleValid(NetworkTime{6, 0.0f}) == 6000.0f);
    REQUIRE(v.SampleValid(NetworkTime{6, 0.5f}) == 6000.0f);

    v.Set(9, 9000.0f);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE_FALSE(v.GetRaw(2));
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE(v.GetRaw(5) == 5000.0f);
    REQUIRE(v.GetRaw(6) == 6000.0f);
    REQUIRE_FALSE(v.GetRaw(7));
    REQUIRE_FALSE(v.GetRaw(8));
    REQUIRE(v.GetRaw(9) == 9000.0f);

    REQUIRE(v.GetClosestRaw(4) == 5000.0f);
    REQUIRE(v.GetClosestRaw(5) == 5000.0f);
    REQUIRE(v.GetClosestRaw(6) == 6000.0f);
    REQUIRE(v.GetClosestRaw(7) == 6000.0f);
    REQUIRE(v.GetClosestRaw(8) == 6000.0f);
    REQUIRE(v.GetClosestRaw(9) == 9000.0f);
    REQUIRE(v.GetClosestRaw(10) == 9000.0f);

    REQUIRE(v.SampleValid(NetworkTime{4, 0.5f}) == 5000.0f);
    REQUIRE(v.SampleValid(NetworkTime{5, 0.0f}) == 5000.0f);
    REQUIRE(v.SampleValid(NetworkTime{5, 0.5f}) == 5500.0f);
    REQUIRE(v.SampleValid(NetworkTime{6, 0.0f}) == 6000.0f);
    REQUIRE(v.SampleValid(NetworkTime{6, 0.5f}) == 6000.0f);
}

TEST_CASE("NetworkValue is repaired on demand")
{
    NetworkValueExtrapolationSettings settings{3};

    NetworkValue<float> v;
    v.Resize(10);

    v.Set(5, 5000.0f);

    REQUIRE(v.RepairAndSample(NetworkTime{4, 0.0f}, settings) == 5000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{4, 0.5f}, settings) == 5000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{5, 0.0f}, settings) == 5000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{5, 0.5f}, settings) == 5000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{6, 0.0f}, settings) == 5000.0f);

    v.Set(10, 10000.0f);

    REQUIRE(v.RepairAndSample(NetworkTime{8, 0.0f}, settings) == 8000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{8, 0.5f}, settings) == 8500.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{9, 0.0f}, settings) == 9000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{9, 0.5f}, settings) == 9500.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{10, 0.0f}, settings) == 10000.0f);

    REQUIRE(v.RepairAndSample(NetworkTime{10, 0.5f}, settings) == 10500.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{11, 0.0f}, settings) == 11000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{11, 0.5f}, settings) == 11500.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{12, 0.0f}, settings) == 12000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{12, 0.5f}, settings) == 12500.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{13, 0.0f}, settings) == 13000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{13, 0.5f}, settings) == 13000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{14, 0.0f}, settings) == 13000.0f);

    v.Set(13, 13000.0f);

    REQUIRE(v.RepairAndSample(NetworkTime{14, 0.5f}, settings) == 13000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{15, 0.0f}, settings) == 13000.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{15, 0.5f}, settings) == 14500.0f);
    REQUIRE(v.RepairAndSample(NetworkTime{16, 0.0f}, settings) == 16000.0f);
}

#if 0
TEST_CASE("NetworkValueVector is updated and sampled")
{
    const unsigned size = 2;

    NetworkValueVector<float> v;
    v.Resize(size, 5);

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE_FALSE(v.GetRaw(2));
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE_FALSE(v.GetRaw(5));

    Append(v, 2, {1000.0f, 10000.0f});

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(IsSame(v.GetRaw(2), {1000.0f, 10000.0f}));
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE_FALSE(v.GetRaw(5));

    REQUIRE(IsSame(v.GetClosestRaw(1), {1000.0f, 10000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(2), {1000.0f, 10000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(5), {1000.0f, 10000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{1, 0.5f}), {1000.0f, 10000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{2, 0.0f}), {1000.0f, 10000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{2, 0.5f}), {1000.0f, 10000.0f}));

    Append(v, 2, {2000.0f, 20000.0f});

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(IsSame(v.GetRaw(2), {1000.0f, 10000.0f}));
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE_FALSE(v.GetRaw(5));

    REQUIRE(IsSame(v.GetClosestRaw(1), {1000.0f, 10000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(2), {1000.0f, 10000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(5), {1000.0f, 10000.0f}));

    Replace(v, 2, {2000.0f, 20000.0f});

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(IsSame(v.GetRaw(2), {2000.0f, 20000.0f}));
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE_FALSE(v.GetRaw(5));

    REQUIRE(IsSame(v.GetClosestRaw(1), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(2), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(5), {2000.0f, 20000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{1, 0.5f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{2, 0.0f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{2, 0.5f}), {2000.0f, 20000.0f}));

    Append(v, 4, {4000.0f, 40000.0f});

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(IsSame(v.GetRaw(2), {2000.0f, 20000.0f}));
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE(IsSame(v.GetRaw(4), {4000.0f, 40000.0f}));
    REQUIRE_FALSE(v.GetRaw(5));

    REQUIRE(IsSame(v.GetClosestRaw(1), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(2), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(3), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(4), {4000.0f, 40000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(5), {4000.0f, 40000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{1, 0.5f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{2, 0.0f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{2, 0.5f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{3, 0.0f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{3, 0.5f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{4, 0.0f}), {4000.0f, 40000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{4, 0.5f}), {4000.0f, 40000.0f}));

    Append(v, 3, {3000.0f, 30000.0f});
    Append(v, 5, {5000.0f, 50000.0f});
    Append(v, 6, {6000.0f, 60000.0f});

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE(IsSame(v.GetRaw(2), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.GetRaw(3), {3000.0f, 30000.0f}));
    REQUIRE(IsSame(v.GetRaw(4), {4000.0f, 40000.0f}));
    REQUIRE(IsSame(v.GetRaw(5), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.GetRaw(6), {6000.0f, 60000.0f}));

    REQUIRE(IsSame(v.GetClosestRaw(5), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(6), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(7), {6000.0f, 60000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{1, 0.5f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{2, 0.0f}), {2000.0f, 20000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{2, 0.5f}), {2500.0f, 25000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{3, 0.0f}), {3000.0f, 30000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{3, 0.5f}), {3500.0f, 35000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{4, 0.0f}), {4000.0f, 40000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{4, 0.5f}), {4500.0f, 45000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{5, 0.75f}), {5750.0f, 57500.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{6, 0.0f}), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{6, 0.5f}), {6000.0f, 60000.0f}));

    Append(v, 9, {9000.0f, 90000.0f});

    REQUIRE_FALSE(v.GetRaw(1));
    REQUIRE_FALSE(v.GetRaw(2));
    REQUIRE_FALSE(v.GetRaw(3));
    REQUIRE_FALSE(v.GetRaw(4));
    REQUIRE(IsSame(v.GetRaw(5), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.GetRaw(6), {6000.0f, 60000.0f}));
    REQUIRE_FALSE(v.GetRaw(7));
    REQUIRE_FALSE(v.GetRaw(8));
    REQUIRE(IsSame(v.GetRaw(9), {9000.0f, 90000.0f}));

    REQUIRE(IsSame(v.GetClosestRaw(4), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(5), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(6), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(7), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(8), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(9), {9000.0f, 90000.0f}));
    REQUIRE(IsSame(v.GetClosestRaw(10), {9000.0f, 90000.0f}));

    REQUIRE(IsSame(v.SampleValid(NetworkTime{4, 0.5f}), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{5, 0.0f}), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{5, 0.5f}), {5500.0f, 55000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{6, 0.0f}), {6000.0f, 60000.0f}));
    REQUIRE(IsSame(v.SampleValid(NetworkTime{6, 0.5f}), {6000.0f, 60000.0f}));
}

TEST_CASE("NetworkValueVector is repaired on demand")
{
    const unsigned size = 2;
    const unsigned maxPenalty = 3;

    NetworkValueVector<float> v;
    v.Resize(size, 10);

    Append(v, 5, {5000.0f, 50000.0f});
    Append(v, 10, {10000.0f, 100000.0f});
    Append(v, 12, {12000.0f, 120000.0f});

    REQUIRE_FALSE(v.RepairAndSample(NetworkTime{4, 0.0f}, maxPenalty));
    REQUIRE_FALSE(v.RepairAndSample(NetworkTime{4, 0.5f}, maxPenalty));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{5, 0.0f}, maxPenalty), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{5, 0.5f}, maxPenalty), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{6, 0.0f}, maxPenalty), {5000.0f, 50000.0f}));

    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{8, 0.0f}, maxPenalty), {5000.0f, 50000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{8, 0.5f}, maxPenalty), {6250.0f, 62500.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{9, 0.0f}, maxPenalty), {7500.0f, 75000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{9, 0.5f}, maxPenalty), {8750.0f, 87500.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{10, 0.0f}, maxPenalty), {10000.0f, 100000.0f}));

    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{10, 0.5f}, maxPenalty), {10500.0f, 105000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{11, 0.0f}, maxPenalty), {11000.0f, 110000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{11, 0.5f}, maxPenalty), {11500.0f, 115000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{12, 0.0f}, maxPenalty), {12000.0f, 120000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{12, 0.5f}, maxPenalty), {12500.0f, 125000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{13, 0.0f}, maxPenalty), {13000.0f, 130000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{13, 0.5f}, maxPenalty), {13500.0f, 135000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{14, 0.0f}, maxPenalty), {14000.0f, 140000.0f}));

    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{14, 0.5f}, maxPenalty), {14000.0f, 140000.0f}));
    REQUIRE(IsSame(v.RepairAndSample(NetworkTime{15, 0.0f}, maxPenalty), {14000.0f, 140000.0f}));

}
#endif
